#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>     /* usleep */

#include "sysdefs.h"
#include "kbd.h"
#include "gui/gdi.h"
#include "gui/dialog.h"
#include "gui/controls/button.h"
#include "gui/processed_cheque.h"
#include "kkt/fd/ad.h"
#include <sys/param.h>
#include "paths.h"


/* ------------------------- константы/состояние ------------------------- */

#define PC_GAP        5
#define PC_BUTTON_H   30
#define PC_BUTTON_W   240
#define PC_LINE_GAP   2
#define PC_PADDING_X  10
#define PC_POLL_US    20000     /* 20 мс при пустой очереди */

/* Коды изменения состояния из pc_process() */
enum {
    PC_RUN_NONE    = 0,         /* экран не трогаем */
    PC_RUN_BUTTONS = 1,         /* перерисовать только кнопки */
    PC_RUN_CONTENT = 2,         /* перерисовать только контент */
    PC_RUN_ALL     = 3,         /* полная перерисовка (после диалога) */
    PC_RUN_EXIT    = -1,        /* выйти из окна */
};

/* Коды возврата action_*() */
enum {
    PC_ACTION_OK     = 0,
    PC_ACTION_CANCEL = 1,
};

typedef struct {
    int expanded;
    int button;                 /* 0 = keep, 1 = remove */
    int scroll_y;
    int max_scroll;
} pc_state_t;

static pc_state_t pc;
static GCPtr      pc_screen;
static FontPtr    pc_font;
static FontPtr    pc_sfont;

/* ------------------------------- helpers ------------------------------- */

static const char *fmt_rub(int64_t v, char *buf, size_t sz) {
    int64_t a = (v < 0) ? -v : v;
    snprintf(buf, sz, "%s%lld.%02lld РУБ.",
             (v < 0) ? "-" : "",
             (long long)(a / 100), (long long)(a % 100));
    return buf;
}

static const char *cheque_type_str(uint8_t t1054) {
    switch (t1054) {
    case 1: return "Приход";
    case 2: return "Возврат прихода";
    case 3: return "Расход";
    case 4: return "Возврат расхода";
    default: return "Неизвестный тип";
    }
}

static const char *pay_type_str(uint8_t m) {
    switch (m) {
    case 1: return "НАЛ";
    case 2: return "БЕЗНАЛ";
    case 3: return "В/П";
    default: return "?";
    }
}

static const char *vat_str(uint8_t n) {
    static const char *names[] = {
        "НДС 20%", "НДС 10%", "НДС 20/120", "НДС 10/110",
        "НДС 0%", "НДС не обл.", "НДС 5%", "НДС 7%",
        "НДС 5/105", "НДС 7/107", "НДС 22%", "НДС 22/122"
    };
    if (n >= 1 && n <= 12) return names[n - 1];
    return "?";
}

/* --------------------------- геометрия окна --------------------------- */

static int pc_header_bottom(void) {
    /* header: PC_GAP (top) + одна строка + PC_GAP (gap вниз) */
    return PC_GAP + pc_font->max_height + PC_GAP;
}

static int pc_buttons_top(void) {
    return DISCY - PC_BUTTON_H - PC_GAP;
}

static int pc_content_h(void) {
    int h = pc_buttons_top() - PC_GAP - pc_header_bottom();
    return (h > 0) ? h : 0;
}

/* ----------------------- оценка высоты контента ----------------------- */

static int calc_content_h(FontPtr f, FontPtr sf) {
    current_cheque_t *cur = &_ad->current;
    int lh  = f->max_height;
    int slh = sf->max_height;
    int h   = 0;

    h += lh + PC_GAP;                         /* title                  */
    h += 5 * (lh + PC_LINE_GAP) + PC_GAP;     /* sums: 5 строк          */
    h += lh + PC_GAP;                         /* buyer                  */
    h += lh + PC_GAP;                         /* docs header            */

    if (pc.expanded) {
        for (size_t i = 0; i < cur->ks_docs.count; i++) {
            K *k = AD_find_k_by_doc_no(cur->ks_docs.values[i]);
            if (!k) continue;
            h += lh;                          /* заголовок K            */
            h += (int)k->llist.count * slh;   /* строки L               */
            h += PC_LINE_GAP;
        }
    }

    /* Footer  берём худший случай (2 строки). Немного завышаем, но это
       безопасно: лишние пиксели внизу просто пустые. */
    h += 3 * PC_GAP + 2 * (lh + PC_LINE_GAP);

    return h;
}

/* --------------------------- секции отрисовки ------------------------ */

static void draw_header(GCPtr s, FontPtr f, int y) {
    current_cheque_t *cur = &_ad->current;
    char buf[128];
    int x = PC_PADDING_X;

    SetFont(s, f);
    SetTextColor(s, clBlack);

    snprintf(buf, sizeof(buf), "Номер ФД в ЕКФО: %u", cur->doc_no);
    TextOut(s, x, y, buf);
    x += TextWidth(f, buf) + PC_GAP * 6;

    snprintf(buf, sizeof(buf), "Номер ФН в ЕКФО: %s", cur->fs_serial);
    TextOut(s, x, y, buf);
}

static int draw_title(GCPtr s, FontPtr f, int y) {
    char buf[64];
    snprintf(buf, sizeof(buf), "Чек (%s)", cheque_type_str(_ad->current.t1054));
    DrawText(s, 0, y, DISCX, f->max_height, buf, DT_CENTER | DT_VCENTER);
    return y + f->max_height + PC_GAP;
}

static int draw_sums(GCPtr s, FontPtr f, int y) {
    current_cheque_t *cur = &_ad->current;
    int x_left  = PC_PADDING_X + 10;
    int x_right = DISCX - PC_PADDING_X;
    int line_h  = f->max_height + PC_LINE_GAP;
    char val_buf[64];

    int64_t total = cur->sum_n + cur->sum_e + cur->sum_p + cur->sum_b;

    const char *keys[] = {
        "Наличными:",
        "Безналичными:",
        "В зачёт ранее внесенных средств:",
        "Встречным предоставлением:",
        "ИТОГО:",
    };
    int64_t vals[] = { cur->sum_n, cur->sum_e, cur->sum_p, cur->sum_b, total };

    SetFont(s, f);
    for (int i = 0; i < 5; i++) {
        TextOut(s, x_left, y, keys[i]);
        DrawText(s, x_left, y, x_right - x_left, f->max_height,
                 fmt_rub(vals[i], val_buf, sizeof(val_buf)),
                 DT_RIGHT | DT_VCENTER);
        y += line_h;
    }
    return y + PC_GAP;
}

static int draw_buyer(GCPtr s, FontPtr f, int y) {
    current_cheque_t *cur = &_ad->current;
    char buf[300];

    SetFont(s, f);
    snprintf(buf, sizeof(buf), "Тел. или e-mail покупателя: %s",
             (cur->pe && cur->pe[0]) ? cur->pe : "не указан");
    TextOut(s, PC_PADDING_X + 10, y, buf);
    return y + f->max_height + PC_GAP;
}

static int draw_docs_header(GCPtr s, FontPtr f, int y) {
    current_cheque_t *cur = &_ad->current;
    char buf[80];

    snprintf(buf, sizeof(buf), "%s Документы чека (%lu)",
             pc.expanded ? "[v]" : "[>]",
             (unsigned long)cur->ks_docs.count);
    DrawText(s, PC_PADDING_X, y, DISCX - PC_PADDING_X * 2, f->max_height,
             buf, DT_CENTER | DT_VCENTER);
    return y + f->max_height + PC_GAP;
}

static int draw_l(GCPtr s, FontPtr sf, int y, L *l) {
    char buf[512], sum_buf[64], vat_buf[64];
    int x = PC_PADDING_X + 30;

    if (l->n == 0) {
        snprintf(buf, sizeof(buf),
                 "%s: %s (НЕ ПОДЛЕЖИТ НАЛОГООБЛОЖЕНИЮ НДС)",
                 l->s ? l->s : "",
                 fmt_rub(l->t, sum_buf, sizeof(sum_buf)));
    } else {
        snprintf(buf, sizeof(buf),
                 "%s: %s В Т.Ч. %s: %s",
                 l->s ? l->s : "",
                 fmt_rub(l->t, sum_buf, sizeof(sum_buf)),
                 vat_str(l->n),
                 fmt_rub(l->c, vat_buf, sizeof(vat_buf)));
    }
    SetFont(s, sf);
    TextOut(s, x, y, buf);
    return y + sf->max_height;
}

static int draw_k(GCPtr s, FontPtr f, FontPtr sf, int y, K *k) {
    char buf[512], sum_buf[64];
    int64_t sum = 0;
    int x = PC_PADDING_X + 10;

    for (list_item_t *li = k->llist.head; li; li = li->next)
        sum += LIST_ITEM(li, L)->t;

    snprintf(buf, sizeof(buf), "Документ N%s (СУММА: %s %s)%s",
             k->d.s ? k->d.s : "",
             fmt_rub(sum, sum_buf, sizeof(sum_buf)),
             pay_type_str(k->m),
             doc_no_is_empty(&k->u)
                 ? "" : " (НЕЗАВЕРШЕННОЕ ПЕРЕОФОРМЛЕНИЕ)");

    SetFont(s, f);
    TextOut(s, x, y, buf);
    y += f->max_height;

    for (list_item_t *li = k->llist.head; li; li = li->next)
        y = draw_l(s, sf, y, LIST_ITEM(li, L));

    return y + PC_LINE_GAP;
}

static int draw_docs_body(GCPtr s, FontPtr f, FontPtr sf, int y) {
    current_cheque_t *cur = &_ad->current;

    for (size_t i = 0; i < cur->ks_docs.count; i++) {
        K *k = AD_find_k_by_doc_no(cur->ks_docs.values[i]);
        if (!k) continue;
        y = draw_k(s, f, sf, y, k);
    }
    return y;
}

static int draw_footer_sums(GCPtr s, FontPtr f, int y) {
    current_cheque_t *cur = &_ad->current;
    int x_left  = PC_PADDING_X + 10;
    int x_right = DISCX - PC_PADDING_X;
    int line_h  = f->max_height + PC_LINE_GAP;
    char buf[128], sum_buf[64];
    int y_start = y;

    int64_t sum_nal = (cur->t1054 == 1 || cur->t1054 == 4)
        ? cur->sum_n : -cur->sum_n;
    int64_t sum_el  = (cur->t1054 == 1 || cur->t1054 == 4)
        ? cur->sum_e : -cur->sum_e;

    y += PC_GAP;
    SetFont(s, f);

    if (sum_nal == 0 && sum_el == 0) {
        TextOut(s, x_left, y,
            "Получение или выдача денежных средств не требуется");
        y += line_h;
    } else {
        if (sum_nal != 0) {
            snprintf(buf, sizeof(buf), "Сумма наличными к %s:",
                     sum_nal > 0 ? "получению от пассажира" : "выдаче пассажиру");
            TextOut(s, x_left, y, buf);
            DrawText(s, x_left, y, x_right - x_left, f->max_height,
                     fmt_rub(sum_nal < 0 ? -sum_nal : sum_nal,
                             sum_buf, sizeof(sum_buf)),
                     DT_RIGHT | DT_VCENTER);
            y += line_h;
        }
        if (sum_el != 0) {
            snprintf(buf, sizeof(buf), "Сумма безналичными к %s:",
                     sum_el > 0 ? "получению от пассажира" : "выдаче пассажиру");
            TextOut(s, x_left, y, buf);
            DrawText(s, x_left, y, x_right - x_left, f->max_height,
                     fmt_rub(sum_el < 0 ? -sum_el : sum_el,
                             sum_buf, sizeof(sum_buf)),
                     DT_RIGHT | DT_VCENTER);
            y += line_h;
        }
    }

    y += PC_GAP;
    DrawBorder(s, PC_PADDING_X, y_start,
               DISCX - PC_PADDING_X * 2, y - y_start,
               1, clGray, clGray);

    return y + PC_GAP;
}

static void draw_buttons(GCPtr s) {
    int gap = PC_GAP * 2;
    int total_w = PC_BUTTON_W * 2 + gap;
    int x0 = (DISCX - total_w) / 2;
    int y  = pc_buttons_top();

    draw_button_ex(s, x0, y, PC_BUTTON_W, PC_BUTTON_H,
                   "Оставить в подкорзине ЕКФО",
                   pc.button == 0, 1);
    draw_button_ex(s, x0 + PC_BUTTON_W + gap, y,
                   PC_BUTTON_W, PC_BUTTON_H,
                   "Удалить из подкорзины ЕКФО",
                   pc.button == 1, 1);
}

/* ---------------------------- перерисовки ----------------------------- */

/* Только кнопки: очистка полосы под ними + отрисовка. */
static void redraw_buttons(GCPtr s) {
    int top = pc_buttons_top();

    SetGCBounds(s, 0, 0, DISCX, DISCY);
    SetBrushColor(s, clSilver);
    SetRop2Mode(s, R2_COPY);
    FillBox(s, 0, top, DISCX, DISCY - top);

    draw_buttons(s);
}

/* Только контент: пересчёт max_scroll, очистка области, отрисовка
   с клиппингом, рамка вокруг вьюпорта. */
static void redraw_content(GCPtr s, FontPtr f, FontPtr sf) {
    int top = pc_header_bottom();
    int h   = pc_content_h();

    /* 1. Пересчёт прокрутки ДО отрисовки. */
    int content_h = calc_content_h(f, sf);
    pc.max_scroll = content_h - h;
    if (pc.max_scroll < 0) pc.max_scroll = 0;
    if (pc.scroll_y > pc.max_scroll) pc.scroll_y = pc.max_scroll;
    if (pc.scroll_y < 0) pc.scroll_y = 0;

    /* 2. Очистить область контента. */
    SetGCBounds(s, 0, 0, DISCX, DISCY);
    SetBrushColor(s, clSilver);
    SetRop2Mode(s, R2_COPY);
    FillBox(s, 0, top, DISCX, h);

    /* 3. Отрисовать содержимое с клиппингом. */
    SetGCBounds(s, 0, top, DISCX, h);
    {
        int y = top - pc.scroll_y;
        y = draw_title(s, f, y);
        y = draw_sums(s, f, y);
        y = draw_buyer(s, f, y);
        y = draw_docs_header(s, f, y);
        if (pc.expanded)
            y = draw_docs_body(s, f, sf, y);
        y = draw_footer_sums(s, f, y);
    }

    /* 4. Снять клиппинг и нарисовать рамку вокруг вьюпорта. */
    SetGCBounds(s, 0, 0, DISCX, DISCY);
    DrawBorder(s, PC_PADDING_X, top, DISCX - PC_PADDING_X * 2, h,
               1, clGray, clGray);
}

/* Полная перерисовка: заголовок, кнопки, контент. */
static void redraw_all(GCPtr s, FontPtr f, FontPtr sf) {
    SetGCBounds(s, 0, 0, DISCX, DISCY);
    ClearGC(s, clSilver);
    SetTextColor(s, clBlack);

    draw_header(s, f, PC_GAP);
    draw_buttons(s);
    redraw_content(s, f, sf);
}

/* --------------------------- init / release --------------------------- */

static void pc_init(void) {
    memset(&pc, 0, sizeof(pc));
    pc.expanded = 1;
    pc.button   = 0;
}

static void pc_release(void) {
    /* сейчас делать нечего */
}

/* --------------------------- действия -------------------------------- */

static int action_keep(void) {
    kbd_flush_queue();
    if (message_box("Предупреждение",
            "Данный чек будет оставлен в подкорзине ЕКФО.\n"
            "Вы уверены в том, что хотите выполнить данное действие?",
            dlg_yes_no, DLG_BTN_NO, al_center) != DLG_BTN_YES)
        return PC_ACTION_CANCEL;

    current_cheque_keep();
    /* current_cheque_keep()  current_cheque_end()  AD_save(),
       но НЕ пересчитывает суммы. Делаем явно. */
    AD_calc_sum();
    return PC_ACTION_OK;
}

static int action_remove(void) {
    kbd_flush_queue();
    if (message_box("Предупреждение",
            "Данный чек будет удалён из подкорзины ЕКФО.\n"
            "Вы уверены в том, что хотите выполнить данное действие?",
            dlg_yes_no, DLG_BTN_NO, al_center) != DLG_BTN_YES)
        return PC_ACTION_CANCEL;

    /* Внутри уже вызывает AD_calc_sum() и AD_save(). */
    current_cheque_remove();
    return PC_ACTION_OK;
}

/* --------------------------- обработка клавиш ------------------------- */

static int pc_process(const struct kbd_event *e) {
    if (!e->pressed)
        return PC_RUN_NONE;

    switch (e->key) {
    case KEY_ESCAPE:
        return PC_RUN_EXIT;

    case KEY_TAB:
    case KEY_LEFT:
    case KEY_RIGHT:
        pc.button = 1 - pc.button;
        return PC_RUN_BUTTONS;

    case KEY_SPACE:
        pc.expanded = !pc.expanded;
        return PC_RUN_CONTENT;

    case KEY_UP:
        pc.scroll_y -= pc_font->max_height;
        return PC_RUN_CONTENT;
    case KEY_DOWN:
        pc.scroll_y += pc_font->max_height;
        return PC_RUN_CONTENT;
    case KEY_PGUP:
        pc.scroll_y -= pc_content_h();
        return PC_RUN_CONTENT;
    case KEY_PGDN:
        pc.scroll_y += pc_content_h();
        return PC_RUN_CONTENT;

    case KEY_ENTER:
        if (pc.button == 0) {
            int r = action_keep();
            return (r == PC_ACTION_OK) ? PC_RUN_EXIT : PC_RUN_ALL;
        } else {
            int r = action_remove();
            return (r == PC_ACTION_OK) ? PC_RUN_EXIT : PC_RUN_ALL;
        }

    default:
        return PC_RUN_NONE;
    }
}

/* --------------------------- главный цикл ----------------------------- */

void processed_cheque_execute(void) {
    if (!current_cheque_active()) {
        message_box("Уведомление",
            "В корзине нет чека с ошибкой формирования.",
            dlg_yes, 0, al_center);
        return;
    }

    GCPtr   s  = CreateGC(0, 0, DISCX, DISCY);
    FontPtr f  = CreateFont(_("fonts/fixedsys8x16.fnt"), false);
    FontPtr sf = f;    /* TODO: если есть отдельный мелкий шрифт  заменить */

    SetFont(s, f);
    SetTextColor(s, clBlack);

    pc_screen = s;
    pc_font   = f;
    pc_sfont  = sf;

    pc_init();
    redraw_all(s, f, sf);       /* первичная отрисовка  один раз */
    kbd_flush_queue();

    int running = 1;
    struct kbd_event e;

    while (running) {
        /* Обнуляем ключевые поля: если kbd_get_event() вернёт
           нет события и не тронет структуру, мы это увидим. */
        e.key      = KEY_NONE;
        e.ch       = 0;
        e.pressed  = false;
        e.repeated = false;

        int r = kbd_get_event(&e);

        /* r <= 0    нет события (для возврата -1 или 0);
           e.key == KEY_NONE  на случай, если возврат истина,
                              но событие не заполнено. */
        if (r <= 0 || e.key == KEY_NONE) {
            usleep(PC_POLL_US);
            continue;
        }

        /* Игнорируем отпускание клавиши. */
        if (!e.pressed)
            continue;

        int change = pc_process(&e);

        switch (change) {
        case PC_RUN_NONE:
            break;
        case PC_RUN_BUTTONS:
            redraw_buttons(s);
            break;
        case PC_RUN_CONTENT:
            redraw_content(s, f, sf);
            break;
        case PC_RUN_ALL:
            redraw_all(s, f, sf);
            break;
        case PC_RUN_EXIT:
            running = 0;
            break;
        }
    }

    DeleteFont(f);
    DeleteGC(s);
    pc_release();

    /* Сбросить ссылки  чтобы случайно не обратиться после delete. */
    pc_screen = NULL;
    pc_font   = NULL;
    pc_sfont  = NULL;
}