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
#include "sterm.h"
#include "termlog.h"

#define BLINK_TIME		25	/* время мигания символа в сотых секунды */
#define CURSOR_TIME		30	/* время мигания курсора */

/* Коды команд для экрана */
#define POS_SCREEN_CUR		0x01
#define POS_SCREEN_CLS		0x02
#define POS_SCREEN_EDIT		0x03
#define POS_SCREEN_MENU		0x04
#define POS_SCREEN_PRINT	0x05
#define POS_SCREEN_COLOR	0x08

typedef struct pos_node_t_	pos_node_t;

/* Тип графического элемента */
enum {
	POS_TYPE_TEXT,		/* Текст */
	POS_TYPE_EDIT, 		/* Строка ввода */
	POS_TYPE_MENU 		/* Меню */
};

/* Функция для узла */
typedef void (*pos_node_func_t)(pos_node_t *node);
/* Функция для обработки событий */
typedef void (*pos_node_process_func_t)(pos_node_t *node, struct kbd_event *e);

/* Узел */
struct pos_node_t_
{
	pos_node_t *next;	 	/* Следующий элемент */
	int type;			/* Тип элемента */
	int x, y;			/* Координаты вывода (в знакоместах) */
	bool can_active;		/* Флаг активизации элемента */
	bool update; 			/* Флаг перерисовки */
	pos_node_func_t draw;		/* Фукция перерисовки */
	pos_node_func_t activate;	/* Фукция активизации */
	pos_node_func_t deactivate;	/* Фукция деактивизации */
	pos_node_func_t free; 		/* Функция удаления доп. данных */
	pos_node_process_func_t process;/* Фукция обработки событий */
};

/* Текст */
typedef struct {
	pos_node_t root;	/* Включение pos_node */
	char *str;		/* Текст для отображения */
	uint8_t fg;		/* Цвет символов */
	uint8_t bg;		/* Цвет фона */
	uint8_t attr;		/* Аттрибуты */
} pos_text_t;

/* Строка ввода */
typedef struct {
	pos_node_t root;	/* Включение pos_node */
	char *name;		/* Имя строки ввода */
	char *text;		/* Текст */
	int count;		/* Количество допустимых символов для ввода */
	int pos;		/* Положение курсора */
	int view_start;		/* Начало отображения при скроллинге */
	int view_width;		/* Ширина на экране в символах */
} pos_edit_t;

/* Меню */
typedef struct {
	pos_node_t root;	/* Включение pos_node */
	char *name;		/* Имя меню */
	char **items;		/* Элементы меню */
	int count;		/* Количество элементов */
	int selected;		/* Выбранный элемент */
	int width;		/* Ширина меню */
	int height;		/* Высота меню */
	int view_top;		/* Начало отображения */
	int view_width;		/* Отображаемая ширина */
	int view_height;	/* Отображаемая высота */
} pos_menu_t;

/* Экран */
typedef struct pos_screen_t_
{
	GCPtr gc;			/* Контекст для вывода */
	int cols;			/* Ширина в знакоместах */
	int rows;			/* Высота в знакоместах */
	pos_node_t *head;		/* Начало списка элементов */
	pos_node_t *tail;		/* Конец списка элементов */
	pos_node_t *active;	 	/* Активный элемент */
	int x, y;			/* Текущие координаты */
	uint8_t fg, bg;			/* Текущие цвет букв и фона */
	bool update; 			/* Флаг перерисовки */
	uint32_t blink_time;		/* Время последнего мигания */
	bool blink_hide;		/* Мигающие символы должны быть погашены */
	bool pos_show_cursor;		/* Показать / спрятать курсор */
	uint32_t cursor_time;		/* Время последнего мигания курсора */
	bool cursor_on_screen;		/* Курсор на экране */
	int cursor_x;			/* Координата по x курсора */
	int cursor_y;			/* Координата по y курсора */
	bool outside;			/* Флаг невозможности добавления новых элементов */
	BitmapPtr bmp_up;		/* Стрелка вверх */
	BitmapPtr bmp_down;		/* Стрелка вниз */
	int x_off, y_off;		/* Отступ для рамки */
	int saved_x, saved_y;		/* позиция курсора для запоминания */
	bool saved_outside;		/* запомненный outside */
} pos_screen_t;

/* Глобальный экран вывода информации */
static pos_screen_t *screen = NULL;

/* Инициализирован ли экран */
bool pos_screen_initialized(void)
{
	return screen != NULL;
}

/* Сохранить текущую позицию курсора */
static void pos_screen_save_pos(void)
{
	if (screen != NULL){
		screen->saved_x = screen->x;
		screen->saved_y = screen->y;
		screen->saved_outside = screen->outside;
	}
}

/* Восстановить сохраненную позицию курсора */
static void pos_screen_restore_pos(void)
{
	if (screen != NULL){
		screen->x = screen->saved_x;
		screen->y = screen->saved_y;
		screen->outside = screen->saved_outside;
	}
}

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

	screen->pos_show_cursor = false;

	screen->outside = false;

	screen->bmp_up = bmp_up;
	screen->bmp_down = bmp_down;

	screen->x_off = x_off;
	screen->y_off = y_off;

	return true;
}

/* Рисование курсора с помощью XOR в заданной позиции */
static void pos_draw_cursor(void)
{
	FontPtr font = screen->gc->pFont;

	int x = screen->cursor_x*font->max_width + screen->x_off;
	int y = screen->cursor_y*font->max_height + font->max_top + screen->y_off;
	int w = font->underline_y2 - font->underline_y1 + 1;
	int h = font->max_bottom - font->max_top + 1;

	screen->gc->pencolor = RGB(128, 128, 128);
	screen->gc->rop2mode = R2_XOR;
	while (h--){
		Line(screen->gc, x, y, x + w - 1, y);
		y++;
	}
	screen->gc->rop2mode = R2_COPY;
}

/* Показать курсор в заданной позиции */
static void pos_show_cursor(int x, int y)
{
	if (!screen->pos_show_cursor){
		screen->cursor_x = x;
		screen->cursor_y = y;
		screen->pos_show_cursor = true;
		screen->cursor_time = u_times();
		screen->cursor_on_screen = true;
	}else if ((x != screen->cursor_x) || (y != screen->cursor_y)){
		if (screen->cursor_on_screen)
			pos_draw_cursor();
		screen->cursor_x = x;
		screen->cursor_y = y;
		screen->cursor_time = u_times();
		screen->cursor_on_screen = true;
	}
	pos_draw_cursor();
}

/* Спрятать курсор */
static void pos_hide_cursor(void)
{
	if (screen->pos_show_cursor){
		if (screen->cursor_on_screen)
			pos_draw_cursor();
		screen->cursor_on_screen = false;
		screen->pos_show_cursor = false;
	}
}

/* Активировать следующий элемент */
static void pos_screen_activate_next(void)
{
	pos_node_t *active = screen->active;
	pos_node_t *current = active;
	bool update = false;
	if (!active){
		for (active = screen->head; active; active = active->next){
			if (active->can_active){
				update = true;
				break;
			}
		}
	}else{
		for (active = active->next; active; active = active->next){
			if (active->can_active)
				break;
		}
		if (!active){
			for (active = screen->head; active; active = active->next){
				if (active->can_active)
					break;
			}
		}
		if (current != active)
			update = true;
	}
	screen->active = active;
	if (update){
		if (current){
			if (current->deactivate)
				current->deactivate(current);
			current->update = true;
		}
		if (active){
			active->update = true;
			if (active->activate)
				active->activate(active);
		}
	}
}

/* Перерисовка экрана */
bool pos_screen_draw(void)
{
	if (!pos_screen_initialized())
		return false;
	if (screen->update)
		ClearGC(screen->gc, symbol_palette[SYM_COLOR_BLACK]);
	bool blink = false;
	uint32_t time = u_times();
	if (time - screen->blink_time >= BLINK_TIME){
		blink = true;
		screen->blink_hide = !screen->blink_hide;
		screen->blink_time = time;
	}
	bool update_cursor = false;
	for (pos_node_t *node = screen->head; node; node = node->next){
		if (node == screen->active)
			continue;
		else if (blink && node->type == POS_TYPE_TEXT){
			pos_text_t *text;

			text = (pos_text_t *)node;

			if (text->attr & SYM_ATTR_BLINK){
				node->draw(node);
				node->update = false;
				continue;
			}
		}
		if (node->update || screen->update){
			if (screen->pos_show_cursor){
				pos_hide_cursor();
				update_cursor = true;
			}
			node->draw(node);
			node->update = false;
		}
	}
	if (screen->active && (screen->active->update || screen->update)){
		if (screen->pos_show_cursor){
			pos_hide_cursor();
			update_cursor = true;
		}
		screen->active->draw(screen->active);
		screen->active->update = false;
	}
	if (update_cursor)
		pos_show_cursor(screen->cursor_x, screen->cursor_y);
	screen->update = false;
	if (screen->pos_show_cursor){
		time = u_times();
		if (time - screen->cursor_time >= CURSOR_TIME){
			screen->cursor_time = time;
			screen->cursor_on_screen = !screen->cursor_on_screen;
			pos_draw_cursor();
		}
	}
	return true;
}

/* Обработка событий */
void pos_screen_process(struct kbd_event *e)
{
	if ((e->key == KEY_LSHIFT) || (e->key == KEY_RSHIFT) || (e->key == KEY_CAPS))
		scr_show_language(true);
	else if (e->pressed && !e->repeated && (e->shift_state == 0)){
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
}

/* Задание положения виртуального курсора */
static bool pos_screen_cur(int x, int y)
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
static bool pos_screen_color(uint8_t fg, uint8_t bg)
{
	screen->fg = fg & 0x7;
	screen->bg = bg & 0x7;
	return true;
}

/* Очистка экрана */
static bool pos_screen_cls()
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
	screen->pos_show_cursor = false;
	screen->outside = false;
	screen->x = 0;
	screen->y = 0;
	screen->fg = SYM_COLOR_WHITE;
	screen->bg = SYM_COLOR_BLACK;

	return true;
}

/* Вставка узла в список экранных элементов */
static void pos_screen_insert_node(pos_node_t *node)
{
	node->next = NULL;
	if (screen->tail)
		screen->tail->next = node;
	else
		screen->head = node;
	screen->tail = node;
}

/* Проверка текста на выход из границ и установка позиции виртуального курсора */
static bool pos_validate_text(const char *text)
{
	int x, y;
	for (x = screen->x, y = screen->y; *text; text++){
		if (*text == '\n'){
			y++;
			if (y >= screen->rows && *(text+1) == 0)
				return false;
			continue;
		}else if (*text == '\r'){
			x = 0;
			continue;
		}
		x++;
		if (x == screen->cols){
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

/* Освобождение дополнительных данных */
static void pos_free_text(pos_text_t *pos_text)
{
	free(pos_text->str);
}

/* Вывод текста в заданной позиции курсора */
static void pos_rich_text_out(int x, int y, rich_text_t *text)
{
	if (!text->count)
		return;
	FontPtr font = screen->gc->pFont;
	GCPtr mem = CreateMemGC(text->count*font->max_width, font->max_height);
	mem->pFont = font;
	text->blink_hide = screen->blink_hide;
	draw_rich_text(mem, 0, 0, text);
	CopyGC(screen->gc, x * font->max_width + screen->x_off,
		y*font->max_height + screen->y_off, mem, 0, 0, GetCX(mem), GetCY(mem));
	DeleteGC(mem);
}

/* Рисование текста */
static void pos_draw_text(pos_text_t *pos_text)
{
	int x = pos_text->root.x;
	int x_out = x;
	int y = pos_text->root.y;
	char *str = pos_text->str;
	rich_text_t rich_text = {
		.text = str,
		.count = 0,
		.fg = pos_text->fg,
		.bg = pos_text->bg,
		.attr = pos_text->attr,
	};
	for (; *str; str++){
		if (*str == '\n'){
			pos_rich_text_out(x_out, y, &rich_text);
			rich_text.count = 0;
			rich_text.text = str + 1;
			x_out = x;
			y++;
			continue;
		}else if (*str == '\r'){
			pos_rich_text_out(x_out, y, &rich_text);
			x = 0;
			x_out = 0;
			rich_text.count = 0;
			rich_text.text = str + 1;
			continue;
		}
		x++;
		rich_text.count++;
		if (x == screen->cols){
			pos_rich_text_out(x_out, y, &rich_text);
			rich_text.count = 0;
			rich_text.text = str + 1;
			x_out = 0;
			x = 0;
			y++;
		}
	}
	pos_rich_text_out(x_out, y, &rich_text);
}

/* Добавление строки */
static bool pos_screen_insert_text(const char *text, uint8_t attr)
{
	if (text == NULL)
		return false;
	pos_text_t *pos_text = malloc(sizeof(pos_text_t));
	if (pos_text == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	pos_text->str = strdup(text);
	if (pos_text->str == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		free(pos_text);
		return false;
	}
	pos_text->root.x = screen->x;
	pos_text->root.y = screen->y;
	if (!pos_validate_text(text)){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)text);
		free(pos_text);
		screen->outside = true;
		return false;
	}
	pos_text->root.type = POS_TYPE_TEXT;
	pos_text->root.can_active = false;
	pos_text->root.update = true;
	pos_text->root.draw = (pos_node_func_t)pos_draw_text;
	pos_text->root.activate = NULL;
	pos_text->root.deactivate = NULL;
	pos_text->root.process = NULL;
	pos_text->root.free = (pos_node_func_t)pos_free_text;
	pos_screen_insert_node((pos_node_t *)pos_text);
	pos_text->fg = screen->fg;
	pos_text->bg = screen->bg;
	pos_text->attr = attr;
	return true;
}

/* Освобождение дополнительных данных */
static void pos_free_edit(pos_edit_t *edit)
{
	free(edit->name);
	free(edit->text);
}

/* Используемые цвета */
#define EDIT_BACKGROUND	RGB(0xFF, 0xFF, 0xFF)

/* Рисование строки ввода */
static void draw_pos_edit(pos_edit_t *edit)
{
	FontPtr font = screen->gc->pFont;
	GCPtr mem = CreateMemGC(edit->view_width*font->max_width + screen->x_off*2,
			font->max_height + screen->y_off*2);
	mem->pFont = font;
	ClearGC(mem, EDIT_BACKGROUND);
	DrawBorder(mem, 0, 0, GetCX(mem), GetCY(mem), 1, clWhite, clGray);
	DrawBorder(mem, 1, 1, GetCX(mem)-2, GetCY(mem)-2, 2, clSilver, clSilver);
	Color frame_color = (screen->active == (pos_node_t *)edit) ? clRed : clBlack;
	DrawBorder(mem, 3, 3, GetCX(mem) - 6, GetCY(mem) - 6, 1, frame_color, frame_color);
	mem->textcolor = clBlack;
	TextOutN(mem, screen->x_off, screen->y_off, edit->text + edit->view_start, edit->view_width);
	CopyGC(screen->gc, edit->root.x * font->max_width, edit->root.y * font->max_height,
			mem, 0, 0, GetCX(mem), GetCY(mem));
	DeleteGC(mem);
}

/* Активация фокуса строки ввода */
static void activate_pos_edit(pos_edit_t *edit)
{
	pos_show_cursor(edit->root.x, edit->root.y);
}

/* Деактивация фокуса строки ввода */
static void deactivate_pos_edit(pos_edit_t *edit)
{
	pos_hide_cursor();
	edit->pos = 0;
	edit->view_start = 0;
}

/* Обработка событий строки ввода */
static void pos_process_edit(pos_edit_t *edit, struct kbd_event *e)
{
	if (!e->pressed)
		return;
	switch (e->key){
		case KEY_BACKSPACE:
			if (edit->pos > 0){
				memmove(edit->text + edit->pos - 1, edit->text + edit->pos,
					strlen(edit->text) - edit->pos + 1);
				edit->pos--;
				if (edit->view_start > edit->pos)
					edit->view_start = edit->pos;
				pos_show_cursor(edit->root.x + edit->pos - edit->view_start,
						screen->cursor_y);
				edit->root.update = true;
			}
			break;
		case KEY_DEL:
			if (edit->text[0] != 0){
				edit->root.update = true;
				if ((kbd_shift_state & SHIFT_CTRL))
					edit->text[0] = 0;
				else{
					if (edit->pos != strlen(edit->text))
						memcpy(edit->text + edit->pos,
							edit->text + edit->pos + 1,
							strlen(edit->text) - edit->pos);
					break;
				}
			}
			__fallthrough__;
		case KEY_HOME:
			if (edit->pos > 0){
				edit->pos = 0;
				if (edit->view_start > 0)
					edit->root.update = true;
				edit->view_start = 0;
				pos_show_cursor(edit->root.x, screen->cursor_y);
			}
			break;
		case KEY_END:
			if (edit->pos < strlen(edit->text)){
				edit->pos = strlen(edit->text);
				if (edit->pos >= (edit->view_start + edit->view_width)){
					edit->view_start = edit->pos - edit->view_width + 1;
					edit->root.update = true;
				}
				pos_show_cursor(edit->root.x + edit->pos - edit->view_start,
						screen->cursor_y);
			}
			break;
		case KEY_LEFT:
			if (edit->pos > 0){
				edit->pos--;
				if (edit->view_start > edit->pos){
					edit->view_start = edit->pos;
					edit->root.update = true;
				}
				pos_show_cursor(edit->root.x + edit->pos - edit->view_start,
						screen->cursor_y);
			}
			break;
		case KEY_RIGHT:
			if (edit->pos < strlen(edit->text)){
				edit->pos++;
				if (edit->pos >= (edit->view_start + edit->view_width)){
					edit->view_start = edit->pos - edit->view_width + 1;
					edit->root.update = true;
				}
				pos_show_cursor(edit->root.x + edit->pos - edit->view_start,
						screen->cursor_y);
			}
			break;
		default:
			if ((e->ch != 0) && (e->ch != 0x1b) && (strlen(edit->text) < edit->count)){
				memmove(edit->text + edit->pos + 1, 
						edit->text + edit->pos,
						strlen(edit->text) - edit->pos + 1);
				edit->text[edit->pos] = e->ch;
				edit->root.update = true;
				edit->pos++;
				if (edit->pos >= edit->view_start + edit->view_width)
					edit->view_start = edit->pos - edit->view_width + 1;
				pos_show_cursor(edit->root.x + edit->pos - edit->view_start,
						screen->cursor_y);
			}
	}
}

/* Добавление строки ввода */
static bool pos_screen_insert_edit(const char *name, const char *initial_text, int count)
{
	if (name == NULL)
		return false;
	int view_width = 0;
	if (screen->x + count > screen->cols)
		view_width = screen->cols - screen->x;
	else
		view_width = count;
	pos_edit_t *pos_edit = malloc(sizeof(pos_edit_t));
	if (pos_edit == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	pos_edit->name = strdup(name);
	if (pos_edit->name == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		free(pos_edit);
		return false;
	}
	pos_edit->text = malloc(count + 1);
	if (pos_edit->text == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		free(pos_edit->name);
		free(pos_edit);
		return false;
	}
	if (initial_text == NULL)
		*pos_edit->text = 0;
	else
		snprintf(pos_edit->text, count, "%s", initial_text);
	pos_edit->root.type = POS_TYPE_EDIT;
	pos_edit->root.x = screen->x;
	pos_edit->root.y = screen->y;
	pos_edit->root.can_active = true;
	pos_edit->root.update = true;
	pos_edit->root.draw = (pos_node_func_t)draw_pos_edit;
	pos_edit->root.activate = (pos_node_func_t)activate_pos_edit;
	pos_edit->root.deactivate = (pos_node_func_t)deactivate_pos_edit;
	pos_edit->root.free = (pos_node_func_t)pos_free_edit;
	pos_edit->root.process = (pos_node_process_func_t)pos_process_edit;
	pos_edit->count = count;
	pos_edit->pos = 0;
	pos_edit->view_start = 0;
	pos_edit->view_width = view_width;
	pos_screen_insert_node((pos_node_t *)pos_edit);
	if (screen->active == NULL)
		pos_screen_activate_next();
	screen->x += view_width;
	if ((screen->x + 1) > screen->cols){
		screen->x = 0;
		screen->y++;
	}
	return true;
}

/* Используемые цвета */
#define MENU_BACKGROUND		RGB(0xBB, 0xBB, 0xBB)
#define MENU_FOCUSED_TEXT	RGB(0xFF, 0xFF, 0xFF)
#define MENU_SELECTED_TEXT	RGB(0x00, 0x00, 0xFF)
#define MENU_TEXT		RGB(0x00, 0x00, 0x00)

/* Рисование меню */
static void pos_draw_menu(pos_menu_t *menu)
{
	FontPtr font = screen->gc->pFont;
	GCPtr mem = CreateMemGC(menu->view_width*font->max_width + screen->x_off * 2,
			menu->view_height * font->max_height + screen->y_off * 2);
	mem->pFont = font;
	ClearGC(mem, RGB(0xBB, 0xBB, 0xBB));
	if (screen->active == (pos_node_t *)menu){
		mem->brushcolor = RGB(0x00, 0x00, 0x64);
		FillBox(mem, screen->x_off,
			(menu->selected - menu->view_top) * font->max_height + screen->y_off,
			menu->view_width * font->max_width, font->max_height);
	}
	DrawBorder(mem, 0, 0, GetCX(mem), GetCY(mem), 1, clWhite, clGray);
	Color frame_color = (screen->active == (pos_node_t *)menu) ?  clRed : clBlack;
	DrawBorder(mem, 3, 3, GetCX(mem)-6, GetCY(mem)-6, 1, frame_color, frame_color);
	mem->textcolor = clBlack;
	for (int i = 0; (i < menu->view_height) && (i + menu->view_top < menu->count); i++){
		if ((menu->view_top + i) == menu->selected){
			if (screen->active == (pos_node_t *)menu)
				mem->textcolor = MENU_FOCUSED_TEXT;
			else
				mem->textcolor = MENU_SELECTED_TEXT;
		}else
			mem->textcolor = MENU_TEXT;
		DrawText(mem, screen->x_off, i * font->max_height + screen->y_off,
				menu->view_width * font->max_width,
				font->max_height, menu->items[i + menu->view_top], 0);
	}
	if (screen->active == (pos_node_t *)menu){
		if (menu->view_top && screen->bmp_up){
			DrawBitmap(mem, screen->bmp_up,
					(mem->box.width - screen->bmp_up->width) / 2, 0,
					-1, -1, false, 0);
		}
		if (((menu->view_top + menu->view_height) < menu->count) && screen->bmp_down){
			DrawBitmap(mem, screen->bmp_down,
					(mem->box.width - screen->bmp_down->width) / 2,
					(mem->box.height - screen->bmp_down->height),
					-1, -1, false, 0);
		}
	}
	CopyGC(screen->gc, menu->root.x * font->max_width, menu->root.y * font->max_height,
			mem, 0, 0, GetCX(mem), GetCY(mem));
	DeleteGC(mem);
}

/* Обработка событий меню */
static void pos_process_menu(pos_menu_t *menu, struct kbd_event *e)
{
	if (!e->pressed)
		return;
	switch (e->key){
		case KEY_UP:
			if (menu->selected > 0){
				menu->selected--;
				if (menu->selected < menu->view_top)
					menu->view_top = menu->selected;
			}
			menu->root.update = true;
			break;
		case KEY_DOWN:
			if ((menu->selected + 1) < menu->count){
				menu->selected++;
				if (menu->selected >= menu->view_top + menu->view_height)
					menu->view_top = menu->selected - menu->view_height + 1;
			}
			menu->root.update = true;
			break;
	}
}

/* Освобождение дополнительных данных */
static void pos_free_menu(pos_menu_t *menu)
{
	for (int i = 0; i < menu->count; i++)
		free(menu->items[i]);
	free(menu->items);
	free(menu->name);
}

/* Добавление меню */
static bool pos_screen_insert_menu(const char *name, char **items, int count, int width, int height)
{
	if ((name == NULL) || (items == NULL) || (count <= 0))
		return false;
	else if (((screen->x + width) > screen->cols) ||
			((screen->y + height) > screen->rows)){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)name);
		screen->outside = true;
		return false;
	}	
	for (int i = 0; i < count; i++){
		if (items[i] == NULL){
			pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)name);
			return false;
		}
	}
	pos_menu_t *pos_menu = malloc(sizeof(pos_menu_t));
	if (pos_menu == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	pos_menu->name = strdup(name);
	if (pos_menu->name == NULL){
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
	pos_menu->root.draw = (pos_node_func_t)pos_draw_menu;
	pos_menu->root.activate = NULL;
	pos_menu->root.deactivate = NULL;
	pos_menu->root.free = (pos_node_func_t)pos_free_menu;
	pos_menu->root.process = (pos_node_process_func_t)pos_process_menu;
	pos_screen_insert_node((pos_node_t *)pos_menu);
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

/* Удаление экрана */
void  pos_screen_destroy(void)
{
	if (pos_screen_initialized()){
		pos_screen_cls();
		DeleteGC(screen->gc);
		free(screen);
		screen = NULL;
		pos_active = false;
	}
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
	if (screen->outside){
		log_err("MENU: невозможно добавить новый элемент.");
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
	char name[33];
	int n = pos_read_array(buf, (uint8_t *)name, sizeof(name) - 1);
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
	char **items = (char **)calloc(item_count, sizeof(char *));
	if (items == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	int i;
	for (i = 0; i < item_count; i++){
		char item[1024];
		n = pos_read_array(buf, (uint8_t *)item, sizeof(item));
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
		for (int j = 0; j < i; j++)
			free(items[j]);
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
	if (screen->outside){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_DEPICT, (intptr_t)"");
		return false;
	}
	char str[2049];
	int n = pos_read_array(buf, (uint8_t *)str, sizeof(str) - 1);
	if (n == -1){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	str[n] = 0;
	recode_str(str, n);
	uint8_t attr = 0;
	if (!pos_read_byte(buf, &attr)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	if (check_only || (n == 0)){
		if (pos_validate_text(str))
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
	uint8_t fg = 0, bg = 0;
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
	bool ret = false;
	for (pos_node_t *node = screen->head; node != NULL; node = node->next){
		if (node->type == POS_TYPE_MENU){
			ret = true;
			break;
		}
	}
	return ret;
}

/* Запись потока от клавиатуры */
bool pos_req_save_keyboard_stream(struct pos_data_buf *buf)
{
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
		static char name[33], value[1025];
		if (node->type == POS_TYPE_MENU){
			char s[12];
			sprintf(s, "%d", ((pos_menu_t *)node)->selected + 1);
			strncpy(name, ((pos_menu_t *)node)->name, sizeof(name) - 1);
			name[32] = 0;
			recode_str(name, -1);
			if (!pos_write_array(buf, (uint8_t *)name,	strlen(name)) ||
					!pos_write_array(buf, (uint8_t *)s, strlen(s)))
				return false;
		}else if (node->type == POS_TYPE_EDIT){
			strncpy(name, ((pos_edit_t *)node)->name, sizeof(name) - 1);
			name[sizeof(name) - 1] = 0;
			recode_str(name, -1);
			strncpy(value, ((pos_edit_t *)node)->text, sizeof(value) - 1);
			value[sizeof(value) - 1] = 0;
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
	if ((s == NULL) || (slices == NULL))
		return 0;
	int n_slices = 0;
	for (int i = 0; s[i] && (n_slices < screen->rows); n_slices++){
/* Пропускаем пробелы в начале строки */
		for (; s[i] && isspace(s[i]); i++);
		if (s[i] == 0)
			break;
		int k = i;	/* конец предыдущего слова */
		int m = i;	/* начало строки */
		while (true){
/* Сканируем слово */
			for (; s[i] && ((i - m) < screen->cols) && !isspace(s[i]); i++);
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
bool pos_write_scr(struct pos_data_buf *buf, const char *msg, uint8_t fg, uint8_t bg)
{
	int l = strlen(msg);
	char tmp[l + 1];
	strcpy(tmp, msg);
	recode_str(tmp, l);
	if (!pos_req_begin(buf) ||
			!pos_req_stream_begin(buf, POS_STREAM_SCREEN) ||
			!pos_req_save_screen_cls(buf) ||
			!pos_req_save_screen_color(buf, fg, bg))
		return false;
	struct slice slices[screen->rows];
	int n = split_str(tmp, slices);
	int k = (screen->rows - n) / 2;
	for (int i = 0; i < n; i++, k++){
		if (!pos_req_save_screen_cur(buf, (screen->cols - slices[i].len) / 2, k) ||
				!pos_req_save_screen_print(buf,
					tmp + slices[i].offs, slices[i].len, SYM_ATTR_DEFAULT))
			return false;
	}
	return pos_req_stream_end(buf) && pos_req_end(buf);
}

/* Запись в поток для экрана сообщения об ошибке */
bool pos_save_err_msg(struct pos_data_buf *buf, char *msg)
{
	return pos_write_scr(buf, msg, RED, BLACK);
}
