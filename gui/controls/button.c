#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/button.h"

typedef struct button_t {
	control_t control;
	char *text;
	char bitnames[8];
	bool pressed;
	int cmd;
	action_t action;
} button_t;

void button_destroy(button_t *button);
void button_draw(button_t *button);
bool button_focus(button_t *button, bool focus);
bool button_handle(button_t *button, const struct kbd_event *e);

#define screen	(button->control.gc)

control_t* button_create(int id, GCPtr gc, int x, int y, int width, int height,
	int cmd, const char *text, action_t action) 
{
    button_t *button = (button_t *)malloc(sizeof(button_t));
    control_api_t api = {
		(void (*)(struct control_t *))button_destroy,
		(void (*)(struct control_t *))button_draw,
		(bool (*)(struct control_t *, bool))button_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))button_handle,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
    };

    control_init(&button->control, id, gc, x, y, width, height, &api);

	if (text != NULL)
	    button->text = strdup(text);

	button->pressed = false;
	button->cmd = cmd;
	button->action = action;

    return (control_t *)button;
}

void button_destroy(button_t *button) {
	if (button->text != NULL)
		free(button->text);
	free(button);
}

void draw_button(GCPtr gc, int x, int y, int width, int height, 
		const char *text, bool focused) {
	Color borderColor;
	Color bgColor;
	Color fgColor;

	if (focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
		fgColor = RGB(32, 32, 32);
	}

	fill_rect(gc, x, y, width, height, 2, borderColor, bgColor);

	if (text) {
		int tw = GetTextWidth(gc, text);

		x += (width - tw) / 2;
		y += (height - GetTextHeight(gc))/2;
		SetTextColor(gc, fgColor);
		TextOut(gc, x, y, text);
	}
}


void button_draw(button_t *button) {
	draw_button(screen, button->control.x, button->control.y,
		button->control.width, button->control.height, button->text,
		button->control.focused);
}

bool button_focus(button_t *button, bool focus) {
	button->control.focused = focus;
	button_draw(button);
	return true;
}

bool button_handle(button_t *button, const struct kbd_event *e) {
	switch (e->key) {
	case KEY_ENTER:
	case KEY_SPACE:
		if (e->pressed && !e->repeated) {
			button->pressed = true;
			button_draw(button);
			button->pressed = false;
			if (button->action)
				button->action(&button->control, button->cmd);
			kbd_flush_queue();
			return true;
		}
		break;
	}
	return false;
}

