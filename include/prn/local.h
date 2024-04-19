/* ����� � �ਣ�த�� �����騬 ���ன�⢮� (���). (c) gsr 2009 */

#if !defined PRN_LOCAL_H
#define PRN_LOCAL_H

#if defined __cplusplus
extern "C" {
#endif

#include "prn/express.h"
#include "cfg.h"
#include "gd.h"

/* ����� ��� */
extern uint8_t lprn_status;
/* ����� SD-����� ����� */
extern uint8_t lprn_sd_status;
/* ����� �����᪮�� ����� ��� */
#define LPRN_NUMBER_LEN		14
/* �����᪮� ����� ��� */
extern uint8_t lprn_number[LPRN_NUMBER_LEN];
/* ��� ���⥫� � ��� */
extern uint8_t lprn_media;
/* ����� ����-���� �� ��� */
#define LPRN_BLANK_NUMBER_LEN	13
/* ����� ���㬥�� � ��� */
extern uint8_t lprn_blank_number[LPRN_BLANK_NUMBER_LEN];
/* �� ��� ����祭 ����� ������ */
extern bool lprn_has_blank_number;
/* ���� �ॢ�襭�� ⠩���� �� �믮������ ����樨 */
extern bool lprn_timeout;

/* ������� ��� */

/* �������⮢� ������� */
#define LPRN_NUL	XPRN_NUL	/* ����� ������� */
#define LPRN_STX	0x02		/* ��砫� ⥪�� �� */
#define LPRN_ETX	0x03		/* ����� ⥪�� �� */
#define LPRN_WIDE_FNT	XPRN_WIDE_FNT	/* �ப�� ���� */
#define LPRN_LF		XPRN_LF		/* ��ॢ�� ��ப� */
#define LPRN_NORM_FNT	XPRN_NORM_FNT	/* ��ଠ��� ���� */
#define LPRN_FORM_FEED	XPRN_FORM_FEED	/* ��१�� � ���ᮬ ������ */
#define LPRN_CR		XPRN_CR		/* ������ ���⪨ */
#define LPRN_DLE1	XPRN_DLE1	/* ��砫� �⢥� �� ��� */
#define LPRN_CPI15	0x19		/* ���⭮��� ���� 15 cpi */
#define LPRN_DLE	XPRN_DLE	/* ��砫� ������� ��� ��� */
#define LPRN_CPI12	0x1c		/* ���⭮��� ���� 12 cpi */
#define LPRN_DLE2	0x1d		/* ��⠭���� ��ࠬ��஢ ���� */
#define LPRN_CPI20	0x1e		/* ���⭮��� ���� 20 cpi */
#define LPRN_FORM_FEED1	0x1f		/* ����� �� ��� ��� ��१�� */
#define LPRN_RST	XPRN_RST	/* ��� ��� */

/* ��������⮢� ������� */
#define LPRN_BCODE	0x01		/* ����� ����� ������ */
#define LPRN_MEDIA	0x04		/* ����� ⨯� ���⥫� */
#define LPRN_INIT	0x06		/* ���樠������ */
#define LPRN_SNAPSHOTS	0x07		/* ����� ��ࠧ�� �⯥�⠭��� ������� */
#define LPRN_ERASE_SD	0x08		/* ���⪠ SD-����� */
#define LPRN_AUXLNG	XPRN_AUXLNG	/* ���室 �� �������⥫��� ���� ᨬ����� */
#define LPRN_MAINLNG	XPRN_MAINLNG	/* ���室 �� �᭮���� ���� ᨬ����� */
#define LPRN_STATUS	0x12		/* ����� ���ﭨ� ��� */
#define LPRN_WR_BCODE2	0x1a		/* ����ᥭ�� ����-���� ��������� ⨯� */
#define LPRN_WR_BCODE1	XPRN_WR_BCODE	/* ����ᥭ�� ����-���� #1 (� ⥪�� �����) */
#define LPRN_POS	XPRN_PRNOP	/* ���⨪��쭮� ��� ��ਧ��⠫쭮� ����樮��஢���� */
#define LPRN_LOG1	0x60		/* ����� �� ����஫쭮� ���� ����� ��2 � */
#define LPRN_INTERLINE	0x69		/* ��⠭���� ������筮�� ���ࢠ�� */
#define LPRN_LOG	0x6c		/* ��砫� ���� �� */
#define LPRN_RD_BCODE	XPRN_RD_BCODE	/* ���뢠��� ����-���� */
#define LPRN_NO_BCODE	0x7b		/* ����� ⥪�� ��� ����஫� ����-���� */

/* ����砭�� ������� LPRN_PRNOP */
#define LPRN_PRNOP_HPOS_ABS	XPRN_PRNOP_HPOS_ABS	/* ��᮫�⭮� ��ਧ��⠫쭮� ����樮��஢���� */
#define LPRN_PRNOP_VPOS_ABS	XPRN_PRNOP_VPOS_ABS	/* ��᮫�⭮� ���⨪��쭮� ����樮��஢���� */

/* �������� ��� ࠧ����� ������ (� ���� ����� ᥪ㭤�) */
#define LPRN_BCODE_TIMEOUT	1600	/* ����� ����� ������ (12 ᥪ) */
#define LPRN_MEDIA_TIMEOUT	400	/* ����� ⨯� ���⥫� (4 ᥪ) */
#define LPRN_INIT_TIMEOUT	250	/* ���樠������ ��� (2.5 ᥪ) */
#define LPRN_STATUS_TIMEOUT	1200	/* ����� ����� (12 ᥪ) */
#define LPRN_LOG_TIMEOUT	1200	/* ����� �� (12 ᥪ �� �����) */
#define LPRN_BCODE_CTL_TIMEOUT	2200	/* ����� ����� � ����஫�� ����-���� (22 ᥪ) */
#define LPRN_TEXT_TIMEOUT	1200	/* ����� ⥪�� ��� ����஫� ����-���� (12 ᥪ) */
#define LPRN_SNAPSHOTS_TIMEOUT	60000	/* ����� ��ࠧ�� ������� (10 ���) */
#define LPRN_ERASE_SD_TIMEOUT	6000	/* ���⪠ SD-����� (60 ᥪ) */
#define LPRN_RD_PARAMS_TIMEOUT	500	/* ����祭�� ��ࠬ��஢ ��� (5 ᥪ) */
#define LPRN_WR_PARAM_TIMEOUT	300	/* ��⠭���� ��ࠬ��� ��� (3 ᥪ) */
#define LPRN_TIME_SYNC_TIMEOUT	300	/* ᨭ�஭����� �६��� (3 ᥪ) */

#define LPRN_INFINITE_TIMEOUT	0	/* ��᪮���� ⠩���� */

/* ��� ���⥫� � ��� */
#define LPRN_MEDIA_UNKNOWN	0x00	/* ��������� ⨯ */
#define LPRN_MEDIA_BLANK	0x30	/* ��� */
#define LPRN_MEDIA_PAPER	0x31	/* �㫮���� �㬠�� */
#define LPRN_MEDIA_BOTH		0x32	/* ��� ���⥫� */

/* ���� �����襭�� �㭪権 ��� ࠡ��� � ��� */
enum {
	LPRN_RET_OK,		/* ������ �믮����� ��� �訡�� */
	LPRN_RET_ERR,		/* ������ �믮����� � �訡��� */
	LPRN_RET_RST,		/* �� �믮������ ����樨 �ந���� ��� �ନ���� */
};

/* ���樠������ ��� � ����祭�� ��� �����᪮�� ����� */
extern int lprn_init(void);
/* �����頥� true, �᫨ �����᪮� ����� ��� ��⮨� �� ����� �㫥� */
extern bool lprn_is_zero_number(void);
/* ����祭�� ����� ��� */
extern int lprn_get_status(void);
/* ����祭�� ⨯� ���⥫� � ��� */
extern int lprn_get_media_type(void);
/* ����祭�� ����� ���㬥�� � ��� */
extern int lprn_get_blank_number(void);
/* ����� ��࠭��� ����ࠦ���� */
extern int lprn_snapshots(void);
/* ���⪠ SD-����� */
extern int lprn_erase_sd(void);
/* ����� ����� ����஫쭮� ����� */
extern int lprn_print_log(const uint8_t *data, size_t len);
/* ����� ����� ����஫쭮� �����, ��襤襣� �� "������" */
extern int lprn_print_log1(const uint8_t *data, size_t len);
/* ����� ⥪�� �� ��� */
extern int lprn_print_ticket(const uint8_t *data, size_t len, bool *sent_to_prn);
/* ����� ��ࠬ��஢ ࠡ��� ��� */
extern int lprn_get_params(struct term_cfg *cfg);
/* ������ ��ࠬ��஢ ࠡ��� ��� */
extern int lprn_set_params(struct term_cfg *cfg);
/* ����஭����� �६��� ��� � �ନ����� */
extern int lprn_sync_time(void);
/* �����⨥ ���ன�⢠ ��� ࠡ��� � ��� */
extern void lprn_close(void);

/* �����஢�� �訡�� ��� */
struct lprn_error_txt {
	int len;		/* ����� ᮮ�饭�� */
	char *txt;		/* ᮮ�饭�� �� �訡�� (koi7) */
};

/* ����祭�� ���ଠ樨 �� �訡�� ��� �� �᭮����� �� ���� */
extern struct lprn_error_txt *lprn_get_error_txt(uint8_t code);
/* ����祭�� ���ଠ樨 �� �訡�� ����� ����� �� �᭮����� �� ���� */
extern struct lprn_error_txt *lprn_get_sd_error_txt(uint8_t code);

#if defined __LOG_LPRN__
extern bool lprn_create_log(void);
#endif

#if defined __cplusplus
}
#endif

#endif		/* PRN_LOCAL_H */
