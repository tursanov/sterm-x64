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

#include "kkt/fd/fd.h"
#include "kkt/fd/tlv.h"
#include "kkt/kkt.h"
#include "kkt/fdo.h"

bool has_unprocessed_operations = false;

static char cashier_name[64+1] = {0};
static char cashier_post[64+1] = {0};
static char cashier_inn[12+1] = {0};
static char cashier_cashier[64+1] = {0};

static void make_cashier() {
	size_t cashier_name_size = strlen(cashier_name);
	size_t cashier_post_size = strlen(cashier_post);
	size_t cashier_cashier_size = cashier_name_size;

	memcpy(cashier_cashier, cashier_name, cashier_name_size);
	if (cashier_post_size > 0 && cashier_name_size < 63) {
		size_t l = 64 - cashier_name_size - 1;
		l = MIN(l, cashier_post_size);

		cashier_cashier[cashier_name_size] = ' ';
		memcpy(cashier_cashier + cashier_name_size + 1, cashier_post, l);
		cashier_cashier_size += l + 1;
	}
	cashier_cashier[cashier_cashier_size] = 0;
}

static uint8_t rereg_data[2048];
static size_t rereg_data_len = sizeof(rereg_data);
static uint8_t reg_tax_systems = 0;
static int64_t user_inn = 0;


static int fa_get_reregistration_data() {
	int ret;
	rereg_data_len = sizeof(rereg_data);
	if ((ret = kkt_get_last_reg_data(rereg_data, &rereg_data_len)) == 0 && rereg_data_len > 0) {
		for (const ffd_tlv_t *tlv = (ffd_tlv_t *)rereg_data,
				*end = (ffd_tlv_t *)(rereg_data + rereg_data_len);
				tlv < end;
				tlv = FFD_TLV_NEXT(tlv)) {
			switch (tlv->tag) {
				case 1018:
					user_inn = atoll(FFD_TLV_DATA_AS_STRING(tlv));
					break;
				case 1062:
					reg_tax_systems = FFD_TLV_DATA_AS_UINT8(tlv);
					break;
			}
		}
	}

	return ret;
}

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

void process_docs();
void print_cheque(SubCart *sc, list_t *klist);
void ui_select_next_subcart();
void get_doc_selection(list_t* sel);
void ui_cart_select_documents();


void ui_cart_create()
{
    make_cashier();
	fa_get_reregistration_data();

	if (ui_cart != NULL)
	{
		ui_cart_destroy();
	}

	ui_cart = __new(ui_cart_t);
	ui_cart->subcarts = __calloc(MAX_SUB_CART, ui_subcart_t);
	ui_sel_subcart = NULL;
	ui_sel_subcart_index = -1;

    bool first = true;
	for (int i = 0; i < MAX_SUB_CART; i++)
	{
		SubCart *val = &cart.sc[i];
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

    for (int i = 0; i < ui_cart->subcart_count; i++)
    {
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		
		for (int j = 0; j < sc->doc_count; j++)
		{
            ui_doc_t *d = &sc->docs[j];
            
            for (list_item_t *li = sel.head; li != NULL; li = li->next)
            {
                D *x = LIST_ITEM(li, D);
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

void ui_cart_redraw_all()
{
	ClearGC(cart_screen, clSilver);
	
	ui_cart_calc_bounds();

	int y = CART_YGAP;
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		printf("ui_cart_redraw_all: i: %d, y: %d, sc->height: %d\n", i, y, sc->height);
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
    		printf("ui_cart_get_y: i: %d, y: %d\n", i, y);
		    return y;
		}
		
		y += _sc->height + CART_YGAP;
   		printf("ui_cart_get_y: sc->height: %d\n", _sc->height);
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
            ui_select_next_subcart();
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

void ui_select_next_subcart()
{
    if (ui_cart->subcart_count == 0)
    {
        return;
    }
    
    ui_subcart_t *sc = ui_sel_subcart;

    ui_sel_subcart_index += 1;
    if (ui_sel_subcart_index >= ui_cart->subcart_count)
        ui_sel_subcart_index = 0;
        
    sc->tab_selected_doc = -1;
    sc->tab_selected_flags = 0;

    ui_sel_subcart = &ui_cart->subcarts[ui_sel_subcart_index];
    
    if (sc_action_enabled(ui_sel_subcart))
    {
        ui_sel_subcart->tab_selected_flags = CART_TAB_SELECTED_ACTION;
    }
    else if (sc_delete_enabled(ui_sel_subcart))
    {
        ui_sel_subcart->tab_selected_flags = CART_TAB_SELECTED_DELETE;
    }
    else
    {
        ui_sel_subcart->tab_selected_doc = 0;
    }

    
    ui_cart_redraw_all();
    
    printf("ui_sel_subcart_index = %d\n", ui_sel_subcart_index);
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
            case KEY_DOWN:
                ui_cart_tab_select_next();
                break;
            case KEY_SPACE:
                ui_cart_process_space();
                break;
            case KEY_ENTER:
                ui_cart_process_enter();
                break;
            case KEY_PGDN:
                ui_select_next_subcart();
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
                list_add(sel, d);
            }
        }
        
        if (sel->count > 0)
        {
            return;
        }
        
		for (int j = 0; j < sc->doc_count; j++)
		{
            ui_doc_t *d = &sc->docs[j];
            
            if (in_bank_process_state(d->val) || in_check_state(d->val))
            {
                list_add(sel, d);
            }
        }
        
        if (sel->count > 0)
        {
            return;
        }
        
		for (int j = 0; j < sc->doc_count; j++)
		{
            ui_doc_t *d = &sc->docs[j];
            
            if (d->val->k->u.s != NULL)
            {
                continue;
            }
            
            if (d->val->group != NULL)
            {
                if (sel->count + d->val->group->count > MAX_DOCS)
                {
                    break;
                }
                
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
                break;
            }
		}
    }
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
        
        ui_subcart_t *sc = ui_sel_subcart;
        print_cheque(sc->val, list);
    }
}

typedef void (*update_screen_func_t)(void *arg);

static bool fa_create_doc(uint16_t doc_type, const uint8_t *pattern_footer,
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

void print_cheque(SubCart *sc, list_t *klist)
{
    C* c = sc->c;
    size_t doc_count = 0;

	ffd_tlv_reset();

	ffd_tlv_add_string(1021, cashier_cashier);
	if (cashier_inn[0])
		ffd_tlv_add_fixed_string(1203, cashier_inn, 12);
	ffd_tlv_add_uint8(1054, c->t1054);
	ffd_tlv_add_uint8(1055, c->t1055);
	if (c->pe)
		ffd_tlv_add_string(1008, c->pe);
	ffd_tlv_add_vln(1031, (uint64_t)c->sum.n);
	ffd_tlv_add_vln(1081, (uint64_t)c->sum.e);
	ffd_tlv_add_vln(1215, (uint64_t)c->sum.p);
	ffd_tlv_add_vln(1216, 0);
	ffd_tlv_add_vln(1217, (uint64_t)c->sum.b);

	char agent_phone[19+1];
	char phone[19+1];
	bool is_same_agent;
	bool attr = kkt_has_param("COMP1057WO1171");
	if (C_is_agent_cheque(c, user_inn, agent_phone, &is_same_agent)) {
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
				if (l->n >= 1 && l->n <= 4) {
					printf("ADD 1198, %lld\n", (long long)l->c);
					ffd_tlv_add_vln(1198, l->c);
					printf("ADD 1200, %lld\n", (long long)l->c);
					ffd_tlv_add_vln(1200, l->c);
				}
				if (l->i == 0) {
					// если ИНН == 0, но есть l->z, значит перевозчик не российский.
					if (l->z && l->z[0] != 0) {
						ffd_tlv_add_fixed_string(1226, "000000000000", 12);
					}
				} else if (l->i != user_inn) {
					char inn[12+1];
					if (c->p > 9999999999ll)
						sprintf(inn, "%.12ld", l->i);
					else
						sprintf(inn, "%.10ld", l->i);
					ffd_tlv_add_fixed_string(1226, inn, 12);
				}
				ffd_tlv_stlv_end();
			}
			doc_count++;
		}
	}

	if (doc_count > 0) {
		uint8_t* pattern_footer = NULL;
		size_t pattern_footer_size = 0;

		if (fa_create_doc(CHEQUE, pattern_footer, pattern_footer_size, update_cheque, NULL)) {
		}
	}
}