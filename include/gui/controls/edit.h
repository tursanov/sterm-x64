#ifndef __EDIT_H__
#define __EDIT_H__

#include "gui/controls/control.h"

typedef enum edit_input_type_t {
	EDIT_INPUT_TYPE_TEXT = 0,
	EDIT_INPUT_TYPE_NUMBER,
	EDIT_INPUT_TYPE_DATE,
	EDIT_INPUT_TYPE_MONEY,
	EDIT_INPUT_TYPE_DOUBLE,
} edit_input_type_t;

control_t* edit_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char *text, edit_input_type_t input_type, size_t max_length);

#endif
