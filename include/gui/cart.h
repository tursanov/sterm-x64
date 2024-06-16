#ifndef UI_CART_H
#define UI_CART_H

#include "kkt/fd/ad.h"

typedef struct ui_cart_t
{
	size_t subcart_count;
	struct ui_subcart_t* subcarts;
} ui_cart_t;

#define MAX_UI_CART_TITLE	64
typedef struct ui_subcart_t
{
	SubCart *val;
	size_t doc_count;
	struct ui_doc_t *docs;
	char title[MAX_UI_CART_TITLE];
	int height;
	int tab_ofs_x;
} ui_subcart_t;

typedef struct ui_doc_t
{
	D *val;
	bool selected;
	bool expanded;
	int height;
} ui_doc_t;

extern void ui_cart_create();
extern void ui_cart_destroy();
extern void ui_cart_draw(GCPtr s, FontPtr f, FontPtr sf);

#endif // UI_CART_H
