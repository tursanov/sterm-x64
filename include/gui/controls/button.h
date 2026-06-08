#ifndef BUTTON_H
#define BUTTON_H

#if defined __cplusplus
extern "C" {
#endif

#include "gui/controls/control.h"

typedef void (*action_t)(control_t *, int cmd);

extern control_t* button_create(int id, GCPtr gc, int x, int y, int width, int height,
	int cmd, const char *text, action_t action);

extern void draw_button(GCPtr screen, int x, int y, int width, int height, 
		const char *text, bool focused);
extern void draw_button_ex(GCPtr gc, int x, int y, int width, int height, 
		const char *text, bool focused, bool enabled);

#if defined __cplusplus
}
#endif

#endif
