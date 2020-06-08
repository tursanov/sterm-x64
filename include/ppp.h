/*
 * Работа с PPP. (c) gsr 2003.
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
 * Таймауты при работе с демоном ppp (сек).
 * Предполагается, что в каждый момент времени запущено не более одного
 * экземпляра демона.
 */
/* Таймаут запуска pppd */
#define PPP_TIMEOUT_START	3
/* Таймаут остановки pppd */
#define PPP_TIMEOUT_STOP	3
/* Таймаут инициализации модема */
#define PPP_TIMEOUT_INIT	3
/* Таймаут дозвона по коммутируемой линии */
#define PPP_TIMEOUT_DIAL	120
/* Таймаут авторизации (не включает PAP/CHAP) */
#define PPP_TIMEOUT_LOGIN	10
/* Таймаут конфигурации TCP/IP с помощью IPCP (включает PAP/CHAP) */
#define PPP_TIMEOUT_IPCP	10

/* Параметры для создания ключа очереди сообщений с помощью ftok */
#define PPP_IPC_FILE		"/etc/inittab"
#define PPP_IPC_ID		0	/* см. chat.c */
/* Тип сообщений pppd (см. chat.c и ipcp.c) */
#define PPP_MSG_TYPE		1000
#define IPCP_MSG_TYPE		1001
/* Максимальный размер текста IPC сообщений от pppd (см. chat.c) */
#define PPP_MSG_MAX_SIZE	256
/* Строки, посылаемые в очередь сообщений для индикации состояния сеанса PPP */
#define PPP_STR_INIT		"INIT"
#define PPP_STR_DIALING		"DIALING"
#define PPP_STR_LOGIN		"LOGIN"
#define PPP_STR_IPCP		"IPCP"
#define PPP_STR_LEASED		"LEASED"

/* Состояние демона pppd */
enum {
	ppp_none,
	ppp_nc,			/* демон не запущен */
	ppp_starting,		/* запуск */
	ppp_stopping,		/* остановка */
	ppp_init,		/* посылка строки инициализации в модем */
	ppp_dialing,		/* дозвон */
	ppp_login,		/* авторизация */
	ppp_ipcp,		/* получение конфигурации TCP/IP */
	ppp_ready,		/* соединение установлено */
};

extern int ppp_state;

extern bool init_ppp_ipc(void);
extern void release_ppp_ipc(void);
extern bool ppp_connect(void);
extern bool ppp_hangup(void);
extern void ppp_process(void);

#endif		/* PPP_H */
