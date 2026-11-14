#if !defined GUI_CONTROLS_COMBOBOX_H
#define GUI_CONTROLS_COMBOBOX_H

#include "gui/controls/edit.h"

__BEGIN_DECLS

extern control_t* combobox_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char *text, int input_type, size_t max_length,
	const char **items, size_t item_count, int value, int flags);

extern control_t* simple_combobox_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char **items, size_t item_count, int value);

extern control_t* edit_combobox_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char *text, int input_type, size_t max_length,
	const char **items, size_t item_count);

__END_DECLS

#endif		/* GUI_CONTROLS_COMBOBOX_H */
