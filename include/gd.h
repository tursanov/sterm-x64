/*
 *	Уровень гарантированной доставки терминала.
 *	(c) gsr & А.Б.Коников 2000-2001, 2004
 */

#ifndef GD_H
#define GD_H

#include <sysdefs.h>

/* Максимальное число попыток инициализации при ошибках сеансового уровня */
#define MAX_INIT_TRIES_SESSION	1
/* Максимальное число попыток инициализации при ошибках транспортного уровня */
#define MAX_INIT_TRIES_TCP	1
/* Максимальное число попыток инициализации при ошибках канальног уровня (PPP) */
#define MAX_INIT_TRIES_PPP	1

/* Типы ошибок */
enum {
	gde_noerror,
	gde_session,
	gde_tcp,
	gde_ppp,
};

#define SERR_BASE	1		/* сеансовый уровень */
#define TCPERR_BASE	20		/* TCP/IP */
#define PPPERR_BASE	80		/* PPP */
#define SLAYER_OK	0

/* Ошибки уровня гарантированной доставки (сеансового) */
enum {
	SERR_COUNTER = SERR_BASE,/* С:01 - ошибка счетчиков гарантированной доставки */
	SERR_CHKSUM,	/* С:02 - ошибка контрольной суммы */
	SERR_FOREIGN,	/* С:03 - "чужой" ответ */
	SERR_SYNTAX,	/* С:04 - синтаксическая ошибка */
	SERR_TIMEOUT,	/* С:05 - таймаут инициализации */
	SERR_LENGTH,	/* С:06 - фактическая длина данных по TCP/IP больше ожидаемой */
	SERR_STOP,	/* С:07 - инициализация прекращкна из-за ошибки */
	SERR_HANDSHAKE,	/* С:08 - инициализационный ответ на прикладной запрос */
	SERR_APPONINIT,	/* С:09 - прикладной текст в ответ на инициализацию */
	SERR_SPECIAL,	/* С:xx - специальный ответ от хост-ЭВМ */
};

/* Ошибки TCP/IP */
enum {
	TCPERR_SOCKET = TCPERR_BASE,	/* Т:20 - ошибка создания сокета */
	TCPERR_HOSTUNREACH,	/* Т:21 - хост недоступен */
	TCPERR_IPREFUSED,	/* Т:22 ip адрес отвергнут */
	TCPERR_TERMREFUSED,	/* Т:23 адрес терминала отвергнут */
	TCPERR_CONNREFUSED,	/* Т:24 попытка соединения отвергнута */
	TCPERR_CONN,		/* Т:25 ошибка соединения */
	TCPERR_SEND,		/* Т:26 ошибка передачи */
	TCPERR_WRESPREFUSED,	/* Т:27 соединение разорвано во время ожидания ответа */
	TCPERR_WRESPCLOSED,	/* Т:28 соединение закрыто во время ожидания ответа */
	TCPERR_WRESP,		/* Т:29 сбой во время ожидания ответа */
	TCPERR_RCVREFUSED,	/* Т:30 соединение разорвано во время приема */
	TCPERR_RCVCLOSED,	/* Т:31 соединение закрыто во время приема */
	TCPERR_RCVTIMEOUT,	/* Т:32 таймаут приема */
	TCPERR_RCV,		/* Т:33 сбой во время приема */
};

/* Ошибки PPP */
enum {
	PPPERR_STARTING = PPPERR_BASE,	/* К:80 ошибка запуска PPP */
	PPPERR_INIT,		/* К:81 ошибка инициализации модема */
	PPPERR_DIALING,		/* К:82 ошибка дозвона */
	PPPERR_LOGIN,		/* К:83 ошибка авторизации */
	PPPERR_IPCP,		/* К:84 ошибка конфигурации TCP/IP */
	PPPERR_CARRIER,		/* К:85 пропадание несущей */
};

/* Длина префикса запроса */
#define REQ_PREFIX_LEN		6
/* Длина суффикса запроса */
#define REQ_SUFFIX_LEN		91
/* Минимальная длина запроса по TCP/IP */
#define MIN_REQ_LEN		(REQ_PREFIX_LEN + REQ_SUFFIX_LEN)

/* Маркеры гарантированной доставки */
#define VERSION_INFO_MARK	0x01	/* информация о версии */
#define REACTION_INFO_MARK	0x07	/* время реакции терминала */
#define XGATE_INFO_MARK		0x08	/* информация о шлюзе */
#define TERM_INFO_MARK		0x09	/* информация о терминале */
#define SESSION_INFO_MARK	0x11	/* информация о сеансе */
#define PRN_INFO_MARK		0x12	/* информация о принтерах */

/* Флаги гарантированной доставки (запрос) */
#define GDF_REQ_REJECT		0x01	/* отказ от заказа */
#define GDF_REQ_CRC		0x02	/* ошибка контрольной суммы ответа */
#define GDF_REQ_FIRST		0x04	/* первый запрос после инициализации */
#define GDF_REQ_SYNTAX		0x08	/* синтаксическая ошибка ответа */
#define GDF_REQ_FOREIGN		0x10	/* чужой ответ */
#define GDF_REQ_TYPE		0x60	/* тип запроса */
#define GDF_REQ_INIT		0x20	/* запрос на инициализацию */
#define GDF_REQ_APP		0x40	/* прикладной запрос */
/* Флаги гарантированной доставки (ответ) */
#define GDF_RESP_REJ_ENABLED	0x01	/* разрешен отказ от заказа */
#define GDF_RESP_INIT		0x20	/* ответ при инициализации */
#define GDF_RESP_APP		0x40	/* прикладной ответ */

#define SET_FLAG(f, v) (f |= v)
#define CLR_FLAG(f, v) (f &= ~v)
#define TST_FLAG(f, v) (f & v)

/* Работа со счетчиками гарантированной доставки */
#define INC_VAL(w) ({ w++; w %= 0x1000; w; })
#define DELTA(w1,w2) (int)(w1 - w2 + 0x1000*((w2-w1)/0x800))
#define RANDOM_VAL (uint16_t)rand() % 0x1000

#define TERM_NUMBER_LEN		13
typedef char term_number[TERM_NUMBER_LEN];

#define PRN_NUMBER_LEN		13

/* Состояние сеансового уровня */
enum {
	ss_new,		/* initialize me */
	ss_initializing,/* initiaization in progress */
	ss_winit,	/* waiting for initalization */
	ss_finit,	/* cannot initialize */
	ss_ready,	/* ready to work */
};

/*
 * Флаг переключения режимов работы терминала. Перед переходом из основного
 * режима в пригородный и наоборот необходимо провести инициализацию и послать
 * в хост-ЭВМ холостой запрос.
 */
extern bool wm_transition;
/* Переключение режима с запросом пользователя */
extern bool wm_transition_interactive;
extern int s_state;
extern int session_error;
extern bool delayed_init;

extern uint32_t init_t0;

/* Счетчики гарантированной доставки */
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
 * Последний переданный запрос. Непосредственно перед ним расположен байт
 * с кодом синтаксической ошибки ответа.
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

#endif		/* GD_H */
