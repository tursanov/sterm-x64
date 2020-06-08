/* Работа с потоком POS-эмулятора для экрана. (c) A.Popov, gsr 2004 */

#if !defined POS_SCREEN_H
#define POS_SCREEN_H

#include "gui/gdi.h"
#include "pos/pos.h"
#include "kbd.h"

typedef struct pos_node_t_ pos_node_t;
typedef struct pos_screen_t_ pos_screen_t;

/* Тип графического элемента */
enum
{
	POS_TYPE_TEXT,	/* Текст */
	POS_TYPE_EDIT, 	/* Строка ввода */
	POS_TYPE_MENU 	/* Меню */
};

/* Узел */

/* Функция для узла */
typedef void (*pos_node_func_t)(pos_node_t *node);
/* Функция для обработки событий */
typedef void (*pos_node_process_func_t)(pos_node_t *node, struct kbd_event *e);

struct pos_node_t_
{
	pos_node_t *next; 	/* Следующий элемент */
	int type;		/* Тип элемента */
	int x, y;		/* Координаты вывода (в знакоместах) */
	bool can_active;	/* Флаг активизации элемента */
	bool update; 		/* Флаг перерисовки */
	pos_node_func_t draw;	/* Фукция перерисовки */
	pos_node_func_t activate;	/* Фукция активизации */
	pos_node_func_t deactivate;	/* Фукция деактивизации */
	pos_node_func_t free; 	/* Функция удаления доп. данных */
	pos_node_process_func_t process;	/* Фукция обработки событий */
};

/* Текст */
typedef struct
{
	pos_node_t root;	/* Включение pos_node */
	char *str;			/* Текст для отображения */
	uint8_t fg;			/* Цвет символов */
	uint8_t bg;			/* Цвет фона */
	uint8_t attr;			/* Аттрибуты */
} pos_text_t;

/* Строка ввода */
typedef struct
{
	pos_node_t root;	/* Включение pos_node */
	char *name;			/* Имя строки ввода */
	char *text;			/* Текст */
	int count;			/* Количество допустимых символов для ввода */
	int pos;			/* Положение курсора */
	int view_start;		/* Начало отображения при скроллинге */
	int view_width;		/* Ширина на экране в символах */
} pos_edit_t;

/* Меню */
typedef struct
{
	pos_node_t root;	/* Включение pos_node */
	char *name;			/* Имя меню */
	char **items;		/* Элементы меню */
	int count;			/* Количество элементов */
	int selected;		/* Выбранный элемент */
	int width;			/* Ширина меню */
	int height;			/* Высота меню */
	int view_top;		/* Начало отображения */
	int view_width;		/* Отображаемая ширина */
	int view_height;	/* Отображаемая высота */
} pos_menu_t;

/* Экран */
struct pos_screen_t_
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
	bool show_cursor;		/* Показать / спрятать курсор */
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
};

/* Создание экрана (x, y - координаты пикселов, cols, rows - в знакоместах,
 * x_off, y_off - отступ в пикселах для рамки, font - шрифт,
 * bmp_up, bmp_down - для кнопок вверх/вниз в меню */
extern bool pos_screen_create(int x, int y, int cols, int rows,
		int x_off, int y_off,
		FontPtr font, BitmapPtr bmp_up, BitmapPtr bmp_down);

/* Сохранить текущую позицию курсора */
extern void pos_screen_save_pos(void);
/* Восстановить сохраненную позицию курсора */
extern void pos_screen_restore_pos(void);
/* Удаление экрана */
extern bool pos_screen_destroy(void);
/* Перерисовка экрана */
extern bool pos_screen_draw(void);
/* Обработка событий */
extern bool pos_screen_process(struct kbd_event *e);

/* Задание положения виртуального курсора */
extern bool pos_screen_cur(int x, int y);
/* Изменение цвета букв и фона */
extern bool pos_screen_color(uint8_t fg, uint8_t bg);
/* Очистка экрана */
extern bool pos_screen_cls(void);
/* Активировать следующий элемент */
extern bool pos_screen_activate_next(void);

/* Добавление текста */
extern bool pos_screen_insert_text(const char *text, uint8_t attr);

/* Добавление строки ввода */
extern bool pos_screen_insert_edit(const char *name, const char *value, int width);

/* Добавление меню */
extern bool pos_screen_insert_menu(const char *name, char **items,
		int item_count,	int width, int height);

/* Коды команд для экрана */
#define POS_SCREEN_CUR			0x01
#define POS_SCREEN_CLS			0x02
#define POS_SCREEN_EDIT			0x03
#define POS_SCREEN_MENU			0x04
#define POS_SCREEN_PRINT		0x05
#define POS_SCREEN_COLOR		0x08

/* Инициализирован ли экран */
extern bool pos_screen_initialized(void);

/* Разбор ответа от POS-эмулятора с выводом на экран */
extern bool pos_parse_screen_stream(struct pos_data_buf *buf, bool check_only);

/* Определение размера потока от клавиатуры */
extern int pos_req_get_keyboard_len(void);
/* Запись потока от клавиатуры */
extern bool pos_req_save_keyboard_stream(struct pos_data_buf *buf);
/* Имеется ли на экране хотя бы одно меню */
extern bool pos_screen_has_menu(void);

/* Вывод на экран сообщения с заданными атрибутами */
extern bool pos_write_scr(struct pos_data_buf *buf, char *msg,
		uint8_t fg, uint8_t bg);
/* Запись в поток для экрана сообщения об ошибке */
extern bool pos_save_err_msg(struct pos_data_buf *buf, char *msg);

#endif		/* POS_SCREEN_H */
