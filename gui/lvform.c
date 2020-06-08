#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/forms.h"
#include "gui/dialog.h"
#include "gui/lvform.h"

static FontPtr fnt = NULL;
static GCPtr screen = NULL;
static int result = 0;
static int item_h;
static int items_y;
static int items_bottom;
static int max_items;
static int active_element = 0;
#define GAP	10
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

lvform_t *lvform_create(const char *title, 
		lvform_column_t* columns, size_t column_count,
		data_source_t *data_source) 
{
	lvform_t *lvform = (lvform_t *)malloc(sizeof(lvform_t));

	lvform->top_index = lvform->selected_index = data_source->list->head ? 0 : -1;

	lvform->data_source = data_source;
	lvform->title = title;
	lvform->columns = columns;
	lvform->column_count = column_count;

	return lvform;
}

void lvform_destroy(lvform_t *lvform) {
	free(lvform);
}

extern void draw_title(GCPtr screen, FontPtr fnt, const char *title);
extern void fill_rect(GCPtr screen, int x, int y, int width, int height, int border_width,
		Color border_color, int bg_color);

#define ACTIVE_COLOR	RGB(48,48,48)

static void lvform_draw_column(lvform_column_t *c, int x, int y, int h) {
	SetGCBounds(screen, x, y, c->width, h);
	SetTextColor(screen, active_element == 0 ? ACTIVE_COLOR : clGray);
	TextOut(screen, 4, 0, c->title);
	SetBrushColor(screen, active_element == 0 ? ACTIVE_COLOR : clGray);
	FillBox(screen, c->width - 2, 0, 2, h);
}

static void lvform_draw_columns(lvform_t *lvform, int y) {
	lvform_column_t *c = lvform->columns;
	int x = GAP + 2;
	int h = fnt->max_height + 1;

	for (int i = 0; i < lvform->column_count; i++, x += c->width, c++)
		lvform_draw_column(c, x, y, h);
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
	SetBrushColor(screen, active_element == 0 ? ACTIVE_COLOR : clGray);
	FillBox(screen, GAP + 2, y + h, DISCX - GAP*2 - 4, 2);
}

static list_item_t *lvform_get_top_item(lvform_t *lvform, int *idx) {
	list_item_t *item = lvform->data_source->list->head;
	int n = 0;
	for (n = 0; n < lvform->top_index && item; n++, item = item->next);

	if (!item) {
		item = lvform->data_source->list->head;
		if (item)
			lvform->top_index = lvform->selected_index = 0;
		else
			lvform->top_index = lvform->selected_index = -1;
	}

	*idx = n;

	return item;
}

static list_item_t *lvform_get_selected_item(lvform_t *lvform) {
	list_item_t *item = lvform->data_source->list->head;
	for (int n = 0; n < lvform->selected_index && item; n++, item = item->next);
	return item;
}

static void lvform_draw_row(lvform_t *lvform, int index, list_item_t *item, 
		int x, int y, int h) {
	lvform_column_t *c = lvform->columns;

	SetGCBounds(screen, x + 2, y, DISCX - x*2  - 4, h);
	SetBrushColor(screen, lvform->selected_index == index ? 
			(active_element == 0 ? clNavy : clGray) : clSilver);
	SetTextColor(screen, lvform->selected_index == index ? clWhite : clBlack);
	FillBox(screen, 0, 0, DISCX - x*2 - 4, h);

	for (int j = 0; j < lvform->column_count; j++, x += c->width, c++) {
		char text[256];
		if (lvform->data_source->get_text(item->obj, j, text, sizeof(text) - 1) == 0) {
			SetGCBounds(screen, x, y, c->width, h);
			TextOut(screen, 4, 0, text);
		}
	}
}

static void lvform_redraw_rows(lvform_t *lvform, int index1, int index2) {
	int x = GAP;
	int y = items_y;
	int n;
	int i1, i2;
	list_item_t *item = lvform_get_top_item(lvform, &n);

	if (index1 < index2) {
		i1 = index1;
		i2 = index2;
	} else {
		i1 = index2;
		i2 = index1;
	}

	for (; item && n <= i2; item = item->next, n++) {
		if (n == i1 || n == i2) {
			lvform_draw_row(lvform, n, item, x, y, item_h);
		}
		y += item_h;
	}
}

void lvform_draw_items(lvform_t *lvform) {
	int y = items_y;

	if (lvform->top_index >= 0) {
		int n;
		list_item_t *item = lvform_get_top_item(lvform, &n);

		for (; item && y + item_h < items_bottom; item = item->next, n++) {
			lvform_draw_row(lvform, n, item, GAP, y, item_h);
			y += item_h;
		}
	}
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

static void lvform_draw_buttons() {
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
	draw_button(screen, GAP + (BUTTON_WIDTH + GAP)*0, items_bottom + GAP, 
			BUTTON_WIDTH, BUTTON_HEIGHT, "Добавить", active_element == 1);
	draw_button(screen, GAP + (BUTTON_WIDTH + GAP)*1, items_bottom + GAP,
			BUTTON_WIDTH, BUTTON_HEIGHT, "Изменить", active_element == 2);
	draw_button(screen, GAP + (BUTTON_WIDTH + GAP)*2, items_bottom + GAP,
			BUTTON_WIDTH, BUTTON_HEIGHT, "Удалить", active_element == 3);
	draw_button(screen, DISCX - BUTTON_WIDTH - GAP, items_bottom + GAP,
			BUTTON_WIDTH, BUTTON_HEIGHT, "Закрыть", active_element == 4);
}

static void lvform_draw_listview(lvform_t *lvform) {
	int x = GAP;
	int y = 30;

	SetGCBounds(screen, 0, 0, DISCX, DISCY);
	fill_rect(screen, x, y, DISCX - GAP*2, items_bottom - y, 2, 
			active_element == 0 ? ACTIVE_COLOR : clGray, -1);
	lvform_draw_columns(lvform, y + 2);
	lvform_draw_items(lvform);
	lvform_draw_buttons();
}

void lvform_draw(lvform_t *lvform) {
	//int x = GAP;
	//int y = 0;

	SetGCBounds(screen, 0, 0, DISCX, DISCY);
	ClearGC(screen, clSilver);
	draw_title(screen, fnt, lvform->title);
	//y += 30;

	lvform_draw_listview(lvform);
}


static void lvform_set_selected_index(lvform_t *lvform, int index) {
	if (lvform->data_source->list->head) {
		if (index < 0)
			index = 0;
		else if (index >= lvform->data_source->list->count)
			index = lvform->data_source->list->count - 1;

		if (index != lvform->selected_index) {
			int old = lvform->selected_index;
			lvform->selected_index = index;

			int y = items_y + (index - lvform->top_index) * item_h;

			if (y < items_y) {
				lvform->top_index = index;
				lvform_draw_items(lvform);
			} else if (y + item_h > items_bottom) {
				lvform->top_index = index - max_items;
				lvform_draw_items(lvform);
			} else {
				lvform_redraw_rows(lvform, old, index);
			}
		}
	}
}

static void lvform_next_element(lvform_t *lvform, bool prev) {
//	int old = active_element;
	if (!prev) {
		if (active_element == 4)
			active_element = 0;
		else
			active_element++;
	} else {
		if (active_element == 0)
			active_element = 4;
		else
			active_element--;
	}

	//if (old == 0 || active_element == 0)
	lvform_draw_listview(lvform);
	//	lvform_redraw_rows(lvform, lvform->selected_index, -1);
	//lvform_draw_buttons();
}

static void lvform_new_action(lvform_t *lvform) {
	void *obj;
	if ((obj = lvform->data_source->new_item(lvform->data_source))) {
		list_add(lvform->data_source->list, obj);

		lvform->selected_index = lvform->data_source->list->count - 1;
		lvform->top_index = lvform->selected_index - max_items;
		if (lvform->top_index < 0)
			lvform->top_index = 0;
	}
	lvform_draw(lvform);
}

static void lvform_edit_action(lvform_t *lvform) {
	if (lvform->data_source->list->head) {
		list_item_t *item = lvform_get_selected_item(lvform);
		if (item) {
			if ((lvform->data_source->edit_item(lvform->data_source, item->obj))) {
			}
			lvform_draw(lvform);
		}
	}
}

static void lvform_delete_action(lvform_t *lvform) {
	if (lvform->data_source->list->head) {
		list_item_t *item = lvform_get_selected_item(lvform);

		if (item) {
			if (message_box("Уведомление", "Выбранный элемент будет удален\nПродолжить?",
						dlg_yes_no, 0, al_center) == DLG_BTN_YES) {
				if (lvform->data_source->remove_item(lvform->data_source, item->obj) == 0) {
					list_remove(lvform->data_source->list, item->obj);

					if (lvform->selected_index == lvform->data_source->list->count) {
						lvform->selected_index--;
						if (lvform->top_index > lvform->selected_index)
							lvform->top_index = lvform->selected_index;
					}

					if (lvform->top_index > 0)
						lvform->top_index--;

				}
			}
			lvform_draw(lvform);
		}
	}
}

static bool lvform_action(lvform_t *lvform) {
	switch (active_element) {
		case 0:
			if (lvform->selected_index >= 0)
				lvform_edit_action(lvform);
			else
				lvform_new_action(lvform);
			break;
		case 1:
			lvform_new_action(lvform);
			break;
		case 2:
			lvform_edit_action(lvform);
			break;
		case 3:
			lvform_delete_action(lvform);
			break;
		default:
			result = 0;
			return false;
	}
	return true;
}

extern int kbd_lang_ex;
extern int kbd_get_char_ex(int key);

static bool lvform_process(lvform_t *lvform, const struct kbd_event *_e) {
	struct kbd_event e = *_e;
	
	if (e.key == KEY_CAPS && e.pressed && !e.repeated) {
		if (kbd_lang_ex == lng_rus)
			kbd_lang_ex = lng_lat;
		else
			kbd_lang_ex = lng_rus;
	}

	e.ch = kbd_get_char_ex(e.key);

	if (e.pressed) {
		switch (e.key) {
			case KEY_ESCAPE:
				result = 0;
				return false;
			case KEY_DOWN:
				lvform_set_selected_index(lvform, lvform->selected_index + 1);
				break;
			case KEY_UP:
				lvform_set_selected_index(lvform, lvform->selected_index - 1);
				break;
			case KEY_PGUP:
				lvform_set_selected_index(lvform, lvform->selected_index - max_items);
				break;
			case KEY_PGDN:
				lvform_set_selected_index(lvform, lvform->selected_index + max_items);
				break;
			case KEY_HOME:
				lvform_set_selected_index(lvform, 0);
				break;
			case KEY_END:
				lvform_set_selected_index(lvform, lvform->data_source->list->count - 1);
				break;
			case KEY_TAB:
				lvform_next_element(lvform, (e.shift_state & SHIFT_SHIFT) != 0);
				break;
			case KEY_ENTER:
				return lvform_action(lvform);
			case KEY_DEL:
				lvform_delete_action(lvform);
		}
	}

	return true;
}

int lvform_execute(lvform_t *lvform) {
	struct kbd_event e;

	fnt = CreateFont(_("fonts/terminal10x18.fnt"), false);
   	screen = CreateGC(0, 0, DISCX, DISCY);
    SetFont(screen, fnt);

	active_element = 0;
	item_h = fnt->max_height + 2;
	items_y = 30 + item_h + 4;
	items_bottom = DISCY - 100;
	max_items = ((items_bottom - items_y) / item_h) - 1;

	lvform_draw(lvform);

	kbd_flush_queue();
	do {
		kbd_get_event(&e);
	} while (lvform_process(lvform, &e));

	DeleteFont(fnt);
	DeleteGC(screen);

	return result;
}

