#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/control.h"

void control_init(control_t *control,
		int id,
		GCPtr gc,
		int x, int y, int width, int height,
		control_api_t* api) {
	control->id = id;
	control->gc = gc;
	control->x = x;
	control->y = y;
	control->width = width;
	control->height = height;
	control->api = *api;
	control->focused = false;
	control->enabled = true;
	control->extra = NULL;
	control->parent.parent = NULL;
	control->parent.draw = NULL;
}

void* control_get_extra(struct control_t *control) {
	return control->extra;
}
void control_set_extra(struct control_t *control, void *extra) {
	control->extra = extra;
}

void control_set_parent(struct control_t *control, control_parent_t *parent) {
	if (parent)
		memcpy(&control->parent, parent, sizeof(*parent));
	else
		memset(&control->parent, 0, sizeof(*parent));
	if (control->api.set_parent)
		control->api.set_parent(control);
}

void control_refresh_parent(struct control_t *control) {
	if (control->parent.parent && control->parent.draw)
		control->parent.draw(control->parent.parent);
}

void control_destroy(control_t *control) {
	control->api.destroy(control);
}

void control_draw(control_t *control) {
	control->api.draw(control);
}

bool control_focus(control_t *control, bool focus) {
	return control->api.focus(control, focus);
}

bool control_handle(struct control_t *control, const struct kbd_event *e) {
	return control->api.handle(control, e);
}

bool control_get_data(struct control_t *control, int what, data_t *data) {
	if (control->api.get_data)
		return control->api.get_data(control, what, data);
	return false;
}

bool control_set_data(struct control_t *control, int what, const void *data, size_t data_len) {
	if (control->api.set_data)
		return control->api.set_data(control, what, data, data_len);
	return false;
}

bool control_is_empty(struct control_t * control) {
	if (control->api.is_empty)
		return control->api.is_empty(control);
	return true;
}

bool control_set_enabled(struct control_t *control, bool enabled) {
	if (control->api.set_enabled)
		return control->api.set_enabled(control, enabled);
	return true;
}

void fill_rect(GCPtr screen, int x, int y, int width, int height, int border_width,
		Color border_color, int bg_color) {
	// рамка
	SetBrushColor(screen, border_color);
	SetRop2Mode(screen, R2_COPY);
	FillBox(screen, x, y, width, border_width);
	FillBox(screen, x, y + height - border_width, width, border_width);
	FillBox(screen, x, y + border_width, border_width, height - border_width - 1);
	FillBox(screen, x + width - border_width, y + border_width, 
		border_width, height - border_width - 1);

	// заполнение
	if (bg_color > 0) {
		SetBrushColor(screen, bg_color);
		FillBox(screen, x + border_width, y + border_width, 
			width - border_width * 2, height - border_width * 2);
	}
}
