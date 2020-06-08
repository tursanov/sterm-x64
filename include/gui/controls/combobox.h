#ifndef COMBOBOX_H
#define COMBOBOX_H

#include "gui/controls/edit.h"


control_t* combobox_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char *text, edit_input_type_t input_type, size_t max_length,
	const char **items, size_t item_count, int value, int flags);

control_t* simple_combobox_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char **items, size_t item_count, int value);

control_t* edit_combobox_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char *text, edit_input_type_t input_type, size_t max_length,
	const char **items, size_t item_count);


#endif // COMBOBOX_H
