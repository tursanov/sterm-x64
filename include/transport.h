/* Работа терминала по TCP/IP. (c) gsr 2000, 2004 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include "sterm.h"

/* TCP-порт "Экспресс-3" */
#define EXPRESS_SERVER_PORT	1001

/* Все таймауты в 1/HZ-секундных единицах */
/* Минимальный интервал между перед отправкой следующего запроса */
#define TCP_SND_INTERVAL	100
/* Таймаут инициализации */
#define INIT_TIMEOUT		3000
/* Пауза после установки связи перед посылкой запроса на инициализацию */
#define INIT_SEND_WAIT		100
/*
 * Если в течение этого времени после отправки запроса произойдет
 * разрыв соединения, то считается, что технологический адрес терминала
 * отвергнут хостом.
 */
#define INIT_REFUSE_WAIT	100
/* Таймаут установки соединения */
#define TCP_WCONN_TIMEOUT	3000
/* Таймаут передачи */
#define TCP_SND_TIMEOUT		3000
/* Таймаут приема */
#define TCP_RCV_TIMEOUT		3000
/*
 * Минимальный интервал, после которого возможна посылка запроса из состояния
 * "Ждите ответа".
 */
#define TCP_WRESP_SILENT_INTERVAL	2000

/* H-байт */
#define HB_OK		0x60	/* Ю */
#define HB_INIT		0x7e	/* Ч */
#define HB_WRESP	0x7d	/* Щ */
#define HB_ERROR	0x7b	/* Ш */

struct bsc_snap{
	uint8_t *p;
	int l;
};

/* Состояния канального уровня */
enum {
	cs_nc,			/* нет соединения */
	cs_ready,		/* готов к приему/передаче */
	cs_hasreq,		/* есть запрос для передачи */
	cs_sending,		/* ожидание подтверждения */
	cs_sent,		/* ожидание ответа */
	cs_selected,		/* получена выборка */
	cs_wconnect,		/* ожидание соединения по TCP/IP */
	cs_wsend		/* пауза после соединения перед передачей при инициализации */
};

extern int c_state;

extern uint8_t resp_buf[RESP_BUF_LEN];
extern int resp_len;
extern int text_offset;
extern int text_len;
extern uint8_t req_buf[REQ_BUF_LEN];
extern int req_len;
/* Время отправки последнего запроса */
extern uint32_t req_time;
/* Время реакции терминала */
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
