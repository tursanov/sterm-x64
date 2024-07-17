#ifndef UI_CART_H
#define UI_CART_H

#include "kkt/fd/ad.h"
#include "kbd.h"

#define CART_XGAP	5
#define CART_YGAP	5
#define CART_YGAP_DOC 5
#define CART_BORDER_WIDTH 2
#define CART_MAX_TAB_COL 5

#define CART_BUTTON_WIDTH  150
#define CART_BUTTON_HEIGHT 30

#define CART_SELECTED_BORDER_COLOR clRopnetDarkBrown

#define CART_MAX_TITLE	64
#define CART_TAB_SELECTED_NONE 0x0
#define CART_TAB_SELECTED_ACTION 0x1
#define CART_TAB_SELECTED_DELETE 0x02


typedef struct dim_t
{
	int w;
	int h;
} dim_t;

typedef struct ui_cart_t
{
	size_t subcart_count;
	struct ui_subcart_t* subcarts;
} ui_cart_t;


typedef struct ui_subcart_t
{
	SubCart *val;
	size_t doc_count;
	struct ui_doc_t *docs;
	char title[CART_MAX_TITLE];
	int height;
	int tab_ofs_x;
	int tab_selected_flags;
	int tab_selected_doc;
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
extern bool cart_process(const struct kbd_event *_e);

extern GCPtr cart_screen;
extern FontPtr cart_fnt;
extern FontPtr cart_sfnt;
extern ui_cart_t *ui_cart;
extern float cart_tab_fr[CART_MAX_TAB_COL];

void ui_subcart_init(ui_subcart_t *sc, SubCart *val, bool tab_selected);
void ui_subcart_free(ui_subcart_t *sc);
void ui_subcart_calc_bounds(ui_subcart_t *sc);
void ui_subcart_draw(ui_subcart_t *sc, int y);


void ui_doc_init(ui_doc_t *d, D *val, bool selected);
void ui_doc_calc_bounds(ui_doc_t *d);
int ui_doc_draw(ui_subcart_t *sc, ui_doc_t *d, int x, int y, int col_width);


extern int kbd_lang_ex;
extern int kbd_get_char_ex(int key);

#endif // UI_CART_H
