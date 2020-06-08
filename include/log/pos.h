/*
 * Новые функции для работы с контрольной лентой ИПТ (БКЛ).
 * (c) gsr 2004, 2008.
 */

#if !defined LOG_POS_H
#define LOG_POS_H

#include <time.h>
#include "log/generic.h"
#include "prn/local.h"
#include "blimits.h"
#include "cfg.h"
#include "ds1990a.h"
#include "gd.h"
#include "genfunc.h"

/* Заголовок БКЛ */
struct plog_header {
	uint32_t tag;		/* признак заголовка */
#define	PLOG_TAG	0x534f503e	/* >POS */
	uint32_t len;		/* длина КЛ без учета заголовка */
	uint32_t n_recs;	/* число записей */
	uint32_t head;		/* смещение первой записи */
	uint32_t tail;		/* смещение последней записи */
	uint32_t cur_number;	/* номер текущей записи */
} __attribute__((__packed__));

/* Заголовок записи БКЛ */
struct plog_rec_header {
	uint32_t tag;		/* признак заголовка записи */
#define PLOG_REC_TAG	0x43455250	/* PREC */
	uint32_t number;	/* номер записи */
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
	uint32_t crc32;		/* контрольная сумма записи вместе с заголовком */
} __attribute__((__packed__));

/* Типы записей на контрольной ленте */
enum {
	PLRT_NORMAL = 0,	/* обычная запись (абзац для вывода на печать) */
	PLRT_ERROR,		/* сообщение об ошибке */
};

/* Данные текущей записи контрольной ленты */
/* NB: данный буфер используется как для чтения, так и для записи */
extern uint8_t plog_data[LOG_BUF_LEN];
/* Длина данных */
extern uint32_t plog_data_len;
/* Индекс текущего обрабатываемого байта в log_data */
extern uint32_t plog_data_index;

extern struct log_handle *hplog;

extern struct plog_rec_header plog_rec_hdr;

extern uint32_t plog_index_for_number(struct log_handle *hlog, uint32_t number);
extern bool  plog_can_print_range(struct log_handle *hlog);
extern bool  plog_can_print(struct log_handle *hlog);
extern bool  plog_can_find(struct log_handle *hlog);
extern uint32_t plog_write_rec(struct log_handle *hlog, uint8_t *data, uint32_t len,
		uint32_t type);
extern bool  plog_read_rec(struct log_handle *hlog, uint32_t index);

extern bool  plog_print_header(void);
extern bool  plog_print_footer(void);
extern bool  plog_print_rec(void);

#endif		/* LOG_POS_H */
