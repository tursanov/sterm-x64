/*
 * Функции для работы с контрольной лентой ППУ (ПКЛ).
 * (c) gsr 2004, 2008, 2009.
 */

#if !defined LOG_LOCAL_H
#define LOG_LOCAL_H

#include <time.h>
#include "log/generic.h"
#include "prn/local.h"
#include "blimits.h"
#include "cfg.h"
#include "ds1990a.h"
#include "gd.h"
#include "genfunc.h"

/* Заголовок ПКЛ */
struct llog_header {
	uint32_t tag;		/* признак заголовка */
#define	LLOG_TAG	0x434f4c3e	/* >LOC */
	uint32_t len;		/* длина ПКЛ без учета заголовка */
	uint32_t n_recs;	/* число записей */
	uint32_t head;		/* смещение первой записи */
	uint32_t tail;		/* смещение последней записи */
	uint32_t cur_number;	/* номер текущей записи */
} __attribute__((__packed__));

/* Заголовок записи ПКЛ */
struct llog_rec_header {
	uint32_t tag;		/* признак заголовка записи */
#define LLOG_REC_TAG	0x4345524c	/* LREC */
	uint32_t number;		/* номер записи */
	uint32_t n_para;		/* номер абзаца в записи */
	uint32_t type;		/* тип записи */
	uint32_t len;		/* длина записи без учета заголовка */
	struct date_time dt;	/* дата и время (по Москве) создания записи */
	struct term_addr addr;	/* адрес терминала в момент создания записи */
	uint32_t term_version;	/* версия ПО терминала, сделавшего запись */
	uint16_t term_check_sum;/* контрольная сумма терминала, сделавшего запись */
	term_number tn;		/* заводской номер терминала, сделавшего запись */
	uint8_t xprn_number[PRN_NUMBER_LEN];	/* заводской номер ОПУ */
	uint8_t aprn_number[PRN_NUMBER_LEN];	/* заводской номер ДПУ */
	uint8_t lprn_number[LPRN_NUMBER_LEN];	/* заводской номер ППУ */
	ds_number dsn;		/* номер ключа DS1990A в момент создания записи */
	int ds_type;		/* тип ключа DS1990A */
	uint16_t ZNtz;		/* информация сеансового уровня */
	uint16_t ZNpo;
	uint8_t ZBp;
	uint16_t ONtz;
	uint16_t ONpo;
	uint8_t OBp;
	uint8_t reaction_time;	/* время реакции */
	bool printed;		/* абзац распечатан при обработке ответа */
	uint32_t crc32;		/* контрольная сумма записи вместе с заголовком */
} __attribute__((__packed__));

/* Типы записей на контрольной ленте пригородного приложения */
enum {
	LLRT_NORMAL = 0,	/* обычная запись (абзац для вывода на ППУ) */
	LLRT_NOTIFY,		/* в ответе нет абзацев для печати */
	LLRT_INIT,		/* инициализация */
	LLRT_BANK,		/* абзац для ИПТ */
	LLRT_ERROR,		/* ошибка работы с ППУ */
	LLRT_RD_ERROR,		/* ошибка чтения номера бланка */
	LLRT_PR_ERROR_BCODE,	/* ошибка печати на БСО с контролем штрих-кода */
	LLRT_PR_ERROR,		/* ошибка печати на БСО без контроля штрих-кода */
	LLRT_EXPRESS,		/* абзац для ОПУ */
	LLRT_AUX,		/* абзац для ДПУ */
	LLRT_FOREIGN,		/* получен чужой ответ */
	LLRT_SPECIAL,		/* ответ от хост-ЭВМ с ошибкой контрольной суммы */
	LLRT_ERASE_SD,		/* очистка карты памяти ППУ */
	LLRT_SD_ERROR,		/* ошибка работы с картой памяти */
	LLRT_REJECT,		/* отказ от заказа */
	LLRT_SYNTAX_ERROR,	/* ошибка синтаксиса ответа */
};

/* Коды ошибок LLRT_ERROR */
enum {
	LLRT_ERROR_TIMEOUT = 1,	/* таймаут */
	LLRT_ERROR_MEDIA,	/* неверный тип носителя */
	LLRT_ERROR_GENERIC,	/* общая ошибка */
};

/* Данные текущей записи контрольной ленты */
/* NB: данный буфер используется как для чтения, так и для записи */
extern uint8_t llog_data[LOG_BUF_LEN];
/* Длина данных */
extern uint32_t llog_data_len;
/* Индекс текущего обрабатываемого байта в log_data */
extern uint32_t llog_data_index;

/* Получение описание кода ошибки LLRT_ERROR по её коду */
extern const char *llog_get_llrt_error_txt(uint8_t code);

extern struct log_handle *hllog;

extern struct llog_rec_header llog_rec_hdr;

extern uint32_t llog_first_for_number(struct log_handle *hlog, uint32_t number);
extern uint32_t llog_last_for_number(struct log_handle *hlog, uint32_t number);
extern uint32_t llog_index_for_number(struct log_handle *hlog, uint32_t number,
		uint32_t n_para);
extern bool  llog_can_print_range(struct log_handle *hlog);
extern bool  llog_can_print(struct log_handle *hlog);
extern bool  llog_can_find(struct log_handle *hlog);
extern uint32_t llog_write_rec(struct log_handle *hlog, uint8_t *data, uint32_t len,
		uint32_t type, uint32_t n_para);
extern bool  llog_write_error(struct log_handle *hlog, uint8_t code, uint32_t n_para);
extern bool  llog_write_rd_error(struct log_handle *hlog, uint8_t code);
extern bool  llog_write_pr_error_bcode(struct log_handle *hlog, uint8_t code,
		uint8_t *number, uint32_t n_para);
extern bool  llog_write_pr_error(struct log_handle *hlog, uint8_t code,
		uint32_t n_para);
extern bool  llog_write_foreign(struct log_handle *hlog, uint8_t gaddr,
		uint8_t iaddr);
extern bool  llog_write_erase_sd(struct log_handle *hlog);
extern bool  llog_write_sd_error(struct log_handle *hlog, uint8_t code,
		uint32_t n_para);
extern bool  llog_write_syntax_error(struct log_handle *hlog, uint8_t code);
extern bool  llog_read_rec(struct log_handle *hlog, uint32_t index);
extern bool  llog_mark_rec_printed(struct log_handle *hlog, uint32_t number,
		uint32_t n_para);

extern bool  llog_begin_print(void);
extern bool  llog_end_print(void);
extern bool  llog_print_header(bool full, uint32_t nr_rd_err);
extern bool  llog_print_header_xprn(void);
extern bool  llog_print_footer(void);
extern bool  llog_print_footer_xprn(void);
extern bool  llog_print_rec_header(void);
extern bool  llog_print_rec_header_short(void);
extern bool  llog_print_rec(bool reset, bool need_header);
extern bool  llog_print_rec_footer(void);

#endif		/* LOG_LOCAL_H */
