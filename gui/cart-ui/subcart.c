#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdefs.h"

#include "kbd.h"
#include "kkt/kkt.h"
#include "gui/gdi.h"
#include "gui/cart.h"
#include "gui/forms.h"
#include "gui/controls/button.h"
#include "kkt/kkt.h"
#include "pos/command.h"

bool _support_1222_1224_1225 = false;


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
			strcpy(sc->title, "ОПЛАТА НАЛИЧНЫЕ");
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
	_support_1222_1224_1225 = kkt_has_param("SUPPORT_1222_1224_1225");

	sc->val = val;
	sc->doc_count = val->documents.count;
	sc->docs = __calloc(sc->doc_count, ui_doc_t);
	ui_subcart_set_title(sc);
    sc->tab_selected_doc = -1;
    sc->enabled_flags = 3;
    
	if (tab_selected)
	{
	    sc->tab_selected_flags = CART_TAB_SELECTED_NONE;
	    sc->tab_selected_doc = 0;
//	    if (sc_action_enabled(sc))
//	    {
//    	    sc->tab_selected_flags = CART_TAB_SELECTED_ACTION;
//        }
//        else if (sc_delete_enabled(sc))
//	    {
//    	    sc->tab_selected_flags = CART_TAB_SELECTED_DELETE;
//        }
//        else
//        {
//            sc->tab_selected_doc = 0;
//        }
	}

	int i = 0;
	for (list_item_t *li = val->documents.head; li; li = li->next, i++)
	{
		D *val = LIST_ITEM(li, D);
		ui_doc_init(&sc->docs[i], val, false);
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

	/* --- Статистика выделения: "2 из 35 (200.00 РУБ.)" --- */
	int     sel_count   = 0;
	int     total_count = (int)sc->doc_count;
	int64_t sel_sum     = 0;

	for (int i = 0; i < sc->doc_count; i++) {
		ui_doc_t *d = &sc->docs[i];
		if (!d->selected)
			continue;
		sel_count++;
		S s;
		K_calc_sum(d->val->k, &s);
		sel_sum += s.a;
	}

	char sumbuf[64];
	printsum(sel_sum, sumbuf);

	char stats[128];
	snprintf(stats, sizeof(stats), "%d из %d (%s)",
	         sel_count, total_count, sumbuf);

	/* --- Габариты: заголовок + отступ + статистика --- */
	int tw_title = GetTextWidth(cart_screen, sc->title);
	int tw_stats = GetTextWidth(cart_screen, stats);
	int gap      = CART_XGAP * 3;
	int max_box_w = w - CART_XGAP * 2;
	int desired_w = tw_title + gap + tw_stats + CART_XGAP * 4;
	int box_w = desired_w < max_box_w ? desired_w : max_box_w;

	DrawBorder(cart_screen, CART_XGAP, y, box_w,
	           cart_fnt->max_height + CART_YGAP * 2, 2, clBlack, clBlack);

	y += CART_YGAP;
	int text_x = x + CART_XGAP;

	/* Заголовок ? всегда */
	TextOut(cart_screen, text_x, y, sc->title);

	/* Статистика ? справа от заголовка.
	 * Влезает целиком ? рисуем целиком.
	 * Не влезает ? рисуем обрезанной до "...". */
	int stats_x = text_x + tw_title + gap;
	int box_right = CART_XGAP + box_w - CART_XGAP;

	if (stats_x + tw_stats <= box_right) {
		TextOut(cart_screen, stats_x, y, stats);
	} else if (box_right - stats_x > 30) {
		int avail  = box_right - stats_x;
		int n      = (int)strlen(stats);
		int chars  = (int)((int64_t)n * avail / tw_stats) - 3;
		if (chars < 1) chars = 1;
		if (chars > n - 4) chars = n - 4;
		char truncated[128];
		memcpy(truncated, stats, chars);
		memcpy(truncated + chars, "...", 3);
		truncated[chars + 3] = 0;
		TextOut(cart_screen, stats_x, y, truncated);
	}

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

typedef void (*doc_func_t)(void* obj, ui_subcart_t *sc, ui_doc_t *doc);

void foreach_selected_documents(ui_subcart_t *sc, void *obj, doc_func_t func)
{
    for (int i = 0; i < sc->doc_count; i++)
    {
        ui_doc_t *doc = &sc->docs[i];
        
        if (doc->selected)
        {
            func(obj, sc, doc);
        }
    }
}

typedef struct {
    bool in_processing_state_initialized;
    bool check_state_initialized;
    bool default_in_processing_state;
    bool default_check_state;
    bool same_processing_state;
    bool same_check_state;
    bool non_finished_bank_state;
    bool has_items;
    bool has_unformed;
    bool in_check_state;
    uint8_t p;
    S sum;
} doc_params_t;

void doc_get_params(doc_params_t *p, __attribute__((unused)) ui_subcart_t *sc, ui_doc_t *d)
{
    p->has_items = true;
    
    if (p->p == 0xff)
    {
        p->p = K_lp(d->val->k);
    }
    
    for (list_item_t *li = d->val->related.head; li != NULL; li = li->next)
    {
        K *k = LIST_ITEM(li, K);
        if (d->val->k->u.s != NULL)
        {
            p->has_unformed = true;
        }
        
        if (!p->in_processing_state_initialized)
        {
            p->default_in_processing_state = k->bank_state != BANK_STATE_NONE
                || k->print_state != PRINT_STATE_NONE;
            p->in_processing_state_initialized = true;
        }

        if (!p->check_state_initialized)
        {
            p->default_check_state = k->check_state;
            p->check_state_initialized = true;
        }
        
        if (k->check_state)
        {
            p->in_check_state = true;
        }
        
        if (k->check_state != p->default_check_state)
        {
            p->same_check_state = false;
        }
        
        bool in_processing_state =
            k->bank_state != BANK_STATE_NONE
            || k->print_state != PRINT_STATE_NONE;
        if (in_processing_state != p->default_in_processing_state)
        {
            p->same_check_state = false;
        }
        
        if (k->bank_state != BANK_STATE_SUCCESS)
        {
            p->non_finished_bank_state = true;
            printf("k: %p, k->bank_state = %d\n", k, k->bank_state);
        }
        
        K_add_sum(p->p, k, &p->sum);
    }
}

const char *get_action_text(ui_subcart_t *sc, bool *is_enabled)
{
    doc_params_t p;
    
    memset(&p, 0, sizeof(p));
    
    p.same_processing_state = true;
    p.same_check_state = true;
    p.p = 0xff;
    
    foreach_selected_documents(sc, &p, (doc_func_t)doc_get_params);
    
    bool first_sc = &ui_cart->subcarts[0] == sc;

    /* H/I (OTHER_ITEMS / ERROR_ITEMS) ? печать только для админа,
     * и независимо от позиции в списке (эти подкорзины всегда последние). */
    bool is_other_or_error = (sc->val->type == OTHER_ITEMS
                           || sc->val->type == ERROR_ITEMS);

    bool action_allowed = is_other_or_error
        ? is_admin_mode()
        : first_sc;

    *is_enabled = p.has_items
        && !pos_incomplete_op
        && !p.has_unformed
        && p.same_processing_state
        && p.same_check_state
        && action_allowed;
    
        
    printf("sc->val->type: '%c'\n", sc->val->type);
    printf("p.has_items: %d, pos_incomplete_op: %d, "
			"p.has_unformed: %d, p.same_processing_state: %d, "
			"p.same_check_state: %d, p.non_finished_bank_state: %d, p.in_check_state: %d, "
			"first_sc: %d\n",
        p.has_items,
        pos_incomplete_op,
        p.has_unformed,
        p.same_processing_state,
        p.same_check_state,
		p.non_finished_bank_state,
		p.in_check_state,
        first_sc); 
       
    if ((sc->val->type == NON_CASH_ITEMS
        || sc->val->type == CANCEL_NON_CASH_ITEMS
        || sc->val->type == CANCEL_REFUND_NON_CASH_ITEMS
        || sc->val->type == REFUND_NON_CASH_ITEMS
        || sc->val->type == FAST_PAYMENT_ITEMS)
        && (!p.has_items || p.non_finished_bank_state))
    {
        if (p.sum.e > 0 || !p.has_items || !*is_enabled)
        {
            switch (sc->val->type)
            {
            case NON_CASH_ITEMS:
                return "КАРТА ОПЛАТА";
            case CANCEL_NON_CASH_ITEMS:
            case CANCEL_REFUND_NON_CASH_ITEMS:
                return "КАРТА ОТМЕНА";
            case REFUND_NON_CASH_ITEMS:
                return "КАРТА ВОЗВРАТ";
            case FAST_PAYMENT_ITEMS:
                return p.in_check_state ? "СБП ПРОВЕРКА" : "СБП ОПЛАТА";
            }
        }
    }
    
    return "ПЕЧАТЬ ЧЕКА";
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
	
	bool is_enabled = false;
	const char *text = get_action_text(sc, &is_enabled);
	
	if (is_enabled)
	{
        sc->enabled_flags |= CART_ACTION_ENABLED;
	}
	else
	{
        sc->enabled_flags &= ~CART_ACTION_ENABLED;
	}
	
	bool action_selected = sc->tab_selected_flags == CART_TAB_SELECTED_ACTION;
	bool delete_selected = sc->tab_selected_flags == CART_TAB_SELECTED_DELETE;
	bool action_enabled = sc_action_enabled(sc);
	bool delete_enabled = sc_delete_enabled(sc);

    draw_button_ex(cart_screen, CART_XGAP * 3, y, CART_BUTTON_WIDTH, CART_BUTTON_HEIGHT,  text, action_selected && action_enabled, action_enabled);
    draw_button_ex(cart_screen, x + w - CART_XGAP*4 - CART_BUTTON_WIDTH, y, CART_BUTTON_WIDTH, CART_BUTTON_HEIGHT,  "Удалить", delete_selected && delete_enabled, delete_enabled);
}


bool ui_subcart_items_disabled(ui_subcart_t *sc)
{
    for (int i = 0; i < sc->doc_count; i++)
    {
        ui_doc_t *d = &sc->docs[i];
        
        if (d->val->k->bank_state != BANK_STATE_NONE
            || d->val->k->print_state != PRINT_STATE_NONE
            || d->val->k->check_state)
        {
            return true;
        }
        
        for (list_item_t *li = d->val->related.head; li; li = li->next)
        {
            K *k = LIST_ITEM(li, K);
            
            if (k->print_state == PRINT_STATE_PRINTING)
            {
                return true;
            }
        }
    }
    
    return false;
}

size_t ui_subcart_get_all_k(ui_subcart_t *sc, list_t *list)
{
    size_t count = 0;
    for (int i = 0; i < sc->doc_count; i++)
    {
        ui_doc_t *d = &sc->docs[i];
        
        for (list_item_t *li = d->val->related.head; li; li = li->next)
        {
            K *k = LIST_ITEM(li, K);

            list_add(list, k);
            count++;
        }
    }
    
    return count;
}
