#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/bitset.h"

typedef struct bitset_t {
	control_t control;
	char *text;
	char **short_items;
	char **items;
	uint8_t *bits;
	int tmp_value;
	intptr_t value;
	size_t item_count;
	bool expanded;
	int dropdown_y;
	int dropdown_height;
	int selected_index;
} bitset_t;

void bitset_destroy(bitset_t *bitset);
void bitset_draw(bitset_t *bitset);
bool bitset_focus(bitset_t *bitset, bool focus);
bool bitset_handle(bitset_t *bitset, const struct kbd_event *e);
bool bitset_get_data(bitset_t *bitset, int what, data_t *data);
bool bitset_set_data(bitset_t *bitset, int what, const void *data, size_t data_len);
bool bitset_is_empty(bitset_t *bitset);

#define screen	(bitset->control.gc)


control_t* bitset_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char **short_items, const char **items, uint8_t *bits, size_t item_count, int value)
{
    bitset_t *bitset = (bitset_t *)malloc(sizeof(bitset_t));
    control_api_t api = {
		(void (*)(struct control_t *))bitset_destroy,
		(void (*)(struct control_t *))bitset_draw,
		(bool (*)(struct control_t *, bool))bitset_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))bitset_handle,
		(bool (*)(struct control_t *, int, data_t *))bitset_get_data,
		(bool (*)(struct control_t *control, int, const void *, size_t))bitset_set_data,
		(bool (*)(struct control_t *control))bitset_is_empty,
		NULL,
		NULL,
    };

    control_init(&bitset->control, id, gc, x, y, width, height, &api);

	bitset->value = value;
	bitset->item_count = item_count;
	bitset->short_items = (char **)malloc(sizeof(char *) * item_count);
	bitset->items = (char **)malloc(sizeof(char *) * item_count);
	bitset->bits = (uint8_t *)malloc(sizeof(uint8_t) * item_count);
	bitset->expanded = false;

	size_t text_size = 0;
	for (size_t i = 0; i < bitset->item_count; i++) {
		bitset->short_items[i] = strdup(short_items[i]);
		bitset->items[i] = strdup(items[i]);
		bitset->bits[i] = bits[i];
		text_size += strlen(short_items[i]) + 1;
	}

	bitset->text = (char *)malloc(text_size + 1);

	bitset->dropdown_height = bitset->item_count * (GetTextHeight(bitset->control.gc) + BORDER_WIDTH * 2) +
		BORDER_WIDTH * 2;
	y = bitset->control.y + bitset->control.height - BORDER_WIDTH;

	if (y + bitset->dropdown_height > DISCY) {
		if (y - bitset->control.height - bitset->dropdown_height < 0)
			y = (DISCY - bitset->dropdown_height) / 2;
		else
			y = bitset->control.y - bitset->dropdown_height + BORDER_WIDTH;
	}
	bitset->dropdown_y = y;
	bitset->selected_index = 0;

    return (control_t *)bitset;
}

void bitset_destroy(bitset_t *bitset) {
	if (bitset->item_count > 0) {
		for (size_t i = 0; i < bitset->item_count; i++) {
			if (bitset->short_items[i])
				free(bitset->short_items[i]);
			if (bitset->items[i])
				free(bitset->items[i]);
		}
		free(bitset->items);
		free(bitset->short_items);
		free(bitset->bits);
	}

	free(bitset->text);

	free(bitset);
}

static void bitset_draw_normal(bitset_t *bitset) {
	Color borderColor;
	Color bgColor;
	Color fgColor;

	char *s = bitset->text;
	for (size_t i = 0; i < bitset->item_count; i++)
		if (bitset->value & bitset->bits[i])
			s += sprintf(s, "%s%s", s != bitset->text ? "," : "",
				bitset->short_items[i]);
	*s = 0;

	if (bitset->control.focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
		fgColor = RGB(32, 32, 32);
	}

	fill_rect(bitset->control.gc, bitset->control.x, bitset->control.y,
		bitset->control.width, bitset->control.height, BORDER_WIDTH,
		borderColor, bgColor);

	SetGCBounds(bitset->control.gc, bitset->control.x + BORDER_WIDTH, bitset->control.y + BORDER_WIDTH,
		bitset->control.width - BORDER_WIDTH * 2, bitset->control.height - BORDER_WIDTH * 2);
	int y = (bitset->control.height - GetTextHeight(bitset->control.gc)) / 2 - BORDER_WIDTH;
	if (*bitset->text) {
		int w = GetTextWidth(bitset->control.gc, bitset->text);
		int x = (bitset->control.width - w) / 2;
		SetTextColor(screen, fgColor);

		TextOut(screen, x, y, bitset->text);
	}
	SetTextColor(screen, borderColor);
	TextOut(screen, bitset->control.width - BORDER_WIDTH * 3 - GetMaxCharWidth(screen),
		y + 1, "\x1f");
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

static void bitset_draw_expanded(bitset_t *bitset) {
	Color borderColor = clRopnetDarkBrown;
	//Color bgColor = clRopnetBrown;

	fill_rect(screen, bitset->control.x, bitset->dropdown_y,
		bitset->control.width, bitset->dropdown_height,
		BORDER_WIDTH, borderColor, -1);

	SetGCBounds(screen, bitset->control.x + BORDER_WIDTH, bitset->dropdown_y + BORDER_WIDTH,
		bitset->control.width - BORDER_WIDTH * 2, bitset->dropdown_height - BORDER_WIDTH * 2);

	int x = BORDER_WIDTH + GetMaxCharWidth(screen)*2;
	int y = 0;
	SetTextColor(screen, clBlack);
	for (size_t i = 0; i < bitset->item_count; i++, y += GetTextHeight(bitset->control.gc) + BORDER_WIDTH * 2) {
		SetBrushColor(screen, i == bitset->selected_index ? clBlue : clRopnetBrown);
		SetTextColor(screen, i == bitset->selected_index ? clWhite : clBlack);
		FillBox(screen, 0, y, bitset->control.width - BORDER_WIDTH * 2,
			GetTextHeight(bitset->control.gc) + BORDER_WIDTH*2);
		if (bitset->tmp_value & (1 << i))
			TextOut(screen, BORDER_WIDTH, y, "\xfb");

		TextOut(screen, x, y, bitset->items[i]);
	}

	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

void bitset_draw(bitset_t *bitset) {
	if (bitset->expanded)
		bitset_draw_expanded(bitset);
	else
		bitset_draw_normal(bitset);
}

bool bitset_focus(bitset_t *bitset, bool focus) {
	bitset->control.focused = focus;
	bitset_draw(bitset);
	return true;
}

bool bitset_handle(bitset_t *bitset, const struct kbd_event *e) {
	switch (e->key) {
	case KEY_ENTER:
	case KEY_SPACE:
		if (e->pressed && !e->repeated) {
			if (!bitset->expanded) {
				bitset->expanded = true;
				bitset->tmp_value = bitset->value;
			} else if (e->key == KEY_SPACE) {
				if (bitset->tmp_value & bitset->bits[bitset->selected_index])
					bitset->tmp_value &= ~bitset->bits[bitset->selected_index];
				else
					bitset->tmp_value |= bitset->bits[bitset->selected_index];
			} else if (e->key == KEY_ENTER) {
				bitset->value = bitset->tmp_value;
				goto L1;
			}
			bitset_draw(bitset);
		}
		return true;
	case KEY_ESCAPE:
L1:
		if (bitset->expanded) {
			bitset->expanded = false;
			control_refresh_parent(&bitset->control);
			return true;
		}
		__fallthrough__;
	case KEY_TAB:
		return bitset->expanded;
	case KEY_DOWN:
		if (bitset->expanded && e->pressed) {
			bitset->selected_index++;
			if (bitset->selected_index >= bitset->item_count)
				bitset->selected_index = 0;
			bitset_draw(bitset);
		}
		return bitset->expanded;
	case KEY_UP:
		if (bitset->expanded && e->pressed) {
			bitset->selected_index--;
			if (bitset->selected_index < 0)
				bitset->selected_index = bitset->item_count - 1;
			bitset_draw(bitset);
		}
		return bitset->expanded;
	}
	return false;
}

bool bitset_get_data(bitset_t *bitset, int what, data_t *data) {
	switch (what) {
	case 0:
		data->data = (void *)bitset->value;
		data->size = 4;
		return true;
	}
	return false;
}

bool bitset_set_data(bitset_t *bitset, int what, const void *data,
		size_t data_len __attribute__((unused))) {
	switch (what) {
	case 0:
		bitset->value = (intptr_t)data;
		bitset_draw(bitset);
		return true;
	}
	return false;
}


bool bitset_is_empty(bitset_t *bitset) {
	return bitset->value == 0;
}
