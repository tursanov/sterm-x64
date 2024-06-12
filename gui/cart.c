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
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "", "‘ã¬¬ " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "", "‘ã¬¬ " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N ª¢¨â ­æ¨¨", "‘ã¬¬ " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N § ª § ", "‘ã¬¬ " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N § ª § ", "‘ã¬¬ " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "", "‘ã¬¬ " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N ª¢¨â ­æ¨¨", "‘ã¬¬ " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N § ª § ", "‘ã¬¬ " },
	{ "N ¤®ªã¬¥­â ", "„ â  ¨ ¢à¥¬ï", "Ž¯¥à æ¨ï", "N § ª § ", "‘ã¬¬ " },
};

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

void ui_subcart_init(ui_subcart_t *sc, SubCart *val)
{
	sc->val = val;
	sc->doc_count = val->documents.count;
	sc->docs = __calloc(sc->doc_count, ui_doc_t);
	ui_subcart_set_title(sc);

	printf("sc->type = %c\n", val->type);

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

	sc->height = h;
	sc->tab_ofs_x = XGAP;

	printf("sc->height: %d\n", sc->height);
}

void ui_subcart_draw(ui_subcart_t *sc, int y)
{
	int x = XGAP;
	int w = DISCX - XGAP * 2;
	int tw = w - sc->tab_ofs_x - XGAP;
	int tcw = tw / MAX_TAB_COL;

	fill_rect(screen, x, y, w, sc->height, BORDER_WIDTH, clBlack, 0);
	
	y += YGAP;
	SetTextColor(screen, clBlack);
	TextOut(screen, x + XGAP, y, sc->title);
	y += YGAP + fnt->max_height;
	x += sc->tab_ofs_x;
	
	const char **coltext = sc_tab_title[SUB_CART_INDEX(sc->val->type)];
	for (int i = 0, cx = x; i < MAX_TAB_COL; i++, cx += tcw, coltext++)
	{
		const char *text = coltext[0];
		if (*text)
			TextOut(screen, cx, y, text);
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

void ui_cart_draw(GCPtr s, FontPtr f)
{
	screen = s;
	fnt = f;
	ClearGC(screen, clSilver);

	printf("ui_cart_draw\n");

	ui_cart_calc_bounds();

	int y = YGAP;
	for (int i = 0; i < ui_cart->subcart_count; i++)
	{
		ui_subcart_t *sc = &ui_cart->subcarts[i];
		ui_subcart_draw(sc, y);
		y += sc->height + YGAP;
	}
}
