/* ����� �ନ���� �� TCP/IP. (c) gsr 2000, 2004 */

#ifndef TCPIP_H
#define TCPIP_H

#if defined __cplusplus
extern "C" {
#endif

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include "sterm.h"

#define EXPRESS_SERVER_PORT	1001

/* �� ⠩����� � ���� ����� ᥪ㭤� */
/* ������� ᮥ������� */
#define TCP_CONN_TIMEOUT	3000
/*
 * ������� ��᫥ ᮥ������� ��। ��砫�� ��।�� �� ���樠����樨
 * (�६�, � �祭�� ���ண� ip-���� �ନ���� ����� ���� �⢥࣭�� ���-���.
 */
#define TCP_INIT_SEND_WAIT	100
/*
 * �६� ��᫥ ���뫪� ����� �� ���樠������, � �祭�� ���ண�
 * �孮�����᪨� ���� �ନ���� ����� ���� �⢥࣭�� ���-���.
 */
#define TCP_REFUSE_TIMEOUT	100
#define TCP_SND_TIMEOUT		3000	/* ⠩���� ��।�� */
#define TCP_RCV_TIMEOUT		3000	/* ⠩���� �ਥ�� */

extern bool	ifup(char *iface);
extern bool	ifdown(char *iface);
extern bool	set_ppp_cfg(void);
extern bool	init_tcpip(void);
extern void	release_tcpip(void);
extern void	process_tcpip(void);
extern bool	release_term_socket(void);

#if defined __cplusplus
}
#endif

#endif		/* TCPIP_H */
