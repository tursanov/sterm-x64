/* Функции для работы с циклической контрольной лентой. (c) gsr 2004, 2008, 2018-2019. */

#if !defined LOG_EXPRESS_H
#define LOG_EXPRESS_H

#include <time.h>
#include "log/generic.h"
#include "blimits.h"
#include "cfg.h"
#include "ds1990a.h"
#include "gd.h"
#include "genfunc.h"

/* Заголовок циклической контрольной ленты */
struct xlog_header {
	uint32_t tag;		/* признак заголовка */
#define	XLOG_TAG	0x474f4c3e	/* >LOG */
	uint32_t len;		/* длина ЦКЛ без учета заголовка */
	uint32_t n_recs;	/* число записей */
	uint32_t head;		/* смещение первой записи */
	uint32_t tail;		/* смещение последней записи */
	uint32_t cur_number;	/* номер текущей записи */
	bool printed;		/* флаг печати ЦКЛ (для совместимости с предыдущими версиями) */
} __attribute__((__packed__));

/* Заголовок записи контрольной ленты */
struct xlog_rec_header {
	uint32_t tag;		/* признак заголовка записи */
#define XLOG_REC_TAG	0x4345523e	/* >REC */
	uint32_t number;	/* номер записи */
	uint32_t n_para;	/* номер абзаца в записи */
	uint32_t type;		/* тип записи */
	uint16_t code;		/* код записи (приходит из хост-ЭВМ) */
	uint32_t len;		/* длина записи без учета заголовка */
	struct date_time dt;	/* дата и время (по Москве) создания записи */
	struct term_addr addr;	/* адрес терминала в момент создания записи */
	uint32_t term_version;	/* версия ПО терминала, сделавшего запись */
	uint16_t term_check_sum;/* контрольная сумма терминала, сделавшего запись */
	term_number tn;		/* заводской номер терминала, сделавшего запись */
	uint8_t xprn_number[PRN_NUMBER_LEN];	/* заводской номер ОПУ */
	uint8_t aprn_number[PRN_NUMBER_LEN];	/* заводской номер ДПУ */
	ds_number dsn;		/* номер ключа DS1990A в момент создания записи */
	int ds_type;		/* тип ключа DS1990A */
	uint16_t ZNtz;		/* информация сеансового уровня */
	uint16_t ZNpo;
	uint8_t ZBp;
	uint16_t ONtz;
	uint16_t ONpo;
	uint8_t OBp;
	uint8_t reaction_time;	/* время реакции */
	uint32_t flags;		/* различные флаги */
#define XLOG_REC_PRINTED	0x00000001	/* абзац распечатан при обработке ответа */
#define XLOG_REC_APC		0x00000002	/* автоматическая печать чека на ККТ (АПЧ) */
#define XLOG_REC_CPC		0x00000004	/* в корзине ФП есть данные для печати чеков (ЧвКФП) */
	uint32_t crc32;		/* контрольная сумма записи вместе с заголовком */
} __attribute__((__packed__));

/* Типы записей на циклической контрольной ленте */
enum {
	XLRT_NORMAL = 0,	/* обычная запись (абзац для вывода на печать) */
	XLRT_NOTIFY = 0x1001,	/* записывется, если в ответе нет абзацев для печати */
	XLRT_INIT,		/* записывается после прихода ответа на инициализацию */
	XLRT_FOREIGN,		/* получен чужой ответ */
	XLRT_AUX,		/* абзац для дополнительного принтера */
	XLRT_SPECIAL,		/* ответ от хост-ЭВМ с ошибкой контрольной суммы */
	XLRT_BANK,		/* абзац для ИПТ */
	XLRT_IPCHANGE,		/* смена ip-адреса хост-ЭВМ на альтернативный */
	XLRT_REJECT,		/* отказ от заказа */
	XLRT_KKT,		/* абзац для ККТ */
};

/* Данные текущей записи контрольной ленты */
/* NB: данный буфер используется как для чтения, так и для записи */
extern uint8_t xlog_data[LOG_BUF_LEN];
/* Длина данных */
extern uint32_t xlog_data_len;
/* Индекс текущего обрабатываемого байта в log_data */
extern uint32_t xlog_data_index;

extern int aux_msg_len;

extern struct log_handle *hxlog;
extern struct xlog_rec_header xlog_rec_hdr;

extern uint32_t xlog_first_for_number(struct log_handle *hlog, uint32_t number);
extern uint32_t xlog_last_for_number(struct log_handle *hlog, uint32_t number);
extern uint32_t xlog_index_for_number(struct log_handle *hlog, uint32_t number,
		uint32_t n_para);
extern uint32_t xlog_write_rec(struct log_handle *hlog, uint8_t *data, uint32_t len,
		uint32_t type, uint32_t n_para);
extern bool  xlog_write_foreign(struct log_handle *hlog, uint8_t gaddr, uint8_t iaddr);
extern bool  xlog_write_ipchange(struct log_handle *hlog, uint32_t old, uint32_t new);
extern bool  xlog_read_rec(struct log_handle *hlog, uint32_t index);
extern bool  xlog_set_rec_flags(struct log_handle *hlog, uint32_t number,
		uint32_t n_para, uint32_t flags);
extern bool  xlog_can_write(struct log_handle *hlog);
extern bool  xlog_can_clear(struct log_handle *hlog);
extern bool  xlog_can_print_range(struct log_handle *hlog);
extern bool  xlog_can_print(struct log_handle *hlog);
extern bool  xlog_can_find(struct log_handle *hlog);

extern bool  xlog_print_header(void);
extern bool  xlog_print_footer(void);
extern bool  xlog_print_rec(void);
extern bool  xlog_print_llrt_normal(void);

#endif		/* LOG_EXPRESS_H */
