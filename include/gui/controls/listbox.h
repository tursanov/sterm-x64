#if !defined GUI_CONTROLS_LISTBOX_H
#define GUI_CONTROLS_LISTBOX_H

#include "gui/controls/control.h"
#include "list.h"

__BEGIN_DECLS

typedef void (*listbox_get_item_text_func_t)(void *item, char text[], size_t text_size);

extern control_t* listbox_create(int id, GCPtr gc, int x, int y, int width, int height,
	list_t *items, listbox_get_item_text_func_t get_item_text_func,
	int selected_index);

__END_DECLS

#endif		/* GUI_CONTROLS_LISTBOX_H */
