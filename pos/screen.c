/* Работа с потоком POS-эмулятора для экрана. (c) A.Popov, gsr 2004 */

#include <sys/times.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "gui/gdi.h"
#include "gui/status.h"
#include "pos/error.h"
#include "pos/pos.h"
#include "pos/screen.h"
#include "genfunc.h"
#include "kbd.h"

#define BLINK_TIME	25	/* время мигания символа в сотых секунды */
#define CURSOR_TIME	30	/* время мигания курсора */

/* Глобальный экран вывода информации */
static pos_screen_t *screen;

/* Сохранить текущую позицию курсора */
void pos_screen_save_pos(void)
{
	if (screen != NULL){
		screen->saved_x = screen->x;
		screen->saved_y = screen->y;
		screen->saved_outside = screen->outside;
	}
}

/* Восстановить сохраненную позицию курсора */
void pos_screen_restore_pos(void)
{
	if (screen != NULL){
		screen->x = screen->saved_x;
		screen->y = screen->saved_y;
		screen->outside = screen->saved_outside;
	}
}

/* Проверка текста на выход из границ и установка позиции виртуального курсора */
static bool validate_pos_text(const char *text);
/* Рисование текста */
static void draw_pos_text(pos_text_t *text);
/* Освобождение дополнительных данных */
static void free_pos_text(pos_text_t *text);

/* Рисование строки ввода */
static void draw_pos_edit(pos_edit_t *edit);
/* Активация фокуса строки ввода */
static void activate_pos_edit(pos_edit_t *edit);
/* Деактивация фокуса строки ввода */
static void deactivate_pos_edit(pos_edit_t *edit);
/* Обработка событий строки ввода */
static void process_pos_edit(pos_edit_t *edit,
		struct kbd_event *e);
/* Освобождение дополнительных данных */
static void free_pos_edit(pos_edit_t *edit);

/* Рисование меню */
static void draw_pos_menu(pos_menu_t *menu);
/* Обработка событий меню */
static void process_pos_menu(pos_menu_t *menu,
		struct kbd_event *e);
/* Освобождение дополнительных данных */
static void free_pos_menu(pos_menu_t *menu);

/* Создание экрана */
bool pos_screen_create(int x, int y, int cols, int rows,
		int x_off, int y_off,
		FontPtr font, BitmapPtr bmp_up, BitmapPtr bmp_down)
{
	/* font->var_size - шрифт переменной длины */
	if (!font || font->var_size){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_SCR_NOT_READY, 0);
		return false;
	}

	if (!(screen = malloc(sizeof(pos_screen_t)))){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}

	/* Создание контекста для рисования c отступом для рамок */
	if (!(screen->gc = CreateGC(x, y,
					font->max_width*cols + x_off*2,
					font->max_height*rows + y_off*2))){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		free(screen);
		screen = NULL;
		return false;
	}
	screen->gc->pFont = font;
	screen->cols = cols;
	screen->rows = rows;
	screen->head = screen->tail = screen->active = NULL;
	screen->x = screen->y = 0;
	screen->fg = SYM_COLOR_WHITE;
	screen->bg = SYM_COLOR_BLACK;
	screen->update = true;
	screen->blink_time = u_times();
	screen->blink_hide = false;

	screen->show_cursor = false;

	screen->outside = false;

	screen->bmp_up = bmp_up;
	screen->bmp_down = bmp_down;

	screen->x_off = x_off;
	screen->y_off = y_off;

	return true;
}

/* Удаление экрана */
bool pos_screen_destroy(void)
{
	if (!screen)
		return false;
	pos_screen_cls();
	DeleteGC(screen->gc);
	free(screen);
	screen = NULL;
	return true;
}

/* Инициализирован ли экран */
bool pos_screen_initialized(void)
{
	return screen != NULL;
}

/* Рисование курсора с помощью XOR в заданной позиции */
static void draw_cursor(void)
{
	int x, y, w, h;
	FontPtr font;

	font = screen->gc->pFont;

	x = screen->cursor_x*font->max_width + screen->x_off;
	y = screen->cursor_y*font->max_height + font->max_top + screen->y_off;
	w = font->underline_y2 - font->underline_y1 + 1;
	h = font->max_bottom - font->max_top + 1;

	screen->gc->pencolor = RGB(128, 128, 128);
	screen->gc->rop2mode = R2_XOR;
	while (h--)
	{
		Line(screen->gc, x, y, x + w - 1, y);
		y++;
	}
	screen->gc->rop2mode = R2_COPY;
}

/* Показать курсор в заданной позиции */
static void show_cursor(int x, int y)
{
	if (!screen->show_cursor)
	{
		screen->cursor_x = x;
		screen->cursor_y = y;
		screen->show_cursor = true;
		screen->cursor_time = u_times();
		screen->cursor_on_screen = true;
	}
	else if (x != screen->cursor_x || y != screen->cursor_y)
	{
		if (screen->cursor_on_screen)
			draw_cursor();
		screen->cursor_x = x;
		screen->cursor_y = y;
		screen->cursor_time = u_times();
		screen->cursor_on_screen = true;
	}
	draw_cursor();
}

/* Спрятать курсор */
static void hide_cursor(void)
{
	if (!screen->show_cursor)
		return;
		
	if (screen->cursor_on_screen)
		draw_cursor();
	screen->cursor_on_screen = false;
	screen->show_cursor = false;
}

/* Перерисовка экрана */
bool pos_screen_draw()
{
	bool blink = false;
	pos_node_t *node;
	uint32_t time;
	bool update_cursor;

	if (!screen)
		return false;
	if (screen->update)
		ClearGC(screen->gc, symbol_palette[SYM_COLOR_BLACK]);
	
	time = u_times();
	if (time - screen->blink_time >= BLINK_TIME)
	{
		blink = true;
		screen->blink_hide = !screen->blink_hide;
		screen->blink_time = time;
	}

	update_cursor = false;

	for (node = screen->head; node; node = node->next)
	{
		if (node == screen->active)
			continue;
		if (blink && node->type == POS_TYPE_TEXT)
		{
			pos_text_t *text;

			text = (pos_text_t *)node;

			if (text->attr & SYM_ATTR_BLINK)
			{
				node->draw(node);
				node->update = false;
				continue;
			}
		}
		if (node->update || screen->update)
		{
			if (screen->show_cursor)
			{
				hide_cursor();
				update_cursor = true;
			}
			node->draw(node);
			node->update = false;
		}
	}

	if (screen->active && (screen->active->update || screen->update)) 
	{
		if (screen->show_cursor)
		{
			hide_cursor();
			update_cursor = true;
		}
		screen->active->draw(screen->active);
		screen->active->update = false;
	}

	if (update_cursor)
		show_cursor(screen->cursor_x, screen->cursor_y);

	screen->update = false;

	if (screen->show_cursor)
	{
		time = u_times();
		if (time - screen->cursor_time >= CURSOR_TIME)
		{
			screen->cursor_time = time;
			screen->cursor_on_screen = !screen->cursor_on_screen;
			draw_cursor();
		}
	}

	return true;
}

/* Обработка событий */
bool pos_screen_process(struct kbd_event *e)
{
	if ((e->key == KEY_LSHIFT) || (e->key == KEY_RSHIFT) || (e->key == KEY_CAPS)){
		scr_show_language(true);
		return true;
	}
	if (e->pressed && !e->repeated && (e->shift_state == 0)){
		switch (e->key){
			case KEY_TAB:
				pos_screen_activate_next();
				break;
			case KEY_ENTER:
			case KEY_NUMENTER:
				pos_set_state(pos_enter);
				break;
		}
		if (screen->active && screen->active->process)
			screen->active->process(screen->active, e);
	}
	return true;
}

/* Задание положения виртуального курсора */
bool pos_screen_cur(int x, int y)
{
	if (x < 0)
		x = 0;
	else if (x >= screen->cols)
		x = screen->cols - 1;
	
	if (y < 0)
		y = 0;
	else if (y >= screen->rows)
		y = screen->rows - 1;
	
	screen->x = x;
	screen->y = y;
	screen->outside = false;
	return true;
}

/* Изменение цвета букв и фона */
bool pos_screen_color(uint8_t fg, uint8_t bg)
{
	screen->fg = fg & 0x7;
	screen->bg = bg & 0x7;
	return true;
}

/* Очистка экрана */
bool pos_screen_cls()
{
	pos_node_t *node;
	
	for (node = screen->head; node;)
	{
		pos_node_t *tmp;

		tmp = node;
		node = node->next;

		if (tmp->free)
			tmp->free(tmp);
		free(tmp);
	}
	screen->head = screen->tail = screen->active = NULL;
	screen->update = true;
	screen->blink_time = u_times();
	screen->blink_hide = false;
	screen->show_cursor = false;
	screen->outside = false;
	screen->x = 0;
	screen->y = 0;
	screen->fg = SYM_COLOR_WHITE;
	screen->bg = SYM_COLOR_BLACK;

	return true;
}

/* Активировать следующий элемент */
bool pos_screen_activate_next()
{
	pos_node_t *active;
	pos_node_t *current;
	bool update;

	active = screen->active;
	current = active;

	update = false;
	
	if (!active)
	{
		for (active = screen->head; active; active = active->next)
			if (active->can_active)
			{
				update = true;
				break;
			}
	}
	else
	{
		for (active = active->next; active; active = active->next)
			if (active->can_active)
				break;
		if (!active)
			for (active = screen->head; active; active = active->next)
				if (active->can_active)
					break;
		if (current != active)
			update = true;
	}
	
	screen->active = active;
	if (update)
	{
		if (current)
		{
			if (current->deactivate)
				current->deactivate(current);
			current->update = true;
		}
		if (active)
		{
			active->update = true;
			if (active->activate)
				active->activate(active);
		}
	}

	return true;
}

/* Вставка узла в список экранных элементов */
static void insert_node(pos_node_t *node)
{
	node->next = NULL;
	if (screen->tail)
		screen->tail->next = node;
	else
		screen->head = node;
	screen->tail = node;
}

/* Строки */

/* Добавление строки */
bool pos_screen_insert_text(
		const char *text, uint8_t attr)
{
	pos_text_t *pos_text;
	
	if (text == NULL)
		return false;

	if (!(pos_text = malloc(sizeof(pos_text_t)))){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}

	if (!(pos_text->str = strdup(text))){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		free(pos_text);
		return false;
	}

	pos_text->root.x = screen->x;
	pos_text->root.y = screen->y;

	if (!validate_pos_text(text)){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)text);
		free(pos_text);
		screen->outside = true;
		return false;
	}

	pos_text->root.type = POS_TYPE_TEXT;
	pos_text->root.can_active = false;
	pos_text->root.update = true;
	pos_text->root.draw = (pos_node_func_t)draw_pos_text;
	pos_text->root.activate = NULL;
	pos_text->root.deactivate = NULL;
	pos_text->root.process = NULL;
	pos_text->root.free = (pos_node_func_t)free_pos_text;

	insert_node((pos_node_t *)pos_text);

	pos_text->fg = screen->fg;
	pos_text->bg = screen->bg;
	pos_text->attr = attr;
		
	return true;
}

/* Проверка текста на выход из границ и установка позиции виртуального курсора */
static bool validate_pos_text(const char *text)
{
	int x, y;

	for (x = screen->x, y = screen->y; *text; text++)
	{
		if (*text == '\n')
		{
			y++;
			if (y >= screen->rows && *(text+1) == 0)
				return false;
			continue;
		}
		else if (*text == '\r')
		{
			x = 0;
			continue;
		}
		x++;
		if (x == screen->cols)
		{
			x = 0;
			y++;
			if (y >= screen->rows && *(text+1))
				return false;
		}
	}
	screen->x = x;
	screen->y = y;

	return true;
}

/* Вывод текста в заданной позиции курсора */
static void pos_rich_text_out(int x, int y,
		rich_text_t *text)
{
	GCPtr mem;
	FontPtr font;

	if (!text->count)
		return;

	font = screen->gc->pFont;

	mem = CreateMemGC(text->count*font->max_width, font->max_height);
	mem->pFont = font;

	text->blink_hide = screen->blink_hide;
	draw_rich_text(mem, 0, 0, text);
			
	CopyGC(screen->gc, x*font->max_width + screen->x_off,
			y*font->max_height + screen->y_off,
			mem, 0, 0, GetCX(mem), GetCY(mem));

	DeleteGC(mem);
}

/* Рисование текста */
static void draw_pos_text(pos_text_t *pos_text)
{
	int x, y;
	rich_text_t rich_text;
	char *str;
	int x_out;

	x = pos_text->root.x;
	y = pos_text->root.y;
	str = pos_text->str;

	rich_text.text = str;
	rich_text.count = 0;
	rich_text.fg = pos_text->fg;
	rich_text.bg = pos_text->bg;
	rich_text.attr = pos_text->attr;

	x_out = x;

	for (; *str; str++)
	{
		if (*str == '\n')
		{
			pos_rich_text_out(x_out, y, &rich_text);
			rich_text.count = 0;
			rich_text.text = str+1;
			x_out = x;
			y++;
			continue;
		}
		else if (*str == '\r')
		{
			pos_rich_text_out(x_out, y, &rich_text);
			x = 0;
			x_out = 0;
			rich_text.count = 0;
			rich_text.text = str+1;
			continue;
		}
		x++;
		rich_text.count++;
		if (x == screen->cols)
		{
			pos_rich_text_out(x_out, y, &rich_text);
			rich_text.count = 0;
			rich_text.text = str+1;
			x_out = 0;
			x = 0;
			y++;
		}
	}
	pos_rich_text_out(x_out, y, &rich_text);
}

/* Освобождение дополнительных данных */
static void free_pos_text(pos_text_t *pos_text)
{
	free(pos_text->str);
}

/* Строки ввода */

/* Добавление строки ввода */
bool pos_screen_insert_edit(
		const char *name, const char *initial_text, int count)
{
	pos_edit_t *pos_edit;
	int view_width;

	if (name == NULL)
		return false;
	if (screen->x + count > screen->cols)
		view_width = screen->cols - screen->x;
	else
		view_width = count;
	if (!(pos_edit = malloc(sizeof(pos_edit_t)))){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	if (!(pos_edit->name = strdup(name))){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		free(pos_edit);
		return false;
	}
	if (!(pos_edit->text = malloc(count+1))){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		free(pos_edit->name);
		free(pos_edit);
		return false;
	}
	if (initial_text){
		strncpy(pos_edit->text, initial_text, count);
		pos_edit->text[count] = 0;
	}else
		pos_edit->text[0] = '\x0';
	pos_edit->root.type = POS_TYPE_EDIT;
	pos_edit->root.x = screen->x;
	pos_edit->root.y = screen->y;
	pos_edit->root.can_active = true;
	pos_edit->root.update = true;
	pos_edit->root.draw = (pos_node_func_t)draw_pos_edit;
	pos_edit->root.activate = (pos_node_func_t)activate_pos_edit;
	pos_edit->root.deactivate = (pos_node_func_t)deactivate_pos_edit;
	pos_edit->root.free = (pos_node_func_t)free_pos_edit;
	pos_edit->root.process = (pos_node_process_func_t)process_pos_edit;
	pos_edit->count = count;
	pos_edit->pos = 0;
	pos_edit->view_start = 0;
	pos_edit->view_width = view_width;
	insert_node((pos_node_t *)pos_edit);
	if (screen->active == NULL)
		pos_screen_activate_next();
	screen->x += view_width;
	if (screen->x >= screen->cols-1){
		screen->x = 0;
		screen->y++;
	}
	return true;
}

/* Используемые цвета */
#define EDIT_BACKGROUND	RGB(0xFF, 0xFF, 0xFF)

/* Рисование строки ввода */
static void draw_pos_edit(pos_edit_t *edit)
{
	GCPtr mem;
	FontPtr font;
	Color frame_color;

	font = screen->gc->pFont;

	mem = CreateMemGC(edit->view_width*font->max_width + screen->x_off*2,
			font->max_height + screen->y_off*2);
	mem->pFont = font;

	ClearGC(mem, EDIT_BACKGROUND);

	DrawBorder(mem, 0, 0, GetCX(mem), GetCY(mem), 1,
			clWhite, clGray);
	
	DrawBorder(mem, 1, 1, GetCX(mem)-2, GetCY(mem)-2, 2,
			clSilver, clSilver);

	frame_color = (screen->active == (pos_node_t *)edit) ?
		clRed : clBlack;
	
	DrawBorder(mem, 3, 3, GetCX(mem) - 6, GetCY(mem) - 6, 1, frame_color,
			frame_color);

	mem->textcolor = clBlack;
	TextOutN(mem, screen->x_off, screen->y_off, edit->text + edit->view_start, 
			edit->view_width);

	CopyGC(screen->gc, edit->root.x*font->max_width,
			edit->root.y*font->max_height,
			mem, 0, 0, GetCX(mem), GetCY(mem));

	DeleteGC(mem);
}

/* Активация фокуса строки ввода */
static void activate_pos_edit(pos_edit_t *edit)
{
	show_cursor(edit->root.x, edit->root.y);
}

/* Деактивация фокуса строки ввода */
static void deactivate_pos_edit(pos_edit_t *edit)
{
	hide_cursor();
	edit->pos = 0;
	edit->view_start = 0;
}

/* Обработка событий строки ввода */
static void process_pos_edit(pos_edit_t *edit,
		struct kbd_event *e)
{
	if (!e->pressed)
		return;

	switch (e->key)
	{
	case KEY_BACKSPACE:
		if (edit->pos > 0)
		{
			memmove(edit->text + edit->pos - 1,
					edit->text + edit->pos,
					strlen(edit->text) - edit->pos + 1);

			edit->pos--;
			if (edit->view_start > edit->pos)
				edit->view_start = edit->pos;
			show_cursor(edit->root.x + edit->pos - edit->view_start,
					screen->cursor_y);
			edit->root.update = true;
		}
		break;
	case KEY_DEL:
		if (edit->text[0] != '\x0')
		{
			edit->root.update = true;
			if ((kbd_shift_state & SHIFT_CTRL))
				edit->text[0] = '\x0';
			else 
			{
				if (edit->pos != strlen(edit->text))
					memcpy(edit->text + edit->pos,
							edit->text + edit->pos + 1,
							strlen(edit->text) - edit->pos);
			
				break;
			}
		}
	case KEY_HOME:
		if (edit->pos > 0)
		{
			edit->pos = 0;
			if (edit->view_start > 0)
				edit->root.update = true;
			edit->view_start = 0;
			show_cursor(edit->root.x, screen->cursor_y);
		}
		break;
	case KEY_END:
		if (edit->pos < strlen(edit->text))
		{
			edit->pos = strlen(edit->text);
			if (edit->pos >= edit->view_start + edit->view_width)
			{
				edit->view_start = edit->pos - edit->view_width + 1;
				edit->root.update = true;
			}
			show_cursor(edit->root.x + edit->pos - edit->view_start,
					screen->cursor_y);
		}
		break;
	case KEY_LEFT:
		if (edit->pos > 0)
		{
			edit->pos--;
			if (edit->view_start > edit->pos)
			{
				edit->view_start = edit->pos;
				edit->root.update = true;
			}
			show_cursor(edit->root.x + edit->pos - edit->view_start,
					screen->cursor_y);
		}
		break;
	case KEY_RIGHT:
		if (edit->pos < strlen(edit->text))
		{
			edit->pos++;
			if (edit->pos >= edit->view_start + edit->view_width)
			{
				edit->view_start = edit->pos - edit->view_width + 1;
				edit->root.update = true;
			}
			show_cursor(edit->root.x + edit->pos - edit->view_start,
					screen->cursor_y);
		}
		break;
	default:
		if ((e->ch != 0) && (e->ch != 0x1b) && strlen(edit->text) < edit->count){
			memmove(edit->text + edit->pos + 1, 
					edit->text + edit->pos,
					strlen(edit->text) - edit->pos + 1);
			edit->text[edit->pos] = e->ch;
			edit->root.update = true;
			edit->pos++;
			if (edit->pos >= edit->view_start + edit->view_width)
				edit->view_start = edit->pos - edit->view_width + 1;
			show_cursor(edit->root.x + edit->pos - edit->view_start,
					screen->cursor_y);
		}
	}
}

/* Освобождение дополнительных данных */
static void free_pos_edit(pos_edit_t *edit)
{
	free(edit->name);
	free(edit->text);
}

/* Добавление меню */
bool pos_screen_insert_menu(
		const char *name, char **items, int count,
		int width, int height)
{
	pos_menu_t *pos_menu;
	int i;
	if (!name || !items || !count)
		return false;
	else if (((screen->x + width) > screen->cols) ||
			((screen->y + height) > screen->rows)){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)name);
		screen->outside = true;
		return false;
	}	
	for (i = 0; i < count; i++){
		if (items[i] == NULL){
			pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)name);
			return false;
		}
	}
	if (!(pos_menu = malloc(sizeof(pos_menu_t)))){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}else if (!(pos_menu->name = strdup(name))){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		free(pos_menu);
		return false;
	}
	pos_menu->items = items;
	pos_menu->count = count;
	pos_menu->width = pos_menu->view_width = width;
	pos_menu->height = pos_menu->view_height = height;
	pos_menu->selected = 0;
	pos_menu->view_top = 0;
	pos_menu->root.type = POS_TYPE_MENU;
	pos_menu->root.x = screen->x;
	pos_menu->root.y = screen->y;
	pos_menu->root.can_active = true;
	pos_menu->root.update = true;
	pos_menu->root.draw = (pos_node_func_t)draw_pos_menu;
	pos_menu->root.activate = NULL;
	pos_menu->root.deactivate = NULL;
	pos_menu->root.free = (pos_node_func_t)free_pos_menu;
	pos_menu->root.process = (pos_node_process_func_t)process_pos_menu;

	insert_node((pos_node_t *)pos_menu);
	if (screen->active == NULL)
		pos_screen_activate_next();
	screen->x += pos_menu->view_width;
	if (screen->x >= screen->cols){
		screen->x = 0;
		screen->y++;
	}
	screen->y += pos_menu->view_height;
	if (screen->y >= screen->rows)
		screen->outside = true;
	return true;
}

/* Используемые цвета */
#define MENU_BACKGROUND		RGB(0xBB, 0xBB, 0xBB)
#define MENU_FOCUSED_TEXT	RGB(0xFF, 0xFF, 0xFF)
#define MENU_SELECTED_TEXT	RGB(0x00, 0x00, 0xFF)
#define MENU_TEXT		RGB(0x00, 0x00, 0x00)

/* Рисование меню */
static void draw_pos_menu(pos_menu_t *menu)
{
	GCPtr mem;
	FontPtr font;
	int i;
	Color frame_color;

	font = screen->gc->pFont;

	mem = CreateMemGC(menu->view_width*font->max_width + screen->x_off*2,
			menu->view_height*font->max_height + screen->y_off*2);
	mem->pFont = font;

	ClearGC(mem, RGB(0xBB, 0xBB, 0xBB));

	if (screen->active == (void *)menu)
	{
		mem->brushcolor = RGB(0x00, 0x00, 0x64);
		FillBox(mem, screen->x_off,
			(menu->selected - menu->view_top)*font->max_height + screen->y_off,
			menu->view_width*font->max_width,
			font->max_height);
	}

	DrawBorder(mem, 0, 0, GetCX(mem), GetCY(mem), 1, clWhite, clGray);
	frame_color = (screen->active == (pos_node_t *)menu) ? 
		clRed : clBlack;
	DrawBorder(mem, 3, 3, GetCX(mem)-6, GetCY(mem)-6, 1, frame_color,
			frame_color);

	mem->textcolor = clBlack;
	for (i = 0; i < menu->view_height &&
			i + menu->view_top < menu->count; i++){
		if ((menu->view_top + i) == menu->selected){
			if (screen->active == (void *)menu)
				mem->textcolor = MENU_FOCUSED_TEXT;
			else
				mem->textcolor = MENU_SELECTED_TEXT;
		}else
			mem->textcolor = MENU_TEXT;
		DrawText(mem, screen->x_off,
				i*font->max_height + screen->y_off,
				menu->view_width*font->max_width,
				font->max_height, menu->items[i + menu->view_top], 0);
	}

	if (screen->active == (pos_node_t *)menu)
	{
		if (menu->view_top && screen->bmp_up)
		{
			DrawBitmap(mem, screen->bmp_up,
					(mem->box.width - screen->bmp_up->width) / 2, 0,
					-1, -1, false, 0);
		}

		if (menu->view_top + menu->view_height < menu->count && screen->bmp_down)
		{
			DrawBitmap(mem, screen->bmp_down,
					(mem->box.width - screen->bmp_down->width) / 2,
					(mem->box.height - screen->bmp_down->height),
					-1, -1, false, 0);
		}
	}

	CopyGC(screen->gc, menu->root.x*font->max_width,
			menu->root.y*font->max_height,
			mem, 0, 0, GetCX(mem), GetCY(mem));

	DeleteGC(mem);
}

/* Обработка событий меню */
static void process_pos_menu(pos_menu_t *menu,
		struct kbd_event *e)
{
	if (!e->pressed)
		return;

	switch (e->key)
	{
	case KEY_UP:
		if (menu->selected > 0)
		{
			menu->selected--;
			if (menu->selected < menu->view_top)
				menu->view_top = menu->selected;
		}
		menu->root.update = true;
		break;
	case KEY_DOWN:
		if (menu->selected < menu->count - 1)
		{
			menu->selected++;
			if (menu->selected >= menu->view_top + menu->view_height)
				menu->view_top = menu->selected - menu->view_height + 1;
		}
		menu->root.update = true;
		break;
	}
}

/* Освобождение дополнительных данных */
static void free_pos_menu(pos_menu_t *menu)
{
	int i;

	for (i = 0; i < menu->count; i++)
		free(menu->items[i]);
	free(menu->items);
	free(menu->name);
}

/* Разбор ответа */
static bool pos_parse_cur(struct pos_data_buf *buf, bool check_only __attribute__((unused)))
{
	uint16_t x, y;
	if (!pos_read_word(buf, &x) || !pos_read_word(buf, &y)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}else if ((x >= screen->cols) || (y >= screen->rows)){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)"");
		return false;
	}
	return pos_screen_cur(x, y);
}

static bool pos_parse_cls(struct pos_data_buf *buf __attribute__((unused)),
	bool check_only __attribute__((unused)))
{
	return pos_screen_cls();
}

static bool pos_parse_edit(struct pos_data_buf *buf, bool check_only)
{
	uint16_t width;
	static char name[33], value[1025];
	int n;
/* Длина поля ввода */
	if (!pos_read_word(buf, &width)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
/* Имя поля ввода */
	n = pos_read_array(buf, (uint8_t *)name, 32);
	if (n <= 0){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	name[n] = 0;
	recode_str(name, n);
/* Проверка выхода за пределы экрана */
	if (screen->outside || (width == 0) ||
			((screen->x + width) > screen->cols)){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)name);
		return false;
	}
/* Значение поля ввода по умолчанию */
	n = pos_read_array(buf, (uint8_t *)value, 1024);
	if ((n == -1) || (n > width)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	value[n] = 0;
	recode_str(value, n);
/* Создание строки редактирования */
	if (check_only){
		screen->x += width;
		if (screen->x >= screen->cols){
			screen->x = 0;
			screen->y++;
			if (screen->y >= screen->rows)
				screen->outside = true;
		}
		return true;
	}else
		return pos_screen_insert_edit(name, value, width);
}

static bool pos_parse_menu(struct pos_data_buf *buf, bool check_only)
{
	uint16_t w, h;
	uint8_t item_count;
	char name[33], item[1024], **items;
	int i, n;

	if (screen->outside){
		printf("%s: MENU: невозможно добавить новый элемент\n", __func__);
		return false;
/* Размеры меню */
	}else if (!pos_read_word(buf, &w) || !pos_read_word(buf, &h)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
/* Количество элементов меню */
	}else if (!pos_read_byte(buf, &item_count) || (item_count == 0)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
/* Имя меню */
	n = pos_read_array(buf, (uint8_t *)name, 32);
	if (n <= 0){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	name[n] = 0;
	recode_str(name, n);
/* Проверка размеров меню */
	if (screen->outside || ((screen->x + w) > screen->cols) ||
			((screen->y + h) > screen->rows)){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)name);
		return false;
	}
/* Элементы */
	items = (char **)calloc(item_count, sizeof(char *));
	if (items == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	for (i = 0; i < item_count; i++){
		n = pos_read_array(buf, (uint8_t *)item, 1024);
		if (n <= 0){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			break;
		}
		recode_str(item, n);
		items[i] = malloc(n + 1);
		if (items[i] == NULL){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			break;
		}
		memcpy(items[i], item, n);
		items[i][n] = 0;
	}
	if (i != item_count){
		for (n = 0; n < i; n++)
			free(items[n]);
		free(items);
		return false;
	}
	if (check_only){
		screen->x += w;
		if (screen->x >= screen->cols){
			screen->x = 0;
			screen->y++;
		}
		screen->y += h;
		if (screen->y >= screen->rows)
			screen->outside = true;
		return true;
	}else
		return pos_screen_insert_menu(name, items, item_count, w, h);
}

static bool pos_parse_print(struct pos_data_buf *buf, bool check_only)
{
	char str[2049];
	uint8_t attr;
	int n;

	if (screen->outside){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)"");
		return false;
	}
	n = pos_read_array(buf, (uint8_t *)str, 2048);
	if (n == -1){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	str[n] = 0;
	recode_str(str, n);
	if (!pos_read_byte(buf, &attr)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	if (check_only || (n == 0)){
		if (validate_pos_text(str))
			return true;
		else{
			pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)str);
			return false;
		}
	}else
		return pos_screen_insert_text(str, attr);
}

static bool pos_parse_color(struct pos_data_buf *buf, bool check_only)
{
	uint8_t fg, bg;
	if (!pos_read_byte(buf, &fg) || !pos_read_byte(buf, &bg)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	return check_only ? true : pos_screen_color(fg, bg);
}

/* Разбор ответа от POS-эмулятора с выводом на экран */
bool pos_parse_screen_stream(struct pos_data_buf *buf, bool check_only)
{
	static struct {
		uint8_t code;
		bool (*parser)(struct pos_data_buf *, bool);
	} parsers[] = {
		{POS_SCREEN_CUR,	pos_parse_cur},
		{POS_SCREEN_CLS,	pos_parse_cls},
		{POS_SCREEN_EDIT,	pos_parse_edit},
		{POS_SCREEN_MENU,	pos_parse_menu},
		{POS_SCREEN_PRINT,	pos_parse_print},
		{POS_SCREEN_COLOR,	pos_parse_color},
	};
	int i, l;
	uint16_t len;
	if (!pos_screen_initialized()){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_SCR_NOT_READY, 0);
		return false;
	}else if (!pos_read_word(buf, &len) || (len > POS_MAX_BLOCK_DATA_LEN)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	if (check_only)
		pos_screen_save_pos();
	l = buf->data_index + len;
	if (l > buf->data_len){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	while (buf->data_index < l){
		uint8_t code;
		if (!pos_read_byte(buf, &code)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		for (i = 0; i < ASIZE(parsers); i++){
			if (code == parsers[i].code){
				if (!parsers[i].parser(buf, check_only))
					return false;
				break;
			}
		}
		if (i == ASIZE(parsers)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
	}
	if (check_only)
		pos_screen_restore_pos();
	if (buf->data_index == l)
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
}

/* Имеется ли на экране хотя бы одно меню */
bool pos_screen_has_menu(void)
{
	pos_node_t *node;
	for (node = screen->head; node != NULL; node = node->next){
		if (node->type == POS_TYPE_MENU)
			return true;
	}
	return false;
}

/* Запись потока от клавиатуры */
bool pos_req_save_keyboard_stream(struct pos_data_buf *buf)
{
	static char name[33], value[1025];
	uint16_t n;
	pos_node_t *node;
/* Начало потока от клавиатуры */
	if (!pos_req_stream_begin(buf, POS_STREAM_KEYBOARD))
		return false;
/* Количество элементов для записи */
	for (node = screen->head, n = 0; node != NULL; node = node->next){
		switch (node->type){
			case POS_TYPE_MENU:
			case POS_TYPE_EDIT:
				n++;
		}
	}
	if (!pos_write_word(buf, n))
		return false;
/* Запись элементов */
	for (node = screen->head; node != NULL; node = node->next){
		if (node->type == POS_TYPE_MENU){
			char s[12];

			sprintf(s, "%d", ((pos_menu_t *)node)->selected + 1);
			strncpy(name, ((pos_menu_t *)node)->name, 32);
			name[32] = 0;
			recode_str(name, -1);
			if (!pos_write_array(buf, (uint8_t *)name,	strlen(name)) ||
					!pos_write_array(buf, (uint8_t *)s, strlen(s)))
				return false;
		}else if (node->type == POS_TYPE_EDIT){
			strncpy(name, ((pos_edit_t *)node)->name, 32);
			name[32] = 0;
			recode_str(name, -1);
			strncpy(value, ((pos_edit_t *)node)->text, 1024);
			value[1024] = 0;
			recode_str(value, -1);
			if (!pos_write_array(buf, (uint8_t *)name, strlen(name)) ||
					!pos_write_array(buf, (uint8_t *)value, strlen(value)))
				return false;
		}
	}
/* Запись конца потока */
	return pos_req_stream_end(buf);
}

/* Запись потока для экрана (используется при обработке ошибок) */
static bool pos_req_save_screen_cur(struct pos_data_buf *buf, int x, int y)
{
	return	pos_write_byte(buf, POS_SCREEN_CUR) &&
		pos_write_word(buf, x) &&
		pos_write_word(buf, y);
}

static bool pos_req_save_screen_cls(struct pos_data_buf *buf)
{
	return pos_write_byte(buf, POS_SCREEN_CLS);
}

static bool pos_req_save_screen_print(struct pos_data_buf *buf, char *s, int l,
		uint8_t attr)
{
	return	pos_write_byte(buf, POS_SCREEN_PRINT) &&
		pos_write_array(buf, (uint8_t *)s, l) &&
		pos_write_byte(buf, attr);
}

static bool pos_req_save_screen_color(struct pos_data_buf *buf,
		uint8_t fg, uint8_t bg)
{
	return	pos_write_byte(buf, POS_SCREEN_COLOR) &&
		pos_write_byte(buf, fg) &&
		pos_write_byte(buf, bg);
}

struct slice {
	int offs;
	int len;
};

/* Разбиение строки на части для вывода на экран. Возвращает число частей. */
static int split_str(char *s, struct slice *slices)
{
	int i, m, k, n_slices = 0;
	if ((s == NULL) || (slices == NULL))
		return 0;
	for (i = 0; s[i] && (n_slices < screen->rows); n_slices++){
/* Пропускаем пробелы в начале строки */
		for (; s[i] && isspace(s[i]); i++);
		if (s[i] == 0)
			break;
		k = m = i;	/* m -- начало строки; k -- конец предыдущего слова */
		while (true) {
/* Сканируем слово */
			for (; s[i] && ((i - m) < screen->cols) &&
					!isspace(s[i]); i++);
			if (s[i] == 0)
				break;
			else if (!isspace(s[i])){
				if (k > m)
					i = k;
				break;
			}else{
/* Пропускаем пробелы в конце слова */
				k = i;
				for (; s[i] && isspace(s[i]); i++);
				if ((i - m) >= screen->cols){
					i = k;
					break;
				}
			}
		}
		slices[n_slices].offs = m;
		slices[n_slices].len = i - m;
	}
	return n_slices;
}

/* Вывод на экран сообщения с заданными атрибутами */
bool pos_write_scr(struct pos_data_buf *buf, char *msg, uint8_t fg, uint8_t bg)
{
	struct slice slices[screen->rows];
	int i, n, k, l = strlen(msg);
	char tmp[l + 1];
	strcpy(tmp, msg);
	recode_str(tmp, l);
	if (!pos_req_begin(buf) ||
			!pos_req_stream_begin(buf, POS_STREAM_SCREEN) ||
			!pos_req_save_screen_cls(buf) ||
			!pos_req_save_screen_color(buf, fg, bg))
		return false;
	n = split_str(tmp, slices);
	k = (screen->rows - n) / 2;
	for (i = 0; i < n; i++, k++){
		if (!pos_req_save_screen_cur(buf,
					(screen->cols - slices[i].len) / 2, k) ||
				!pos_req_save_screen_print(buf,
					tmp + slices[i].offs, slices[i].len,
					SYM_ATTR_DEFAULT))
			return false;
	}
	return pos_req_stream_end(buf) && pos_req_end(buf);
}

/* Запись в поток для экрана сообщения об ошибке */
bool pos_save_err_msg(struct pos_data_buf *buf, char *msg)
{
	return pos_write_scr(buf, msg, RED, BLACK);
}
