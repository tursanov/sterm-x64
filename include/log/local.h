/*
 * �㭪樨 ��� ࠡ��� � ����஫쭮� ���⮩ ��� (���).
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

/* ��������� ��� */
struct llog_header {
	uint32_t tag;		/* �ਧ��� ��������� */
#define	LLOG_TAG	0x434f4c3e	/* >LOC */
	uint32_t len;		/* ����� ��� ��� ��� ��������� */
	uint32_t n_recs;	/* �᫮ ����ᥩ */
	uint32_t head;		/* ᬥ饭�� ��ࢮ� ����� */
	uint32_t tail;		/* ᬥ饭�� ��᫥���� ����� */
	uint32_t cur_number;	/* ����� ⥪�饩 ����� */
} __attribute__((__packed__));

/* ��������� ����� ��� */
struct llog_rec_header {
	uint32_t tag;		/* �ਧ��� ��������� ����� */
#define LLOG_REC_TAG	0x4345524c	/* LREC */
	uint32_t number;		/* ����� ����� */
	uint32_t n_para;		/* ����� ����� � ����� */
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
	uint16_t ZNtz;		/* ���ଠ�� ᥠ�ᮢ��� �஢�� */
	uint16_t ZNpo;
	uint8_t ZBp;
	uint16_t ONtz;
	uint16_t ONpo;
	uint8_t OBp;
	uint8_t reaction_time;	/* �६� ॠ�樨 */
	bool printed;		/* ����� �ᯥ�⠭ �� ��ࠡ�⪥ �⢥� */
	uint32_t crc32;		/* ����஫쭠� �㬬� ����� ����� � ���������� */
} __attribute__((__packed__));

/* ���� ����ᥩ �� ����஫쭮� ���� �ਣ�த���� �ਫ������ */
enum {
	LLRT_NORMAL = 0,	/* ���筠� ������ (����� ��� �뢮�� �� ���) */
	LLRT_NOTIFY,		/* � �⢥� ��� ����楢 ��� ���� */
	LLRT_INIT,		/* ���樠������ */
	LLRT_BANK,		/* ����� ��� ��� */
	LLRT_ERROR,		/* �訡�� ࠡ��� � ��� */
	LLRT_RD_ERROR,		/* �訡�� �⥭�� ����� ������ */
	LLRT_PR_ERROR_BCODE,	/* �訡�� ���� �� ��� � ����஫�� ����-���� */
	LLRT_PR_ERROR,		/* �訡�� ���� �� ��� ��� ����஫� ����-���� */
	LLRT_EXPRESS,		/* ����� ��� ��� */
	LLRT_AUX,		/* ����� ��� ��� */
	LLRT_FOREIGN,		/* ����祭 �㦮� �⢥� */
	LLRT_SPECIAL,		/* �⢥� �� ���-��� � �訡��� ����஫쭮� �㬬� */
	LLRT_ERASE_SD,		/* ���⪠ ����� ����� ��� */
	LLRT_SD_ERROR,		/* �訡�� ࠡ��� � ���⮩ ����� */
	LLRT_REJECT,		/* �⪠� �� ������ */
	LLRT_SYNTAX_ERROR,	/* �訡�� ᨭ⠪�� �⢥� */
};

/* ���� �訡�� LLRT_ERROR */
enum {
	LLRT_ERROR_TIMEOUT = 1,	/* ⠩���� */
	LLRT_ERROR_MEDIA,	/* ������ ⨯ ���⥫� */
	LLRT_ERROR_GENERIC,	/* ���� �訡�� */
};

/* ����� ⥪�饩 ����� ����஫쭮� ����� */
/* NB: ����� ���� �ᯮ������ ��� ��� �⥭��, ⠪ � ��� ����� */
extern uint8_t llog_data[LOG_BUF_LEN];
/* ����� ������ */
extern uint32_t llog_data_len;
/* ������ ⥪�饣� ��ࠡ��뢠����� ���� � log_data */
extern uint32_t llog_data_index;

/* ����祭�� ���ᠭ�� ���� �訡�� LLRT_ERROR �� �� ���� */
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
