#if !defined GUI_CONTROLS_EDIT_H
#define GUI_CONTROLS_EDIT_H

#include "gui/controls/control.h"

__BEGIN_DECLS

typedef enum edit_input_type_t {
	EDIT_INPUT_TYPE_TEXT = 0,
	EDIT_INPUT_TYPE_NUMBER,
	EDIT_INPUT_TYPE_DATE,
	EDIT_INPUT_TYPE_MONEY,
	EDIT_INPUT_TYPE_DOUBLE,
} edit_input_type_t;

extern control_t* edit_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char *text, int input_type, size_t max_length);

__END_DECLS

#endif		/* GUI_CONTROLS_EDIT_H */
