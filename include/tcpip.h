/* Работа терминала по TCP/IP. (c) gsr 2000, 2004 */

#ifndef TCPIP_H
#define TCPIP_H

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include "sterm.h"

#define EXPRESS_SERVER_PORT	1001

/* Все таймауты в сотых долях секунды */
/* Таймаут соединения */
#define TCP_CONN_TIMEOUT	3000
/*
 * Таймаут после соединения перед началом передачи при инициализации
 * (время, в течение которого ip-адрес терминала может быть отвергнут хост-ЭВМ.
 */
#define TCP_INIT_SEND_WAIT	100
/*
 * Время после посылки запроса на инициализацию, в течение которого
 * технологический адрес терминала может быть отвергнут хост-ЭВМ.
 */
#define TCP_REFUSE_TIMEOUT	100
#define TCP_SND_TIMEOUT		3000	/* таймаут передачи */
#define TCP_RCV_TIMEOUT		3000	/* таймаут приема */

extern bool	ifup(char *iface);
extern bool	ifdown(char *iface);
extern bool	set_ppp_cfg(void);
extern bool	init_tcpip(void);
extern void	release_tcpip(void);
extern void	process_tcpip(void);
extern bool	release_term_socket(void);

#endif		/* TCPIP_H */
