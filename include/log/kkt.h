/* Функции для работы с контрольной лентой ККТ (ККЛ, КЛ3). (c) gsr 2018-2019 */

#if !defined LOG_KKT_H
#define LOG_KKT_H

#include <sys/timeb.h>
#include <pthread.h>
#include <time.h>
#include "kkt/kkt.h"
#include "log/generic.h"
#include "blimits.h"
#include "cfg.h"
#include "ds1990a.h"
#include "genfunc.h"

/* Заголовок ККЛ */
struct klog_header {
	uint32_t tag;		/* признак заголовка */
#define	KLOG_TAG	0x544b4b3e	/* >KKT */
	uint32_t len;		/* длина КЛ без учета заголовка */
	uint32_t n_recs;	/* число записей */
	uint32_t head;		/* смещение первой записи */
	uint32_t tail;		/* смещение последней записи */
	uint32_t cur_number;	/* номер текущей записи */
} __attribute__((__packed__));

/* Уровни детализации при записи на ККЛ */
#define KLOG_LEVEL_ALL		0	/* запись всей информации */
#define KLOG_LEVEL_INF		1	/* команды без данных и коды завершения */
#define KLOG_LEVEL_WARN		2	/* команды без данных и коды завершения в случае ошибок */
#define KLOG_LEVEL_ERR		3	/* коды завершения в случае ошибок */
#define KLOG_LEVEL_OFF		4	/* запись на ККЛ не ведётся */

/* Типы потоков ККЛ */
#define KLOG_STREAM_CTL		1	/* команды управления */
#define KLOG_STREAM_PRN		2	/* команды печати */
#define KLOG_STREAM_FDO		4	/* обмен с ОФД */
#define KLOG_STREAM_ALL		(KLOG_STREAM_CTL | KLOG_STREAM_PRN | KLOG_STREAM_FDO)
#define KLOG_STREAM_MASK	KLOG_STREAM_ALL
#define KLOG_STREAM(s)		((s) & KLOG_STREAM_MASK)

/* Заголовок записи ККЛ */
struct klog_rec_header {
	uint32_t tag;		/* признак заголовка записи */
#define KLOG_REC_TAG	0x4345524b	/* KREC */
	uint32_t number;	/* номер записи */
	uint32_t stream;	/* идентификатор потока, а также счётчик пустых запросов ОФД */
	uint32_t len;		/* длина записи без учета заголовка, а также признак АПЧ */
#define KLOG_REC_APC		0x80000000
#define KLOG_REC_FLAG_MASK	(KLOG_REC_APC)
#define KLOG_REC_LEN_MASK	~KLOG_REC_FLAG_MASK
#define KLOG_REC_LEN(l)		(l & KLOG_REC_LEN_MASK)
	uint16_t req_len;	/* длина переданных данных */
	uint16_t resp_len;	/* длина принятых данных */
	struct date_time dt;	/* дата и время (по Москве) создания записи */
	uint16_t ms;		/* миллисекунды создания записи */
	uint32_t op_time;	/* время операции в мс */
	struct term_addr addr;	/* адрес терминала в момент создания записи */
	uint32_t term_version;	/* версия ПО терминала, сделавшего запись */
	uint16_t term_check_sum;/* контрольная сумма терминала, сделавшего запись */
	term_number tn;		/* заводской номер терминала, сделавшего запись */
	char kkt_nr[KKT_NR_MAX_LEN];/* заводской номер ККТ в момент создания записи */
	ds_number dsn;		/* номер ключа DS1990A в момент создания записи */
	int ds_type;		/* тип ключа DS1990A */
	uint8_t cmd;		/* команда, переданная в ККТ */
	uint8_t status;		/* статус завершения операции */
	uint32_t crc32;		/* контрольная сумма записи вместе с заголовком */
} __attribute__((__packed__));

extern pthread_mutex_t klog_mtx;

extern struct log_handle *hklog;

/* Данные текущей записи контрольной ленты */
/* NB: данный буфер используется как для чтения, так и для записи */
extern uint8_t klog_data[LOG_BUF_LEN];
/* Длина данных */
extern uint32_t klog_data_len;
/* Индекс текущего обрабатываемого байта в log_data */
extern uint32_t klog_data_index;

extern struct klog_rec_header klog_rec_hdr;

extern const char *klog_get_stream_name(int stream);
extern uint32_t klog_index_for_number(struct log_handle *hlog, uint32_t number);
extern bool  klog_can_print_range(struct log_handle *hlog);
extern bool  klog_can_print(struct log_handle *hlog);
extern bool  klog_can_find(struct log_handle *hlog);
extern uint32_t klog_write_rec(struct log_handle *hlog, const struct timeb *t0,
	const uint8_t *req, uint16_t req_len,
	uint8_t status, const uint8_t *resp, uint16_t resp_len, uint32_t flags);
extern bool  klog_read_rec(struct log_handle *hlog, uint32_t index);

extern bool  klog_print_header(void);
extern bool  klog_print_footer(void);
extern bool  klog_print_rec(void);

#endif		/* LOG_KKT_H */
