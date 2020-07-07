#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/edit.h"

typedef struct edit_t {
	control_t control;
	edit_input_type_t input_type;
	char *text;
	size_t capacity;
	size_t length;
	size_t max_length;
	size_t cur_pos;
	size_t text_draw_start;
	int text_draw_x;
	size_t text_draw_width;
} edit_t;

#define screen edit->control.gc
#define DEFAULT_CAPACITY    256

void edit_destroy(edit_t *edit);
void edit_draw(edit_t *edit);
bool edit_focus(edit_t *edit, bool focus);
bool edit_handle(edit_t *edit, const struct kbd_event *e);
bool edit_set_cursor_pos(edit_t *edit, int pos);
bool edit_get_data(edit_t *edit, int what, data_t *text);
bool edit_set_data(edit_t *edit, int what, const void *data, size_t data_len);
bool edit_is_empty(edit_t *edit);

/* Программный курсор */
#define CURSOR_BLINK_DELAY	50	/* ссек */
#define CURSOR_COLOR		clGray
static int cursor_xor = 0;
static bool cursor_visible = false;

/* Рисование курсора */
static void draw_cursor(edit_t *edit)
{
	int x, y, h;

	if (edit != NULL && edit->control.focused) {
		x = edit->control.x + BORDER_WIDTH +
            edit->text_draw_x +
			(edit->cur_pos - edit->text_draw_start) * GetMaxCharWidth(screen);
		y = edit->control.y + BORDER_WIDTH;
		h = edit->control.height - BORDER_WIDTH * 2;
		SetPenColor(screen, CURSOR_COLOR);
		SetRop2Mode(screen, R2_XOR);
		Line(screen, x,     y, x,     y + h);
		Line(screen, x + 1, y, x + 1, y + h);
		SetRop2Mode(screen, R2_COPY);
		cursor_xor ^= 1;
	}
}

/* Установка видимости курсора */
static void set_cursor_visibility(edit_t *edit, bool visibility)
{
	cursor_visible = visibility;
	if (cursor_visible) {
		if (!cursor_xor)
			draw_cursor(edit);
	} else {
		if (cursor_xor)
			draw_cursor(edit);
	}
}

/* Мигание курсора */
static void do_cursor_blinking(edit_t *edit, bool restart)
{
	static uint32_t t0 = 0;

	if (restart) {
		t0 = u_times();
		return;
	}

	uint32_t t = u_times();

	if ((t - t0) >= CURSOR_BLINK_DELAY){
		if (cursor_visible)
			draw_cursor(edit);
		t0 = t;
	}
}


control_t* edit_create(int id, GCPtr gc, int x, int y, int width, int height,
	const char *text, edit_input_type_t input_type, size_t max_length)
{
    edit_t *edit = (edit_t *)malloc(sizeof(edit_t));
    control_api_t api = {
		(void (*)(struct control_t *))edit_destroy,
		(void (*)(struct control_t *))edit_draw,
		(bool (*)(struct control_t *, bool))edit_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))edit_handle,
		(bool (*)(struct control_t *, int, data_t *))edit_get_data,
		(bool (*)(struct control_t *control, int, const void *, size_t))edit_set_data,
		(bool (*)(struct control_t *control))edit_is_empty,
		NULL,
		NULL,
    };

    control_init(&edit->control, id, gc, x, y, width, height, &api);
 
    edit->input_type = input_type;

    edit->text_draw_width = width - BORDER_WIDTH * 2;
    edit->max_length = max_length;

    /*if (edit->max_length <= DEFAULT_CAPACITY)
        edit->capacity = edit->max_length;
    else*/
    edit->capacity = DEFAULT_CAPACITY;

    if (text != NULL) {
    	size_t len = strlen(text);
    	if (len > edit->max_length)
    		len = edit->max_length;
    	if (len > edit->capacity)
	    	edit->capacity = len;
	    edit->length = len;
    } else
    	edit->length = 0;

    edit->text = (char *)malloc(edit->capacity + 1);
	memset(edit->text, 'M', edit->capacity);
    if (edit->length > 0)
    	memcpy(edit->text, text, edit->length);
    edit->text[edit->length] = 0;

    edit->text_draw_start = edit->text_draw_x = 0;
    edit->cur_pos = 0;

    return (control_t *)edit;
}

void edit_destroy(edit_t *edit) 
{
    free(edit->text);
    free(edit);
}

void edit_draw(edit_t *edit)
{
	int x, y;
	Color borderColor;
	Color bgColor;
	Color fgColor;

	if (edit->control.enabled) {
		if (edit->control.focused) {
			borderColor = clRopnetDarkBrown;
			bgColor = clRopnetBrown;
			fgColor = clBlack;
		} else {
			borderColor = RGB(184, 184, 184);
			bgColor = RGB(200, 200, 200);
			fgColor = RGB(32, 32, 32);
		}
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
		fgColor = clGray;
	}

	fill_rect(screen, edit->control.x, edit->control.y,
		edit->control.width, edit->control.height, BORDER_WIDTH,
		borderColor, bgColor);

	if (edit->length > 0) {
		SetGCBounds(screen, edit->control.x + BORDER_WIDTH, edit->control.y + BORDER_WIDTH,
			edit->control.width - BORDER_WIDTH * 2, edit->control.height - BORDER_WIDTH * 2);
		const char *text = edit->text + edit->text_draw_start;
		const char *s = text;
		size_t outw = 0;
		size_t len = 0;
		for (int i = edit->text_draw_start; i < edit->length &&
				outw < edit->text_draw_width; s++, len++, i++)
			outw += screen->pFont->glyphs[(uint8_t)*s].width;

		x = edit->text_draw_x;
		y = (edit->control.height - GetTextHeight(screen))  /  2 - BORDER_WIDTH;

		SetTextColor(screen, fgColor);
		TextOutN(screen, x, y, text, len);
	}
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

bool edit_focus(edit_t *edit, bool focus) {
	if (focus) {
		edit->control.focused = true;
		edit_draw(edit);
		cursor_xor = 0;
		do_cursor_blinking(edit, true);
		set_cursor_visibility(edit, true);
	} else {
		edit->control.focused = false;
		edit_draw(edit);
	}

	return true;
}

static void edit_grow_text(edit_t *edit, size_t grow_size) {
	size_t new_capacity = edit->capacity;
	while (edit->length + grow_size > edit->capacity)
		new_capacity *= 2;

	if (new_capacity > edit->capacity) {
		edit->capacity = new_capacity;
		//if (edit->capacity > edit->max_length)
		//	edit->capacity = edit->max_length;
		edit->text = (char *)realloc(edit->text, edit->capacity + 1);
		assert(edit->text != NULL);
	}
}

static void edit_calc_draw_params(edit_t *edit, int cursorX) {
	int nch = (cursorX + GetMaxCharWidth(screen) - 1) / GetMaxCharWidth(screen);
	edit->text_draw_start = edit->cur_pos - nch;
 	edit->text_draw_x = cursorX - nch * GetMaxCharWidth(screen);
}

bool edit_set_cursor_pos(edit_t *edit, int pos) {
	if (edit == NULL)
		return false;
    int oldPos = edit->cur_pos;
	int cur_x = 0;
	bool recalc = false;

    if (pos < 0)
       pos = 0;
    else if (pos > edit->length)
        pos = edit->length;
    if (pos == oldPos)
        return false;

    if (edit->control.focused)
        set_cursor_visibility(edit, false);

    if (pos - oldPos > 0) {
        int x = (pos - edit->text_draw_start) * GetMaxCharWidth(screen) + edit->text_draw_x;

        if (x > edit->text_draw_width) {
           	cur_x = (edit->text_draw_width * 2) / 3;
			int rwidth = (edit->length - pos) * GetMaxCharWidth(screen) + 2;
			if (rwidth < cur_x)
				cur_x = edit->text_draw_width - rwidth;
			recalc = true;
        }
    } else {
        int x = (pos - edit->text_draw_start) * GetMaxCharWidth(screen) + edit->text_draw_x;
		if (x < 0) {
           	cur_x = edit->text_draw_width / 3;
			int rwidth = (pos * GetMaxCharWidth(screen));
			if (rwidth < cur_x)
				cur_x = rwidth;
			recalc = true;
		}
	}

    edit->cur_pos = pos;
	if (recalc) {
		edit_calc_draw_params(edit, cur_x);
		if (edit->control.focused)
	 		edit_draw(edit);
	}

	if (edit->control.focused) {
		do_cursor_blinking(edit, true);
    	set_cursor_visibility(edit, true);
	}

    return true;
}

bool edit_insert_char(edit_t *edit, char ch) {
//	int oldPos;

	if (edit == NULL)
		return false;

	if (edit->max_length == edit->length)
		return false;

//	oldPos = edit->cur_pos;

	if (edit->control.focused)
    	set_cursor_visibility(edit, false);

	edit_grow_text(edit, 1);
	if (edit->cur_pos != edit->length)
		memmove(edit->text + edit->cur_pos + 1, edit->text + edit->cur_pos,
				edit->length - edit->cur_pos);

	edit->text[edit->cur_pos] = ch;
    edit->cur_pos++;
	edit->length++;
	edit->text[edit->length] = 0;

	int x = edit->text_draw_x + (edit->cur_pos - edit->text_draw_start) * GetMaxCharWidth(screen);
	if (x > edit->text_draw_width)
		edit_calc_draw_params(edit, (int)edit->text_draw_width - 2);

	edit_draw(edit);
	if (edit->control.focused) {
		do_cursor_blinking(edit, true);
    	set_cursor_visibility(edit, true);
	}

	return true;
}

bool edit_replace_char(edit_t *edit, char ch) {
	if (edit->cur_pos >= edit->length)
		return false;
		
	if (edit->control.focused)
    	set_cursor_visibility(edit, false);
    	
	edit->text[edit->cur_pos] = ch;
	
	edit->cur_pos++;

	edit_draw(edit);
	if (edit->control.focused) {
		do_cursor_blinking(edit, true);
	   	set_cursor_visibility(edit, true);
   	}
   	return true;
}

bool edit_remove_char(edit_t *edit) {
	if (edit == NULL)
		return false;
	if (edit->length == 0 || edit->cur_pos == edit->length)
		return false;

	if (edit->control.focused)
    	set_cursor_visibility(edit, false);

	edit->length--;
	if (edit->cur_pos < edit->length)
		memmove(edit->text + edit->cur_pos, edit->text + edit->cur_pos + 1,
				edit->length - edit->cur_pos + 1);
	edit->text[edit->length] = 0;

	int x = edit->text_draw_x + (edit->cur_pos - edit->text_draw_start) * GetMaxCharWidth(screen);
	int rtw = (edit->length - edit->cur_pos) * GetMaxCharWidth(screen);
	int ltw = edit->cur_pos * GetMaxCharWidth(screen);
	int r = edit->text_draw_width - x;

	if (rtw > r && ltw > x)
		edit_calc_draw_params(edit, edit->text_draw_width - rtw - 2);

//	printf("x: %d, w: %d, r: %d\n", x, w, r);

/*	if (w > r) {
		printf("cur_x: %d\n", edit->text_draw_width - w - 2);
		edit_calc_draw_params(edit, edit->text_draw_width - w - 2);
	}*/

	if (edit->control.focused) {
		edit_draw(edit);
		do_cursor_blinking(edit, true);
    	set_cursor_visibility(edit, true);
	}

	return true;
}

bool edit_backspace_char(edit_t *edit) {
	if (edit == NULL)
		return false;
	if (edit->length == 0 || edit->cur_pos == 0)
		return false;

	bool oldFocused = edit->control.focused;
	edit->control.focused = false;
	edit_set_cursor_pos(edit, edit->cur_pos - 1);
	edit->control.focused = oldFocused;
	return edit_remove_char(edit);
}

bool edit_handle(edit_t *edit, const struct kbd_event *e) {
	do_cursor_blinking(edit, false);

	if (!e->pressed)
		return false;

	if (e->ch != 0 && e->key != KEY_ESCAPE) {
		switch (edit->input_type) {
		case EDIT_INPUT_TYPE_MONEY: {
			char *s = strchr(edit->text, '.');
			if (s != NULL) {
				int pos = s - edit->text;
				if (edit->cur_pos > pos && edit->length - pos >= 3) {
					if (edit->cur_pos < edit->length) {
						if (isdigit(e->ch)) {
							edit_replace_char(edit, e->ch);
						}
					}
					return true;
				}
			}
			if (e->ch == '.' && s == NULL && edit->cur_pos >= edit->length - 1) {
				break;
			}
		}
		__fallthrough__;
		case EDIT_INPUT_TYPE_NUMBER:
		case EDIT_INPUT_TYPE_DATE:
			if (!isdigit(e->ch))
				return true;
			__fallthrough__;
		case EDIT_INPUT_TYPE_DOUBLE:
			if (!isdigit(e->ch)) {
				if (e->ch == '.') {
					if (strchr(edit->text, '.'))
						return true;
					if (edit->cur_pos == 0)
						edit_insert_char(edit, '0');
				} else
					return true;
			}
		}

		edit_insert_char(edit, e->ch);
		if (edit->input_type == EDIT_INPUT_TYPE_DATE) {
			if (edit->cur_pos == 2 || edit->cur_pos == 5)
				edit_insert_char(edit, '.');
		}
	} else {
 		switch (e->key) {
			case KEY_DEL:
				if (edit->input_type != EDIT_INPUT_TYPE_DATE ||
					(edit->cur_pos != 2 && edit->cur_pos != 5))
				edit_remove_char(edit);
				return true;
			case KEY_BACKSPACE:
				edit_backspace_char(edit);
				if (edit->input_type == EDIT_INPUT_TYPE_DATE) {
					if (edit->cur_pos == 2 || edit->cur_pos == 5)
						edit_backspace_char(edit);
				}
				return true;
			case KEY_HOME:
				edit_set_cursor_pos(edit, 0);
				return true;
			case KEY_END:
				edit_set_cursor_pos(edit, edit->length);
				return true;
			case KEY_RIGHT:
				edit_set_cursor_pos(edit, edit->cur_pos + 1);
				return true;
			case KEY_LEFT:
				edit_set_cursor_pos(edit, edit->cur_pos - 1);
				return true;
/*			case KEY_ENTER:
			case KEY_NUMENTER:
				if (edit->control.form->item_count == 3) {
					edit->control.form->result = 1;
					kbd_flush_queue();
				}*/
				return true;
		}
	}

    return false;
}

bool edit_get_data(edit_t *edit, int what, data_t *data) {
	const char *text = edit->text;
	size_t l = edit->length;

	if (what == 1 && l > 0) {
		const char *end = text + l - 1;

		while (isspace(*text)) {
			text++;
			l--;
		}

		while (end > text && isspace(*end)) {
			end--;
			l--;
		}
	}

	data->data = (void *)text;
	data->size = l;

	return true;
}

bool edit_set_data(edit_t *edit, int what, const void *data, size_t data_len) {
	switch (what) {
	case 0:
		if (data != NULL) {
			if (data_len > edit->max_length)
				data_len = edit->max_length;
			edit_grow_text(edit, data_len);
			memcpy(edit->text, data, data_len);
			edit->text[data_len] = 0;
		    edit->length = data_len;
   			edit->text_draw_start = edit->text_draw_x = 0;
			edit->cur_pos = 0;
			edit_draw(edit);
			return true;
		}
	}
	return false;
}

bool edit_is_empty(edit_t *edit) {
	return edit->length == 0;
}
