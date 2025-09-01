#ifndef BITSET_H
#define BITSET_H

#if defined __cplusplus
extern "C" {
#endif

#include "gui/controls/control.h"

control_t* bitset_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char **short_items, const char **items, uint8_t* bits, size_t item_count, int value);

#endif
