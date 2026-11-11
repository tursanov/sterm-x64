#if !defined GUI_CONTROLS_WINDOW_H
#define GUI_CONTROLS_WINDOW_H

#include "controls.h"

__BEGIN_DECLS

struct window_t;
typedef struct window_t window_t;
typedef bool (*window_handle_event_func_t)(window_t *w, const struct kbd_event *e);

typedef enum {
	align_left,
	align_center,
	align_right,
} align_t;

extern window_t *window_create(GCPtr gc, const char *title, window_handle_event_func_t handle_event_func);
extern void window_destroy(window_t *w);
extern void window_set_dialog_result(window_t *w, int result);
extern control_t *window_add_control(window_t *w, control_t *c);
extern void window_add_label(window_t *w, int x, int y, align_t align, const char *text);
extern void window_add_label_with_id(window_t *w, int id, int x, int y, align_t align, const char *text);
extern void window_draw(window_t *w);


extern int window_show_dialog(window_t *w, int focus_id);
extern GCPtr window_get_gc(window_t *w);
extern control_t *window_get_control(window_t *w, int id);
extern void window_set_label_text(window_t *w, int id, const char *text, bool redraw);
static inline void window_redraw_control(window_t *w, int id) {
	control_t *c = window_get_control(w, id);
	if (c != NULL)
		control_draw(c);
}

extern control_t *window_get_focus(window_t *w);
extern bool window_set_focus(window_t *w, int id);

extern bool window_get_data(window_t *w, int id, int what, data_t *data);
extern bool window_set_data(window_t *w, int id, int what, const void *data, size_t data_size);
extern bool window_set_enabled(window_t *w, int id, bool enabled);

static inline int window_get_int_data(window_t *w, int id, int what, int default_value) {
	data_t data;
	if (window_get_data(w, id, what, &data))
		return (int)(intptr_t)data.data;
	return default_value;
}

static inline void * window_get_ptr_data(window_t *w, int id, int what) {
	data_t data;
	if (window_get_data(w, id, what, &data))
		return (void *)data.data;
	return NULL;
}

extern void window_show_error(window_t *w, int id, const char *text);

__END_DECLS

#endif
