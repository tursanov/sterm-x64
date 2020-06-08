#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/combobox.h"

typedef struct combobox_t {
	control_t control;
	char **items;
	size_t item_count;
	bool expanded;
	int dropdown_y;
	int dropdown_height;
	intptr_t selected_index;
	int tmp_selected_index;
	control_t *edit;
} combobox_t;

void combobox_destroy(combobox_t *combobox);
void combobox_draw(combobox_t *combobox);
bool combobox_focus(combobox_t *combobox, bool focus);
bool combobox_handle(combobox_t *combobox, const struct kbd_event *e);
bool combobox_get_data(combobox_t *combobox, int what, data_t *data);
bool combobox_set_data(combobox_t *combobox, int what, const void *data, size_t data_len);
bool combobox_is_empty(combobox_t *combobox);

#define screen combobox->control.gc

control_t* combobox_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char *text, edit_input_type_t input_type, size_t max_length,
	const char **items, size_t item_count, int value, int flags)
{
    combobox_t *combobox = (combobox_t *)malloc(sizeof(combobox_t));
    control_api_t api = {
		(void (*)(struct control_t *))combobox_destroy,
		(void (*)(struct control_t *))combobox_draw,
		(bool (*)(struct control_t *, bool))combobox_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))combobox_handle,
		(bool (*)(struct control_t *, int, data_t *))combobox_get_data,
		(bool (*)(struct control_t *control, int, const void *, size_t))combobox_set_data,
		(bool (*)(struct control_t *control))combobox_is_empty,
		NULL,
		NULL,
    };

    control_init(&combobox->control, id, gc, x, y, width, height, &api);

	combobox->item_count = item_count;
	combobox->items = (char **)malloc(sizeof(char *) * combobox->item_count);
	combobox->expanded = false;


	if ((flags & 1) != 0)
		combobox->edit = edit_create(0, gc, x, y, width - 20, height, text, input_type, max_length);
	else
		combobox->edit = NULL;

	for (size_t i = 0; i < combobox->item_count; i++)
		combobox->items[i] = strdup(items[i]);

	combobox->dropdown_height = combobox->item_count * (GetTextHeight(screen) + BORDER_WIDTH * 2) +
		BORDER_WIDTH * 2;
	y = combobox->control.y + combobox->control.height - BORDER_WIDTH;

	if (y + combobox->dropdown_height > DISCY) {
		if (y - combobox->control.height - combobox->dropdown_height < 0)
			y = (DISCY - combobox->dropdown_height) / 2;
		else
			y = combobox->control.y - combobox->dropdown_height + BORDER_WIDTH;
	}
	combobox->dropdown_y = y;
	combobox->selected_index = value;

    return (control_t *)combobox;
}

control_t* simple_combobox_create(int id, GCPtr gc, int x, int y, int width, int height,
		const char **items, size_t item_count, int value) {
	return combobox_create(id, gc, x, y, width, height,
		NULL, 0, 0, items, item_count, value, 0);
}

control_t* edit_combobox_create(int id, GCPtr gc, int x, int y, int width, int height,
		const char *text, edit_input_type_t input_type, size_t max_length,
		const char **items, size_t item_count) {
	return combobox_create(id, gc, x, y, width, height,
		text, input_type, max_length, items, item_count, -1, 1);
}

void combobox_destroy(combobox_t *combobox) {
	if (combobox->item_count > 0) {
		for (size_t i = 0; i < combobox->item_count; i++) {
			if (combobox->items[i])
				free(combobox->items[i]);
		}
		free(combobox->items);
	}

	if (combobox->edit)
		control_destroy(combobox->edit);

	free(combobox);
}

static void combobox_draw_normal(combobox_t *combobox) {
	Color borderColor;
	Color bgColor;
	Color fgColor;

	if (combobox->control.focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
		fgColor = RGB(32, 32, 32);
	}
	
	int y = (combobox->control.height - GetTextHeight(screen)) / 2 - BORDER_WIDTH;
	
	if (combobox->edit) {
		control_draw(combobox->edit);
		fill_rect(screen, combobox->control.x + combobox->control.width - 20 - BORDER_WIDTH,
			combobox->control.y,
			20 + BORDER_WIDTH, combobox->control.height, BORDER_WIDTH,
			borderColor, bgColor);
		SetTextColor(screen, borderColor);
		TextOut(screen, combobox->control.x + combobox->control.width - 
			BORDER_WIDTH * 3 - GetMaxCharWidth(screen),
			combobox->control.y + y + 3, "\x1f");
		return;
	}
	

	fill_rect(screen, combobox->control.x, combobox->control.y,
		combobox->control.width, combobox->control.height, BORDER_WIDTH,
		borderColor, bgColor);

	SetGCBounds(screen, combobox->control.x + BORDER_WIDTH, combobox->control.y + BORDER_WIDTH,
		combobox->control.width - BORDER_WIDTH * 2, combobox->control.height - BORDER_WIDTH * 2);
	//int y = (combobox->control.height - GetTextHeight(screen)) / 2 - BORDER_WIDTH;
	if (combobox->selected_index >= 0 && combobox->selected_index < combobox->item_count) {
		const char *text = combobox->items[combobox->selected_index];
		int w = GetTextWidth(screen, text);
		int x = (combobox->control.width - w) / 2;
		SetTextColor(screen, fgColor);

		TextOut(screen, x, y, text);
	}
	SetTextColor(screen, borderColor);
	TextOut(screen, combobox->control.width - BORDER_WIDTH * 3 - GetMaxCharWidth(screen),
		y + 1, "\x1f");
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

static void combobox_draw_expanded(combobox_t *combobox) {
	Color borderColor = clRopnetDarkBrown;
	//Color bgColor = clRopnetBrown;

	fill_rect(screen, combobox->control.x, combobox->dropdown_y,
		combobox->control.width, combobox->dropdown_height,
		BORDER_WIDTH, borderColor, -1);

	SetGCBounds(screen, combobox->control.x + BORDER_WIDTH, combobox->dropdown_y + BORDER_WIDTH,
		combobox->control.width - BORDER_WIDTH * 2, combobox->dropdown_height - BORDER_WIDTH * 2);

	int x = BORDER_WIDTH + GetMaxCharWidth(screen)*2;
	int y = 0;
	SetTextColor(screen, clBlack);
	for (size_t i = 0; i < combobox->item_count; i++, y += GetTextHeight(screen) + BORDER_WIDTH * 2) {
		SetBrushColor(screen, i == combobox->tmp_selected_index ? clBlue : clRopnetBrown);
		SetTextColor(screen, i == combobox->tmp_selected_index ? clWhite : clBlack);
		FillBox(screen, 0, y, combobox->control.width - BORDER_WIDTH * 2,
			GetTextHeight(screen) + BORDER_WIDTH*2);
		TextOut(screen, x, y, combobox->items[i]);
	}
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

void combobox_draw(combobox_t *combobox) {
	if (combobox->expanded)
		combobox_draw_expanded(combobox);
	else
		combobox_draw_normal(combobox);
}

bool combobox_focus(combobox_t *combobox, bool focus) {
	if (combobox->edit)
		control_focus(combobox->edit, focus);
	combobox->control.focused = focus;
	combobox_draw(combobox);
	return true;
}

static void combobox_sync_edit_text(combobox_t *combobox) {
	if (combobox->edit) {
		if (combobox->selected_index >= 0) {
			char *text = combobox->items[combobox->selected_index];
			int textlen = strlen(text);
			control_set_data(combobox->edit, 0, text, (size_t)textlen);
		}
	}
}

bool combobox_handle(combobox_t *combobox, const struct kbd_event *e) {
	if (combobox->edit) {
		bool ret = control_handle(combobox->edit, e);
		if (ret)
			return ret;
	}
	switch (e->key) {
	case KEY_ENTER:
	case KEY_SPACE:
		if (e->pressed && !e->repeated) {
			if (!combobox->expanded) {
				if (combobox->item_count > 0) {
					combobox->tmp_selected_index = combobox->selected_index;
					combobox->expanded = true;
					combobox_draw(combobox);
				} else
					return false;
			} else if (e->key == KEY_ENTER) {
				combobox->selected_index = combobox->tmp_selected_index;
				combobox->expanded = false;

				if (combobox->edit)
					combobox_sync_edit_text(combobox);

				control_refresh_parent(&combobox->control);
			}
		}
		return true;
	case KEY_ESCAPE:
		if (combobox->expanded) {
			combobox->expanded = false;
			control_refresh_parent(&combobox->control);
			return true;
		}
		break;
	case KEY_TAB:
		return combobox->expanded;
	case KEY_DOWN:
		if (combobox->expanded && e->pressed) {
			combobox->tmp_selected_index++;
			if (combobox->tmp_selected_index >= combobox->item_count)
				combobox->tmp_selected_index = 0;
			combobox_draw(combobox);
		}
		return combobox->expanded;
	case KEY_UP:
		if (combobox->expanded && e->pressed) {
			combobox->tmp_selected_index--;
			if (combobox->tmp_selected_index < 0)
				combobox->tmp_selected_index = combobox->item_count - 1;
			combobox_draw(combobox);
		}
		return combobox->expanded;
	case KEY_PGUP:
		if (e->pressed && !combobox->expanded && combobox->item_count > 0) {
			combobox->selected_index--;
			if (combobox->selected_index < 0)
				combobox->selected_index = combobox->item_count - 1;
			if (combobox->edit)
				combobox_sync_edit_text(combobox);
			else
				combobox_draw(combobox);
			return true;
		}
		break;
	case KEY_PGDN:
		if (e->pressed && !combobox->expanded && combobox->item_count > 0) {
			combobox->selected_index++;
			if (combobox->selected_index >= combobox->item_count)
				combobox->selected_index = 0;
			if (combobox->edit)
				combobox_sync_edit_text(combobox);
			else
				combobox_draw(combobox);
			return true;
		}
		break;
	}
	return false;
}

bool combobox_get_data(combobox_t *combobox, int what, data_t *data) {
	if (combobox->edit)
		return control_get_data(combobox->edit, what, data);
	switch (what) {
	case 0:
		data->data = (void *)combobox->selected_index;
		data->size = 4;
		return true;
	}
	return false;
}

bool combobox_set_data(combobox_t *combobox, int what, const void *data, size_t data_len) {
	if (combobox->edit)
		return control_set_data(combobox->edit, what, data, data_len);
	switch (what) {
	case 0:
		combobox->selected_index = (intptr_t)data;
		combobox_draw(combobox);
		return true;
	}
	return false;
}

bool combobox_is_empty(combobox_t *combobox) {
	if (combobox->edit) {
		data_t data;
		if (control_get_data(combobox->edit, 0, &data))
			return data.size == 0;
		return true;
	} else
		return combobox->selected_index < 0;
}	
