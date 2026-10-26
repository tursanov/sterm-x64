#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/param.h>
#include <ctype.h>

#include "sysdefs.h"
#include "kbd.h"
#include "gui/gdi.h"
#include "gui/dialog.h"
#include "gui/cart.h"
#include "gui/forms.h"
#include "gui/fa.h"

#include "pos/pos.h"
#include "pos/command.h"

#include "kkt/kkt.h"

#include "kkt/fd/fd.h"
#include "kkt/fd/tlv.h"
#include "kkt/fdo.h"

/* Скроллбар справа от области корзины */
#define CART_SCROLLBAR_WIDTH       4    /* толщина ползунка/трека */
#define CART_SCROLLBAR_MARGIN      0     /* отступ от краёв трека */
#define CART_SCROLLBAR_MIN_THUMB   20    /* минимальная высота ползунка */
#define CART_SCROLLBAR_TOP_PAD     5   /* отступ сверху от вьюпорта до трека */
#define CART_SCROLLBAR_BOTTOM_PAD  5   /* отступ снизу от трека до края */

#define CART_SCROLL_BOTTOM_GAP 0
#define CART_SCROLL_TOP_GAP 0

int cart_start_y = 0;

bool is_admin_mode() {
	return (kt != key_reg);
}

static bool cart_toggle_document(ui_subcart_t *sc, ui_doc_t *doc, bool ctrl);
static void ui_cart_process_delete(void);
static void ui_cart_move_subcart(bool up);


static void update_cheque(void *arg __attribute__((unused))) {
	kbd_flush_queue();
	//cheque_draw();
}


static size_t get_phone(char *src, char *dst) {
	if (src == NULL)
		return 0;
	char ch = *src;
	size_t len = 0;
	if (ch == '8') {
		*dst++ = '+';
		*dst++ = '7';
		len += 2;
		src++;
	} else if (isdigit(ch)) {
		*dst++ = '+';
		len++;
	}

	while (*src) {
		*dst++ = *src++;
		len++;
	}
	*dst = 0;

	return len;
}




GCPtr cart_screen = NULL;
FontPtr cart_fnt = NULL;
FontPtr cart_sfnt = NULL;
ui_cart_t *ui_cart = NULL;
ui_subcart_t *ui_sel_subcart = NULL;
int ui_sel_subcart_index = -1;
int cart_scroll_y = 0;

void process_docs();
bool print_cheque(SubCart *sc, list_t *klist);
void ui_select_next_subcart();
void ui_select_prev_subcart();
void get_doc_selection(list_t* sel);
void ui_cart_select_documents();


void ui_cart_create()
{
    cart_scroll_y = 0;
    cart_start_y = pos_incomplete_op ? 35 : 0;
    
	pos_incomplete_op = pos_check_last_operation();
	
	printf("ui_cart_create: pos_incomplete_op: %d\n", pos_incomplete_op);

	if (ui_cart != NULL)
	{
		ui_cart_destroy();
	}

	ui_cart = __new(ui_cart_t);
	ui_cart->subcarts = __calloc(MAX_SUB_CART, ui_subcart_t);
	ui_sel_subcart = NULL;
	ui_sel_subcart_index = -1;

    bool first = true;
	for (int i = 0; i < cart.ordered_count; i++)
	{
		SubCart *val = cart.ordered[i];
		if (val->documents.count > 0)
		{
		    int n = ui_cart->subcart_count;
			ui_subcart_t *sc = &ui_cart->subcarts[n];
			
			if (first)
			{
			    ui_sel_subcart = sc;
			    ui_sel_subcart_index = n;
			}
			
			ui_subcart_init(sc, val, first);
			
			first = false;
			
			ui_cart->subcart_count++;
		}
	}
	
	ui_cart_select_documents();
}

void ui_cart_select_documents()
{
	LIST_INIT(sel, NULL);
	get_doc_selection(&sel);
	
	printf("sel.count = %lu, sel.head: %p\n", sel.count, sel.head);

    for (int i = 0; i < ui_cart->subcart_count; i++)
    {
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		
		for (int j = 0; j < sc->doc_count; j++)
		{
            ui_doc_t *d = &sc->docs[j];
            
            for (list_item_t *li = sel.head; li != NULL; li = li->next)
            {
                D *x = LIST_ITEM(li, D);
                
                //printf("x = %p, d->val = %p\n", x, d->val);
                
                if (d->val == x)
                {
                    d->selected = true;
                }
            }
        }
    }
    
    list_clear(&sel);
}

void ui_cart_destroy()
{
    if (!ui_cart)
    {
        return;
    }

	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_free(&ui_cart->subcarts[i]);
	}

	free(ui_cart->subcarts);
	free(ui_cart);
	ui_cart = NULL;
}

void ui_cart_calc_bounds()
{
    if (!ui_cart)
    {
        return;
    }
    
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		ui_subcart_calc_bounds(sc);
	}
}

/*
 * Общая высота всех подкорзин (без верхнего и нижнего паддинга).
 */
static int cart_content_total_height(void)
{
	int total = 0;
	int n = ui_cart->subcart_count;
	for (int i = 0; i < n; i++)
		total += ui_cart->subcarts[i].height;
	if (n > 1)
		total += (n - 1) * CART_YGAP;
	return total;
}

/*
 * Абсолютная Y-координата верхней границы подкорзины (без учёта скролла).
 */
static int ui_subcart_get_abs_y(ui_subcart_t *sc)
{
	int y = CART_YGAP + cart_start_y;
	for (int i = 0; i < ui_cart->subcart_count; i++) {
		ui_subcart_t *_sc = &ui_cart->subcarts[i];
		if (_sc == sc)
			return y;
		y += _sc->height + CART_YGAP;
	}
	return 0;
}

/*
 * Абсолютная Y-координата панели кнопок (ACTION / DELETE) подкорзины.
 * Соответствует y, с которого ui_subcart_draw начинает рисовать кнопки:
 *   заголовок подкорзины + заголовок таблицы + все документы.
 */
static int ui_subcart_get_buttons_abs_y(ui_subcart_t *sc)
{
    int y = ui_subcart_get_abs_y(sc);
    y += CART_YGAP * 4 + cart_fnt->max_height * 2;
    for (int i = 0; i < sc->doc_count; i++)
        y += sc->docs[i].height + CART_YGAP_DOC;
    return y;
}

static int ui_subcart_get_selected_abs_y(ui_subcart_t *sc)
{
    /* Если выделена кнопка (Tab переключил на ACTION / DELETE),
     * цель ? кнопка, а не документ. */
    if (sc_action_selected(sc) || sc_delete_selected(sc))
        return ui_subcart_get_buttons_abs_y(sc);

    int y = ui_subcart_get_abs_y(sc);
    y += CART_YGAP * 4 + cart_fnt->max_height * 2;
    if (sc->tab_selected_doc >= 0) {
        for (int i = 0; i < sc->tab_selected_doc; i++)
            y += sc->docs[i].height + CART_YGAP_DOC;
    }
    return y;
}

static int ui_subcart_get_selected_height(ui_subcart_t *sc)
{
    if (sc_action_selected(sc) || sc_delete_selected(sc))
        return CART_BUTTON_HEIGHT;
    if (sc->tab_selected_doc >= 0)
        return sc->docs[sc->tab_selected_doc].height;
    return CART_BUTTON_HEIGHT;
}

/*
 * Рисует вертикальный скроллбар справа от области корзины.
 *
 * Трек занимает всю высоту экрана минус отступы CART_SCROLLBAR_TOP_PAD
 * (сверху, от y=0) и CART_SCROLLBAR_BOTTOM_PAD (снизу, от y=DISCY).
 * Внутри трека ? маленький CART_SCROLLBAR_MARGIN между треком и ползунком.
 *
 * Высота и позиция ползунка рассчитываются от реального диапазона прокрутки
 * (viewport), а не от высоты трека ? иначе при наличии баннера незавершённой
 * операции ползунок двигался бы непропорционально фактическому скроллу.
 *
 * Если содержимое полностью влезает в вьюпорт ? скроллбар не рисуется.
 */
static void ui_cart_draw_scrollbar(void)
{
    if (ui_cart == NULL || ui_cart->subcart_count == 0)
        return;

    /* Трек ? по краям экрана, независимо от cart_start_y. */
    int track_top    = CART_SCROLLBAR_TOP_PAD;
    int track_bottom = DISCY - CART_SCROLLBAR_BOTTOM_PAD;
    int track_h      = track_bottom - track_top;

    if (track_h <= 2 * CART_SCROLLBAR_MARGIN)
        return;

    /* Реальная видимая область контента (учитывает баннер). */
    int viewport_h = DISCY - cart_start_y;
    if (viewport_h <= 0)
        return;

    int content_h = cart_content_total_height()
                  + CART_YGAP + CART_SCROLL_BOTTOM_GAP;
    if (content_h <= viewport_h)
        return;                                 /* всё видно ? скроллбар не нужен */

    int max_scroll = content_h - viewport_h;
    if (max_scroll <= 0)
        return;

    int bar_x = DISCX - CART_SCROLLBAR_WIDTH - CART_SCROLLBAR_MARGIN;
    int bar_w = CART_SCROLLBAR_WIDTH;

    /* --- Трек (светлая полоса) --- */
    fill_rect(cart_screen,
              bar_x,
              track_top + CART_SCROLLBAR_MARGIN,
              bar_w,
              track_h - CART_SCROLLBAR_MARGIN * 2,
              0,
              clSilver, clSilver);

    /* --- Ползунок --- */
    /* Пропорционален доле видимой части от всего контента.
     * Рассчитывается от track_h (визуальная длина трека),
     * а не от viewport_h ? иначе ползунок ?прилипнет? к одному краю. */
    int thumb_h = (int)((int64_t)track_h * viewport_h / content_h);
    if (thumb_h < CART_SCROLLBAR_MIN_THUMB)
        thumb_h = CART_SCROLLBAR_MIN_THUMB;
    int max_thumb_h = track_h - CART_SCROLLBAR_MARGIN * 2;
    if (thumb_h > max_thumb_h)
        thumb_h = max_thumb_h;

    if (cart_scroll_y < 0)
        cart_scroll_y = 0;
    if (cart_scroll_y > max_scroll)
        cart_scroll_y = max_scroll;

    int free_space = track_h - CART_SCROLLBAR_MARGIN * 2 - thumb_h;
    int thumb_y = track_top + CART_SCROLLBAR_MARGIN
                + (int)((int64_t)cart_scroll_y * free_space / max_scroll);

    fill_rect(cart_screen,
              bar_x,
              thumb_y,
              bar_w,
              thumb_h,
              1,
              RGB(0, 128, 0),
              RGB(0, 200, 0));
}

/*
 * Подогнать cart_scroll_y так, чтобы выбранный элемент был полностью виден.
 *
 * Логика:
 *   - если элемент выше вьюпорта ? показываем его верх;
 *   - если элемент ушёл выше вьюпорта ? сдвигаем содержимое вниз (уменьшаем скролл);
 *   - если элемент ушёл ниже вьюпорта ? сдвигаем содержимое вверх так,
 *     чтобы нижняя граница элемента совпала с нижней границей вьюпорта.
 */
static void ui_cart_ensure_visible(void)
{
    if (ui_sel_subcart == NULL)
        return;

    ui_subcart_t *sc = ui_sel_subcart;
    int viewport_top = cart_start_y;
    int viewport_h = DISCY - cart_start_y;
    if (viewport_h <= 0)
        return;

    /*
     * Случай ?выбрана кнопка? (Tab перевёл фокус на ACTION или DELETE).
     * Прокручиваем так, чтобы нижняя граница подкорзины (панель кнопок +
     * её нижний бордер + CART_YGAP*3 отступ) была видна внизу вьюпорта
     * с зазором CART_SCROLL_BOTTOM_GAP.
     *
     * При этом игнорируем положение выделенного документа: пользователь
     * явно переключил фокус на кнопки и хочет их видеть.
     */
    if (sc_action_selected(sc) || sc_delete_selected(sc)) {
        int subcart_bottom =
            ui_subcart_get_abs_y(sc) + sc->height;
        int bottom_scroll =
            subcart_bottom
            - (viewport_top + viewport_h - CART_SCROLL_BOTTOM_GAP);
        if (bottom_scroll < 0)
            bottom_scroll = 0;
        cart_scroll_y = bottom_scroll;
    } else {
        /* Случай ?выбран документ?. */
        int abs_y = ui_subcart_get_selected_abs_y(sc);
        int h_sel = ui_subcart_get_selected_height(sc);

        if (h_sel > viewport_h) {
            cart_scroll_y = abs_y - viewport_top;
        } else {
            int drawn_y = abs_y - cart_scroll_y;
            if (drawn_y < viewport_top) {
                cart_scroll_y = abs_y - viewport_top;
            } else if (drawn_y + h_sel
                    > viewport_top + viewport_h - CART_SCROLL_BOTTOM_GAP) {
                cart_scroll_y = abs_y + h_sel
                              - (viewport_top + viewport_h
                                 - CART_SCROLL_BOTTOM_GAP);
            }
        }

        /* Последний документ ? заодно подтягиваем панель кнопок. */
        if (sc->tab_selected_doc == (int)sc->doc_count - 1) {
            int btn_bottom =
                ui_subcart_get_buttons_abs_y(sc) + CART_BUTTON_HEIGHT;
            int needed = btn_bottom
                       - (viewport_top + viewport_h - CART_SCROLL_BOTTOM_GAP);
            if (cart_scroll_y < needed)
                cart_scroll_y = needed;
        }

        /* Первый документ ? верхняя граница подкорзины в кадре.
         * Только в этом режиме (флаг NONE), чтобы не перебивать
         * прокрутку до кнопок при Tab. */
        if (sc->tab_selected_doc == 0) {
            int subcart_top_scroll =
                ui_subcart_get_abs_y(sc) - viewport_top - CART_SCROLL_TOP_GAP;
            if (subcart_top_scroll < 0)
                subcart_top_scroll = 0;
            if (cart_scroll_y > subcart_top_scroll)
                cart_scroll_y = subcart_top_scroll;
        }
    }

    if (cart_scroll_y < 0)
        cart_scroll_y = 0;

    int max_scroll = cart_content_total_height()
                   + CART_YGAP + CART_SCROLL_BOTTOM_GAP - viewport_h;
    if (max_scroll < 0)
        max_scroll = 0;
    if (cart_scroll_y > max_scroll)
        cart_scroll_y = max_scroll;
}
/*
 * Прокрутить так, чтобы верхняя граница подкорзины оказалась
 * у верхнего края вьюпорта (с небольшим отступом).
 * Используется при переключении между подкорзинами (PgUp/PgDn).
 *
 * При короткой подкорзине (влезает в экран) это естественно ставит
 * кнопки её нижнего ряда близко к нижнему краю видимой области.
 * При длинной подкорзине ? показывает её сверху (документы видны),
 * кнопки окажутся ниже вьюпорта; до них можно добраться Tab'ом
 * (там сработает ui_cart_ensure_visible).
 */
static void ui_cart_scroll_to_subcart_top(ui_subcart_t *sc)
{
    if (sc == NULL)
        return;

    int viewport_top = cart_start_y;
    int viewport_h = DISCY - cart_start_y;
    if (viewport_h <= 0)
        return;

    int subcart_abs_y = ui_subcart_get_abs_y(sc);
    cart_scroll_y = subcart_abs_y - viewport_top - CART_SCROLL_TOP_GAP;

    if (cart_scroll_y < 0)
        cart_scroll_y = 0;

    int max_scroll = cart_content_total_height()
                   + CART_YGAP
                   + CART_SCROLL_BOTTOM_GAP
                   - viewport_h;
    if (max_scroll < 0)
        max_scroll = 0;
    if (cart_scroll_y > max_scroll)
        cart_scroll_y = max_scroll;
}

void ui_cart_redraw_all()
{
	ClearGC(cart_screen, clSilver);

	ui_cart_calc_bounds();
	
	if (pos_incomplete_op)
		cart_start_y = 35;
	else
		cart_start_y = 0;

	/*
	 * Рисуем подкорзины с учётом скролла.
	 * Подкорзины, полностью ушедшие над вьюпортом, не рисуем ?
	 * они всё равно будут затёрты серебром ниже.
	 * Как только y вышел за нижнюю границу панели ? обрываем цикл.
	 */
	int y = CART_YGAP + cart_start_y - cart_scroll_y;
	for (int i = 0; i < ui_cart->subcart_count; i++) {
		ui_subcart_t *sc = &ui_cart->subcarts[i];

		if (y >= DISCY)
			break;

		if (y + sc->height > cart_start_y)
			ui_subcart_draw(sc, y);

		y += sc->height + CART_YGAP;
	}

	/*
	 * Затираем всё, что оказалось выше вьюпорта (включая область,
	 * где потом будет нарисован баннер незавершённой операции).
	 * Это нужно, потому что GDI не умеет клиппинг, а контент,
	 * сдвинутый скроллом вверх, может заехать на баннер.
	 */
	if (cart_start_y > 0) {
		fill_rect(cart_screen, 0, 0, DISCX, cart_start_y,
		          0, clSilver, clSilver);
	}
	
	ui_cart_draw_scrollbar();

	/* Баннер ?есть незавершённая операция? ? рисуем поверх всего */
	if (pos_incomplete_op) {
		const char *text = "ВНИМАНИЕ! Имеется незавершенная операция в ИПТ";
		int bx = CART_XGAP * 3;
		int by = CART_YGAP;
		int bw = DISCX - CART_XGAP * 6;
		int bh = CART_BUTTON_HEIGHT;

		fill_rect(cart_screen, bx, by, bw, bh,
		          2, RGB(200, 100, 100), RGB(200, 150, 150));

		int tw = GetTextWidth(cart_screen, text);
		bx += (bw - tw) / 2;
		by += (bh - GetTextHeight(cart_screen)) / 2;
		SetTextColor(cart_screen, RGB(100, 50, 50));
		TextOut(cart_screen, bx, by, text);
	}

	if (ui_cart->subcart_count == 0) {
		draw_button(cart_screen,
		    CART_XGAP * 3, CART_YGAP + cart_start_y,
		    DISCX - CART_XGAP * 6, CART_BUTTON_HEIGHT,
		    "Нет документов для обработки",
		    true);
	}
}
void ui_cart_draw(GCPtr s, FontPtr f, FontPtr sf)
{
	cart_screen = s;
	cart_fnt = f;
	cart_sfnt = sf;
	
	SetFont(cart_screen, cart_fnt);

	ui_cart_redraw_all();
}

int ui_cart_get_y(ui_subcart_t *sc)
{
	int y = CART_YGAP + cart_start_y;
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *_sc = &ui_cart->subcarts[i];
		
		if (_sc == sc)
		{
    		printf("ui_cart_get_y: i: %d, y: %d\n", i, y);
		    return y;
		}
		
		y += _sc->height + CART_YGAP;
   		printf("ui_cart_get_y: sc->height: %d\n", _sc->height);
	}
	
	return 0;
}

static void ui_cart_tab_select_next(void)
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL)
        return;

    bool action_en = sc_action_enabled(sc);
    bool delete_en = sc_delete_enabled(sc);

    if (sc_action_selected(sc)) {
        /* ACTION -> DELETE, если активна */
        if (delete_en) {
            sc->tab_selected_flags = CART_TAB_SELECTED_DELETE;
        } else if (sc->tab_selected_doc >= 0) {
            /* иначе возвращаемся на ранее выбранный документ */
            sc->tab_selected_flags = CART_TAB_SELECTED_NONE;
        }
        /* если DELETE неактивна и документа нет ? остаёмся на ACTION */
    }
    else if (sc_delete_selected(sc)) {
        /* DELETE -> документ (тот, что был) */
        if (sc->tab_selected_doc >= 0) {
            sc->tab_selected_flags = CART_TAB_SELECTED_NONE;
        } else if (action_en) {
            /* если документа не было (теоретически) ? на ACTION */
            sc->tab_selected_flags = CART_TAB_SELECTED_ACTION;
        }
    }
    else if (sc->tab_selected_doc >= 0) {
        /* документ -> ACTION (или DELETE, если ACTION неактивна) */
        if (action_en) {
            sc->tab_selected_flags = CART_TAB_SELECTED_ACTION;
        } else if (delete_en) {
            sc->tab_selected_flags = CART_TAB_SELECTED_DELETE;
        }
        /* если обе неактивны ? остаёмся на документе */
    }

    ui_cart_ensure_visible();
    ui_cart_redraw_all();
}

static void ui_cart_tab_select_prev(void)
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL)
        return;

    bool action_en = sc_action_enabled(sc);
    bool delete_en = sc_delete_enabled(sc);

    if (sc_action_selected(sc)) {
        /* ACTION -> документ */
        if (sc->tab_selected_doc >= 0) {
            sc->tab_selected_flags = CART_TAB_SELECTED_NONE;
        }
    }
    else if (sc_delete_selected(sc)) {
        /* DELETE -> ACTION, иначе -> документ */
        if (action_en) {
            sc->tab_selected_flags = CART_TAB_SELECTED_ACTION;
        } else if (sc->tab_selected_doc >= 0) {
            sc->tab_selected_flags = CART_TAB_SELECTED_NONE;
        }
    }
    else if (sc->tab_selected_doc >= 0) {
        /* документ -> DELETE, иначе -> ACTION */
        if (delete_en) {
            sc->tab_selected_flags = CART_TAB_SELECTED_DELETE;
        } else if (action_en) {
            sc->tab_selected_flags = CART_TAB_SELECTED_ACTION;
        }
    }

    ui_cart_ensure_visible();
    ui_cart_redraw_all();
}

static void ui_cart_doc_select_next(void)
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL || sc->doc_count == 0)
        return;

    sc->tab_selected_flags = CART_TAB_SELECTED_NONE;
    if (sc->tab_selected_doc < 0)
        sc->tab_selected_doc = 0;
    else if (sc->tab_selected_doc < (int)sc->doc_count - 1)
        sc->tab_selected_doc++;

    ui_cart_ensure_visible();
    ui_cart_redraw_all();
}

static void ui_cart_doc_select_prev(void)
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL || sc->doc_count == 0)
        return;

    sc->tab_selected_flags = CART_TAB_SELECTED_NONE;
    if (sc->tab_selected_doc < 0)
        sc->tab_selected_doc = 0;
    else if (sc->tab_selected_doc > 0)
        sc->tab_selected_doc--;

    ui_cart_ensure_visible();
    ui_cart_redraw_all();
}

void ui_select_next_subcart()
{
    if (ui_cart->subcart_count == 0)
        return;

    if (ui_sel_subcart_index >= (int)ui_cart->subcart_count - 1)
        return;

    /* Сброс выделения в текущей (уходящей) подкорзине */
    if (ui_sel_subcart != NULL) {
        ui_sel_subcart->tab_selected_doc = -1;
        ui_sel_subcart->tab_selected_flags = CART_TAB_SELECTED_NONE;
    }

    ui_sel_subcart_index += 1;
    ui_sel_subcart = &ui_cart->subcarts[ui_sel_subcart_index];

    /* Курсор ? на первый документ новой подкорзины */
    ui_sel_subcart->tab_selected_flags = CART_TAB_SELECTED_NONE;
    ui_sel_subcart->tab_selected_doc = 0;

    ui_cart_scroll_to_subcart_top(ui_sel_subcart);
    ui_cart_redraw_all();
}

void ui_select_prev_subcart()
{
    if (ui_cart->subcart_count == 0)
        return;

    if (ui_sel_subcart_index <= 0)
        return;

    /* Сброс выделения в текущей (уходящей) подкорзине */
    if (ui_sel_subcart != NULL) {
        ui_sel_subcart->tab_selected_doc = -1;
        ui_sel_subcart->tab_selected_flags = CART_TAB_SELECTED_NONE;
    }

    ui_sel_subcart_index -= 1;
    ui_sel_subcart = &ui_cart->subcarts[ui_sel_subcart_index];

    /* Курсор ? на первый документ новой подкорзины */
    ui_sel_subcart->tab_selected_flags = CART_TAB_SELECTED_NONE;
    ui_sel_subcart->tab_selected_doc = 0;

    ui_cart_scroll_to_subcart_top(ui_sel_subcart);
    ui_cart_redraw_all();
}

/*
 * Перемещение выделенной подкорзины вверх/вниз в пользовательском порядке.
 *
 *   up = true  -> поменять местами с предыдущей подкорзиной
 *   up = false -> поменять местами со следующей
 *
 * Если перемещение невозможно (первая/последняя), ничего не делает.
 *
 * Меняет порядок в двух местах:
 *   - ui_cart->subcarts[] ? то, что видит оператор;
 *   - cart.ordered[]      ? ?истина? для последующих ui_cart_create().
 *
 * ВАЖНО: полная перестройка cart_build() (после удаления/проведения
 * документов) сбрасывает порядок к дефолтному из cart_order(). Если нужно
 * сохранять порядок между сессиями ? нужен отдельный persistent-механизм
 * (см. комментарий в конце функции).
 */
static void ui_cart_move_subcart(bool up)
{
    if (ui_cart == NULL || ui_sel_subcart == NULL)
        return;
    if (ui_cart->subcart_count < 2)
        return;

    int src = ui_sel_subcart_index;
    int dst = up ? src - 1 : src + 1;
    if (dst < 0 || dst >= (int)ui_cart->subcart_count)
        return;

    /* Swap UI-элементов (полное копирование структуры:
     * указатель на docs, val, состояние выделения и т.д.) */
    ui_subcart_t tmp = ui_cart->subcarts[src];
    ui_cart->subcarts[src] = ui_cart->subcarts[dst];
    ui_cart->subcarts[dst] = tmp;

    /* Swap в cart.ordered[] ? только если массивы синхронны по длине.
     * В норме: cart.ordered включает только непустые подкорзины,
     * и ui_cart->subcarts строится ровно из них же. */
    if ((int)cart.ordered_count == (int)ui_cart->subcart_count
        && dst < (int)cart.ordered_count) {
        SubCart *tmp_o = cart.ordered[src];
        cart.ordered[src] = cart.ordered[dst];
        cart.ordered[dst] = tmp_o;
    }

    /* Наша подкорзина теперь по индексу dst. Указатель ui_sel_subcart
     * до этого указывал на &ui_cart->subcarts[src] ? тот же адрес,
     * но содержимое там теперь другое. Перенаправляем. */
    ui_sel_subcart_index = dst;
    ui_sel_subcart = &ui_cart->subcarts[dst];

    /* Скроллим к верху подкорзины на новом месте. */
    ui_cart_scroll_to_subcart_top(ui_sel_subcart);
    ui_cart_redraw_all();
}

void ui_cart_process_enter()
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL)
    {
        return;
    }

    printf("ui_cart_process_enter start\n");

    /*
     * Порядок важен: если фокус на кнопке, приоритет у кнопки.
     * tab_selected_doc при этом может быть >= 0 ? это ?документ,
     * к которому вернёмся при повторном Tab?, а не цель Enter.
     */
    if (sc_action_selected(sc))
    {
        process_docs();
    }
    else if (sc_delete_selected(sc))
    {
        ui_cart_process_delete();
    }
    else if (sc->tab_selected_doc >= 0)
    {
        ui_doc_t *d = &sc->docs[sc->tab_selected_doc];
        d->expanded = !d->expanded;

        /* Высота документа изменилась ? пересчитать её и перерисовать,
         * а если раскрытый не помещается ? прокрутить. */
        ui_cart_ensure_visible();
        ui_cart_redraw_all();
    }

    printf("ui_cart_process_enter end\n");
}

void ui_cart_process_space(bool ctrl)
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL)
    {
        return;
    }
    
    if (sc->tab_selected_doc < 0)
        return;
    
    ui_doc_t *d = &sc->docs[sc->tab_selected_doc];
    
    if (cart_toggle_document(sc, d, ctrl))
        ui_cart_redraw_all();
}


bool cart_process(const struct kbd_event *_e) {
	struct kbd_event e = *_e;

	if (e.key == KEY_CAPS && e.pressed && !e.repeated) {
		if (kbd_lang_ex == lng_rus)
			kbd_lang_ex = lng_lat;
		else
			kbd_lang_ex = lng_rus;
	}

	e.ch = kbd_get_char_ex(e.key);

	if (e.pressed) {
	    printf("cart_process: e.key: %d\n", e.key);
	
        switch (e.key) {
            case KEY_ESCAPE:
                return 0;

            /* Документы вверх/вниз */
            case KEY_UP:
                ui_cart_doc_select_prev();
                break;
            case KEY_DOWN:
                ui_cart_doc_select_next();
                break;

            /* Tab / Left / Right ? документ ? кнопки */
            case KEY_TAB:
            case KEY_RIGHT:
                ui_cart_tab_select_next();
                break;
            case KEY_LEFT:
                ui_cart_tab_select_prev();
                break;

            case KEY_SPACE:
                ui_cart_process_space((e.shift_state & SHIFT_CTRL) != 0);
                break;
            case KEY_ENTER:
                ui_cart_process_enter();
                break;

            /* Подкорзины */
            case KEY_PGDN:
                if ((e.shift_state & SHIFT_CTRL) != 0)
                    ui_cart_move_subcart(false);        /* Ctrl+PgDn ? вниз */
                else
                    ui_select_next_subcart(false);      /* PgDn ? к след. подкорзине */
                break;

            case KEY_PGUP:
                if ((e.shift_state & SHIFT_CTRL) != 0)
                    ui_cart_move_subcart(true);         /* Ctrl+PgUp ? вверх */
                else
                    ui_select_prev_subcart(false);      /* PgUp ? к пред. подкорзине */
                break;            
        }
    }

	return 1;
}

static bool in_print_state(D *d)
{
    for (list_item_t *li = d->related.head; li != NULL; li = li->next)
    {
        K *k = LIST_ITEM(li, K);
        
        if (k->print_state == PRINT_STATE_PRINTING)
        {
            return true;
        }
    }
    
    return false;
}

static bool in_bank_process_state(D *d)
{
    return d->k->bank_state != BANK_STATE_NONE;
}

static bool in_check_state(D *d)
{
    return d->k->check_state;
}


void get_doc_selection(list_t* sel)
{
    for (int i = 0; i < ui_cart->subcart_count; i++)
    {
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		
		for (int j = 0; j < sc->doc_count; j++)
		{
            ui_doc_t *d = &sc->docs[j];
            
            if (in_print_state(d->val))
            {
                list_add(sel, d->val);
            }
        }
        
        if (sel->count > 0)
        {
            printf("get_doc_selection: in_print_state\n");
            return;
        }
        
		for (int j = 0; j < sc->doc_count; j++)
		{
            ui_doc_t *d = &sc->docs[j];
            
            if (in_bank_process_state(d->val) || in_check_state(d->val))
            {
                printf("get_doc_selection: in_bank_state or in_check_state\n");
                list_add(sel, d->val);
            }
        }
        
        if (sel->count > 0)
        {
            printf("get_doc_selection: sel->count > 0\n");
            return;
        }
        
		for (int j = 0; j < sc->doc_count; j++)
		{
            ui_doc_t *d = &sc->docs[j];
            
            if (d->val->k->u.s != NULL)
            {
                printf("get_doc_selection: has_u\n");
                continue;
            }
            
            printf("get_doc_selection: group != NULL: %d\n", d->val->group != NULL);
            if (d->val->group != NULL && d->val->group->count > 0)
            {
                if (sel->count + d->val->group->count > MAX_DOCS)
                {
                    printf("get_doc_selection: sel->count: %lu, d->val->group->count: %lu\n",
                        sel->count, d->val->group->count);
                    break;
                }
                
                printf("get_doc_selection: d->val->group->head: %p\n",
                                        d->val->group->head);
                for (list_item_t *li = d->val->group->head; li != NULL; li = li->next)
                {
                    D *x = LIST_ITEM(li, D);
                    list_add_if_not_exist(sel, x);
                }
            }
            else
            {
                if (sel->count > MAX_DOCS)
                {
                    break;
                }
                
                list_add_if_not_exist(sel, d->val);
            }
            
            if (d->val->k->c != 0 || sc->val->type == 'F')
            {
                printf("d->val->k->c != 0 || sc->val->type == 'F': true");
                break;
            }
		}
    }
}

int get_unprocessed_non_cash_annulment_invoice()
{
    int ret = 0;
    LIST_INIT(cancel_non_cash_items, NULL);
    LIST_INIT(non_cash_items, NULL);
    
    for (int i = 0; i < ui_cart->subcart_count; i++)
    {
        ui_subcart_t *sc = &ui_cart->subcarts[i];
     
        if (sc->val->type == CANCEL_NON_CASH_ITEMS)
        {
            ui_subcart_get_all_k(sc, &cancel_non_cash_items);
        }

        if (sc->val->type == NON_CASH_ITEMS)
        {
            ui_subcart_get_all_k(sc, &non_cash_items);
        }
        
        if (ui_subcart_items_disabled(sc))
        {
            goto LOut;
        }        
    }
    
    for (list_item_t *li = cancel_non_cash_items.head; li; li = li->next)
    {
        K *cancel_non_cash_k = LIST_ITEM(li, K);
        for (list_item_t *li1 = non_cash_items.head; li1; li1 = li1->next)
        {
            K *non_cash_k = LIST_ITEM(li1, K);
            
            if (doc_no_compare(&cancel_non_cash_k->d, &non_cash_k->d) == 0 &&
                K_calc_total_sum(cancel_non_cash_k) == K_calc_total_sum(non_cash_k))
            {
                ret = cancel_non_cash_k->c;
                goto LOut;
            }
        }
    }
LOut:    
    
    list_clear(&cancel_non_cash_items);
    list_clear(&non_cash_items);
    
    return ret;
}

typedef struct
{
    ui_subcart_t *sc;
    list_t dlist;
    list_t klist;
    list_t klist_by_p[2];
    uint8_t p;
    S sum;    
    bool has_unprocessed_bank_op;
    bool has_y;
} selected_docs_t;

void free_selected_docs(selected_docs_t *sd)
{
    list_clear(&sd->dlist);
    list_clear(&sd->klist);
    list_clear(&sd->klist_by_p[0]);
    list_clear(&sd->klist_by_p[1]);
}

void get_selected_docs(selected_docs_t* sd)
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL)
    {
        return;
    }
    
    sd->sc = sc;
    ui_doc_t *d = sc->docs;
    uint8_t kp = 255;
    
    for (size_t i = 0; i < sc->doc_count; i++, d++)
    {
        if (d->selected)
        {
            D* doc = d->val;
            
            list_add(&sd->dlist, doc);
            
            for (list_item_t* li = doc->related.head; li; li = li->next)
            {
                K *k = LIST_ITEM(li, K);
                uint8_t p = K_lp(k);
                int index;
                
                if (kp == 255)
                {
                    kp = p;
                    index = 0;
                    sd->p = p;
                }
                else if (kp == p)
                {
                    index = 0;
                }
                else
                {
                    index = 1;
                }
                
                list_add(&sd->klist_by_p[index], k);
                list_add(&sd->klist, k);
                K_add_sum(kp, k, &sd->sum);

                if (k->bank_state != BANK_STATE_SUCCESS)
                {
                    sd->has_unprocessed_bank_op = true;
                }
                
                if (k->y != NULL)
                {
                    sd->has_y = true;
                }
            }
        }
    }
}

void process_print_docs(selected_docs_t* sd);


typedef struct 
{
    int order_id;
    int64_t primary_sum;
    int64_t secondary_sum;
    int64_t amount;
    char *ords;
    bool is_fast_payment;
    bool in_check_state;
    char rfnd_info[128];
} bank_items_t;

bank_items_t *get_bank_items(list_t *sel)
{
    bank_items_t *r = calloc(1, sizeof(bank_items_t));
    if (r == NULL)
        return NULL;

    size_t ords_capacity = 128;
    char *ords = malloc(ords_capacity + 1);
    if (ords == NULL) {
        free(r);
        return NULL;
    }
    size_t ords_len = 0;
    ords[0] = 0;

    for (list_item_t *li = sel->head; li; li = li->next)
    {
        D *d = LIST_ITEM(li, D);

        /* В C# здесь все элементы уже отфильтрованы K.Y != null.
         * Здесь фильтруем на месте, чтобы не падать. */
        if (d->k->y == NULL)
            continue;

        if (r->order_id == 0 && d->name && strcmp(d->name, "ВСПП") != 0)
        {
            r->order_id = d->k->y->req_id;
        }

        r->is_fast_payment = d->k->y->op == '*';

        int64_t sum = 0;
        uint8_t d_lp = K_lp(d->k);
        for (list_item_t *li1 = d->related.head; li1; li1 = li1->next)
        {
            K *k = LIST_ITEM(li1, K);
            int64_t s = K_calc_total_sum(k);
            if (d_lp != K_lp(k))
                s = -s;

            sum += s;

            if (k->check_state)
                r->in_check_state = true;
        }

        r->amount += sum;

        if (d->name && strcmp(d->name, "ВОЗВРАТ") == 0)
        {
            if (d->k->v == 1)
                r->primary_sum += sum;
            else if (d->k->v == 2)
                r->secondary_sum += sum;
        }

        /* Паддинг нулями до 14 символов, как .PadLeft(14, '0') в C#. */
        char doc_buf[15];
        const char *doc_s = d->k->d.s ? d->k->d.s : "";
        size_t slen = strlen(doc_s);
        if (slen <= 14) {
            memset(doc_buf, '0', 14);
            memcpy(doc_buf + 14 - slen, doc_s, slen);
        } else {
            memcpy(doc_buf, doc_s + slen - 14, 14);
        }
        doc_buf[14] = 0;

        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%s/%ld;", doc_buf, (long)sum);
        if (len < 0)
            continue;

        /* Расширяем буфер, пока не хватит (с запасом на NULL). */
        while (ords_len + (size_t)len + 1 > ords_capacity) {
            size_t new_cap = ords_capacity + 128;
            char *tmp = realloc(ords, new_cap + 1);
            if (tmp == NULL) {
                free(ords);
                free(r);
                return NULL;
            }
            ords = tmp;
            ords_capacity = new_cap;
        }

        memcpy(ords + ords_len, buf, len);
        ords_len += len;
        ords[ords_len] = 0;
    }

    r->ords = ords;

    /* Найти первый документ с Y ? как C# Where(d => d.K.Y != null). */
    D *first_with_y = NULL;
    for (list_item_t *li = sel->head; li; li = li->next) {
        D *d = LIST_ITEM(li, D);
        if (d->k->y != NULL) {
            first_with_y = d;
            break;
        }
    }

    if (first_with_y != NULL)
    {
        const char *term_id = first_with_y->k->y->term_id;
        int term_len = term_id ? strlen(term_id) : 0;
        if (term_len >= 4)
        {
            uint8_t railway_code = term_id[3];

            snprintf(r->rfnd_info, sizeof(r->rfnd_info),
                "PAKOSN/%02x/%ld;PAKPVD/%02x/%ld\x1dINN:%ld",
                railway_code, (long)r->primary_sum,
                railway_code, (long)r->secondary_sum,
                (long)user_inn);
        }
    }

    return r;
}

void free_bank_items(bank_items_t *items)
{
    if (items == NULL)
        return;
    free(items->ords);
    free(items);
}

void process_other_items(__attribute__((unused)) selected_docs_t *sd)
{
    if (message_box("Предупреждение",
            "ВНИМАНИЕ! Вы хотите УДАЛИТЬ выделенные документы БЕЗ проведения операций по ним?",
            dlg_yes_no, 0, al_center) == DLG_BTN_YES)
    {
        ui_cart_redraw_all();
        if (message_box("Предупреждение",
            "ВНИМАНИЕ! Вы действительно хотите УДАЛИТЬ выделенные документы БЕЗ проведения операций по ним?",
            dlg_yes_no, 0, al_center) == DLG_BTN_YES)
        {
            AD_remove_K_list(&sd->klist);
        
            cart_build();
            ui_cart_create();
            ui_cart_redraw_all();
            return;
        }
    }
    ui_cart_redraw_all();
}

/*
 * Удаление выделенных документов из текущей подкорзины.
 *
 * Требует ДВОЙНОГО подтверждения одним и тем же диалогом ? защита от
 * случайного нажатия. Удаление выполняется только если пользователь
 * оба раза ответил ?Да?.
 */
static void ui_cart_process_delete(void)
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL)
        return;

    /* Собираем документы, реально помеченные ?галочками? в этой подкорзине */
    selected_docs_t sd;
    memset(&sd, 0, sizeof(sd));
    get_selected_docs(&sd);

    if (sd.klist.count == 0) {
        free_selected_docs(&sd);
        message_box("Предупреждение",
            "Не выделено ни одного документа для удаления",
            dlg_yes, 0, al_center);
        ui_cart_redraw_all();
        return;
    }

    /* Не удаляем, если в подкорзине есть документы в процессе
     * банковской операции / печати / проверки. */
    if (ui_subcart_items_disabled(sc)) {
        free_selected_docs(&sd);
        message_box("Ошибка",
            "В подкорзине есть документы в процессе обработки.\n"
            "Дождитесь завершения операций и повторите удаление.",
            dlg_yes, 0, al_center);
        ui_cart_redraw_all();
        return;
    }

    /* Первое подтверждение */
    if (message_box("Предупреждение",
            "ВНИМАНИЕ!\n"
            "Вы хотите УДАЛИТЬ выделенные документы БЕЗ проведения операций по ним?",
            dlg_yes_no, 0, al_center) != DLG_BTN_YES) {
        free_selected_docs(&sd);
        ui_cart_redraw_all();
        return;
    }

    /* Второе подтверждение ? тем же вопросом */
    if (message_box("Предупреждение",
            "ВНИМАНИЕ!\n"
            "Вы хотите УДАЛИТЬ выделенные документы БЕЗ проведения операций по ним?",
            dlg_yes_no, 0, al_center) != DLG_BTN_YES) {
        free_selected_docs(&sd);
        ui_cart_redraw_all();
        return;
    }

    /* Удаляем и перестраиваем корзину */
    AD_remove_K_list(&sd.klist);
    free_selected_docs(&sd);

    cart_build();
    ui_cart_create();
    ui_cart_redraw_all();
}

void process_non_cash_items(selected_docs_t *sd)
{
    printf("process_non_cash_items start\n");

    if (get_unprocessed_non_cash_annulment_invoice())
    {
        message_box("Ошибка",
            "НЕ ВСЕ НОМЕРА ДОКУМЕНТОВ БЛИ ПОГАШЕН. ПОГАСИТЕ ДОКУМЕНТ И ПОВТОРИТЕ ОПЕРАЦИЮ",
            dlg_yes, 0, al_center);
        return;
    }
    
    uint8_t code = 0xa3;
    
    if (sd->sc->val->type == CANCEL_NON_CASH_ITEMS
        || sd->sc->val->type == CANCEL_REFUND_NON_CASH_ITEMS)
    {
        code = 0xa5;
    }
    else if (sd->sc->val->type == REFUND_NON_CASH_ITEMS)
    {
        code = 0xa4;
    }

    char dt[16];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    
    strftime(dt, sizeof(dt), "%Y%m%d%H%M%S", &tm);
    
    for (list_item_t *li = sd->dlist.head; li; li = li->next)
    {
        D *d = LIST_ITEM(li, D);
        d->k->bank_state = code;
        
    
        if (d->k->bank_dt)
            free(d->k->bank_dt);    
        d->k->bank_dt = strdup(dt);
    }
    
    AD_save();
    
    bank_items_t *bi = get_bank_items(&sd->dlist);
    
    printf("ords: %s\n", bi->ords);
    printf("rfnd_info: %s\n", bi->rfnd_info);
    
    struct pos_query_params params =
    {
        .amount = bi->amount,
        .order_id = bi->order_id,
        .can_edit = false,
        .time = t,
        .ords = bi->ords,
        .type = bi->is_fast_payment ? "SBP" : NULL,
        .subtype = bi->is_fast_payment
            ? bi->in_check_state
                ? "CHECK"
                : "PAY"
            : NULL,
        .famio = cashier_get_name_alt(),
        .rfnd_info = bi->rfnd_info,
        .mtype = code
    };

    struct pos_response* resp = pos_query(&params);
   
    free_bank_items(bi);
    
    if (resp == NULL)
    {
        message_box("Ошибка", "ИПТ не подключен или не отвечает", dlg_yes, 0, al_center);
        ui_cart_redraw_all();
        return;
    }
    
    if (resp->res_code == POS_QUERY_SUCCESS
        || resp->res_code == POS_QUERY_INCOMPLETED
        || resp->res_code == POS_QUERY_CHECK_SBP)
    {
        for (list_item_t *li = sd->dlist.head; li; li = li->next)
        {
            D *d = LIST_ITEM(li, D);
            
            for (list_item_t *li1 = d->related.head; li1; li1 = li1->next)
            {
                K *k = LIST_ITEM(li1, K);
                
                if (k->c == 0 && resp->res_code == POS_QUERY_SUCCESS)
                {
                    k->c = resp->invoice;
                    k->check_state = resp->res_code == POS_QUERY_CHECK_SBP;
                    k->bank_state = resp->res_code == POS_QUERY_CHECK_SBP
                        ? BANK_STATE_NONE
                        : BANK_STATE_SUCCESS;
                        
                    if (k->y->op != '*')
                    {
                        list_add(&_ad->archive_items, k);
                    }
                }
                
                printf("k: %p, k->c = %d, k->bank_state = %d\n", k, k->c, k->bank_state);
            }
        }
        
        AD_archive_save();
    }        
    else
    {
        for (list_item_t *li = sd->dlist.head; li; li = li->next)
        {
            D *d = LIST_ITEM(li, D);
            
            d->k->bank_state = BANK_STATE_NONE;
            
            if (resp->res_code == POS_QUERY_ERROR)
            {
                for (list_item_t *li1 = d->related.head; li1; li1 = li1->next)
                {
                    K *k = LIST_ITEM(li1, K);
                
                    k->check_state = false;
                }
            }
        }
    }
    
    AD_save();
    
    ui_cart_redraw_all();
    
    if ((code == 0xa2 && resp->res_code == POS_QUERY_SUCCESS)
        || (resp->res_code == POS_QUERY_INCOMPLETED_NOT_FOUND
            && strcmp(resp->resp_code, "007") == 0)
        || (code == 0xa1 && resp->res_code == POS_QUERY_SUCCESS))
    {
        list_clear(&_ad->archive_items);
        AD_archive_save();
    }
}

void process_docs()
{
    selected_docs_t sd;
    memset(&sd, 0, sizeof(sd));
    get_selected_docs(&sd);
    
    char type = ui_sel_subcart->val->type;
    
    printf("process_docs start\n");

    if (type == OTHER_ITEMS)
    {
        process_other_items(&sd);
    }
    else if ((type == NON_CASH_ITEMS
            || type == CANCEL_REFUND_NON_CASH_ITEMS
            || type == REFUND_NON_CASH_ITEMS
            || type == CANCEL_NON_CASH_ITEMS
            || type == FAST_PAYMENT_ITEMS)
            && sd.has_unprocessed_bank_op
            && sd.has_y
            && sd.sum.e > 0)
    {
        printf("process_docs: process_non_cash_items\n");
        process_non_cash_items(&sd);
    }
    else
    {
        printf("process_docs: process_print_docs\n");
        process_print_docs(&sd);
        ui_cart_redraw_all();
    }

    free_selected_docs(&sd);
    
    printf("process_docs end\n");
}

void set_printing_state(list_t *list, uint8_t state)
{
    for (list_item_t *i1 = list->head; i1; i1 = i1->next)
    {
        K *k = LIST_ITEM(i1, K);
		k->print_state = state;
    }
    AD_save();
}

void process_print_docs(selected_docs_t* sd)
{
    for (int i = 0; i < 2; i++)
    {
        list_t *list = &sd->klist_by_p[i];
        if (list->count == 0)
        {
            continue;
        }
        
        ui_subcart_t *sc = ui_sel_subcart;

        set_printing_state(list, PRINT_STATE_PRINTING);
       
        bool ret = print_cheque(sc->val, list);
        
        printf("process_print_docs: last_cheque_process_started: %d\n", last_cheque_process_started);
        
        if (last_cheque_process_started)
        {
            AD_remove_K_list(list);
            cart_build();
            ui_cart_create();
            ui_cart_redraw_all();
        }
        else
        {
            set_printing_state(list, PRINT_STATE_NONE);
        }
        
        if (!ret)
        {
            break;
        }
    }
}

typedef void (*update_screen_func_t)(void *arg);

static bool fa_create_doc1(uint16_t doc_type, const uint8_t *pattern_footer,
		size_t pattern_footer_size, 
		update_screen_func_t update_func, void *update_func_arg) 
{
	uint8_t status;

	fdo_suspend();
	if ((status = fd_create_doc(doc_type, pattern_footer, pattern_footer_size)) != 0) {
		if (status == 0x46) {
			struct kkt_last_doc_info ldi;
			uint8_t err_info[32];
			size_t err_info_len;

			//printf("#1 %d\n", doc_type);
			err_info_len = sizeof(err_info);
			status = kkt_get_last_doc_info(&ldi, err_info, &err_info_len);
			if (status != 0) {
				//printf("#2: %d\n", status);
				fd_set_error(doc_type, status, err_info, err_info_len);
			} else {
				//printf("#3: %d, %d\n", ldi.last_nr, ldi.last_printed_nr);
				if (ldi.last_nr != ldi.last_printed_nr) {
					message_box("Ошибка", "Последний сформированный документ не был напечатан.\n"
							"Для его печати в меню фискального приложения выберите пункт\n"
							"\"Печать последнего сформированного документа\"",
							dlg_yes, 0, al_center);
					fdo_resume();
					return false;
/*					if (update_func)
						update_func(update_func_arg);
					status = fd_print_last_doc(ldi.last_type);

					//printf("LD: status = %d\n", status);

					if (status != 0)
						fd_set_error(doc_type, status, err_info, err_info_len);*/
				}
			}
		}

		fdo_resume();

		if (status != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("Ошибка", error, dlg_yes, 0, al_center);
			if (update_func)
				update_func(update_func_arg);
		} else
			return true;

		printf("status: %.2X\n", status);

/*		if (status == 0x41 || status == 0x42 || status == 0x44)
			goto LCheckLastDocNo;*/

		return false;
	}
	return true;
}

bool print_cheque(SubCart *sc, list_t *klist)
{
    size_t doc_count = 0;
    
    S sum;
    K_list_calc_sum(klist, &sum);
    
    C *cheque = sc->c;
    
	ffd_tlv_reset();

	ffd_tlv_add_string(1021, cashier_get_cashier());
	
	const char *cashier_inn = cashier_get_inn();
	if (cashier_inn[0])
		ffd_tlv_add_fixed_string(1203, cashier_inn, 12);
	ffd_tlv_add_uint8(1054, cheque->t1054);
	ffd_tlv_add_uint8(1055, cheque->t1055);
//	if (c->pe)
//		ffd_tlv_add_string(1008, c->pe);
	ffd_tlv_add_vln(1031, (uint64_t)sum.n);
	ffd_tlv_add_vln(1081, (uint64_t)sum.e);
	ffd_tlv_add_vln(1215, (uint64_t)sum.p);
	ffd_tlv_add_vln(1216, 0);
	ffd_tlv_add_vln(1217, (uint64_t)sum.b);

	char agent_phone[19+1];
	char phone[19+1];
	bool is_same_agent;
	bool attr = kkt_has_param("COMP1057WO1171");
	if (C_is_same_inn(cheque, user_inn, agent_phone, &is_same_agent)) {
		ffd_tlv_add_uint8(1057, 1 << 6);

		if (!attr || is_same_agent) {
			get_phone(agent_phone, phone);
			ffd_tlv_add_string(1171, phone);
		}
	}

	if (_ad->t1086 != NULL) {
		ffd_tlv_stlv_begin(1084, 320);
		ffd_tlv_add_string(1085, "ТЕРМИНАЛ");
		ffd_tlv_add_string(1086, _ad->t1086);
		ffd_tlv_stlv_end();
	}

	for (list_item_t *i1 = klist->head; i1; i1 = i1->next) {
		K *k = LIST_ITEM(i1, K);
		if (doc_no_is_not_empty(&k->u)) {
//					have_u = true;
//			have_unformed_docs = true;
		} else {
			for (list_item_t *i2 = k->llist.head; i2; i2 = i2->next) {
				L *l = LIST_ITEM(i2, L);
				ffd_tlv_stlv_begin(1059, 1024);
				ffd_tlv_add_uint8(1214, l->r);
				char s1030[256];
				sprintf(s1030, "%s\n\rдокумент \xfc%s", l->s, k->b.s ? k->b.s : "");
				ffd_tlv_add_string(1030, s1030);
				ffd_tlv_add_vln(1079, l->t);
				ffd_tlv_add_fvln(1023, 1, 0);
				if (l->n > 0)
					ffd_tlv_add_uint8(1199, l->n);
					
				if ((l->n >= 1 && l->n <= 4) || l->n >= 7) {
					printf("ADD 1198, %lld\n", (long long)l->c);
					ffd_tlv_add_vln(1198, l->c);
					printf("ADD 1200, %lld\n", (long long)l->c);
					ffd_tlv_add_vln(1200, l->c);
				}
				else {
				
					printf("NON NDS: %d\n", l->n);
				
				}
				
				if (l->i == 0) {
					// если ИНН == 0, но есть l->z, значит перевозчик не российский.
					if (l->z && l->z[0] != 0) {
						ffd_tlv_add_fixed_string(1226, "000000000000", 12);

						if (_support_1222_1224_1225)
						{
						    ffd_tlv_add_uint8(1222, 64);
					    	ffd_tlv_stlv_begin(1224, 512);
					        	ffd_tlv_add_string(1225, l->z);
					      	    if (l->h && strcmp(l->h, agent_phone) != 0) {
					        	    ffd_tlv_add_string(1171, l->h);
					        	}
					   	 	ffd_tlv_stlv_end();
						}

					}
					
					
				} else if (l->i != user_inn) {
					char inn[12+1];
					if (cheque->p > 9999999999ll)
						sprintf(inn, "%.12ld", l->i);
					else
						sprintf(inn, "%.10ld", l->i);
					ffd_tlv_add_fixed_string(1226, inn, 12);
					
					if (_support_1222_1224_1225)
					{
					    ffd_tlv_add_uint8(1222, 64);
					    ffd_tlv_stlv_begin(1224, 512);
					        ffd_tlv_add_string(1225, l->z);
					        if (l->h && strcmp(l->h, agent_phone) != 0) {
					            ffd_tlv_add_string(1171, l->h);
					        }
					    ffd_tlv_stlv_end();
					}
				}
				
				ffd_tlv_stlv_end();
			}
			doc_count++;
		}
	}
	
	

	if (doc_count > 0) {
		uint8_t* pattern_footer = NULL;
		size_t pattern_footer_size = 0;

		if (fa_create_doc1(CHEQUE, pattern_footer, pattern_footer_size, update_cheque, NULL)) {
		    return true;
		}
		
		return false;
	}
	
	return true;
}

/*
 * Установить selected = value для всех документов, входящих в группу.
 * Аналог SelectMany(s => s.Documents).Where(v => d.Document.Group.Contains(v.Document)).
 * Ищет по всем подкорзинам, потому что группа может быть ?размазана?.
 */
static void cart_set_group_selected(list_t *group, bool value)
{
    if (group == NULL || ui_cart == NULL)
        return;

    for (int i = 0; i < ui_cart->subcart_count; i++) {
        ui_subcart_t *sc = &ui_cart->subcarts[i];
        for (int j = 0; j < sc->doc_count; j++) {
            D *v = sc->docs[j].val;
            for (list_item_t *li = group->head; li; li = li->next) {
                if (li->obj == v) {
                    sc->docs[j].selected = value;
                    break;
                }
            }
        }
    }
}



static bool cart_toggle_document(ui_subcart_t *sc, ui_doc_t *doc, bool ctrl)
{
    if (sc == NULL || doc == NULL || ui_cart == NULL)
        return false;

    char type = sc->val->type;

    /* ============================================================
     * Ctrl+Space ? групповые операции по подкорзине.
     * ============================================================ */
    if (ctrl) {
        /* Если что-то уже выделено ? снять всё в подкорзине. */
        if (doc->selected) {
            bool changed = false;
            for (int i = 0; i < sc->doc_count; i++) {
                if (sc->docs[i].selected) {
                    sc->docs[i].selected = false;
                    changed = true;
                }
            }
            return changed;
        }

        /* C/G: только одна квитанция. Ctrl+Space = выделить всё по
         * квитанции первого документа. */
        if (type == CANCEL_NON_CASH_ITEMS || type == CANCEL_REFUND_NON_CASH_ITEMS) {
            int inv = doc->val->k->c;
            if (inv == 0) {
                message_box("Предупреждение",
                    "Документ не имеет связанной квитанции. Выбор невозможен.",
                    dlg_yes, 0, al_center);
                return false;
            }
            bool changed = false;
            for (int i = 0; i < sc->doc_count; i++) {
                ui_doc_t *d = &sc->docs[i];
                if (d->val->k->c == inv && !d->selected) {
                    d->selected = true;
                    changed = true;
                }
            }
            return changed;
        }

        /* F: только в пределах одного вида (ОСН/ПВД). */
        if (type == REFUND_NON_CASH_ITEMS) {
            int v = doc->val->k->v;
            bool changed = false;
            for (int i = 0; i < sc->doc_count; i++) {
                ui_doc_t *d = &sc->docs[i];
                if (d->val->k->v == v && !d->selected) {
                    d->selected = true;
                    changed = true;
                }
            }
            return changed;
        }

        /* D/E: до лимита MAX_DOCS, группу ? целиком. */
        if (type == NON_CASH_ITEMS || type == FAST_PAYMENT_ITEMS) {
            int checked = 0;
            for (int i = 0; i < sc->doc_count; i++)
                if (sc->docs[i].selected) checked++;

            bool changed = false;
            for (int i = 0; i < sc->doc_count; i++) {
                ui_doc_t *d = &sc->docs[i];
                if (d->selected) continue;

                int delta = (d->val->group && d->val->group->count > 0)
                          ? (int)d->val->group->count : 1;
                if (checked + delta > MAX_DOCS)
                    break;

                if (d->val->group && d->val->group->count > 0) {
                    cart_set_group_selected(d->val->group, true);
                    checked = 0;
                    for (int j = 0; j < sc->doc_count; j++)
                        if (sc->docs[j].selected) checked++;
                } else {
                    d->selected = true;
                    checked++;
                }
                changed = true;
            }
            return changed;
        }

        /* A, B, H, I ? без ограничений. */
        bool changed = false;
        for (int i = 0; i < sc->doc_count; i++) {
            if (!sc->docs[i].selected) {
                sc->docs[i].selected = true;
                changed = true;
            }
        }
        return changed;
    }

    /* ============================================================
     * Обычный клик (без Ctrl).
     * ============================================================ */

    /* --- C, G: одна квитанция, атомарно. --- */
    if (type == CANCEL_NON_CASH_ITEMS || type == CANCEL_REFUND_NON_CASH_ITEMS) {
        int inv = doc->val->k->c;
        if (inv == 0) {
            message_box("Предупреждение",
                "Документ не имеет связанной квитанции. Выбор невозможен.",
                dlg_yes, 0, al_center);
            return false;
        }
        if (!doc->selected) {
            for (int i = 0; i < sc->doc_count; i++) {
                ui_doc_t *d = &sc->docs[i];
                if (d->selected && d->val->k->c != inv) {
                    message_box("Предупреждение",
                        "Можно выбирать только документы,\n"
                        "относящиеся к одной квитанции.",
                        dlg_yes, 0, al_center);
                    return false;
                }
            }
        }
        bool new_val = !doc->selected;
        for (int i = 0; i < sc->doc_count; i++) {
            ui_doc_t *d = &sc->docs[i];
            if (d->val->k->c == inv)
                d->selected = new_val;
        }
        return true;
    }

    /* --- F: не смешивать ОСН и ПВД; группа ? атомарно. --- */
    if (type == REFUND_NON_CASH_ITEMS) {
        if (!doc->selected) {
            int v = doc->val->k->v;

            /* Проверка на несовместимость с уже выделенными */
            for (int i = 0; i < sc->doc_count; i++) {
                ui_doc_t *d = &sc->docs[i];
                if (d->selected && d->val->k->v != v) {
                    message_box("Предупреждение",
                        "Нельзя одновременно выбирать документы ОСН и ПВД\n"
                        "в подкорзине \"КАРТА ВОЗВРАТ\".",
                        dlg_yes, 0, al_center);
                    return false;
                }
            }

            /* Проверка однородности группы */
            if (doc->val->group && doc->val->group->count > 0) {
                for (list_item_t *li = doc->val->group->head; li; li = li->next) {
                    D *gd = LIST_ITEM(li, D);
                    if (gd->k->v != v) {
                        message_box("Предупреждение",
                            "Документы в группе имеют разный вид деятельности (ОСН/ПВД).",
                            dlg_yes, 0, al_center);
                        return false;
                    }
                }
            }
        }
        bool new_val = !doc->selected;
        if (doc->val->group && doc->val->group->count > 0)
            cart_set_group_selected(doc->val->group, new_val);
        else
            doc->selected = new_val;
        return true;
    }

    /* --- D, E: лимит 35, группа ? атомарно. --- */
    if (type == NON_CASH_ITEMS || type == FAST_PAYMENT_ITEMS) {
        if (!doc->selected) {
            int delta = (doc->val->group && doc->val->group->count > 0)
                      ? (int)doc->val->group->count : 1;
            int checked = 0;
            for (int i = 0; i < sc->doc_count; i++)
                if (sc->docs[i].selected) checked++;

            if (checked + delta > MAX_DOCS) {
                message_box("Предупреждение",
                    "Нельзя добавить дополнительные документы:\n"
                    "будет превышен лимит 35.",
                    dlg_yes, 0, al_center);
                return false;
            }
            if (doc->val->group && doc->val->group->count > 0)
                cart_set_group_selected(doc->val->group, true);
            else
                doc->selected = true;
        } else {
            if (doc->val->group && doc->val->group->count > 0)
                cart_set_group_selected(doc->val->group, false);
            else
                doc->selected = false;
        }
        return true;
    }

    /* --- A, B, H, I ? без ограничений. --- */
    doc->selected = !doc->selected;
    return true;
}