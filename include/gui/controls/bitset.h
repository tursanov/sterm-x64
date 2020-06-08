#ifndef BITSET_H
#define BITSET_H

#include "gui/controls/control.h"

control_t* bitset_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char **short_items, const char **items, size_t item_count, int value);

#endif
