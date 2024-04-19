/* �㭪樨 ��� ࠡ��� � ����஫쭮� ���⮩ ��� (���, ��3). (c) gsr 2018-2019 */

#if !defined LOG_KKT_H
#define LOG_KKT_H

#if defined __cplusplus
extern "C" {
#endif

#include <sys/timeb.h>
#include <pthread.h>
#include <time.h>
#include "kkt/kkt.h"
#include "log/generic.h"
#include "blimits.h"
#include "cfg.h"
#include "ds1990a.h"
#include "genfunc.h"

/* ��������� ��� */
struct klog_header {
	uint32_t tag;		/* �ਧ��� ��������� */
#define	KLOG_TAG	0x544b4b3e	/* >KKT */
	uint32_t len;		/* ����� �� ��� ��� ��������� */
	uint32_t n_recs;	/* �᫮ ����ᥩ */
	uint32_t head;		/* ᬥ饭�� ��ࢮ� ����� */
	uint32_t tail;		/* ᬥ饭�� ��᫥���� ����� */
	uint32_t cur_number;	/* ����� ⥪�饩 ����� */
} __attribute__((__packed__));

/* �஢�� ��⠫���樨 �� ����� �� ��� */
#define KLOG_LEVEL_ALL		0	/* ������ �ᥩ ���ଠ樨 */
#define KLOG_LEVEL_INF		1	/* ������� ��� ������ � ���� �����襭�� */
#define KLOG_LEVEL_WARN		2	/* ������� ��� ������ � ���� �����襭�� � ��砥 �訡�� */
#define KLOG_LEVEL_ERR		3	/* ���� �����襭�� � ��砥 �訡�� */
#define KLOG_LEVEL_OFF		4	/* ������ �� ��� �� ������� */

/* ���� ��⮪�� ��� */
#define KLOG_STREAM_CTL		1	/* ������� �ࠢ����� */
#define KLOG_STREAM_PRN		2	/* ������� ���� */
#define KLOG_STREAM_FDO		4	/* ����� � ��� */
#define KLOG_STREAM_ALL		(KLOG_STREAM_CTL | KLOG_STREAM_PRN | KLOG_STREAM_FDO)
#define KLOG_STREAM_MASK	KLOG_STREAM_ALL
#define KLOG_STREAM(s)		((s) & KLOG_STREAM_MASK)

/* ��������� ����� ��� */
struct klog_rec_header {
	uint32_t tag;		/* �ਧ��� ��������� ����� */
#define KLOG_REC_TAG	0x4345524b	/* KREC */
	uint32_t number;	/* ����� ����� */
	uint32_t stream;	/* �����䨪��� ��⮪�, � ⠪�� ����稪 ������ ����ᮢ ��� */
	uint32_t len;		/* ����� ����� ��� ��� ���������, � ⠪�� �ਧ��� ��� */
#define KLOG_REC_APC		0x80000000
#define KLOG_REC_FLAG_MASK	(KLOG_REC_APC)
#define KLOG_REC_LEN_MASK	~KLOG_REC_FLAG_MASK
#define KLOG_REC_LEN(l)		(l & KLOG_REC_LEN_MASK)
	uint16_t req_len;	/* ����� ��।����� ������ */
	uint16_t resp_len;	/* ����� �ਭ���� ������ */
	struct date_time dt;	/* ��� � �६� (�� ��᪢�) ᮧ����� ����� */
	uint16_t ms;		/* �����ᥪ㭤� ᮧ����� ����� */
	uint32_t op_time;	/* �६� ����樨 � �� */
	struct term_addr addr;	/* ���� �ନ���� � ������ ᮧ����� ����� */
	uint32_t term_version;	/* ����� �� �ନ����, ᤥ���襣� ������ */
	uint16_t term_check_sum;/* ����஫쭠� �㬬� �ନ����, ᤥ���襣� ������ */
	term_number tn;		/* �����᪮� ����� �ନ����, ᤥ���襣� ������ */
	char kkt_nr[KKT_NR_MAX_LEN];/* �����᪮� ����� ��� � ������ ᮧ����� ����� */
	ds_number dsn;		/* ����� ���� DS1990A � ������ ᮧ����� ����� */
	int ds_type;		/* ⨯ ���� DS1990A */
	uint8_t cmd;		/* �������, ��।����� � ��� */
	uint8_t status;		/* ����� �����襭�� ����樨 */
	uint32_t crc32;		/* ����஫쭠� �㬬� ����� ����� � ���������� */
} __attribute__((__packed__));

extern pthread_mutex_t klog_mtx;

extern struct log_handle *hklog;

/* ����� ⥪�饩 ����� ����஫쭮� ����� */
/* NB: ����� ���� �ᯮ������ ��� ��� �⥭��, ⠪ � ��� ����� */
extern uint8_t klog_data[LOG_BUF_LEN];
/* ����� ������ */
extern uint32_t klog_data_len;
/* ������ ⥪�饣� ��ࠡ��뢠����� ���� � log_data */
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

#if defined __cplusplus
}
#endif

#endif		/* LOG_KKT_H */
