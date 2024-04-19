/* Диалоговые окна. (c) gsr, Alex Popov 2000-2004 */

#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

/* Кнопки в окне диалога */
enum {
	dlg_none = 0,
	dlg_yes,
	dlg_yes_no,
	dlg_custom,
};

/* Выравнивание текста в окне диалога */
enum {
	al_left,
	al_right,
	al_center,
};

/* Команды, возвращаемые по окончании модального состояния окна */
enum {
	DLG_BTN_YES = 0,
	DLG_BTN_NO,
	DLG_BTN_CANCEL,
	DLG_BTN_RETRY,
};

extern int message_box(const char *title, const char *message,
	intptr_t buttons, int active_btn, int align);
extern bool begin_message_box(const char *title, const char *message,
	int align);
extern bool end_message_box(void);

/*
 * message_box в качестве параметра buttons может принимать указатель на массив
 * структур, описывающих специальные кнопки.
 */
struct custom_btn {
	const char *text;	/* текст кнопки; для последнего элемента массива -- NULL */
	int cmd;		/* команда, возвращаемая при нажатии на кнопку */
};

/*
 * Задание диапазона записей контрольной ленты.
 * min, max - возвращают диапазон, введенный пользователем;
 * min_val, max_val - граничные значения диапазона.
 * Функция возвращает DLG_BTN_YES или DLG_BTN_NO.
 */
extern int log_range_dlg(uint32_t *min, uint32_t *max, uint32_t min_val, uint32_t max_val);
/* Поиск записи контрольной ленты по дате ее создания */
extern int find_log_date_dlg(int *hour, int *day, int *mon, int *year);
/* Поиск записи контрольной ленты по номеру */
extern int find_log_number_dlg(uint32_t *number, uint32_t min_number,
		uint32_t max_number);

#if defined __cplusplus
}
#endif

#endif		/* GUI_DIALOG_H */
