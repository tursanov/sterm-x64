/* ����� �ନ���� �� TCP/IP. (c) gsr 2000, 2004 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include "sterm.h"

/* TCP-���� "������-3" */
#define EXPRESS_SERVER_PORT	1001

/* �� ⠩����� � 1/HZ-ᥪ㭤��� ������� */
/* ��������� ���ࢠ� ����� ��। ��ࠢ��� ᫥���饣� ����� */
#define TCP_SND_INTERVAL	100
/* ������� ���樠����樨 */
#define INIT_TIMEOUT		3000
/* ��㧠 ��᫥ ��⠭���� �裡 ��। ���뫪�� ����� �� ���樠������ */
#define INIT_SEND_WAIT		100
/*
 * �᫨ � �祭�� �⮣� �६��� ��᫥ ��ࠢ�� ����� �ந������
 * ࠧ�� ᮥ�������, � ��⠥���, �� �孮�����᪨� ���� �ନ����
 * �⢥࣭�� ��⮬.
 */
#define INIT_REFUSE_WAIT	100
/* ������� ��⠭���� ᮥ������� */
#define TCP_WCONN_TIMEOUT	3000
/* ������� ��।�� */
#define TCP_SND_TIMEOUT		3000
/* ������� �ਥ�� */
#define TCP_RCV_TIMEOUT		3000
/*
 * ��������� ���ࢠ�, ��᫥ ���ண� �������� ���뫪� ����� �� ���ﭨ�
 * "���� �⢥�".
 */
#define TCP_WRESP_SILENT_INTERVAL	2000

/* H-���� */
#define HB_OK		0x60	/* � */
#define HB_INIT		0x7e	/* � */
#define HB_WRESP	0x7d	/* � */
#define HB_ERROR	0x7b	/* � */

struct bsc_snap{
	uint8_t *p;
	int l;
};

/* ����ﭨ� �����쭮�� �஢�� */
enum {
	cs_nc,			/* ��� ᮥ������� */
	cs_ready,		/* ��⮢ � �ਥ��/��।�� */
	cs_hasreq,		/* ���� ����� ��� ��।�� */
	cs_sending,		/* �������� ���⢥ত���� */
	cs_sent,		/* �������� �⢥� */
	cs_selected,		/* ����祭� �롮ઠ */
	cs_wconnect,		/* �������� ᮥ������� �� TCP/IP */
	cs_wsend		/* ��㧠 ��᫥ ᮥ������� ��। ��।�祩 �� ���樠����樨 */
};

extern int c_state;

extern uint8_t resp_buf[RESP_BUF_LEN];
extern int resp_len;
extern int text_offset;
extern int text_len;
extern uint8_t req_buf[REQ_BUF_LEN];
extern int req_len;
/* �६� ��ࠢ�� ��᫥����� ����� */
extern uint32_t req_time;
/* �६� ॠ�樨 �ନ���� */
extern uint32_t reaction_time;

extern bool ifup(char *iface);
extern bool ifdown(char *iface);
extern bool set_ppp_cfg(void);
extern void release_term_socket(void);
extern void open_transport(void);
extern void close_transport(void);
extern void begin_transmit(void);
extern bool process_transport(void);
extern bool can_send_request(void);
extern bool prepare_raw_resp(void);

#endif		/* TRANSPORT_H */
