#if !defined GUI_CONTROLS_BITSET_H
#define GUI_CONTROLS_BITSET_H

#include "gui/controls/control.h"

__BEGIN_DECLS

extern control_t* bitset_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char **short_items, const char **items, uint8_t* bits, size_t item_count, int value);

__END_DECLS

#endif		/* GUI_CONTROLS_BITSET_H */
