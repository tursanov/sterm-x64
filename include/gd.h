/*
 *	�஢��� ��࠭�஢����� ���⠢�� �ନ����.
 *	(c) gsr & �.�.������� 2000-2001, 2004
 */

#ifndef GD_H
#define GD_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

/* ���ᨬ��쭮� �᫮ ����⮪ ���樠����樨 �� �訡��� ᥠ�ᮢ��� �஢�� */
#define MAX_INIT_TRIES_SESSION	1
/* ���ᨬ��쭮� �᫮ ����⮪ ���樠����樨 �� �訡��� �࠭ᯮ�⭮�� �஢�� */
#define MAX_INIT_TRIES_TCP	1
/* ���ᨬ��쭮� �᫮ ����⮪ ���樠����樨 �� �訡��� �����쭮� �஢�� (PPP) */
#define MAX_INIT_TRIES_PPP	1

/* ���� �訡�� */
enum {
	gde_noerror,
	gde_session,
	gde_tcp,
	gde_ppp,
};

#define SERR_BASE	1		/* ᥠ�ᮢ� �஢��� */
#define TCPERR_BASE	20		/* TCP/IP */
#define PPPERR_BASE	80		/* PPP */
#define SLAYER_OK	0

/* �訡�� �஢�� ��࠭�஢����� ���⠢�� (ᥠ�ᮢ���) */
enum {
	SERR_COUNTER = SERR_BASE,/* �:01 - �訡�� ���稪�� ��࠭�஢����� ���⠢�� */
	SERR_CHKSUM,	/* �:02 - �訡�� ����஫쭮� �㬬� */
	SERR_FOREIGN,	/* �:03 - "�㦮�" �⢥� */
	SERR_SYNTAX,	/* �:04 - ᨭ⠪��᪠� �訡�� */
	SERR_TIMEOUT,	/* �:05 - ⠩���� ���樠����樨 */
	SERR_LENGTH,	/* �:06 - 䠪��᪠� ����� ������ �� TCP/IP ����� ��������� */
	SERR_STOP,	/* �:07 - ���樠������ �४�骭� ��-�� �訡�� */
	SERR_HANDSHAKE,	/* �:08 - ���樠����樮��� �⢥� �� �ਪ������ ����� */
	SERR_APPONINIT,	/* �:09 - �ਪ������ ⥪�� � �⢥� �� ���樠������ */
	SERR_SPECIAL,	/* �:xx - ᯥ樠��� �⢥� �� ���-��� */
};

/* �訡�� TCP/IP */
enum {
	TCPERR_SOCKET = TCPERR_BASE,	/* �:20 - �訡�� ᮧ����� ᮪�� */
	TCPERR_HOSTUNREACH,	/* �:21 - ��� ������㯥� */
	TCPERR_IPREFUSED,	/* �:22 ip ���� �⢥࣭�� */
	TCPERR_TERMREFUSED,	/* �:23 ���� �ନ���� �⢥࣭�� */
	TCPERR_CONNREFUSED,	/* �:24 ����⪠ ᮥ������� �⢥࣭�� */
	TCPERR_CONN,		/* �:25 �訡�� ᮥ������� */
	TCPERR_SEND,		/* �:26 �訡�� ��।�� */
	TCPERR_WRESPREFUSED,	/* �:27 ᮥ������� ࠧ�ࢠ�� �� �६� �������� �⢥� */
	TCPERR_WRESPCLOSED,	/* �:28 ᮥ������� ������ �� �६� �������� �⢥� */
	TCPERR_WRESP,		/* �:29 ᡮ� �� �६� �������� �⢥� */
	TCPERR_RCVREFUSED,	/* �:30 ᮥ������� ࠧ�ࢠ�� �� �६� �ਥ�� */
	TCPERR_RCVCLOSED,	/* �:31 ᮥ������� ������ �� �६� �ਥ�� */
	TCPERR_RCVTIMEOUT,	/* �:32 ⠩���� �ਥ�� */
	TCPERR_RCV,		/* �:33 ᡮ� �� �६� �ਥ�� */
};

/* �訡�� PPP */
enum {
	PPPERR_STARTING = PPPERR_BASE,	/* �:80 �訡�� ����᪠ PPP */
	PPPERR_INIT,		/* �:81 �訡�� ���樠����樨 ������ */
	PPPERR_DIALING,		/* �:82 �訡�� ������� */
	PPPERR_LOGIN,		/* �:83 �訡�� ���ਧ�樨 */
	PPPERR_IPCP,		/* �:84 �訡�� ���䨣��樨 TCP/IP */
	PPPERR_CARRIER,		/* �:85 �ய������ ����饩 */
};

/* ����� ��䨪� ����� */
#define REQ_PREFIX_LEN		6
/* ����� ���䨪� ����� */
#define REQ_SUFFIX_LEN		91
/* �������쭠� ����� ����� �� TCP/IP */
#define MIN_REQ_LEN		(REQ_PREFIX_LEN + REQ_SUFFIX_LEN)

/* ��થ�� ��࠭�஢����� ���⠢�� */
#define VERSION_INFO_MARK	0x01	/* ���ଠ�� � ���ᨨ */
#define REACTION_INFO_MARK	0x07	/* �६� ॠ�樨 �ନ���� */
#define XGATE_INFO_MARK		0x08	/* ���ଠ�� � � */
#define TERM_INFO_MARK		0x09	/* ���ଠ�� � �ନ���� */
#define SESSION_INFO_MARK	0x11	/* ���ଠ�� � ᥠ�� */
#define PRN_INFO_MARK		0x12	/* ���ଠ�� � �ਭ��� */

/* ����� ��࠭�஢����� ���⠢�� (�����) */
#define GDF_REQ_REJECT		0x01	/* �⪠� �� ������ */
#define GDF_REQ_CRC		0x02	/* �訡�� ����஫쭮� �㬬� �⢥� */
#define GDF_REQ_FIRST		0x04	/* ���� ����� ��᫥ ���樠����樨 */
#define GDF_REQ_SYNTAX		0x08	/* ᨭ⠪��᪠� �訡�� �⢥� */
#define GDF_REQ_FOREIGN		0x10	/* �㦮� �⢥� */
#define GDF_REQ_TYPE		0x60	/* ⨯ ����� */
#define GDF_REQ_INIT		0x20	/* ����� �� ���樠������ */
#define GDF_REQ_APP		0x40	/* �ਪ������ ����� */
/* ����� ��࠭�஢����� ���⠢�� (�⢥�) */
#define GDF_RESP_REJ_ENABLED	0x01	/* ࠧ�襭 �⪠� �� ������ */
#define GDF_RESP_INIT		0x20	/* �⢥� �� ���樠����樨 */
#define GDF_RESP_APP		0x40	/* �ਪ������ �⢥� */

#define SET_FLAG(f, v) (f |= v)
#define CLR_FLAG(f, v) (f &= ~v)
#define TST_FLAG(f, v) (f & v)

/* ����� � ���稪��� ��࠭�஢����� ���⠢�� */
#define INC_VAL(w) ({ w++; w %= 0x1000; w; })
#define DELTA(w1,w2) (int)(w1 - w2 + 0x1000*((w2-w1)/0x800))
#define RANDOM_VAL (uint16_t)rand() % 0x1000

#define TERM_NUMBER_LEN		13
typedef char term_number[TERM_NUMBER_LEN];

#define PRN_NUMBER_LEN		13

/* ����ﭨ� ᥠ�ᮢ��� �஢�� */
enum {
	ss_new,		/* initialize me */
	ss_initializing,/* initiaization in progress */
	ss_winit,	/* waiting for initalization */
	ss_finit,	/* cannot initialize */
	ss_ready,	/* ready to work */
};

/*
 * ���� ��४��祭�� ०���� ࠡ��� �ନ����. ��। ���室�� �� �᭮�����
 * ०��� � �ਣ�த�� � ������� ����室��� �஢��� ���樠������ � ��᫠��
 * � ���-��� 宫��⮩ �����.
 */
extern bool wm_transition;
/* ��४��祭�� ०��� � ����ᮬ ���짮��⥫� */
extern bool wm_transition_interactive;
extern int s_state;
extern int session_error;
extern bool delayed_init;

extern uint32_t init_t0;

/* ���稪� ��࠭�஢����� ���⠢�� */
extern uint16_t ZNtz;
extern uint16_t oldZNtz;
extern uint16_t ZNpo;
extern uint16_t oldZNpo;
extern uint8_t ZBp;
extern uint8_t oldZBp;
extern uint16_t ONpo;
extern uint16_t ONtz;
extern uint8_t OBp;

/*
 * ��᫥���� ��।���� �����. �����।�⢥��� ��। ��� �ᯮ����� ����
 * � ����� ᨭ⠪��᪮� �訡�� �⢥�.
 */
extern char *last_request;

extern void init_gd(void);
extern void release_gd(void);
extern void reset_gd(bool need_init);
extern uint16_t x3_crc(uint8_t *p, int l);
extern int get_req_offset(void);
extern int get_max_req_len(int offset);
extern bool wrap_text(void);
extern void reject(void);
extern void slayer_error(int e);
extern void begin_initialization(void);
extern void wait_initialization(void);
extern bool check_min_resp_len(void);
extern bool check_crc(void);
extern bool check_gd_rules(void);
extern void set_text_len(void);

#if defined __cplusplus
}
#endif

#endif		/* GD_H */
