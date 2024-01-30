#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/listview.h"

typedef struct listview_t {
	control_t control;
	listview_column_t* columns;
	size_t column_count;
	list_t *items;
	int top_index;
	intptr_t selected_index;
	int header_height;
	int item_height;
	int max_items;
	listview_get_item_text_func_t get_item_text_func;
	listview_selected_changed_t selected_changed_func;
} listview_t;

void listview_destroy(listview_t *listview);
void listview_draw(listview_t *listview);
bool listview_focus(listview_t *listview, bool focus);
bool listview_handle(listview_t *listview, const struct kbd_event *e);
bool listview_get_data(listview_t *listview, int what, data_t *data);
bool listview_set_data(listview_t *listview, int what, const void *data, size_t data_len);
bool listview_is_empty(listview_t *listview);
void listview_set_parent(listview_t *listview);

#define screen listview->control.gc
#define BORDER_WIDTH	2

static bool listview_redraw_row(listview_t *listview, int index);

static void listview_selected_index_changed(listview_t *lv) {
	if (lv->selected_changed_func) {
		list_item_t *li;
		if (lv->items->count > 0) {
			li = lv->items->head;
			for (int i = 0; li && i < lv->selected_index; i++, li = li->next);
		} else
			li = NULL;
		lv->selected_changed_func(&lv->control, lv->selected_index, li ? li->obj : NULL);
	}
}

static void listview_set_selected_index(listview_t *lv, int index, bool refresh) {
	if (lv->items->count == 0)
		return;

	if (index < 0)
		index = 0;
	else if (index >= lv->items->count)
		index = lv->items->count - 1;

	if (index != lv->selected_index) {
		int old = lv->selected_index;
		bool refresh_all = false;

		if (index < lv->top_index) {
			lv->top_index = index;
			refresh_all = true;
		} else {
			int new_top_index = index - lv->max_items + 1;
			if (new_top_index > lv->top_index) {
				if (new_top_index < 0)
					new_top_index = 0;
				lv->top_index = new_top_index;
				refresh_all = true;
			}
		}

		if (lv->top_index < 0)
			lv->top_index = 0;

		lv->selected_index = index;
		//for (int i = 0; i < index; i++, lv->selected_item = lv->selected_item->next);

		if (refresh) {
			if (refresh_all) {
				//for (int i = 0; i < lv->top_index; i++, lv->top_item = lv->top_item->next);
				listview_draw(lv);
			} else {
				listview_redraw_row(lv, old);
				listview_redraw_row(lv, lv->selected_index);
			}
		}
		listview_selected_index_changed(lv);
	}
}


control_t* listview_create(int id, GCPtr gc, int x, int y, int width, int height,
		listview_column_t *columns, size_t column_count,
		list_t *items, listview_get_item_text_func_t get_item_text_func,
		listview_selected_changed_t selected_changed_func,
        int selected_index) {
    listview_t *listview = (listview_t *)malloc(sizeof(listview_t));
    control_api_t api = {
		(void (*)(struct control_t *))listview_destroy,
		(void (*)(struct control_t *))listview_draw,
		(bool (*)(struct control_t *, bool))listview_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))listview_handle,
		(bool (*)(struct control_t *, int, data_t *))listview_get_data,
		(bool (*)(struct control_t *control, int, const void *, size_t))listview_set_data,
		(bool (*)(struct control_t *control))listview_is_empty,
		(void (*)(struct control_t *))listview_set_parent,
		NULL,
    };

    control_init(&listview->control, id, gc, x, y, width, height, &api);

	listview->columns = (listview_column_t *)malloc(sizeof(listview_column_t) * column_count);
	listview->column_count = column_count;
    listview_column_t *c = listview->columns;
    listview_column_t *c1 = columns;
    for (int i = 0; i < column_count; i++, c++, c1++) {
        c->title = strdup(c1->title);
        c->width = c1->width;
    }

	listview->items = items;
	listview->get_item_text_func = get_item_text_func;
	listview->selected_changed_func = selected_changed_func;
	listview->header_height = GetTextHeight(screen) + 2;
	listview->item_height = GetTextHeight(screen) + 2;
	listview->max_items = (listview->control.height - 
			listview->header_height - BORDER_WIDTH*3) / listview->item_height;
	listview->top_index = 0;
	if (items->count > 0) {
		listview->selected_index = selected_index + 1;
		listview_set_selected_index(listview, selected_index, false);
	} else {
		listview->selected_index = -1;
		listview_draw(listview);
	}

    return (control_t *)listview;
}

void listview_destroy(listview_t *listview) {
    listview_column_t *c = listview->columns;
    for (int i = 0; i < listview->column_count; i++, c++)
        free(c->title);
    free(listview->columns);
	free(listview);
}

void listview_set_parent(listview_t *listview __attribute__((unused))) {
//	if (listview->control.parent.parent && listview->selected_index >= 0)
//		listview_selected_index_changed(listview);
}

static void listview_draw_column(listview_t *listview, listview_column_t *c, 
		int x, int y, int h, bool last) {
	SetGCBounds(screen, x, y, c->width, h);
	SetTextColor(screen, listview->control.focused ? clBlack: clGray);
	TextOut(screen, 4, 0, c->title);
	if (!last)
		FillBox(screen, c->width - 2, 0, 2, h);
}

static void listview_draw_columns(listview_t *listview) {
	listview_column_t *c = listview->columns;
	int x = listview->control.x + 2;
    int y = listview->control.y + 2;
    int w = listview->control.width;
	int h = GetTextHeight(screen) + 2;

	SetBrushColor(screen, listview->control.focused ? clRopnetDarkBrown : RGB(184, 184, 184));
	FillBox(screen, 0, h, w - 4, 2);
	for (int i = 0; i < listview->column_count; i++, x += c->width, c++)
		listview_draw_column(listview, c, x, y, h, i == listview->column_count - 1);
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

static list_item_t * listview_get_top_item(listview_t *listview) {
	list_item_t *li = listview->items->head;
	for (int i = 0; i < listview->top_index; i++, li = li->next);
	return li;
}

static void listview_draw_row(listview_t *listview, int index, list_item_t *i, int left, int top, int w, int h) {
	listview_column_t *c = listview->columns;
	SetBrushColor(screen, listview->selected_index == index ?
			(listview->control.focused ? clNavy : clGray) : 
			(listview->control.focused ? clRopnetBrown : clSilver));
	SetTextColor(screen, listview->selected_index == index ? clWhite : clBlack);

	SetGCBounds(screen, left, top, w, h);
	FillBox(screen, 0, 0, w, h);

	int x = left;
	for (int j = 0; j < listview->column_count; j++, x += c->width, c++) {
		char text[256];
		listview->get_item_text_func(i->obj, j, text, sizeof(text) - 1);
		SetGCBounds(screen, x, top, c->width, h);
		TextOut(screen, 4, 0, text);
	}
}

static bool listview_redraw_row(listview_t *listview, int index) {
	if (index >= listview->top_index && index < listview->top_index + listview->max_items &&
			index >= 0 && index < listview->items->count) {
		list_item_t *li = listview_get_top_item(listview);
		if (li) {
			int x = listview->control.x + 2;
			int y = listview->control.y + listview->header_height + 4;
			int w = listview->control.width - 4;
			int h = listview->item_height;
			for (int i = 0; i < index - listview->top_index; i++, li = li->next,
					y += listview->item_height);
			listview_draw_row(listview, index, li, x, y, w, h);
			SetGCBounds(screen, 0, 0, DISCX, DISCY);

			return true;
		}
	}
	return false;
}

void listview_draw_items(listview_t *listview) {
	list_item_t *li = listview_get_top_item(listview);
	int x = listview->control.x + 2;
	int y = listview->control.y + listview->header_height + 4;
	int w = listview->control.width - 4;
	int h = listview->item_height;
	for (int i = 0; li && i < listview->max_items; i++, li = li->next, y += listview->item_height) {
		//printf("i = %d, li = %p\n", i, li);
		listview_draw_row(listview, listview->top_index + i, li, x, y, w, h);
	}
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

void listview_draw(listview_t *listview) {
	Color borderColor;
	Color bgColor;
//	Color fgColor;

	if (listview->control.focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
//		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
//		fgColor = RGB(32, 32, 32);
	}
	
	fill_rect(screen, listview->control.x, listview->control.y,
		listview->control.width, listview->control.height, BORDER_WIDTH,
		borderColor, bgColor);

	SetGCBounds(screen, listview->control.x + BORDER_WIDTH, listview->control.y + BORDER_WIDTH,
		listview->control.width - BORDER_WIDTH * 2, listview->control.height - BORDER_WIDTH * 2);

	if (listview->items->count > 0) {
	   	if (listview->selected_index < 0) {
			listview->top_index = 0;
			listview->selected_index = 0;
		} else if (listview->selected_index >= listview->items->count) {
			listview_set_selected_index(listview, listview->items->count - 1, false);
		}
	}

	listview_draw_columns(listview);
	if (listview->items->count > 0)
		listview_draw_items(listview);

	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

bool listview_focus(listview_t *listview, bool focus) {
	listview->control.focused = focus;
	listview_draw(listview);
	return true;
}

bool listview_handle(listview_t *listview, const struct kbd_event *e) {
	if (!e->pressed)
		return false;
	switch (e->key) {
	case KEY_DOWN:
		listview_set_selected_index(listview, listview->selected_index + 1, true);
		return true;
	case KEY_UP:
		listview_set_selected_index(listview, listview->selected_index - 1, true);
		return true;
	case KEY_HOME:
		listview_set_selected_index(listview, 0, true);
		return true;
	case KEY_END:
		listview_set_selected_index(listview, listview->items->count - 1, true);
		return true;
	case KEY_PGUP:
		listview_set_selected_index(listview, listview->selected_index - listview->max_items, true);
		return true;
	case KEY_PGDN:
		listview_set_selected_index(listview, listview->selected_index + listview->max_items, true);
		return true;
	default:
		break;
	}
	return false;
}

bool listview_get_data(listview_t *listview, int what, data_t *data) {
	switch (what) {
	case 0:
		data->data = (void *)listview->selected_index;
		data->size = 4;
		return true;
	case 1:
		if (listview->selected_index >= 0) {
			list_item_t *li = listview->items->head;
			for (int i = 0; li && i < listview->selected_index; i++, li = li->next);
			if (li) {
				data->data = (void *)li->obj;
				data->size = sizeof(list_item_t *);
				return true;
			}
		}

		data->data = NULL;
		data->size = sizeof(list_item_t *);
		return true;
	}
	return false;
}

bool listview_set_data(listview_t *listview, int what, const void *data,
		size_t data_len __attribute__((unused))) {
	switch (what) {
	case 0:
		listview->selected_index = (int32_t)(intptr_t)data;
		listview_draw(listview);
		return true;
	case 1:
		listview_selected_index_changed(listview);
		return true;
	}
	return false;
}

bool listview_is_empty(listview_t *listview) {
	return listview->selected_index < 0;
}
