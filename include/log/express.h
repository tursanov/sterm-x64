/* �㭪樨 ��� ࠡ��� � 横���᪮� ����஫쭮� ���⮩. (c) gsr 2004, 2008, 2018-2019. */

#if !defined LOG_EXPRESS_H
#define LOG_EXPRESS_H

#if defined __cplusplus
extern "C" {
#endif

#include <time.h>
#include "log/generic.h"
#include "blimits.h"
#include "cfg.h"
#include "ds1990a.h"
#include "gd.h"
#include "genfunc.h"

/* ��������� 横���᪮� ����஫쭮� ����� */
struct xlog_header {
	uint32_t tag;		/* �ਧ��� ��������� */
#define	XLOG_TAG	0x474f4c3e	/* >LOG */
	uint32_t len;		/* ����� ��� ��� ��� ��������� */
	uint32_t n_recs;	/* �᫮ ����ᥩ */
	uint32_t head;		/* ᬥ饭�� ��ࢮ� ����� */
	uint32_t tail;		/* ᬥ饭�� ��᫥���� ����� */
	uint32_t cur_number;	/* ����� ⥪�饩 ����� */
	uint32_t printed;	/* 䫠� ���� ��� (��� ᮢ���⨬��� � �।��騬� ����ﬨ) */
} __attribute__((__packed__));

/* ��������� ����� ����஫쭮� ����� */
struct xlog_rec_header {
	uint32_t tag;		/* �ਧ��� ��������� ����� */
#define XLOG_REC_TAG	0x4345523e	/* >REC */
	uint32_t number;	/* ����� ����� */
	uint32_t n_para;	/* ����� ����� � ����� */
	uint32_t type;		/* ⨯ ����� */
	uint16_t code;		/* ��� ����� (��室�� �� ���-���) */
	uint32_t len;		/* ����� ����� ��� ��� ��������� */
	struct date_time dt;	/* ��� � �६� (�� ��᪢�) ᮧ����� ����� */
	struct term_addr addr;	/* ���� �ନ���� � ������ ᮧ����� ����� */
	uint32_t term_version;	/* ����� �� �ନ����, ᤥ���襣� ������ */
	uint16_t term_check_sum;/* ����஫쭠� �㬬� �ନ����, ᤥ���襣� ������ */
	term_number tn;		/* �����᪮� ����� �ନ����, ᤥ���襣� ������ */
	uint8_t xprn_number[PRN_NUMBER_LEN];	/* �����᪮� ����� ��� */
	uint8_t aprn_number[PRN_NUMBER_LEN];	/* �����᪮� ����� ��� */
	ds_number dsn;		/* ����� ���� DS1990A � ������ ᮧ����� ����� */
	int ds_type;		/* ⨯ ���� DS1990A */
	uint16_t ZNtz;		/* ���ଠ�� ᥠ�ᮢ��� �஢�� */
	uint16_t ZNpo;
	uint8_t ZBp;
	uint16_t ONtz;
	uint16_t ONpo;
	uint8_t OBp;
	uint8_t reaction_time;	/* �६� ॠ�樨 */
	uint32_t flags;		/* ࠧ���� 䫠�� */
#define XLOG_REC_PRINTED	0x00000001	/* ����� �ᯥ�⠭ �� ��ࠡ�⪥ �⢥� */
#define XLOG_REC_APC		0x00000002	/* ��⮬���᪠� ����� 祪� �� ��� (���) */
#define XLOG_REC_CPC		0x00000004	/* � ��২�� �� ���� ����� ��� ���� 祪�� (�����) */
	uint32_t crc32;		/* ����஫쭠� �㬬� ����� ����� � ���������� */
} __attribute__((__packed__));

/* ���� ����ᥩ �� 横���᪮� ����஫쭮� ���� */
enum {
	XLRT_NORMAL = 0,	/* ���筠� ������ (����� ��� �뢮�� �� �����) */
	XLRT_NOTIFY = 0x1001,	/* �����뢥���, �᫨ � �⢥� ��� ����楢 ��� ���� */
	XLRT_INIT,		/* �����뢠���� ��᫥ ��室� �⢥� �� ���樠������ */
	XLRT_FOREIGN,		/* ����祭 �㦮� �⢥� */
	XLRT_AUX,		/* ����� ��� �������⥫쭮�� �ਭ�� */
	XLRT_SPECIAL,		/* �⢥� �� ���-��� � �訡��� ����஫쭮� �㬬� */
	XLRT_BANK,		/* ����� ��� ��� */
	XLRT_IPCHANGE,		/* ᬥ�� ip-���� ���-��� �� ����ୠ⨢�� */
	XLRT_REJECT,		/* �⪠� �� ������ */
	XLRT_KKT,		/* ����� ��� ��� */
};

/* ����� ⥪�饩 ����� ����஫쭮� ����� */
/* NB: ����� ���� �ᯮ������ ��� ��� �⥭��, ⠪ � ��� ����� */
extern uint8_t xlog_data[LOG_BUF_LEN];
/* ����� ������ */
extern uint32_t xlog_data_len;
/* ������ ⥪�饣� ��ࠡ��뢠����� ���� � log_data */
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

#if defined __cplusplus
}
#endif

#endif		/* LOG_EXPRESS_H */
