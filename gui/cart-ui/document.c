#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdefs.h"
#include "kbd.h"
#include "gui/gdi.h"
#include "gui/cart.h"
#include "gui/forms.h"

const char *printsum(int64_t sum, char *buf)
{
    sprintf(buf, "%ld.%.2ld “.", sum / 100, sum % 100);
    
    return buf;
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

void ui_doc_init(ui_doc_t *d, D *val, bool selected)
{
	d->val = val;
    d->selected = selected;
}

void ui_doc_calc_bounds(ui_doc_t *d)
{
   	d->height = cart_fnt->max_height;
   	
	if (d->expanded)
	{
		for (list_item_t* li = d->val->related.head; li; li = li->next)
		{
			K *k = LIST_ITEM(li, K);
			
			if (k != d->val->k)
			{
    			d->height += cart_sfnt->max_height;
            }
			
    		for (list_item_t* li1 = k->llist.head; li1; li1 = li1->next)
	    	{
    			d->height += cart_sfnt->max_height;
			}
        }
	    d->height += CART_YGAP;
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

static const char *make_l_str(char *buf, L* l)
{
    const char *svat[] =
    {
        "„‘ 20%",
        "„‘ 20%",
        "„‘ 20/120",
        "„‘ 10/110",
        "„‘ 0%",
        "„‘ ­¥ ®¡«."
    };
    char v[128];
    char s[32];
    
    if (l->n == 0)
    {
        sprintf(v, "(… Ž„‹…†ˆ’ €‹ŽƒƒŽ‹Ž†…ˆž „‘)");
    }
    else
    {
        sprintf(v, "‚ ’.—. %s%s%s",
            svat[l->n - 1],
            l->n < 5 ? ": " : "",
            l->n < 5 ? printsum(l->c, s) : "");
    }
    
    sprintf(buf, "%s: %s %s", l->s, printsum(l->t, s), v);
    
    return buf;
}

int ui_doc_draw(ui_subcart_t *sc, ui_doc_t *d, int x, int y, int col_width)
{
	float *fr = cart_tab_fr;
	char buf[256];
	S s;
	int x_orig = x;
	
	K_calc_sum(d->val->k, &s);
	
	Color borderColor = sc->tab_selected_doc >= 0 && &sc->docs[sc->tab_selected_doc] == d
	    ? CART_SELECTED_BORDER_COLOR
	    : clSilver;
    DrawBorder(cart_screen, x - CART_XGAP - 2, y - 2, DISCX - CART_XGAP * 3, cart_fnt->max_height + 3, 2, borderColor, borderColor);
	
	if (d->selected)
    {	
    	DrawText(cart_screen, x, y, 15, 17, "*", DT_CENTER | DT_VCENTER);
        DrawBorder(cart_screen, x, y, 15, 15, 2, clBlack, clBlack);
        SetTextColor(cart_screen, clBlack);
    }
	DrawText(cart_screen, x + 20, y, col_width, cart_fnt->max_height, d->val->k->d.s, DT_LEFT | DT_VCENTER);
	x += (float)col_width * fr[0];
	DrawText(cart_screen, x, y, col_width, cart_fnt->max_height,
			strdatetime(buf, sizeof(buf), d->val->k->dt), DT_LEFT | DT_VCENTER);
	x += (float)col_width * fr[1];

	SetFont(cart_screen, cart_sfnt);
	DrawText(cart_screen, x, y, col_width, cart_fnt->max_height, ui_doc_get_op(sc, d, &s), DT_LEFT | DT_VCENTER);
	x += (float)col_width * fr[2];

	SetFont(cart_screen, cart_fnt);
	DrawText(cart_screen, x, y, col_width, cart_fnt->max_height, ui_doc_get_n(sc, d, buf), DT_LEFT | DT_VCENTER);
	x += (float)col_width * fr[3];
	DrawText(cart_screen, x, y, col_width, cart_fnt->max_height, printsum(s.a, buf), DT_RIGHT | DT_VCENTER);
	
	
	y += cart_fnt->max_height + CART_YGAP;
	x = x_orig + 20;

    if (d->expanded)
    {	
        SetFont(cart_screen, cart_sfnt);
		for (list_item_t* li = d->val->related.head; li; li = li->next)
		{
			K *k = LIST_ITEM(li, K);
			
			if (k != d->val->k)
			{
    			char sign = k_lp(d->val->k) == k_lp(k) ? '+' : '-';
      			sprintf(buf, "(%c) %s", sign, k->d.s);
      			DrawText(cart_screen, x, y, DISCX, cart_fnt->max_height, buf, DT_LEFT | DT_VCENTER);
  			
      			y += cart_sfnt->max_height;
  			}
  			
    		for (list_item_t* li1 = k->llist.head; li1; li1 = li1->next)
	    	{
    			L *l = LIST_ITEM(li1, L);
    			
    			
            	DrawText(cart_screen, x + CART_XGAP, y, DISCX, cart_fnt->max_height,
            	    make_l_str(buf, l), DT_LEFT | DT_VCENTER);
            	
      			y += cart_sfnt->max_height;
			}
        }
        SetFont(cart_screen, cart_fnt);
        
        y += CART_YGAP;
	}
	
	return y;
}

