/* Работа с потоком POS-эмулятора для экрана. (c) A.Popov, gsr 2004 */

#if !defined POS_SCREEN_H
#define POS_SCREEN_H

#if defined __cplusplus
extern "C" {
#endif

#include "gui/gdi.h"
#include "pos/pos.h"
#include "kbd.h"

/* Создание экрана (x, y - координаты пикселов, cols, rows - в знакоместах,
 * x_off, y_off - отступ в пикселах для рамки, font - шрифт,
 * bmp_up, bmp_down - для кнопок вверх/вниз в меню */
extern bool pos_screen_create(int x, int y, int cols, int rows, int x_off, int y_off,
		FontPtr font, BitmapPtr bmp_up, BitmapPtr bmp_down);
/* Удаление экрана */
extern void pos_screen_destroy(void);
/* Перерисовка экрана */
extern bool pos_screen_draw(void);
/* Инициализирован ли экран */
extern bool pos_screen_initialized(void);
/* Обработка событий */
extern void pos_screen_process(struct kbd_event *e);

/* Разбор ответа от POS-эмулятора с выводом на экран */
extern bool pos_parse_screen_stream(struct pos_data_buf *buf, bool check_only);
/* Запись потока от клавиатуры */
extern bool pos_req_save_keyboard_stream(struct pos_data_buf *buf);
/* Имеется ли на экране хотя бы одно меню */
extern bool pos_screen_has_menu(void);
/* Вывод на экран сообщения с заданными атрибутами */
extern bool pos_write_scr(struct pos_data_buf *buf, const char *msg, uint8_t fg, uint8_t bg);
/* Запись в поток для экрана сообщения об ошибке */
extern bool pos_save_err_msg(struct pos_data_buf *buf, char *msg);

#if defined __cplusplus
}
#endif

#endif		/* POS_SCREEN_H */
