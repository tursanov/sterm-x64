/*
 * ����� � PPP. (c) gsr 2003.
 */

#ifndef PPP_H
#define PPP_H

#include "gui/options.h"
#include "transport.h"

extern char *make_pppd_string(void);

#define PPP_DAEMON		"/usr/sbin/pppd"
#define PPP_CHAT		"/usr/sbin/chat"
#define PPP_CFG_DIR		"/etc/ppp/"
#define PPP_IP_UP_SCRIPT	PPP_CFG_DIR "ip-up"
#define PPP_IP_DOWN_SCRIPT	PPP_CFG_DIR "ip-down"
#define PPP_PAP_FILE		PPP_CFG_DIR "pap-secrets"
#define PPP_CHAP_FILE		PPP_CFG_DIR "chap-secrets"
#define PPP_CHAT_FILE		PPP_CFG_DIR "chat.script"

/*
 * �������� �� ࠡ�� � ������� ppp (ᥪ).
 * �।����������, �� � ����� ������ �६��� ����饭� �� ����� ������
 * ������� ������.
 */
/* ������� ����᪠ pppd */
#define PPP_TIMEOUT_START	3
/* ������� ��⠭���� pppd */
#define PPP_TIMEOUT_STOP	3
/* ������� ���樠����樨 ������ */
#define PPP_TIMEOUT_INIT	3
/* ������� ������� �� �������㥬�� ����� */
#define PPP_TIMEOUT_DIAL	120
/* ������� ���ਧ�樨 (�� ����砥� PAP/CHAP) */
#define PPP_TIMEOUT_LOGIN	10
/* ������� ���䨣��樨 TCP/IP � ������� IPCP (����砥� PAP/CHAP) */
#define PPP_TIMEOUT_IPCP	10

/* ��ࠬ���� ��� ᮧ����� ���� ��।� ᮮ�饭�� � ������� ftok */
#define PPP_IPC_FILE		"/etc/inittab"
#define PPP_IPC_ID		0	/* �. chat.c */
/* ��� ᮮ�饭�� pppd (�. chat.c � ipcp.c) */
#define PPP_MSG_TYPE		1000
#define IPCP_MSG_TYPE		1001
/* ���ᨬ���� ࠧ��� ⥪�� IPC ᮮ�饭�� �� pppd (�. chat.c) */
#define PPP_MSG_MAX_SIZE	256
/* ��ப�, ���뫠��� � ��।� ᮮ�饭�� ��� ������樨 ���ﭨ� ᥠ�� PPP */
#define PPP_STR_INIT		"INIT"
#define PPP_STR_DIALING		"DIALING"
#define PPP_STR_LOGIN		"LOGIN"
#define PPP_STR_IPCP		"IPCP"
#define PPP_STR_LEASED		"LEASED"

/* ����ﭨ� ������ pppd */
enum {
	ppp_none,
	ppp_nc,			/* ����� �� ����饭 */
	ppp_starting,		/* ����� */
	ppp_stopping,		/* ��⠭���� */
	ppp_init,		/* ���뫪� ��ப� ���樠����樨 � ����� */
	ppp_dialing,		/* ������ */
	ppp_login,		/* ���ਧ��� */
	ppp_ipcp,		/* ����祭�� ���䨣��樨 TCP/IP */
	ppp_ready,		/* ᮥ������� ��⠭������ */
};

extern int ppp_state;

extern bool init_ppp_ipc(void);
extern void release_ppp_ipc(void);
extern bool ppp_connect(void);
extern bool ppp_hangup(void);
extern void ppp_process(void);

#endif		/* PPP_H */
