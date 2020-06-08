/* Общие функции для работы с контрольными лентами. (c) gsr 2008. */

#if !defined LOG_GENERIC_H
#define LOG_GENERIC_H

#include "blimits.h"
#include "genfunc.h"
#include "sysdefs.h"

/* Общая часть заголовка контрольных лент */
struct log_header {
	uint32_t tag;		/* признак заголовка (для каждой КЛ свой) */
	uint32_t len;		/* длина КЛ без учета заголовка */
	uint32_t n_recs;	/* число записей */
	uint32_t head;		/* смещение первой записи */
	uint32_t tail;		/* смещение последней записи */
	uint32_t cur_number;	/* номер текущей записи */
};

/*
 * Карта контрольной ленты. Каждый элемент карты является смещением заголовка
 * соответствующей записи. Т.к. в начале ЛКЛ идет заголовок, то любое реальное
 * смещение должно быть больше нуля. Нулевое смещение обозначает неиспользуемый
 * элемент. Карта циклическая, т.е. при достижении конца массива, заполнение
 * продолжается с его начала.
 */
/* 30.08.04: элементами карты стали структуры типа map_entry_t */
/* 29.10.18: в структуру добавлено поле tag */
struct map_entry_t {
	uint32_t offset;		/* смещение заголовка записи в КЛ */
	uint32_t number;		/* номер записи */
	struct date_time dt;		/* время создания записи */
	uint32_t tag;			/* дополнительные данные */
} __attribute__((__packed__));

/* Структура для работы с контрольной лентой заданного типа */
struct log_handle {
	struct log_header *hdr;		/* заголовок */
/*
 * log_header -- это общая часть заголовка, а для конкретного типа КЛ
 * в заголовке могут быть дополнительные поля. hdr_len -- это длина заголовка
 * для конкретного типа контрольной ленты.
 */
	uint32_t hdr_len;
	uint32_t full_len;		/* полная длина файла контрольной ленты */
	const char *log_type;		/* тип контрольной ленты (ЦКЛ/БКЛ/ЛКЛ и т.д.) */
	const char *log_name;		/* имя файла контрольной ленты */
/* Дескриптор для чтения файла КЛ. Открыт всегда. */
	int rfd;
	int wfd;			/* дескриптор для записи на КЛ */
	struct map_entry_t *map;	/* карта контрольной ленты */
	uint32_t map_size;		/* максимальное количество элементов в карте */
	uint32_t map_head;		/* индекс первого элемента карты */
/* Специфические действия при открытии контрольной ленты (может быть NULL) */
	void (*on_open)(struct log_handle *hlog);
/* Специфические действия при закрытии контрольной ленты (может быть NULL) */
	void (*on_close)(struct log_handle *hlog);
/* Инициализация заголовка контрольной ленты */
	void (*init_log_hdr)(struct log_handle *hlog);
/* Вызывается при очистке контрольной ленты */
	void (*clear)(struct log_handle *hlog);
/* Заполнение карты контрольной ленты */
	bool (*fill_map)(struct log_handle *hlog);
/* Чтение заголовка контрольной ленты */
	bool (*read_header)(struct log_handle *hlog);
/* Запись заголовка контрольной ленты */
	bool (*write_header)(struct log_handle *hlog);
};

#define try_fn(fn) ({ if (!fn) return false; })

/* Инициализация подсистемы работы с контрольными лентами */
extern bool log_init_generic(void);
/* Создание контрольной ленты заданного типа */
extern bool log_create(struct log_handle *hlog);
/* Позиционирование и чтение непрерывного блока данных из КЛ */
extern bool log_atomic_read(struct log_handle *hlog, uint32_t offs, uint8_t *buf,
		uint32_t l);
/* Чтение файла контрольной ленты */
extern bool log_read(struct log_handle *hlog, uint32_t offs, uint8_t *buf, uint32_t l);
/* Позиционирование и запись непрерывного блока данных на КЛ */
extern bool log_atomic_write(struct log_handle *hlog, uint32_t offs, uint8_t *buf,
		uint32_t l);
/* Запись в файл линейной ленты */
extern bool log_write(struct log_handle *hlog, uint32_t offs, uint8_t *buf, uint32_t l);
/* Начало записи на контрольную ленту */
extern bool log_begin_write(struct log_handle *hlog);
/* Окончание записи на контрольную ленту */
extern void log_end_write(struct log_handle *hlog);
/* Расстояние между двумя смещениями в кольцевом буфере */
extern uint32_t log_delta(struct log_handle *hlog, uint32_t tail, uint32_t head);
/* Увеличение индекса в кольцевом буфере */
extern uint32_t log_inc_index(struct log_handle *hlog, uint32_t index, uint32_t delta);
/* Определение свободного пространства на КЛ */
extern uint32_t log_free_space(struct log_handle *hlog);
/* Определение номеров крайних записей на КЛ */
extern bool log_get_boundary_numbers(struct log_handle *hlog,
		uint32_t *n0, uint32_t *n1);
/* Определение длины диапазона, ограниченного индексами */
extern uint32_t log_get_interval_len(struct log_handle *hlog,
		uint32_t index1, uint32_t index2);
/*
 * Поиск записи КЛ по номеру. Возвращает индекс записи на КЛ или -1,
 * если запись не найдена
 */
extern uint32_t log_find_rec_by_number(struct log_handle *hlog, uint32_t number);
/*
 * Поиск записи на КЛ по дате создания. Возвращает индекс записи с ближайшей
 * датой создания.
 */
extern uint32_t log_find_rec_by_date(struct log_handle *hlog,
		struct date_time *dt);
/* Очистка контрольной ленты */
extern bool log_clear(struct log_handle *hlog, bool need_write);
/*
 * Если при проверке записи на КЛ будет обнаружена ошибка, эта и все
 * последующие записи будут отброшены.
 */
extern bool log_truncate(struct log_handle *hlog, uint32_t index, uint32_t tail);
/* Открытие контрольной ленты */
extern bool log_open(struct log_handle *hlog, bool can_create);
/* Закрытие контрольной ленты */
extern void log_close(struct log_handle *hlog);
/* Заполнение поля заводского номера принтера */
extern void fill_prn_number(uint8_t *dst, const char *src, size_t len);

/* Буфер печати контрольной ленты */
extern uint8_t log_prn_buf[PRN_BUF_LEN];
/* Длина данных в буфере печати */
extern uint32_t log_prn_data_len;

/* Запись символа в буфер печати без перекодировки */
extern bool prn_write_char_raw(uint8_t c);
/* Запись символа в буфер печати с перекодировкой */
extern bool prn_write_char(uint8_t c);
/* Запись группы символов без перекодировки */
extern bool prn_write_chars_raw(const char *s, int n);
/* Запись в буфер строки с перекодировкой */
extern bool prn_write_str(const char *str);
/* Запись в буфер строки по формату с перекодировкой */
extern bool prn_write_str_fmt(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
/* Запись в буфер печати символа конца строки */
extern bool prn_write_eol(void);
/* Запись в буфер печати текущих даты и времени */
extern bool prn_write_cur_date_time(void);
/* Запись в буфер печати заданных даты и времени */
extern bool prn_write_date_time(struct date_time *dt);
/* Сброс буфера печати в начальное состояние */
extern bool log_reset_prn_buf(void);
/* Вывод на печать штрих-кода */
extern bool log_print_bcode(const uint8_t *log_data, uint32_t log_data_len,
	uint32_t *log_data_index);

#endif		/* LOG_GENERIC_H */
