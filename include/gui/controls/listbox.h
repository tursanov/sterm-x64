#ifndef LISTBOX_H
#define LISTBOX_H

#include "gui/controls/control.h"
#include "list.h"

typedef void (*listbox_get_item_text_func_t)(void *item, char text[], size_t text_size);

control_t* listbox_create(int id, GCPtr gc, int x, int y, int width, int height,
		list_t *items, listbox_get_item_text_func_t get_item_text_func,
		int selected_index);

#endif
