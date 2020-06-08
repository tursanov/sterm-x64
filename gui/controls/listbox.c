#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/listbox.h"

typedef struct listbox_t {
	control_t control;
	list_t *items;
	int top_index;
	intptr_t selected_index;
	int item_height;
	int max_items;
	listbox_get_item_text_func_t get_item_text_func;
} listbox_t;

void listbox_destroy(listbox_t *listbox);
void listbox_draw(listbox_t *listbox);
bool listbox_focus(listbox_t *listbox, bool focus);
bool listbox_handle(listbox_t *listbox, const struct kbd_event *e);
bool listbox_get_data(listbox_t *listbox, int what, data_t *data);
bool listbox_set_data(listbox_t *listbox, int what, const void *data, size_t data_len);
bool listbox_is_empty(listbox_t *listbox);

#define screen listbox->control.gc
#define BORDER_WIDTH	2

static bool listbox_redraw_row(listbox_t *listbox, int index);

static void listbox_set_selected_index(listbox_t *lv, int index, bool refresh) {
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
				listbox_draw(lv);
			} else {
				listbox_redraw_row(lv, old);
				listbox_redraw_row(lv, lv->selected_index);
			}
		}
	}
}

control_t* listbox_create(int id, GCPtr gc, int x, int y, int width, int height,
		list_t *items, listbox_get_item_text_func_t get_item_text_func,
		int selected_index) {
    listbox_t *listbox = (listbox_t *)malloc(sizeof(listbox_t));
    control_api_t api = {
		(void (*)(struct control_t *))listbox_destroy,
		(void (*)(struct control_t *))listbox_draw,
		(bool (*)(struct control_t *, bool))listbox_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))listbox_handle,
		(bool (*)(struct control_t *, int, data_t *))listbox_get_data,
		(bool (*)(struct control_t *control, int, const void *, size_t))listbox_set_data,
		(bool (*)(struct control_t *control))listbox_is_empty,
		NULL,
		NULL,
    };

    control_init(&listbox->control, id, gc, x, y, width, height, &api);

	listbox->items = items;
	listbox->get_item_text_func = get_item_text_func;
	listbox->item_height = GetTextHeight(screen) + 2;
	listbox->max_items = (listbox->control.height - BORDER_WIDTH*2) / listbox->item_height;
	listbox->top_index = 0;
	listbox->selected_index = selected_index + 1;
	listbox_set_selected_index(listbox, selected_index, false);

    return (control_t *)listbox;
}

void listbox_destroy(listbox_t *listbox) {
	free(listbox);
}

static list_item_t * listbox_get_top_item(listbox_t *listbox) {
	list_item_t *li = listbox->items->head;
	for (int i = 0; i < listbox->top_index; i++, li = li->next);
	return li;
}

static void listbox_draw_row(listbox_t *listbox, int index, list_item_t *i, int left, int top, int w, int h) {
	SetBrushColor(screen, listbox->selected_index == index ?
			(listbox->control.focused ? clNavy : clGray) : 
			(listbox->control.focused ? clRopnetBrown : clSilver));
	SetTextColor(screen, listbox->selected_index == index ? clWhite : clBlack);
	
	SetGCBounds(screen, left, top, w, h);
	FillBox(screen, 0, 0, w, h);

	char text[256];
	listbox->get_item_text_func(i->obj, text, sizeof(text) - 1);
	SetGCBounds(screen, left, top, w, h);
	TextOut(screen, 4, 0, text);
}

static bool listbox_redraw_row(listbox_t *listbox, int index) {
	if (index >= listbox->top_index && index < listbox->top_index + listbox->max_items &&
			index >= 0 && index < listbox->items->count) {
		list_item_t *li = listbox_get_top_item(listbox);
		if (li) {
			int x = listbox->control.x + 2;
			int y = listbox->control.y + 4;
			int w = listbox->control.width - 4;
			int h = listbox->item_height;
			for (int i = 0; i < index - listbox->top_index; i++, li = li->next,
					y += listbox->item_height);
			listbox_draw_row(listbox, index, li, x, y, w, h);
			SetGCBounds(screen, 0, 0, DISCX, DISCY);

			return true;
		}
	}
	return false;
}

void listbox_draw_items(listbox_t *listbox) {
	list_item_t *li = listbox_get_top_item(listbox);
	int x = listbox->control.x + 2;
	int y = listbox->control.y + 4;
	int w = listbox->control.width - 4;
	int h = listbox->item_height;
	for (int i = 0; li && i < listbox->max_items; i++, li = li->next, y += listbox->item_height) {
		//printf("i = %d, li = %p\n", i, li);
		listbox_draw_row(listbox, listbox->top_index + i, li, x, y, w, h);
	}
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

void listbox_draw(listbox_t *listbox) {
	Color borderColor;
	Color bgColor;
//	Color fgColor;

	if (listbox->control.focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
//		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
//		fgColor = RGB(32, 32, 32);
	}
	
	fill_rect(screen, listbox->control.x, listbox->control.y,
		listbox->control.width, listbox->control.height, BORDER_WIDTH,
		borderColor, bgColor);

	SetGCBounds(screen, listbox->control.x + BORDER_WIDTH, listbox->control.y + BORDER_WIDTH,
		listbox->control.width - BORDER_WIDTH * 2, listbox->control.height - BORDER_WIDTH * 2);

	if (listbox->items->count > 0) {
	   	if (listbox->selected_index < 0) {
			listbox->top_index = 0;
			listbox->selected_index = 0;
		} else if (listbox->selected_index >= listbox->items->count) {
			listbox_set_selected_index(listbox, listbox->items->count - 1, false);
		}
	}

	if (listbox->items->count > 0)
		listbox_draw_items(listbox);

	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

bool listbox_focus(listbox_t *listbox, bool focus) {
	listbox->control.focused = focus;
	listbox_draw(listbox);
	return true;
}

bool listbox_handle(listbox_t *listbox, const struct kbd_event *e) {
	if (!e->pressed)
		return false;
	switch (e->key) {
	case KEY_DOWN:
		listbox_set_selected_index(listbox, listbox->selected_index + 1, true);
		return true;
	case KEY_UP:
		listbox_set_selected_index(listbox, listbox->selected_index - 1, true);
		return true;
	case KEY_HOME:
		listbox_set_selected_index(listbox, 0, true);
		return true;
	case KEY_END:
		listbox_set_selected_index(listbox, listbox->items->count - 1, true);
		return true;
	case KEY_PGUP:
		listbox_set_selected_index(listbox, listbox->selected_index - listbox->max_items, true);
		return true;
	case KEY_PGDN:
		listbox_set_selected_index(listbox, listbox->selected_index + listbox->max_items, true);
		return true;
	default:
		break;
	}
	return false;
}

bool listbox_get_data(listbox_t *listbox, int what, data_t *data) {
	switch (what) {
	case 0:
		data->data = (void *)listbox->selected_index;
		data->size = 4;
		return true;
	case 1:
		if (listbox->selected_index >= 0) {
			list_item_t *li = listbox->items->head;
			int si = listbox->selected_index - listbox->top_index;
			for (int i = 0; li && i < listbox->max_items; i++, li = li->next) {
				if (i == si) {
					data->data = (void *)li->obj;
					data->size = sizeof(list_item_t *);
					return true;
				}
			}
		}

		data->data = NULL;
		data->size = sizeof(list_item_t *);
		return true;
	}
	return false;
}

bool listbox_set_data(listbox_t *listbox, int what, const void *data, size_t data_len) {
	switch (what) {
	case 0:
		listbox_set_selected_index(listbox, (int32_t)(intptr_t)data, false);
		return true;
	}
	return false;
}

bool listbox_is_empty(listbox_t *listbox) {
	return listbox->selected_index < 0;
}
