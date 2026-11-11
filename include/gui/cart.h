#if !defined GUI_CART_H
#define GUI_CART_H

#include "gui/gdi.h"
#include "kkt/fd/ad.h"
#include "kbd.h"

__BEGIN_DECLS

extern bool _support_1222_1224_1225;

#define MAX_DOCS    35

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

#define CART_ACTION_ENABLED 0x01
#define CART_DELETE_ENABLED 0x02

extern int cart_start_y;

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
	int enabled_flags;
} ui_subcart_t;

#define sc_action_enabled(sc) (((sc)->enabled_flags & CART_ACTION_ENABLED) != 0)
#define sc_delete_enabled(sc) (((sc)->enabled_flags & CART_DELETE_ENABLED) != 0)

#define sc_action_selected(sc) ((sc)->tab_selected_flags == CART_TAB_SELECTED_ACTION)
#define sc_delete_selected(sc) ((sc)->tab_selected_flags == CART_TAB_SELECTED_DELETE)

typedef struct ui_doc_t
{
	D *val;
	bool selected;
	bool expanded;
	int height;
} ui_doc_t;

extern bool is_admin_mode(void);

extern void ui_cart_create(void);
extern void ui_cart_destroy(void);
extern void ui_cart_draw(GCPtr s, FontPtr f, FontPtr sf);
extern bool cart_process(const struct kbd_event *_e);

extern GCPtr cart_screen;
extern FontPtr cart_fnt;
extern FontPtr cart_sfnt;
extern ui_cart_t *ui_cart;
extern float cart_tab_fr[CART_MAX_TAB_COL];

extern void ui_subcart_init(ui_subcart_t *sc, SubCart *val, bool tab_selected);
extern void ui_subcart_free(ui_subcart_t *sc);
extern void ui_subcart_calc_bounds(ui_subcart_t *sc);
extern void ui_subcart_draw(ui_subcart_t *sc, int y);
extern bool ui_subcart_items_disabled(ui_subcart_t *sc);
extern size_t ui_subcart_get_all_k(ui_subcart_t *sc, list_t *list);


void ui_doc_init(ui_doc_t *d, D *val, bool selected);
void ui_doc_calc_bounds(ui_doc_t *d);
extern int ui_doc_draw(ui_subcart_t *sc, ui_doc_t *d, int x, int y, int col_width);


extern int kbd_lang_ex;
extern int kbd_get_char_ex(int key);

extern bool pos_check_last_operation(void);
extern void pos_day_open(void);
extern void pos_day_close(void);
extern void pos_service_operations(void);

extern const char *printsum(int64_t sum, char *buf);

__END_DECLS

#endif		/* GUI_CART_H */
