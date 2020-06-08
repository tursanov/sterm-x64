/* Общие функции для работы с контрольными лентами. (c) gsr 2009 */

#if !defined GUI_LOG_GENERIC_H
#define GUI_LOG_GENERIC_H

#include "gui/gdi.h"
#include "log/generic.h"
#include "blimits.h"
#include "genfunc.h"
#include "kbd.h"

/* Структура для отображения КЛ на экране */
struct log_gui_context {
	struct log_handle *hlog;
	bool *active;		/* флаг активности окна просмотра */
	const char *title;	/* заголовок окна КЛ */
	uint32_t cur_rec_index;	/* номер (не идентификатор) текущей записи КЛ */
	uint32_t nr_head_lines;	/* число строк в заголовке записи */
	uint32_t nr_hint_lines;	/* количество строк подсказки */
/*
 * Буфер для вывода записи КЛ на экран. Данные организованы в виде строк с
 * завершающим нулем. В начале каждой строки идет байт столбца первого символа.
 * Если старший бит символа установлен в 1, символ выводится в инверсном виде.
 */
	uint8_t *scr_data;
	uint32_t scr_data_size;	/* размер экранного буфера */
	uint32_t scr_data_len;	/* длина данных в экранном буфере */
	uint32_t scr_data_lines;/* число строк в экранном буфере */
	uint32_t first_line;	/* номер первой выводимой строки */
	GCPtr gc;
	GCPtr mem_gc;
	bool modal;
	bool asis;		/* показывать символы без перекодировки */
/* Поздняя инициализация структуры */
	void (*init)(struct log_gui_context *ctx);
/* Можно ли показывать запись */
	bool (*filter)(struct log_handle *hlog, uint32_t index);
/* Чтение записи по индексу */
	bool (*read_rec)(struct log_handle *hlog, uint32_t index);
/* Получение заданной строки заголовка */
	const char *(*get_head_line)(uint32_t n);
/* Получение строки подсказки */
	const char *(*get_hint_line)(uint32_t n);
/* Заполнение экранного буфера */
	void (*fill_scr_data)(struct log_gui_context *ctx);
/* Обработчик событий клавиатуры */
	int  (*handle_kbd)(struct log_gui_context *ctx, const struct kbd_event *e);
};

/*
 * Размер группы записей контрольной ленты.
 * Используется для быстрого перемещения.
 */
#define LOG_GROUP_LEN		10
#define LOG_BGROUP_LEN		100

/* Используется для рисования */
#define LOG_SCREEN_LINES	22
#define LOG_SCREEN_COLS		80

/* Очистка экранного буфера */
extern void log_clear_ctx(struct log_gui_context *ctx);
/* Определение длины команды без учёта Ар2 */
extern int  log_get_cmd_len(const uint8_t *data, uint32_t len, int index);
/* Запись в экранный буфер заданной строки */
extern void log_fill_scr_str(struct log_gui_context *ctx,
	const char *format, ...) __attribute__((format (printf, 2, 3)));
/*
 * Добавление в экранный буфер строки. Возвращает false в случае переполнения
 * экранного буфера.
 */
extern bool log_add_scr_str(struct log_gui_context *ctx, bool need_recode,
	const char *format, ...) __attribute__((format (printf, 3, 4)));
/* Рисование контрольной ленты */
extern bool log_draw(struct log_gui_context *ctx);
/* Перерисовка всего экрана */
extern void log_redraw(struct log_gui_context *ctx);
/* Поиск записи контрольной ленты по ее номеру */
extern bool log_show_rec_by_number(struct log_gui_context *ctx, uint32_t number);
/* Поиск записи контрольной ленты по дате создания */
extern bool log_show_rec_by_date(struct log_gui_context *ctx,
		struct date_time *dt);
/* Обработчик событий клавиатуры */
extern int  log_process(struct log_gui_context *ctx, struct kbd_event *e);
/* Инициализация интерфейса пользователя */
extern bool log_init_view(struct log_gui_context *ctx);
/* Освобождение интерфейса пользователя */
extern void log_release_view(struct log_gui_context *ctx);

#endif		/* GUI_LOG_GENERIC_H */
