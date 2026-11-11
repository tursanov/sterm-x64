#if !defined GUI_CONTROLS_BUTTON_H
#define GUI_CONTROLS_BUTTON_H

#include "gui/controls/control.h"

__BEGIN_DECLS

typedef void (*action_t)(control_t *, int cmd);

extern control_t* button_create(int id, GCPtr gc, int x, int y, int width, int height,
	int cmd, const char *text, action_t action);

extern void draw_button(GCPtr screen, int x, int y, int width, int height, 
		const char *text, bool focused);
extern void draw_button_ex(GCPtr gc, int x, int y, int width, int height, 
		const char *text, bool focused, bool enabled);

__END_DECLS

#endif
