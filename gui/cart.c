#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdefs.h"
#include "kbd.h"
#include "gui/gdi.h"
#include "gui/cart.h"
#include "gui/forms.h"

static GCPtr screen = NULL;
static FontPtr fnt = NULL;
static FontPtr sfnt = NULL;
static ui_cart_t *ui_cart = NULL;

void ui_subcart_init(ui_subcart_t *sc, SubCart *val, bool tab_selected);
void ui_subcart_free(ui_subcart_t *sc);
void ui_doc_init(ui_doc_t *d, D *val, bool selected);
void ui_subcart_calc_bounds(ui_subcart_t *sc);
void ui_doc_calc_bounds(ui_doc_t *d);
extern int kbd_lang_ex;
extern int kbd_get_char_ex(int key);

#define XGAP	5
#define YGAP	5
#define YGAP_DOC 5
#define BORDER_WIDTH 2
#define MAX_TAB_COL 5

#define BUTTON_WIDTH  150
#define BUTTON_HEIGHT 30

#define SELECTED_BORDER_COLOR clRopnetDarkBrown

static const char* sc_tab_title[MAX_SUB_CART][MAX_TAB_COL] =
{
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "", "‘ã¬¬  " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "", "‘ã¬¬  " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N ª¢¨â ­æ¨¨", "‘ã¬¬  " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N § ª § ", "‘ã¬¬  " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N § ª § ", "‘ã¬¬  " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "", "‘ã¬¬  " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N ª¢¨â ­æ¨¨", "‘ã¬¬  " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N § ª § ", "‘ã¬¬  " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N § ª § ", "‘ã¬¬  " },
};

static float tab_fr[MAX_TAB_COL] = { 1, 1, 1.4, 0.6, 1 };

void ui_cart_create()
{
	if (ui_cart != NULL)
	{
		ui_cart_destroy();
	}

	ui_cart = __new(ui_cart_t);
	ui_cart->subcarts = __calloc(MAX_SUB_CART, ui_subcart_t);

    bool first = true;
	for (int i = 0; i < MAX_SUB_CART; i++)
	{
		SubCart *val = &cart.sc[i];
		if (val->documents.count > 0)
		{
			ui_subcart_t *sc = &ui_cart->subcarts[ui_cart->subcart_count++];
			
			ui_subcart_init(sc, val, first);
			
			first = false;
		}
	}
}

void ui_cart_destroy()
{
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
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		ui_subcart_calc_bounds(sc);
	}
}

static void ui_subcart_set_title(ui_subcart_t* sc)
{
	switch (sc->val->type)
	{
		case 'A':
			strcpy(sc->title, "Ž‹€’€ €‹ˆ—");
			break;
		case 'B':
			strcpy(sc->title, "‚Ž‡‚€’ €‹ˆ—›…/Ž’Œ…€ €‹ˆ—›…/‚Ž‡‚€’ € Š€’“ ‹€’…‹œ™ˆŠ€");
			break;
		case 'C':
			strcpy(sc->title, "Ž’Œ…€ Š‚ˆ’€–ˆ‰ Ž‹€’›");
			break;
		case 'D':
			strcpy(sc->title, "Š€’€ Ž‹€’€/„Ž‹€’€");
			break;
		case 'E':
			strcpy(sc->title, "‘ Ž‹€’€/„Ž‹€’€");
			break;
		case 'F':
			strcpy(sc->title, "Š€’€ ‚Ž‡‚€’");
			break;
		case 'G':
			strcpy(sc->title, "Ž’Œ…€ ‚Ž‡‚€’€ € Š€’“");
			break;
		case 'H':
			strcpy(sc->title, "Ž’Œ…€ …‚Ž‡ŒŽ†€");
			break;
		default:
			strcpy(sc->title, "Ž˜ˆŠ€");
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
	    sc->tab_selected_flags = TAB_SELECTED_ACTION;
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

	h += YGAP * 2; // ®âáâã¯ ®â ªà ñ¢
	h += fnt->max_height; // § £®«®¢®ª ¯®¤ª®à§¨­ë
	h += YGAP; // ®âáâã¯
	h += fnt->max_height; // § £®«®¢®ª â ¡«¨æë

	for (int i = 0; i < sc->doc_count; i++)
	{
		ui_doc_t *d = &sc->docs[i];
		ui_doc_calc_bounds(d);
		h += d->height + YGAP_DOC;
	}

	h += BUTTON_HEIGHT + YGAP * 3;
	
	sc->height = h;
	sc->tab_ofs_x = XGAP;
}

static int ui_subcart_header_with_box_draw(ui_subcart_t *sc, int x, int y, int w)
{
	fill_rect(screen, x, y, w, sc->height, BORDER_WIDTH, clBlack, 0);
	
	SetTextColor(screen, clBlack);
	
	int tw = GetTextWidth(screen, sc->title);
    DrawBorder(screen, XGAP, y, tw + XGAP * 4, fnt->max_height + YGAP * 2, 2, clBlack, clBlack);
    
	y += YGAP;
	TextOut(screen, x + XGAP, y, sc->title);
	
	
	y += YGAP * 2 + fnt->max_height;

	return y;
}

static int ui_subcart_table_header_draw(ui_subcart_t *sc, int x, int y, int col_width)
{
	const char **coltext = sc_tab_title[SUB_CART_INDEX(sc->val->type)];
	float *fr = tab_fr;

	int flags = DT_LEFT | DT_VCENTER;

	for (int i = 0; i < MAX_TAB_COL; i++, coltext++, fr++)
	{
		int w = (float)col_width * fr[0];
		if (i == MAX_TAB_COL - 1)
			flags = DT_RIGHT | DT_VCENTER;

		const char *text = coltext[0];
		if (*text)
		{
			DrawText(screen, x, y, w, fnt->max_height, text, flags);
		}
		x += w;
	}

	y += fnt->max_height + YGAP;

	return y;
}

static char *strdatetime(char *buf, size_t max, time_t t)
{
	struct tm* tm;
	tm = localtime(&t);
	strftime(buf, max, "%d.%m.%y %H:%M:%S", tm);

	return buf;
}

const char *ui_doc_get_op(ui_subcart_t *sc, ui_doc_t *d, S *s)
{
    D* doc = d->val;
    K *k = d->val->k;
    char t = sc->val->type;
	
	if (k->u.s)
	{
		return "…‡€‚…˜…Ž… ……Ž”ŽŒ‹…ˆ…";
	}
	else if (s->a == s->p)
	{
		return "…‡€‚…˜…Ž… ……Ž”ŽŒ‹…ˆ…";
	}
	else if (t == 'A')
	{
	    return d->val->k->m == 2 ? "Ž’Œ…€ ‚Ž‡‚‚’‚ € ‘—…’ ‹€’…‹œ™ˆŠ€" : "Ž‹€’€ €‹ˆ—…";
	}
	else if (t == 'D' || t == 'E')
	{
	    if (k->bank_state == 1)
	    {
	        return "…—€’œ —…Š€";
	    }
	    else if (doc->related.count == 0)
	    {
	        return t == 'D' ? "Š€’€ Ž‹€’€" : "‘ Ž‹€’€";
	    }
	    else
	    {
	        return t == 'D' ? "Š€’€ „Ž‹€’€" : "‘ „Ž‹€’€";
	    
	    }
	}
	else if (t == 'C' || t == 'H')
	{
	    return k->bank_state == 1 ? "…—€’œ —…Š€" : "Š€’€ Ž’Œ…€";
	}
	else if (t == 'B')
	{
	    if (k->m == 2)
	    {
    	    if (doc->related.count > 0 && k->n.s && !k->a_flag)
    	    {
    	        return "‚‹€’€ € Š€’“ ‹€’…‹œ™ˆŠ€";
    	    }
    	    else
    	    {
    	        return "‚Ž‡‚€’ € Š€’“ ‹€’…‹œ™ˆŠ€";
    	    }
	    }
	    else
	    {
	        return "‚Ž‡‚€’/Ž’Œ…€ €‹ˆ—…";
	    }
	}
	else if (t == 'F')
	{
	    if (k->v == 1)
	    {
	        return k->bank_state == 1 ? "(Ž‘) …—€’œ —…Š€" : "(Ž‘) Š€’€ ‚Ž‡‚€’";
	    }
	    else if (k->v == 2)
	    {
	        return k->bank_state == 1 ? "(‚„) …—€’œ —…Š€" : "(‚„) Š€’€ ‚Ž‡‚€’";
	    }
	    else
	    {
	        return k->bank_state == 1 ? "…—€’œ —…Š€" : "Š€’€ ‚Ž‡‚€’";
	    }
	}
	else if (t == 'G')
	{
        return k->bank_state == 1 ? "…—€’œ —…Š€" : "Š€’€ Ž’Œ…€ ‚Ž‡‚€’";
	}
	else if (t == 'I')
	{
	    return "Ž€Ž’Š€ ‡€…™…€";
	}
	else
	{
	    return "";
	}
}

const char *ui_doc_get_n(ui_subcart_t *sc, ui_doc_t *d, char *buf)
{
    K *k = d->val->k;
    char t = sc->val->type;
    
    if (k->y && (t == 'D' || t == 'H' || t == 'I' || t == 'E'))
    {
        sprintf(buf, "%7d", k->y->req_id);
    }
    else if (t == 'C' || t == 'G')
    {
        sprintf(buf, "%d", k->c);
    }
    else
    {
        buf[0] = 0;
    }
    
    return buf;
}

const char *printsum(int64_t sum, char *buf)
{
    sprintf(buf, "%ld.%.2ld “.", sum / 100, sum % 100);
    
    return buf;
}


static int ui_subcart_doc_draw(ui_subcart_t *sc, ui_doc_t *d, int x, int y, int col_width)
{
	float *fr = tab_fr;
	char buf[32];
	S s;
	K_calc_sum(d->val->k, &s);
	
	Color borderColor = sc->tab_selected_doc >= 0 && &sc->docs[sc->tab_selected_doc] == d
	    ? SELECTED_BORDER_COLOR
	    : clSilver;
    DrawBorder(screen, x - XGAP - 2, y - 2, DISCX - XGAP * 3, fnt->max_height + 3, 2, borderColor, borderColor);

	
	if (d->selected)
    {	
    	DrawText(screen, x, y, 15, 17, "*", DT_CENTER | DT_VCENTER);
        DrawBorder(screen, x, y, 15, 15, 2, clBlack, clBlack);
        SetTextColor(screen, clBlack);
    }
	DrawText(screen, x + 20, y, col_width, fnt->max_height, d->val->k->d.s, DT_LEFT | DT_VCENTER);
	x += (float)col_width * fr[0];
	DrawText(screen, x, y, col_width, fnt->max_height,
			strdatetime(buf, sizeof(buf), d->val->k->dt), DT_LEFT | DT_VCENTER);
	x += (float)col_width * fr[1];

	SetFont(screen, sfnt);
	DrawText(screen, x, y, col_width, fnt->max_height, ui_doc_get_op(sc, d, &s), DT_LEFT | DT_VCENTER);
	x += (float)col_width * fr[2];

	SetFont(screen, fnt);
	DrawText(screen, x, y, col_width, fnt->max_height, ui_doc_get_n(sc, d, buf), DT_LEFT | DT_VCENTER);
	x += (float)col_width * fr[3];
	DrawText(screen, x, y, col_width, fnt->max_height, printsum(s.a, buf), DT_RIGHT | DT_VCENTER);
	
	
	y += fnt->max_height + YGAP;
	
	return y;
}

void ui_subcart_draw(ui_subcart_t *sc, int y)
{
	int x = XGAP;
	int w = DISCX - XGAP * 2;
	int tw = w - sc->tab_ofs_x - XGAP * 2;
	int tcw = tw / MAX_TAB_COL;

	y = ui_subcart_header_with_box_draw(sc, x, y, w);
	x += XGAP + sc->tab_ofs_x;
	y = ui_subcart_table_header_draw(sc, x, y, tcw);
	
	for (int i = 0; i < sc->doc_count; i++)
	{
		ui_doc_t *d = &sc->docs[i];
		y = ui_subcart_doc_draw(sc, d, x, y, tcw);
	}
	
	bool action_selected = sc->tab_selected_flags == TAB_SELECTED_ACTION;
	bool delete_selected = sc->tab_selected_flags == TAB_SELECTED_DELETE;
	
    draw_button(screen, XGAP * 3, y, BUTTON_WIDTH, BUTTON_HEIGHT,  "¥ç âì ç¥ª ", action_selected);
    draw_button(screen, x + w - XGAP*4 - BUTTON_WIDTH, y, BUTTON_WIDTH, BUTTON_HEIGHT,  "“¤ «¨âì", delete_selected);
}

void ui_doc_init(ui_doc_t *d, D *val, bool selected)
{
	d->val = val;
    d->selected = selected;
}

void ui_doc_calc_bounds(ui_doc_t *d)
{
	if (d->expanded)
	{
		d->height = fnt->max_height;
	}
	else
	{
		d->height = fnt->max_height;
	}
}

void ui_cart_draw(GCPtr s, FontPtr f, FontPtr sf)
{
	screen = s;
	fnt = f;
	sfnt = sf;
	ClearGC(screen, clSilver);
	
	SetFont(screen, fnt);

	ui_cart_calc_bounds();

	int y = YGAP;
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		ui_subcart_draw(sc, y);
		y += sc->height + YGAP;
	}
}

int ui_cart_get_y(ui_subcart_t *sc)
{
	int y = YGAP;
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *_sc = &ui_cart->subcarts[i];
		
		if (_sc == sc)
		{
		    return y;
		}
		
		y += sc->height + YGAP;
	}
	
	return 0;
}

void tab_select_next()
{
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		
		if (sc->tab_selected_doc >= 0)
		{
		    sc->tab_selected_doc++;
		    if (sc->tab_selected_doc >= sc->doc_count)
		    {
		        sc->tab_selected_doc = -1;
		        sc->tab_selected_flags = TAB_SELECTED_ACTION;
		    }
        }
        else if (sc->tab_selected_flags == TAB_SELECTED_ACTION)
        {
            sc->tab_selected_flags = TAB_SELECTED_DELETE;
        }
        else if (sc->tab_selected_flags == TAB_SELECTED_DELETE)
        {
            sc = i == ui_cart->subcart_count - 1
                ? &ui_cart->subcarts[0]
                : &ui_cart->subcarts[i + 1];
            sc->tab_selected_doc = 0;
            sc->tab_selected_flags = TAB_SELECTED_NONE;
        }
        
        ui_subcart_draw(sc, ui_cart_get_y(sc));
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
                tab_select_next();
                break;
            case KEY_SPACE:
                break;
        }
	}

	return 1;
}
