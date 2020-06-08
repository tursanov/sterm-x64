#ifndef BUTTON_H
#define BUTTON_H

#include "gui/controls/control.h"

typedef void (*action_t)(control_t *, int cmd);

control_t* button_create(int id, GCPtr gc, int x, int y, int width, int height,
	int cmd, const char *text, action_t action);

void draw_button(GCPtr screen, int x, int y, int width, int height, 
		const char *text, bool focused);

#endif
