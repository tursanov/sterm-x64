#include "kbd.h"
#include <sys/param.h>
#include <ctype.h>
#include "paths.h"
#include "gui/gdi.h"
#include "gui/cheque.h"
#include "gui/forms.h"
#include "gui/fa.h"
#include "gui/dialog.h"
#include "kkt/fd/ad.h"
#include <string.h>
#include <stdlib.h>

static FontPtr fnt = NULL;
static FontPtr fnt1 = NULL;
static GCPtr screen = NULL;
extern int kbd_lang_ex;
extern int kbd_get_char_ex(int key);
static int active_button = 0;
static C *current_c = NULL;
static list_item_t *active_item = NULL;
static int active_item_n = 0;
static int active_item_child = 0;
static int64_t sumN = 0;
static int64_t sumE = 0;
static bool has_cashless_payments = false;
static bool has_cash_payments = false;
static list_item_t *first = NULL;
static int first_n = 0;
static bool main_view = true;
static int expanded_top_n = 0;
static bool scroll_enabled = false;
static int draw_flags = 0;

#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

static void calc_sum()
{
	sumN = 0;
	sumE = 0;
	has_cashless_payments = false;
	has_cash_payments = false;

	for (list_item_t *li1 = _ad->clist.head; li1 != NULL; li1 = li1->next) {
		C *c = LIST_ITEM(li1, C);

		int64_t n = 0;
		int64_t e = 0;

		for (list_item_t *li2 = c->klist.head; li2 != NULL; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			if (!doc_no_is_empty(&k->u))
				continue;

			for (list_item_t *li3 = k->llist.head; li3 != NULL; li3 = li3->next) {
				L *l = LIST_ITEM(li3, L);
				switch (k->m) {
				case 1:
					if (l->t > 0)
						has_cash_payments = true;
					n += l->t;
					break;
				case 2:
					if (l->t > 0)
						has_cashless_payments = true;
					e += l->t;
					break;
				}
			}

			if (k->a > 0) {
				switch (k->m) {
				case 1:
					n -= k->a;
					break;
				case 2:
					e -= k->a;
					break;
				}
			}
		}

		if (c->t1054 == 1 || c->t1054 == 4) {
			sumN += n;
			sumE += e;
		} else {
			sumN -= n;
			sumE -= e;
		}
	}
}

int cheque_init(void) {
	if (fnt == NULL)
		fnt = CreateFont(_("fonts/fixedsys8x16.fnt"), false);
	if (fnt1 == NULL)
		fnt1 = CreateFont(_("fonts/terminal10x18.fnt"), false);

	if (screen == NULL)
	  	screen = CreateGC(0, 0, DISCX, DISCY);
	SetFont(screen, fnt);

	active_item = NULL;
	active_item_n = 0;
	active_item_child = 0;
	if (!_ad->clist.head)
		active_button = 1;
	else
		active_button = 0;

	first = _ad->clist.head;
	first_n = 0;
	main_view = true;

	calc_sum();

	cheque_draw();
	current_c = NULL;

	return 0;
}

void cheque_release(void) {
	if (fnt) {
		DeleteFont(fnt);
		fnt = NULL;
	}
	if (fnt1) {
		DeleteFont(fnt1);
		fnt1 = NULL;
	}
	if (screen) {
		DeleteGC(screen);
		screen = NULL;
	}
}

#define GAP 10

#define IR_COLOR	clGray

static char *get_str_pay_method(K *k) {
	switch (k->m) {
		case 1:
			return "НАЛ";
		case 2:
			return "БЕЗНАЛ";
		case 3:
			return "Д/ПР";
		default:
			return "???";
	}
}

static int email_or_phone_draw(C *c, int start_y) {
	int y = start_y + fnt->max_height*5 + GAP + 4;
	char email_or_phone[128];
	const char *title = "Тел. или e-mail покупателя:    ";
	sprintf(email_or_phone, "%s", c->pe ? c->pe : "не указан");
	int tw = TextWidth(fnt, title);
	int tw2 = TextWidth(fnt, email_or_phone);
	Color selectedColor = (active_item && LIST_ITEM(active_item, C) == c &&
			active_item_child == 0) ? clRopnetDarkBrown : clSilver;

	SetTextColor(screen, c->pe ? clBlack : IR_COLOR);
	TextOut(screen, GAP*2, y, title);
	TextOut(screen, GAP*2 + tw, y, email_or_phone);

	fill_rect(screen, GAP*2 + tw - 2, y - 2, tw2 + 4, fnt->max_height + 4, 2, selectedColor, 0);

	y += fnt->max_height;

	return y;
}

static int doc_view_collapsed_draw(C *c, int start_y) {
	int y = start_y;
	char text[256];
	char docs[128];
	int n = 0;
	char *p = docs;

	for (list_item_t *li1 = c->klist.head; li1 != NULL && n < 4; li1 = li1->next, n++) {
		K *k = LIST_ITEM(li1, K);
		p += sprintf(p, "%s%s", (k->d.s ? k->d.s : ""),
			(n < 3 ? (li1->next ? ", " : "") : (li1->next ? "..." : "")));
	}

	snprintf(text, sizeof(text), "Просмотр документов чека (%zu) [%s]", c->klist.count, docs);
	int tw = TextWidth(fnt, text);
	Color selectedColor = (active_item && LIST_ITEM(active_item, C) == c &&
			active_item_child == 1) ? clRopnetDarkBrown : clSilver;

	SetTextColor(screen, clBlack);
	TextOut(screen, GAP*2, y, text);

	fill_rect(screen, GAP*2 - 2, y - 2, tw + 4, fnt->max_height + 4, 2, selectedColor, 0);

	y += fnt->max_height + GAP;

	return y;
}

static int doc_view_expanded_draw(C *c, int start_y) {
	int y = start_y;
	char text[2048];

	SetTextColor(screen, clBlack);

	int i = 0;
	for (list_item_t *li1 = c->klist.head; li1 != NULL; li1 = li1->next, i++) {
		if (i < expanded_top_n)
			continue;

		K *k = LIST_ITEM(li1, K);
		unsigned long long sum = 0;

		int y1 = y + fnt->max_height;
		for (list_item_t *li2 = k->llist.head; li2 != NULL; li2 = li2->next) {
			L *l = LIST_ITEM(li2, L);
			sum += l->t;
			y1 += fnt->max_height;
		}

#define MAX_Y	540
		if (y1 > MAX_Y) {
			scroll_enabled = true;
			break;
		} else
			scroll_enabled = false;

		sprintf(text, "Документ N%s (СУММА: %.1lld.%.2lld %s)%s",
			k->d.s ? k->d.s : "",
			sum / 100, sum % 100,
			get_str_pay_method(k),
			doc_no_is_empty(&k->u) ? "" : " (НЕЗАВЕРШЕННОЕ ПЕРЕОФОРМЛЕНИЕ)");
		TextOut(screen, GAP*2, y, text);
		y += fnt->max_height;
		
		for (list_item_t *li2 = k->llist.head; li2 != NULL; li2 = li2->next) {
			const char *svat[] = {
				"НДС 20%",
				"НДС 10%",
				"НДС 20/120",
				"НДС 10/110",
				"",
				"",
            	"НДС 5%",
            	"НДС 7%",
            	"НДС 5/105",
            	"НДС 7/107"
			};
			char *p = text;
			L *l = LIST_ITEM(li2, L);
			p += sprintf(p, "%s: %.1lld.%.2lld", l->s, (long long)l->t / 100, (long long)l->t % 100);
			if ((l->n >= 1 && l->n <= 4) || (l->n >= 7 && l->n <= 10)) {
				sprintf(p, " (в т.ч. %s: %.1lld.%.2lld)", svat[l->n - 1],
					(long long)l->c / 100, (long long)l->c % 100);
			}
			
			#define MAX_CH 90
			int len = strlen(text);
			p = text;
			for (int i = 0; i < len; i+= MAX_CH, p += MAX_CH)
			{
			    TextOutN(screen, GAP*4, y, p, MAX_CH);
			    y += fnt->max_height;
			}
		}
	}
	if (scroll_enabled || expanded_top_n > 0) {
	    SetFont(screen, fnt1);
	    if (expanded_top_n > 0)
			TextOut(screen, DISCX - 30, 140, "\x1e");
		if (scroll_enabled)
			TextOut(screen, DISCX - 30, DISCY - 100, "\x1f");
	    SetFont(screen, fnt);
	}
	y = MAX_Y;

	return y;
}

static int cheque_draw_cheque(C *c, int n, int start_y, bool doc_info_collapsed_view) {
	const char *cheque_type[] = { "Приход", "Возврат прихода", "Расход", "Возврат расхода" };
	const char *payout_kind[] = { "Наличными:", "Безналичными:", 
		"В зачет ранее внесенных средств:", "Встречным предоставлением:", "ИТОГО:" };
	int64_t s[5] = { c->sum.n, c->sum.e, c->sum.p, c->sum.b, c->sum.a };
	char ss[5][32];
	char cheque_title[32];
	char cheque_n[32];
	int sw;
	int x, w, w1, h;
	int y = start_y;

	sprintf(cheque_title, "Чек (%s)", cheque_type[c->t1054 - 1]);
	sprintf(cheque_n, "%d", n + 1);
	w = TextWidth(fnt, cheque_title);
	w1 = TextWidth(fnt, cheque_n);
	h = fnt->max_height + GAP;
	x = (DISCX - w - GAP*5);

	if (draw_flags == 0) {
		fill_rect(screen, x - GAP, y, w + GAP*5, h, 2, clGray, clSilver);
		SetTextColor(screen, clBlack);
		TextOut(screen, x, y + GAP/2, cheque_title);
		SetBrushColor(screen, clGray);
		FillBox(screen, DISCX - w1 - GAP * 3, y + GAP/2 - 4, 2, h - 3);
		TextOut(screen, DISCX - w1 - GAP * 2, y + GAP/2, cheque_n);
	}

	y += GAP;

	w = 0;
	sw = 0;
	for (int i = 0; i < ASIZE(payout_kind); i++) {
		sprintf(ss[i], "%.1lld.%.2lld", (long long)s[i] / 100, (long long)s[i] % 100);
		int tw = TextWidth(fnt, payout_kind[i]);
		if (tw > w)
			w = tw;
		tw = TextWidth(fnt, ss[i]);
		if (tw > sw)
			sw = tw;
	}

	for (int i = 0; i < ASIZE(payout_kind); i++) {
		if (draw_flags == 0) {
			int tw = TextWidth(fnt, ss[i]);
			SetTextColor(screen, s[i] > 0 ? clBlack : IR_COLOR);
			TextOut(screen, GAP*2, y, payout_kind[i]);
			TextOut(screen, GAP*3 + w + sw - tw, y, ss[i]);
		}
		y += fnt->max_height;
		if (i == ASIZE(payout_kind) - 2)
			y += 4;
	}

	if (draw_flags == 0)
		y = email_or_phone_draw(c, start_y);
	else
		y = fnt->max_height*7 + GAP + 10;

	if (doc_info_collapsed_view)
		y = doc_view_collapsed_draw(c, y);
	else
		y = doc_view_expanded_draw(c, y);

	if (draw_flags == 0)
		fill_rect(screen, 10, start_y, DISCX - 20, y - start_y, 2, clGray, 0);
	y += GAP;

	return y;
}

static int vln_printf(char *buffer, int64_t v) {
	return sprintf(buffer, "%.lld.%.2lld", (long long)v / 100, (long long)v % 100);
}

static int cheque_draw_sum(int start_y) {
	char title[2][256] = { {0}, {0} };
	char value[2][64] = { {0}, {0} };
	int count = 0;
	int sw;
	int w;
	int y = start_y;

	//printf("sumN: %lld, sumE: %lld\n", sumN, sumE);

	if (sumN == 0 && !has_cashless_payments) {
		sprintf(title[count], "Получение или выдача денежных средств не требуется");
		count++;
	} else {
		if (sumN != 0) {
			sprintf(title[count], "Сумма наличными к %s:", sumN > 0 ? "получению от пассажира" : 
				"выдаче пассажиру");
			vln_printf(value[count], labs(sumN));
			count++;
		}
		if (has_cashless_payments) {
			sprintf(title[count], "Расчет безналичными согласно данным чека(ов)");
			value[count][0] = 0;
			count++;
		}
	}

/*	sprintf(cheque_title, "Чек (%s)", cheque_type[c->t1054 - 1]);
	w = TextWidth(fnt, cheque_title);
	h = fnt->max_height + GAP;
	x = (DISCX - w - GAP*2);
	fill_rect(x - GAP, y, w + GAP*2, h, 2, clGray, clSilver);
	SetTextColor(screen, clBlack);
	TextOut(screen, x, y + GAP/2, cheque_title);
	y += GAP;*/

	y += 4;

	w = 0;
	sw = 0;
	for (int i = 0; i < count; i++) {
		int tw = TextWidth(fnt, title[i]);
		if (tw > w)
			w = tw;
		tw = TextWidth(fnt, value[i]);
		if (tw > sw)
			sw = tw;
	}

	for (int i = 0; i < count; i++) {
		SetTextColor(screen, clBlack);
		TextOut(screen, GAP*2, y, title[i]);
		if (value[i][0] != 0)
			TextOut(screen, GAP*3 + w, y, value[i]);
		y += fnt->max_height;
	}
	y += 4;

	fill_rect(screen, 10, start_y, DISCX - 20, y - start_y, 2, clGray, 0);
	y += GAP;

	return y;
}

static int cheque_draw_cashier(int start_y) {
	char text[512];
	char *p = text;

	p += sprintf(text, "Кассир: %s", cashier_get_name());

	if (cashier_get_post()[0])
		p += sprintf(p, ", Должность: %s", cashier_get_post());

	if (cashier_get_inn()[0])
		p += sprintf(p, ", ИНН: %s", cashier_get_inn());

	int x = 10;
	int y = start_y + (BUTTON_HEIGHT - fnt->max_height) / 2;

	SetTextColor(screen, clBlack);
	TextOut(screen, x, y, text);

	x += TextWidth(fnt, text) + 10;

	draw_button(screen, x, start_y, BUTTON_WIDTH, BUTTON_HEIGHT, "Изменить", active_button == 2);

	return start_y + BUTTON_HEIGHT + 4;
}


#define MAX_CHEQUE_PER_PAGE	3
static int cheque_main_draw() {
	int x;
	int y = 8;
	if (first) {
		char title[256];
		sprintf(title, "Всего чеков: %zd текущая страница (с %d по %zd чек)",
			_ad->clist.count, first_n + 1, MIN(first_n + MAX_CHEQUE_PER_PAGE, _ad->clist.count));

		draw_title(screen, fnt, title);

		y += 20;
		y = cheque_draw_cashier(y);

		size_t n = 0;
		for (list_item_t *i1 = first; i1 && n < MAX_CHEQUE_PER_PAGE; i1 = i1->next, n++) {
			C *c = LIST_ITEM(i1, C);
			y = cheque_draw_cheque(c, first_n + n, y, true);
		}

		y = cheque_draw_sum(y);

		x = ((DISCX - (BUTTON_WIDTH*2 + GAP)) / 2);
		draw_button(screen, x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Печать", active_button == 0);
	} else {
		const char *text = "Нет документов для печати";
		SetTextColor(screen, clBlack);

		x = (DISCX - TextWidth(fnt, text)) / 2;

		TextOut(screen, x, y, text);

		x = ((DISCX - BUTTON_WIDTH) / 2) - BUTTON_WIDTH + GAP;
		y += fnt->max_height + 8;
	}

	draw_button(screen,  x + BUTTON_WIDTH + GAP, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Отмена", active_button == 1);

	return 0;
}

static void cheque_info_draw() {
	int x = (DISCX - BUTTON_WIDTH) / 2;
	int y = GAP*2;
	C *c = LIST_ITEM(active_item, C);

	if (draw_flags == 0) {
		char title[256];
		sprintf(title, "Просмотр информации чека %d", active_item_n + 1);
		draw_title(screen, fnt, title);
	}

	y = cheque_draw_cheque(c, active_item_n, y + 2, false);

	if (draw_flags == 0) {
		draw_button(screen, x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Закрыть", true);
	}
}

int cheque_draw_ex(int flags) {
	draw_flags = flags;
	if (draw_flags == 0)
		ClearGC(screen, clSilver);
	else {
		SetBrushColor(screen, clSilver);
		FillBox(screen, 20, 134, DISCX - 40, DISCY - 200);
	}
	if (main_view)
		cheque_main_draw();
	else
		cheque_info_draw();
	draw_flags = 0;
	return 0;
}

int cheque_draw() {
	cheque_draw_ex(0);
	return 0;
}

static void next_focus() {
	if (active_button == 0)
		active_button = 1;
	else if (active_button == 1)
		active_button = 2;
	else if (active_button == 2) {
		if (first) {
			active_item = first;
			active_item_n = first_n;
			active_item_child = 0;
			active_button = -1;
		} /*else {
			active_button = 0;
		}*/
	} else {
		if (active_item_child == 0)
			active_item_child = 1;
		else {
			if (++active_item_n >= first_n + MAX_CHEQUE_PER_PAGE ||
					active_item_n >= _ad->clist.count) {
				active_item = NULL;
				active_button = 0;
			} else {
				if (active_item != NULL)
					active_item = active_item->next;
				else {
					active_item = first;
					active_item_n = first_n;
				}
				active_item_child = 0;
			}
		}
	}
	cheque_draw();
}

static void prev_focus() {
	if (active_button == 2)
		active_button = 1;
	else if (active_button == 1)
		active_button = 0;
	else if (active_button == 0) {
		if (first) {
			active_item = first;
			active_item_n = first_n;
			active_item_child = 1;
			active_button = -1;
			for (int i = 0; i < MAX_CHEQUE_PER_PAGE - 1; i++) {
				if (active_item->next) {
					active_item = active_item->next;
					active_item_n++;
					active_item_child = 1;
				}
			}
		} else {
			active_button = 2;
		}
	} else {
		if (active_item_child == 1)
			active_item_child = 0;
		else {
			if (active_item_n > first_n) {
				active_item_n--;
				int i = first_n;
				for (active_item = first; i < active_item_n; active_item = active_item->next, i++);
				active_item_child = 1;
			} else {
				active_item = NULL;
				active_button = 2;
			}
		}
	}
	cheque_draw();
}

static bool check_phone(const char *s) {
	if (*s++ != '+')
		return false;
	for (; *s; s++)
		if (!isdigit(*s))
			return false;
	return true;
}

static bool check_email(const char *s) {
	int left = 0;
	int at = 0;
	int right = 0;

	for (; *s; s++) {
		if (*s == '@') {
			if (at++ > 0)
				return false;
		} else if (!at)
			left++;
		else
			right++;
	}

	return left > 0 && right > 0;
}

bool check_phone_or_email(const char *pe, bool allowNone) {
	if (pe[0] == '+')
		return check_phone(pe);

	if (allowNone && strcmp(pe, "none") == 0)
		return true;

	return check_email(pe);
}

static void select_phone_or_email() {
	C *c = LIST_ITEM(active_item, C);
	form_t *form = NULL;
	char title[256];
	size_t item_count = _ad->phones.count + _ad->emails.count;
	char **items = (char **)malloc(sizeof(char *) * item_count);
	size_t n = 0;

	for (size_t i = 0; i < _ad->phones.count; i++, n++)
		items[n] = _ad->phones.values[i];
	for (size_t i = 0; i < _ad->emails.count; i++, n++)
		items[n] = _ad->emails.values[i];

	sprintf(title, "Тел. или e-mail (%zd):", item_count);

	BEGIN_FORM(form, "Ввод телефона или e-mail получателя чека")
		FORM_ITEM_EDIT_COMBOBOX(1008, title, c->pe, FORM_INPUT_TYPE_TEXT, 64,
			(const char **)items, item_count)
		FORM_ITEM_BUTTON(1, "ОК")
		FORM_ITEM_BUTTON(0, "Отмена")
	END_FORM()

	free(items);

	kbd_lang_ex = lng_lat;
	while (form_execute(form) == 1) {
		form_data_t data;
		form_get_data(form, 1008, 1, &data);

		if (data.size > 0 && !check_phone_or_email((const char *)data.data, false)) {
			message_box("Ошибка", "Номер тел. или e-mail имеют недопустимый формат.\n"
				"Пример ввода: +71111111111 или name@mail.ru", dlg_yes, 0, al_center);
			form_draw(form);
			form_focus(form, 1008);

			continue;
		}

		if (c->pe)
		   free(c->pe);
		if (data.size > 0)
			c->pe = strdup((char *)data.data);
		else
			c->pe = NULL;
		AD_save();
		break;
	}

	cheque_draw();
	form_destroy(form);
}

static void show_doc_info() {
	main_view = false;
	expanded_top_n = 0;
	scroll_enabled = false;
	cheque_draw();
}

static void change_cashier() {
	form_t *form = NULL;

	BEGIN_FORM(form, "Изменение данных кассира")
		FORM_ITEM_EDIT_TEXT(1021, "Кассир:", cashier_get_name(), FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "Должность кассира:", cashier_get_post(), FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "ИНН Кассира:", cashier_get_inn(), FORM_INPUT_TYPE_NUMBER, 12)

		FORM_ITEM_BUTTON(1, "ОК")
		FORM_ITEM_BUTTON(0, "Отмена")
	END_FORM()

	kbd_lang_ex = lng_lat;
	while (form_execute(form) == 1) {
		form_data_t cashier;
		form_data_t post;
		form_data_t inn;

		form_get_data(form, 1021, 1, &cashier);
		form_get_data(form, 9999, 1, &post);
		form_get_data(form, 1203, 1, &inn);

		if (cashier.size == 0) {
			form_focus(form, 1021);
			message_box("Ошибка", "Обязательное поле не заполнено", dlg_yes, 0, al_center);
			form_draw(form);
			continue;
		}

		if (inn.size > 0 && inn.size != 12 && inn.size != 10) {
			form_focus(form, 1203);
			message_box("Ошибка", "Длина поля \"ИНН кассира\" должна быть 10 или 12 цифр", dlg_yes, 0, al_center);
			form_draw(form);
			continue;
		}

		cashier_set(cashier.data, post.data, inn.data);
		break;
	}

	cheque_draw();
	form_destroy(form);
}

static bool cheque_process(const struct kbd_event *_e) {
	struct kbd_event e = *_e;

	if (e.key == KEY_CAPS && e.pressed && !e.repeated) {
		if (kbd_lang_ex == lng_rus)
			kbd_lang_ex = lng_lat;
		else
			kbd_lang_ex = lng_rus;
	}

	e.ch = kbd_get_char_ex(e.key);

	if (e.pressed) {
		if (main_view) {
			switch (e.key) {
				case KEY_ESCAPE:
					current_c = NULL;
					return 0;
				case KEY_TAB:
				case KEY_DOWN:
				case KEY_UP:
					if ((e.key == KEY_TAB && (e.shift_state & SHIFT_SHIFT) == 0) 
							|| e.key == KEY_DOWN)
						next_focus();
					else
						prev_focus();
					return 1;
				case KEY_ENTER:
					if (active_button == 0) {
						current_c = LIST_ITEM(_ad->clist.head, C);
						return 0;
					} else if (active_button == 1) {
						current_c = NULL;
						return 0;
					} else if (active_button == 2) {
						change_cashier();
					} else if (active_item_child == 0) {
						select_phone_or_email();
					} else if (active_item_child == 1) {
						show_doc_info();
					}
					break;
				case KEY_RIGHT:
				case KEY_PGDN:
					if (first) {
						list_item_t *li = first;
						int li_n = first_n;
						for (int i = 0; li && i < MAX_CHEQUE_PER_PAGE; i++, li = li->next, li_n++);
						if (!li)
							break;
						first = li;
						first_n = li_n;
						if (active_button == -1) {
							active_item = first;
							active_item_n = first_n;
						}
						cheque_draw();
					}
					break;
				case KEY_LEFT:
				case KEY_PGUP:
					if (first) {
						if (!first_n)
							break;
						first_n -= MAX_CHEQUE_PER_PAGE;
						first = _ad->clist.head;
						if (first_n < 0)
							first_n = 0;
						else
							for (int i = 0; i < first_n; i++, first = first->next, i++);
						if (active_button == -1) {
							active_item = first;
							active_item_n = first_n;
						}
						cheque_draw();
					}
					break;
			}
		} else {
			switch (e.key) {
				case KEY_DOWN:
					if (scroll_enabled) {
						expanded_top_n++;
						cheque_draw_ex(1);
					}
					break;
				case KEY_UP:
					if (expanded_top_n > 0) {
						expanded_top_n--;
						cheque_draw_ex(1);
					}
					break;
				case KEY_ESCAPE:
				case KEY_ENTER:
					main_view = true;
					cheque_draw();
					break;
			}
		}
	}

	return 1;
}

bool cheque_execute(void) {
	struct kbd_event e;
	int ret;

	kbd_flush_queue();

	do {
		kbd_get_event(&e);
	} while ((ret = cheque_process(&e)) > 0);

	return current_c != NULL;
}


void cheque_sync_first(void) {
	int i = 0;
	for (first = _ad->clist.head; first && i < first_n; i++, first = first->next);
}

void cheque_begin_op(const char *title) {
	SetGCBounds(screen, 0, 0, DISCX, DISCY);

	SetTextColor(screen, clBlack);
	fill_rect(screen, 100, 240, DISCX - 100*2, DISCY - 240*2, 2, clRopnetDarkBrown, clRopnetBrown);
	DrawText(screen, 100, 240, DISCX - 100*2, DISCY - 240*2, title, DT_CENTER | DT_VCENTER);
}

void cheque_end_op() {
	cheque_draw();
}
