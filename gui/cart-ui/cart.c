#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysdefs.h"
#include "kbd.h"
#include "gui/gdi.h"
#include "gui/cart.h"
#include "gui/forms.h"

GCPtr cart_screen = NULL;
FontPtr cart_fnt = NULL;
FontPtr cart_sfnt = NULL;
ui_cart_t *ui_cart = NULL;
ui_subcart_t *ui_sel_subcart = NULL;

void process_docs();
void print_cheque(list_t *klist);


void ui_cart_create()
{
	if (ui_cart != NULL)
	{
		ui_cart_destroy();
	}

	ui_cart = __new(ui_cart_t);
	ui_cart->subcarts = __calloc(MAX_SUB_CART, ui_subcart_t);
	ui_sel_subcart = NULL;

    bool first = true;
	for (int i = 0; i < MAX_SUB_CART; i++)
	{
		SubCart *val = &cart.sc[i];
		if (val->documents.count > 0)
		{
			ui_subcart_t *sc = &ui_cart->subcarts[ui_cart->subcart_count++];
			
			if (first)
			{
			    ui_sel_subcart = sc;
			}
			
			ui_subcart_init(sc, val, first);
			
			first = false;
		}
	}
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

void ui_cart_redraw_all()
{
	ClearGC(cart_screen, clSilver);
	
	ui_cart_calc_bounds();

	int y = CART_YGAP;
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		ui_subcart_draw(sc, y);
		y += sc->height + CART_YGAP;
	}
	
	if (ui_cart->subcart_count == 0)
	{
        draw_button(cart_screen,
            CART_XGAP * 3, CART_YGAP,
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
	int y = CART_YGAP;
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *_sc = &ui_cart->subcarts[i];
		
		if (_sc == sc)
		{
		    return y;
		}
		
		y += sc->height + CART_YGAP;
	}
	
	return 0;
}


static void ui_cart_tab_select_next()
{
    ui_subcart_t *sc = ui_sel_subcart;
    
    if (sc == NULL)
    {
        return;
    }

    if (sc->tab_selected_doc >= 0)
    {
        sc->tab_selected_doc++;
        if (sc->tab_selected_doc >= sc->doc_count)
        {
            sc->tab_selected_doc = -1;
            sc->tab_selected_flags = CART_TAB_SELECTED_ACTION;
        }
    }
    else if (sc->tab_selected_flags == CART_TAB_SELECTED_ACTION)
    {
        sc->tab_selected_flags = CART_TAB_SELECTED_DELETE;
    }
    else if (sc->tab_selected_flags == CART_TAB_SELECTED_DELETE)
    {
        sc->tab_selected_doc = 0;
        sc->tab_selected_flags = CART_TAB_SELECTED_NONE;
    }

    ui_subcart_draw(sc, ui_cart_get_y(sc));
}

void ui_cart_process_enter()
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL)
    {
        return;
    }
    
    if (sc->tab_selected_doc >= 0)
    {
        ui_doc_t *d = &sc->docs[sc->tab_selected_doc];
        d->expanded = !d->expanded;
        
        ui_cart_redraw_all();
    }
    else if (sc->tab_selected_flags == CART_TAB_SELECTED_ACTION)
    {
        process_docs();    
    }
    else if (sc->tab_selected_flags == CART_TAB_SELECTED_DELETE)
    {
    }
}

void ui_cart_process_space()
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL)
    {
        return;
    }
    
    if (sc->tab_selected_doc >= 0)
    {
        ui_doc_t *d = &sc->docs[sc->tab_selected_doc];
        d->selected = !d->selected;
        
        ui_cart_redraw_all();
    }    
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
		switch (e.key) {
			case KEY_ESCAPE:
				return 0;
            case KEY_TAB:
            case KEY_RIGHT:
                ui_cart_tab_select_next();
                break;
            case KEY_SPACE:
                ui_cart_process_space();
                break;
            case KEY_ENTER:
                ui_cart_process_enter();
                break;
        }
	}

	return 1;
}

void get_all_k_from_doc(list_t klist[2])
{
    ui_subcart_t *sc = ui_sel_subcart;
    if (sc == NULL)
    {
        return;
    }
    
    ui_doc_t *d = sc->docs;
    uint8_t kp = 255;

    
    for (size_t i = 0; i < sc->doc_count; i++, d++)
    {
        if (d->selected)
        {
            D* doc = d->val;
            
            for (list_item_t* li = doc->related.head; li; li = li->next)
            {
                K *k = LIST_ITEM(li, K);
                uint8_t p = LIST_ITEM(k->llist.head, L)->p;
                int index;
                
                if (kp == 255)
                {
                    kp = p;
                    index = 0;
                }
                else if (kp == p)
                {
                    index = 0;
                }
                else
                {
                    index = 1;
                }
                
                list_add(&klist[index], k);
            }
        }
    }
}

void process_docs()
{
    list_t klist[2] =
    {
        { NULL, NULL, 0, NULL },
        { NULL, NULL, 0, NULL },
    };
    get_all_k_from_doc(klist);
    
    for (int i = 0; i < 2; i++)
    {
        if (klist->count == 0)
        {
            continue;
        }
        list_t *list = &klist[i];
        
        print_cheque(list);
    }
}

void print_cheque(list_t *klist)
{
	size_t doc_count = 0;
    
}