/*
 * ���� �㭪樨 ��� ࠡ��� � ����஫쭮� ���⮩ ��� (���).
 * (c) gsr 2004, 2008.
 */

#if !defined LOG_POS_H
#define LOG_POS_H

#if defined __cplusplus
extern "C" {
#endif

#include <time.h>
#include "log/generic.h"
#include "prn/local.h"
#include "blimits.h"
#include "cfg.h"
#include "ds1990a.h"
#include "gd.h"
#include "genfunc.h"

/* ��������� ��� */
struct plog_header {
	uint32_t tag;		/* �ਧ��� ��������� */
#define	PLOG_TAG	0x534f503e	/* >POS */
	uint32_t len;		/* ����� �� ��� ��� ��������� */
	uint32_t n_recs;	/* �᫮ ����ᥩ */
	uint32_t head;		/* ᬥ饭�� ��ࢮ� ����� */
	uint32_t tail;		/* ᬥ饭�� ��᫥���� ����� */
	uint32_t cur_number;	/* ����� ⥪�饩 ����� */
} __attribute__((__packed__));

/* ��������� ����� ��� */
struct plog_rec_header {
	uint32_t tag;		/* �ਧ��� ��������� ����� */
#define PLOG_REC_TAG	0x43455250	/* PREC */
	uint32_t number;	/* ����� ����� */
	uint32_t type;		/* ⨯ ����� */
	uint32_t len;		/* ����� ����� ��� ��� ��������� */
	struct date_time dt;	/* ��� � �६� (�� ��᪢�) ᮧ����� ����� */
	struct term_addr addr;	/* ���� �ନ���� � ������ ᮧ����� ����� */
	uint32_t term_version;	/* ����� �� �ନ����, ᤥ���襣� ������ */
	uint16_t term_check_sum;/* ����஫쭠� �㬬� �ନ����, ᤥ���襣� ������ */
	term_number tn;		/* �����᪮� ����� �ନ����, ᤥ���襣� ������ */
	uint8_t xprn_number[PRN_NUMBER_LEN];	/* �����᪮� ����� ��� */
	uint8_t aprn_number[PRN_NUMBER_LEN];	/* �����᪮� ����� ��� */
	uint8_t lprn_number[LPRN_NUMBER_LEN];	/* �����᪮� ����� ��� */
	ds_number dsn;		/* ����� ���� DS1990A � ������ ᮧ����� ����� */
	int ds_type;		/* ⨯ ���� DS1990A */
	uint32_t crc32;		/* ����஫쭠� �㬬� ����� ����� � ���������� */
} __attribute__((__packed__));

/* ���� ����ᥩ �� ����஫쭮� ���� */
enum {
	PLRT_NORMAL = 0,	/* ���筠� ������ (����� ��� �뢮�� �� �����) */
	PLRT_ERROR,		/* ᮮ�饭�� �� �訡�� */
};

/* ����� ⥪�饩 ����� ����஫쭮� ����� */
/* NB: ����� ���� �ᯮ������ ��� ��� �⥭��, ⠪ � ��� ����� */
extern uint8_t plog_data[LOG_BUF_LEN];
/* ����� ������ */
extern uint32_t plog_data_len;
/* ������ ⥪�饣� ��ࠡ��뢠����� ���� � log_data */
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

#if defined __cplusplus
}
#endif

#endif		/* LOG_POS_H */
