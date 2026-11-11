#if !defined GUI_CONTROLS_CONTROL_H
#define GUI_CONTROLS_CONTROL_H

#include "gui/gdi.h"

__BEGIN_DECLS

#define BORDER_WIDTH	2

typedef struct data_t {
	void *data;
	size_t size;
} data_t;

struct control_t;
typedef struct control_api_t {
	void (*destroy)(struct control_t *control);
	void (*draw)(struct control_t *control);
	bool (*focus)(struct control_t *control, bool focus);
	bool (*handle)(struct control_t *control, const struct kbd_event *e);
	bool (*get_data)(struct control_t *control, int what, data_t *data);
	bool (*set_data)(struct control_t *control, int what, const void *data, size_t data_len);
	bool (*is_empty)(struct control_t *control);
	void (*set_parent)(struct control_t *control);
	bool (*set_enabled)(struct control_t *control, bool enabled);
} control_api_t;

typedef void (*draw_func_t)(void *obj);

typedef struct {
	void *parent;
	draw_func_t draw;
} control_parent_t;

typedef struct control_t {
	int id;
	GCPtr gc;
	int x;
   	int y;
   	int width;
   	int height;
   	control_api_t api;
   	bool focused;
   	bool enabled;
	void *extra;
	control_parent_t parent;
} control_t;

extern void control_init(control_t *control, int id, GCPtr gc, int x, int y, int width, int height,
	control_api_t* api);

#define CONTROL_EXTRA(c, type) ((type *)control_get_extra(c))

extern void control_destroy(control_t *control);
extern void control_draw(control_t *control);
extern bool control_focus(control_t *control, bool focus);
extern bool control_handle(struct control_t *control, const struct kbd_event *e);
extern void* control_get_extra(struct control_t *control);
extern void control_set_extra(struct control_t *control, void *extra);
extern bool control_get_data(struct control_t *control, int what, data_t *data);
extern bool control_set_data(struct control_t *control, int what, const void *data, size_t data_len);
extern bool control_is_empty(struct control_t * control);
extern void control_set_parent(struct control_t *control, control_parent_t *parent);
extern void control_refresh_parent(struct control_t *control);
bool control_set_enabled(struct control_t *control, bool enabled);

extern void fill_rect(GCPtr screen, int x, int y, int width, int height, int border_width,
		Color border_color, int bg_color);

__END_DECLS

#endif		/* GUI_CONTROLS_CONTROL_H */
