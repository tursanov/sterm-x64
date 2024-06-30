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

void ui_subcart_init(ui_subcart_t *sc, SubCart *val);
void ui_subcart_free(ui_subcart_t *sc);
void ui_doc_init(ui_doc_t *d, D *val);
void ui_subcart_calc_bounds(ui_subcart_t *sc);
void ui_doc_calc_bounds(ui_doc_t *d);

#define XGAP	5
#define YGAP	5
#define YGAP_DOC 5
#define BORDER_WIDTH 2
#define MAX_TAB_COL 5

static const char* sc_tab_title[MAX_SUB_CART][MAX_TAB_COL] =
{
	{ "N ���㬥��", "��� � �६�", "������", "", "�㬬� " },
	{ "N ���㬥��", "��� � �६�", "������", "", "�㬬� " },
	{ "N ���㬥��", "��� � �६�", "������", "N ���⠭樨", "�㬬� " },
	{ "N ���㬥��", "��� � �६�", "������", "N ������", "�㬬� " },
	{ "N ���㬥��", "��� � �६�", "������", "N ������", "�㬬� " },
	{ "N ���㬥��", "��� � �६�", "������", "", "�㬬� " },
	{ "N ���㬥��", "��� � �६�", "������", "N ���⠭樨", "�㬬� " },
	{ "N ���㬥��", "��� � �६�", "������", "N ������", "�㬬� " },
	{ "N ���㬥��", "��� � �६�", "������", "N ������", "�㬬� " },
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

	for (int i = 0; i < MAX_SUB_CART; i++)
	{
		SubCart *val = &cart.sc[i];
		if (val->documents.count > 0)
		{
			ui_subcart_t *sc = &ui_cart->subcarts[ui_cart->subcart_count++];
			ui_subcart_init(sc, val);
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
			strcpy(sc->title, "������ ������");
			break;
		case 'B':
			strcpy(sc->title, "������� ��������/������ ��������/������� �� ����� �����������");
			break;
		case 'C':
			strcpy(sc->title, "������ ��������� ������");
			break;
		case 'D':
			strcpy(sc->title, "����� ������/�������");
			break;
		case 'E':
			strcpy(sc->title, "��� ������/�������");
			break;
		case 'F':
			strcpy(sc->title, "����� �������");
			break;
		case 'G':
			strcpy(sc->title, "������ �������� �� �����");
			break;
		case 'H':
			strcpy(sc->title, "������ ����������");
			break;
		default:
			strcpy(sc->title, "������");
			break;
	}
}

void ui_subcart_init(ui_subcart_t *sc, SubCart *val)
{
	sc->val = val;
	sc->doc_count = val->documents.count;
	sc->docs = __calloc(sc->doc_count, ui_doc_t);
	ui_subcart_set_title(sc);

	int i = 0;
	for (list_item_t *li = val->documents.head; li; li = li->next, i++)
	{
		D *val = LIST_ITEM(li, D);
		ui_doc_init(&sc->docs[i], val); 
	}
}

void ui_subcart_free(ui_subcart_t *sc)
{
	free(sc->docs);
}

void ui_subcart_calc_bounds(ui_subcart_t *sc)
{
	int h = 0;

	h += YGAP * 2; // ����� �� ���
	h += fnt->max_height; // ��������� �����২��
	h += YGAP; // �����
	h += fnt->max_height; // ��������� ⠡����

	for (int i = 0; i < sc->doc_count; i++)
	{
		ui_doc_t *d = &sc->docs[i];
		ui_doc_calc_bounds(d);
		h += d->height + YGAP_DOC;
	}

	sc->height = h;
	sc->tab_ofs_x = XGAP;
}

static int ui_subcart_header_with_box_draw(ui_subcart_t *sc, int x, int y, int w)
{
	fill_rect(screen, x, y, w, sc->height, BORDER_WIDTH, clBlack, 0);
	
	y += YGAP;
	SetTextColor(screen, clBlack);
	TextOut(screen, x + XGAP, y, sc->title);
	y += YGAP + fnt->max_height;

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
		return "������������� ��������������";
	}
	else if (s->a == s->p)
	{
		return "������������� ��������������";
	}
	else if (t == 'A')
	{
	    return d->val->k->m == 2 ? "������ �������� �� ���� �����������" : "������ �������";
	}
	else if (t == 'D' || t == 'E')
	{
	    if (k->bank_state == 1)
	    {
	        return "������ ����";
	    }
	    else if (doc->related.count == 0)
	    {
	        return t == 'D' ? "����� ������" : "��� ������";
	    }
	    else
	    {
	        return t == 'D' ? "����� �������" : "��� �������";
	    
	    }
	}
	else if (t == 'C' || t == 'H')
	{
	    return k->bank_state == 1 ? "������ ����" : "����� ������";
	}
	else if (t == 'B')
	{
	    if (k->m == 2)
	    {
    	    if (doc->related.count > 0 && k->n.s && !k->a_flag)
    	    {
    	        return "������ �� ����� �����������";
    	    }
    	    else
    	    {
    	        return "������� �� ����� �����������";
    	    }
	    }
	    else
	    {
	        return "�������/������ �������";
	    }
	}
	else if (t == 'F')
	{
	    if (k->v == 1)
	    {
	        return k->bank_state == 1 ? "(���) ������ ����" : "(���) ����� �������";
	    }
	    else if (k->v == 2)
	    {
	        return k->bank_state == 1 ? "(���) ������ ����" : "(���) ����� �������";
	    }
	    else
	    {
	        return k->bank_state == 1 ? "������ ����" : "����� �������";
	    }
	}
	else if (t == 'G')
	{
        return k->bank_state == 1 ? "������ ����" : "����� ������ �������";
	}
	else if (t == 'I')
	{
	    return "��������� ���������";
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
        sprintf(buf, "%7d", k->y->id);
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
    sprintf(buf, "%ld.%.2ld ���.", sum / 100, sum % 100);
    
    return buf;
}


static int ui_subcart_doc_draw(ui_subcart_t *sc, ui_doc_t *d, int x, int y, int col_width)
{
	float *fr = tab_fr;
	char buf[32];
	S s;
	K_calc_sum(d->val->k, &s);


	DrawText(screen, x, y, col_width, fnt->max_height, d->val->k->d.s, DT_LEFT | DT_VCENTER);
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
}

void ui_doc_init(ui_doc_t *d, D *val)
{
	d->val = val;
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
