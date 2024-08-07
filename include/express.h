/* �ਪ������ ��⮪�� "������". (c) gsr 2000-2004, 2009, 2018, 2020, 2024 */

#if !defined EXPRESS_H
#define EXPRESS_H

#if defined __cplusplus
extern "C" {
#endif

#include "genfunc.h"

/* �ਧ��� ࠡ��� �ନ���� �१ ��⥣��� */
extern bool use_integrator;

/* ���� ����� */
enum {
	req_regular,		/* ����� ����� */
	req_pos_cheque,		/* ����� 祪� ��� */
	req_grid_xprn,		/* ����� ࠧ��⪨ ������ ��� ��� */
	req_grid_kkt,		/* ����� ࠧ��⪨ ������ ��� ��� */
	req_icon_xprn,		/* ����� ���⮣ࠬ�� ��� ��� */
	req_icon_kkt,		/* ����� ���⮣ࠬ�� ��� ��� */
	req_patterns,		/* ����� 蠡���� ���� ��� */
	req_xslt,		/* ����� ⠡���� �࠭��ଠ樨 XML */
};

/* ��� ⥪�饣� ����� */
extern int req_type;

/* ������� "������" */
#define X_DLE		0x1b	/* Esc ��䨪� ������� */
#define X_WRITE		0x31	/* 1 ������ */
#define X_READ_ALL	0x32	/* 2 ���� ����� ���� */
#define X_CLEAR		0x35	/* 5 ��࠭��/������ */
#define X_READ		0x36	/* 6 ���� ���� */
#define X_DELAY		0x42	/* B ��㧠 */
#define X_RD_PROM	0x43	/* C �⥭�� �� ��� */
#define X_PARA_END_N	0x44	/* D ����� �����; ��३� � ᫥���饬� */
#define X_80_20		0x46	/* F ��३� � ०�� 80x20 */
#define X_RD_ROM	0x48	/* H �⥭�� �� ��� ����⠭� (���) */
#define X_SCR		0x49	/* I ����� ��� �⮡ࠦ���� �� ��࠭� */
#define X_KEY		0x4b	/* K ����� ��� ��� ���祩 */
#define X_XPRN		0x4d	/* M ����� ��� ��� */
#define X_ROM		0x4e	/* N ����� ��� ��� ����⠭� (���) */
#define X_QOUT		0x4f	/* O ����� ��� ��� ������ ��� �⮡ࠦ���� �� ��࠭� */
#define X_SPRN		0x50	/* P ����� ��� ��� */
#define X_KPRN		0x51	/* Q ����� ��� ���� �� ��� */
#define X_XML		0x52	/* R ����� � �ଠ� XML */
#define X_REPEAT	0x53	/* S ��⮯���� ᨬ���� */
#define X_REQ		0x55	/* U ����� ����� ��� ��� ������ */
#define X_ATTR		0x56	/* V ��ਡ�� ᨬ���� ��� �뢮�� �� ��࠭ */
#define X_W_PROM	0x57	/* W ������ � ��� */
#define X_APRN		0x58	/* X ����� ��� ��� */
#define X_OUT		0x5a	/* Z ����� ��� ��� ������ */
#define X_PARA_END	0x5c	/* \ ����� ����� */
#define X_PROM_END	0x5e	/* ^ ����� ������ ��� ��� */
#define X_BANK		0x62	/* � ����� ��� ࠡ��� � ��� */
#define X_SWRES		0x66	/* � �६����� ��४��祭�� �ଠ� ��࠭� */
#define X_KKT		0x79	/*   ����� ��� ��� */
#define X_WR_LOG	0x7c	/* � ������ �� �� (��� ��� ���) */

/* ���⠪��᪨� �訡�� �⢥� */
#define E_OK		0x00	/* ��� �訡�� */
#define E_NOETX		0x02	/* ��� ETX */
#define E_REPKEYS	0x04	/* ����ୠ� ������� ����� � ��� ���祩 */
#define E_XPROM		0x05	/* ������ ������� ����� � ��� */
#define E_NOSPROM	0x06	/* ����� ����� � ��� ��� ��砫� ����� */
#define E_NESTEDOUT	0x07	/* �������� ������� �뢮�� �� ���譥� ���ன�⢮ */
#define E_NOEPROM	0x08	/* ��� ���� ����� � ��� */
#define E_DUMMYATTR	0x09	/* ������� �८�ࠧ������ ᨬ����� �⮨� � ����� ��� ��� ����� */
#define E_ILLATTR	0x10	/* ����������� ������� �८�ࠧ������ */
#define E_MISPLACE	0x11	/* ��ᮮ⢥��⢨� ������� ��࠭���� ���譥�� ���ன��� */
#define E_DBLCLEAR	0x12	/* ����ୠ� ������� ���⪨ */
#define E_CLEAR		0x14	/* ������� ���⪨ �� � ��砫� �⢥� */
#define E_CMDINKEYS	0x15	/* ������� ����� ��� ���祩 */
#define E_LCODEINBODY	0x16	/* ������� ࠡ��� � ����-����� ������ ���� ���묨 � ����� */
#define E_NO_LPRN_CMD	0x17	/* ���������� ��ࠬ���� ࠡ��� � ��� ��� ��� */
#define E_NO_LPRN_CUT	0x18	/* ��� ������� ��१�� ��� ��� ��� */
#define E_NODEVICE	0x35	/* ����⪠ �뢮�� �� ���������饥 ���ன�⢮ */
#define E_KEYOVER	0x40	/* ��९������� ��� ���祩 */
#define E_PROMOVER	0x50	/* ��९������� ��� */
#define E_NOPEND	0x70	/* ��� ���� ����� */
#define E_NOKEND	0x71	/* �� ����祭� ������ � ��� ���祩 */
#define E_UNKDEVICE	0x73	/* �� 㪠���� ���譥� ���ன�⢮ */
#define E_KEYID		0x77	/* ����� �����䨪��� ���祩 ����� 10 ᨬ����� */
#define E_KEYDELIM	0x78	/* �訡�� � ࠧ����⥫�� ���祩 */
#define E_KEYEDELIM	0x79	/* �訡�� � ���楢�� ࠧ����⥫�� ���祩 */
#define E_ADDR		0x80	/* ���饭�� � ��� ����⠭� ��� ��� �� ���������饬� ����� */
#define E_ARRUNK	0x81	/* ����������� ������� � ��� ��� ��� ����⠭� */
#define E_BIGPARA	0x82	/* ���誮� ������ ����� */
#define E_SHORT		0x83	/* ����� �ਪ������� ⥪�� ��᫥ ��ࠡ�⪨ ����� ���� ࠢ�� 0 */
#define E_BCODE		0x86	/* �訡�� � ������� ���客��� ���� */
#define E_SWPOS		0x87	/* ����୮� �ᯮ������� ������� ��४��祭�� ࠧ�襭�� */
#define E_SWOP		0x89	/* ������ ���࠭� ������� ��४��祭�� ࠧ�襭�� */
#define E_BANK		0x90	/* ������ �ଠ� ����� ��� ࠡ��� � ��� */
#define E_NO_BANK	0x91	/* ��ନ��� �� ����஥� ��� ࠡ��� � ��� */
#define E_NO_KKT	0x92	/* ��� ��������� */
#define E_KKT_NONFISCAL	0x93	/* ��� ��室���� � ���᪠�쭮� ०��� */
#define E_KKT_XML	0x94	/* �訡�� � XML ��� ��� */
#define E_KKT_ILL_ATTR	0x95	/* ����୮� ���祭�� ��ਡ�� � XML ��� ��� */
#define E_KKT_NO_ATTR	0x96	/* ��������� ��易⥫�� ��ਡ�� ��� ��� */
#define E_LBL_LEN	0x98	/* � ��砫� ����� ��� ��� ������ ����� ������� ����� ������ */
#define E_XML_SHORT			0xa0	/* ���誮� ���⪨� ����� � XML */
#define E_XML_RECODE			0xa1	/* ������ �����䨪��� ��४���஢�� */
#define E_XML_SCR_TRANSFORM_TYPE	0xa2	/* ������ ⨯ �࠭��ଠ樨 ��� ��࠭� */
#define E_XML_PRN_TRANSFORM_TYPE	0xa3	/* ������ ⨯ �࠭��ଠ樨 ��� ���� */
#define E_XML_XSLT_LEN			0xa4	/* ������ �ଠ� ����� ���஥���� ⠡���� �࠭��ଠ樨 */
#define E_XML_NO_SCR_TRANSFORM		0xa5	/* ��������� ���஥���� ⠡��� �࠭��ଠ樨 ��� ��࠭� */
#define E_XML_NO_PRN_TRANSFORM		0xa6	/* ��������� ���஥���� ⠡��� �࠭��ଠ樨 ��� ���� */
#define E_XML_LEN			0xa7	/* ������ �ଠ� ����� XML */
#define E_XML_HDR			0xa8	/* �� ������ ��������� XML */
#define E_XML_PRN_PARSE			0xa9	/* �訡�� ࠧ��� XML ��� ���� */
#define E_XML_PRN_TRANSFORM		0xaa	/* �訡�� �८�ࠧ������ XML ��� ���� */
#define E_XML_SCR_PARSE			0xab	/* �訡�� ࠧ��� XML ��� ��࠭� */
#define E_XML_SCR_TRANSFORM		0xac	/* �訡�� �८�ࠧ������ XML ��� ��࠭� */
#define E_UNKNOWN	0xff	/* �������⭠� �訡�� */

/* ���� ���������⥩ �ନ���� (��।����� �� ��஬ ���� �����䨪���) */
#define TCAP_TERM	0x00	/* �ନ��� */
#define TCAP_XSLT	0x01	/* �����প� �⢥� � �ଠ� XML/XSLT (����� 蠡������) */
#define TCAP_BNK2	0x02	/* ��ࠡ�⪠ ������᪨� ����楢 ������ ⨯� */
#define TCAP_KKT	0x04	/* ��� */
#define TCAP_EX_BCODE	0x08	/* ��� �����ন���� ���७�� ����-���� */
#define TCAP_NO_POS	0x10	/* ��� ��� */
#define TCAP_UNIBLANK	0x20	/* ����� �� 㭨���ᠫ쭮� ������ */
#define TCAP_FPS	0x40	/* ��⥬� ������� ���⥦�� */
#define TCAP_MASK	0x7f

/* ���ன�⢮ ��� �뢮�� ����� �⢥� */
enum {
	dst_none,	/*  0 */
	dst_sys,	/*  1 */
	dst_text,	/*  2 */
	dst_xprn,	/*  3 ��� */
	dst_tprn = dst_xprn,
	dst_aprn,	/*  4 ��� */
	dst_sprn,	/*  5 ��� */
	dst_out,	/*  6 */
	dst_qout,	/*  7 */
	dst_hash,	/*  8 */
	dst_keys,	/*  9 */
	dst_bank,	/* 10 */
	dst_log,	/* 11 */
	dst_kkt,	/* 12 */
	dst_kprn,	/* 13 */
};

/* ���ଠ�� �� ����� �⢥� */
struct para_info{
	int offset;		/* ᬥ饭�� ��砫� ⥪�� ����� � resp_buf */
	int dst;		/* ���ன�⢮ ��� �뢮�� ����� */
	bool can_print;		/* ����� �� ������ ����� ����� */
	bool jump_next;		/* ����� �� ��⮬���᪨ ��३� � ��ࠡ�⪥ ᫥���饣� ����� */
	bool auto_handle;	/* ����� �� ��⮬���᪨ ��ࠡ���� ����� ����� */
	bool handled;		/* ��ࠡ�⠭ �� ����� */
	bool log_handled;	/* ��� ��ࠡ�⪨ ��2X */
	bool jit_req;		/* ����� �� ��ࠢ��� ����� ��᫥ ��ࠡ�⪨ ������� ����� */
	uint8_t delay;		/* ��㧠 �� ��ࠡ�⪥ (��B) */
	int scr_mode;		/* ࠧ�襭�� ��࠭� (��2�) */
	int xml_idx;		/* ������ � ⠡��� XML (-1, �᫨ ����� ��� ����� ��� */
};

/* ���ଠ�� �� ����� ��� ��� */
#define BNK_REQ_ID_LEN		7
#define BNK_TERM_ID_LEN		6
#define BNK_BLANK_NR_LEN	14
#define BNK_AMOUNT_MIN_LEN	1
#define BNK_AMOUNT_MAX_LEN	10
#define BNK_MAX_DOCS		4

/* ���ଠ�� � ���㬥�� � ������ */
struct bank_doc_info {
	char blank_nr[BNK_BLANK_NR_LEN + 1];		/* ����� ���㬥�� */
	uint64_t amount;				/* �㬬� � �������� */
};

/* ���ଠ�� � ������᪮� ����� */
struct bank_data {
	uint32_t req_id;				/* ����� ������ � ��⥬� */
	char term_id[BNK_TERM_ID_LEN + 1];		/* �孮�����᪨� ����� �ନ���� */
	char op;					/* ⨯ ���⥦� (-;+;*) */
	bool ticket;					/* ��/��� */
	char repayment;					/* �ਧ��� ��८�ଫ���� */
	char prev_blank_nr[BNK_BLANK_NR_LEN + 1];	/* ����� �।��饣� ���㬥�� */
	struct bank_doc_info doc_info[BNK_MAX_DOCS];	/* ���ଠ�� � ���㬥��� � ������ */
	size_t nr_docs;					/* ������⢮ ���㬥�⮢ � ������ */
};

/* ������ � ������᪮� ��২�� */
struct bank_info {
	uint32_t req_id;				/* ����� ������ � ��⥬� */
	char term_id[BNK_TERM_ID_LEN + 1];		/* �孮�����᪨� ����� �ନ���� */
	char op;					/* ⨯ ���⥦� (-;+;*) */
	bool ticket;					/* ��/��� */
	char blank_nr[BNK_BLANK_NR_LEN + 1];		/* ����� ���㬥�� */
	char repayment;					/* �ਧ��� ��८�ଫ���� */
	char prev_blank_nr[BNK_BLANK_NR_LEN + 1];	/* ����� �।��饣� ���㬥�� */
	uint64_t amount;				/* �㬬� � �������� */
};

/* ����� ������᪮�� ����� */
extern struct bank_data bd;
/* ���⪠ ������᪮� ���ଠ樨 */
extern void clear_bank_info(void);
/* ����祭�� ���ଠ樨 � ������᪮� ����� */
extern ssize_t get_bank_info(struct bank_info *items, size_t nr_items);

/* ����� ����� �⢥� �� �� �� ��ࠡ�⪥ �⢥� */
extern uint32_t log_para;
/* �������� "�����" �⢥� */
extern int make_resp_map(void);
/* ������� �� ������ ��� ��ࠡ�⠭�� */
extern void mark_all(void);
/* ��᫮ ����ࠡ�⠭��� ����楢 */
extern int n_unhandled(void);
/* ����� �� �뢥�� ����� �� ��࠭ */
extern bool can_show(int dst);
/* ����� �� �뢥�� ����� �� �ਭ�� */
extern bool can_print(int n);
/* ����� �� ��⠭������� �� �������� ����� */
extern bool can_stop(int n);
/* ��ࠡ�⠭ �� �।��騩 ����� */
extern bool prev_handled(void);
/* ��३� � ᫥���饬� ������ */
extern int next_para(int n);
/* ������ ᫥���饣� ����� ��� �뢮�� �� ����� */
extern int next_printable(void);
/* �����頥� true, �᫨ � �ࠢ�� ��� ��ப� ����� �뢮����� ᮮ�饭�� �� �訡�� */
extern bool is_rstatus_error_msg(void);

/* �஢�ઠ ᨭ⠪�� �⢥� */
extern uint8_t *check_syntax(uint8_t *txt, int l, int *ecode);
/* �஢�ઠ �⢥�, ���������� ������, � ⠪�� ��� ���祩 */
extern bool check_raw_resp(void);

/* �������室�� ࠧ��� */
extern int handle_para(int n_para);

/* ����祭�� ���ଠ樨 ������᪮�� ����� �� �६� ��ࠡ�⪨ �⢥� */
extern const struct bank_data *get_bi(void);

/* ����祭�� ������ ����ࠦ���� ��� ��� */
extern bool find_pic_data(int *data, int *req);

/* ��⥣�ਨ ������, ��� ������ �ॡ���� ᨭ�஭����� � "������" */
#define X3_SYNC_NONE			0x00000000	/* ᨭ�஭����� ������ �� �ॡ���� */
#define X3_SYNC_XPRN_GRIDS		0x00000001	/* �ॡ���� ᨭ�஭����� ࠧ��⮪ ������� ��� */
#define X3_SYNC_XPRN_ICONS		0x00000002	/* �ॡ���� ᨭ�஭����� ���⮣ࠬ� ��� */
#define X3_SYNC_KKT_GRIDS		0x00000004	/* �ॡ���� ᨭ�஭����� ࠧ��⮪ ������� ��� */
#define X3_SYNC_KKT_ICONS		0x00000008	/* �ॡ���� ᨭ�஭����� ���⮣ࠬ� ��� */
#define X3_SYNC_KKT_PATTERNS		0x00000010	/* �ॡ���� ᨭ�஭����� 蠡����� ���� ��� */
#define X3_SYNC_XSLT			0x00000020	/* �ॡ���� ᨭ�஭����� ⠡��� XSLT */

/* �஢�ઠ ����室����� ᨭ�஭����� ������ � "������" */
extern uint32_t need_x3_sync(void);

/* ��ࠡ�⪠ ⥪�� �⢥�. �����頥� false, �᫨ ���� ��३� � ��� ������ */
extern bool execute_resp(void);

#if defined __cplusplus
}
#endif

#endif		/* EXPRESS_H */
