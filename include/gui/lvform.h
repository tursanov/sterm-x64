#ifndef lLVFORM_H
#define LVFORM_H

#include "list.h"

typedef struct lvform_column_t {
	char *title;
	int width;
} lvform_column_t;

typedef struct data_source_t {
	list_t *list;
	int (*get_text)(void *obj, int index, char *text, size_t text_size);
	void* (*new_item)(struct data_source_t *ds);
	int (*edit_item)(struct data_source_t *ds, void *obj);
	int (*remove_item)(struct data_source_t *ds, void *obj);
} data_source_t;

typedef struct lvform_t {
	const char *title;
	lvform_column_t* columns;
	size_t column_count;
	int top_index;
	int selected_index;
	data_source_t *data_source;
} lvform_t;

lvform_t *lvform_create(const char *title, 
	lvform_column_t* columns, 
	size_t column_count,
	data_source_t *data_source);

void lvform_destroy(lvform_t *lvform);
int lvform_execute(lvform_t *lvform);
void lvform_draw(lvform_t *lvform);

#endif // LVFORM_H
