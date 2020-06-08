#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "gui/controls/control.h"
#include "list.h"

typedef struct listview_column_t {
	char *title;
	int width;
} listview_column_t;

typedef void (*listview_get_item_text_func_t)(void *item, int index, char text[], size_t text_size);
typedef void (*listview_selected_changed_t)(control_t *lv, int index, void *item);

control_t* listview_create(int id, GCPtr gc, int x, int y, int width, int height,
        listview_column_t *columns, size_t column_count,
		list_t *items, listview_get_item_text_func_t get_item_text_func,
		listview_selected_changed_t selected_changed_func,
        int selected_index);

#endif // LISTVIEW
