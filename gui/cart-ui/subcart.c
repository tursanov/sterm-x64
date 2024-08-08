#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdefs.h"
#include "kbd.h"
#include "gui/gdi.h"
#include "gui/cart.h"
#include "gui/forms.h"

static const char* sc_tab_title[MAX_SUB_CART][CART_MAX_TAB_COL] =
{
	{ "N документа", "Дата и время", "Операция", "", "Сумма " },
	{ "N документа", "Дата и время", "Операция", "", "Сумма " },
	{ "N документа", "Дата и время", "Операция", "N квитанции", "Сумма " },
	{ "N документа", "Дата и время", "Операция", "N заказа", "Сумма " },
	{ "N документа", "Дата и время", "Операция", "N заказа", "Сумма " },
	{ "N документа", "Дата и время", "Операция", "", "Сумма " },
	{ "N документа", "Дата и время", "Операция", "N квитанции", "Сумма " },
	{ "N документа", "Дата и время", "Операция", "N заказа", "Сумма " },
	{ "N документа", "Дата и время", "Операция", "N заказа", "Сумма " },
};

float cart_tab_fr[CART_MAX_TAB_COL] = { 1, 1, 1.4, 0.6, 1 };


static void ui_subcart_set_title(ui_subcart_t* sc)
{
	switch (sc->val->type)
	{
		case 'A':
			strcpy(sc->title, "ОПЛАТА НАЛИЧН");
			break;
		case 'B':
			strcpy(sc->title, "ВОЗВРАТ НАЛИЧНЫЕ/ОТМЕНА НАЛИЧНЫЕ/ВОЗВРАТ НА КАРТУ ПЛАТЕЛЬЩИКА");
			break;
		case 'C':
			strcpy(sc->title, "ОТМЕНА КВИТАНЦИЙ ОПЛАТЫ");
			break;
		case 'D':
			strcpy(sc->title, "КАРТА ОПЛАТА/ДОПЛАТА");
			break;
		case 'E':
			strcpy(sc->title, "СБП ОПЛАТА/ДОПЛАТА");
			break;
		case 'F':
			strcpy(sc->title, "КАРТА ВОЗВРАТ");
			break;
		case 'G':
			strcpy(sc->title, "ОТМЕНА ВОЗВРАТА НА КАРТУ");
			break;
		case 'H':
			strcpy(sc->title, "ОТМЕНА НЕВОЗМОЖНА");
			break;
		default:
			strcpy(sc->title, "ОШИБКА");
			break;
	}
}



void ui_subcart_init(ui_subcart_t *sc, SubCart *val, bool tab_selected)
{
	sc->val = val;
	sc->doc_count = val->documents.count;
	sc->docs = __calloc(sc->doc_count, ui_doc_t);
	ui_subcart_set_title(sc);
    sc->tab_selected_doc = -1;
    
	if (tab_selected)
	{
	    sc->tab_selected_flags = CART_TAB_SELECTED_ACTION;
	}

	int i = 0;
	for (list_item_t *li = val->documents.head; li; li = li->next, i++)
	{
		D *val = LIST_ITEM(li, D);
		ui_doc_init(&sc->docs[i], val, tab_selected);
	}
}

void ui_subcart_free(ui_subcart_t *sc)
{
	free(sc->docs);
}

void ui_subcart_calc_bounds(ui_subcart_t *sc)
{
	int h = 0;

	h += CART_YGAP * 2; // отступ от краёв
	h += cart_fnt->max_height; // заголовок подкорзины
	h += CART_YGAP; // отступ
	h += cart_fnt->max_height; // заголовок таблицы

	for (int i = 0; i < sc->doc_count; i++)
	{
		ui_doc_t *d = &sc->docs[i];
		ui_doc_calc_bounds(d);
		h += d->height + CART_YGAP_DOC;
	}

	h += CART_BUTTON_HEIGHT + CART_YGAP * 3;
	
	sc->height = h;
	sc->tab_ofs_x = CART_XGAP;
}



static int ui_subcart_header_with_box_draw(ui_subcart_t *sc, int x, int y, int w)
{
	fill_rect(cart_screen, x, y, w, sc->height, BORDER_WIDTH, clBlack, 0);
	
	SetTextColor(cart_screen, clBlack);
	
	int tw = GetTextWidth(cart_screen, sc->title);
    DrawBorder(cart_screen, CART_XGAP, y, tw + CART_XGAP * 4, cart_fnt->max_height + CART_YGAP * 2, 2, clBlack, clBlack);
    
	y += CART_YGAP;
	TextOut(cart_screen, x + CART_XGAP, y, sc->title);
	
	
	y += CART_YGAP * 2 + cart_fnt->max_height;

	return y;
}


static int ui_subcart_table_header_draw(ui_subcart_t *sc, int x, int y, int col_width)
{
	const char **coltext = sc_tab_title[SUB_CART_INDEX(sc->val->type)];
	float *fr = cart_tab_fr;

	int flags = DT_LEFT | DT_VCENTER;

	for (int i = 0; i < CART_MAX_TAB_COL; i++, coltext++, fr++)
	{
		int w = (float)col_width * fr[0];
		if (i == CART_MAX_TAB_COL - 1)
			flags = DT_RIGHT | DT_VCENTER;

		const char *text = coltext[0];
		if (*text)
		{
			DrawText(cart_screen, x, y, w, cart_fnt->max_height, text, flags);
		}
		x += w;
	}

	y += cart_fnt->max_height + CART_YGAP;

	return y;
}

void ui_subcart_draw(ui_subcart_t *sc, int y)
{
	int x = CART_XGAP;
	int w = DISCX - CART_XGAP * 2;
	int tw = w - sc->tab_ofs_x - CART_XGAP * 2;
	int tcw = tw / CART_MAX_TAB_COL;

	y = ui_subcart_header_with_box_draw(sc, x, y, w);
	x += CART_XGAP + sc->tab_ofs_x;
	y = ui_subcart_table_header_draw(sc, x, y, tcw);
	
	for (int i = 0; i < sc->doc_count; i++)
	{
		ui_doc_t *d = &sc->docs[i];
		y = ui_doc_draw(sc, d, x, y, tcw);
	}
	
	bool action_selected = sc->tab_selected_flags == CART_TAB_SELECTED_ACTION;
	bool delete_selected = sc->tab_selected_flags == CART_TAB_SELECTED_DELETE;
	
    draw_button(cart_screen, CART_XGAP * 3, y, CART_BUTTON_WIDTH, CART_BUTTON_HEIGHT,  "Печать чека", action_selected);
    draw_button(cart_screen, x + w - CART_XGAP*4 - CART_BUTTON_WIDTH, y, CART_BUTTON_WIDTH, CART_BUTTON_HEIGHT,  "Удалить", delete_selected);
}

