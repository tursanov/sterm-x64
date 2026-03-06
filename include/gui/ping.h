/*
 * Проверка связи по TCP/IP с помощью ICMP и подключения по TCP.
 * 11.05.2019 добавлена проверка ОФД путём установки соединения TCP.
 * 08.11.2024 процессинговые серверы проверяются так же, как и ОФД.
 * 06.03.2026 исправлена ошибка формирования ICMP-пакетов, изменён состав данных.
 * (c) gsr, Alex P. Popov 2002, 2004, 2005, 2019, 2024, 2026.
 */

#if !defined GUI_PING_H
#define GUI_PING_H

#if defined __cplusplus
extern "C" {
#endif

#include "gd.h"
#include "kbd.h"
#include "gui/scr.h"

#define ICMP_PROTO_NAME		"icmp"
#define MAX_IP_LEN		60
#define MAX_ICMP_LEN		76

/* Данные, передаваемые в ICMP-пакете */
struct ping_data {
	uint32_t sig;				/* слово PING */
#define STERM_PING_SIG	0x474e4950
	uint32_t uptime;			/* время в секундах с момента включения теминала */
	uint32_t t;				/* время отправки пакета */
	uint8_t version[3];			/* версия терминала */
	uint16_t crc;				/* контрольная сумма терминала */
	uint8_t nr[TERM_NUMBER_LEN];		/* заводской номер терминала */
	uint8_t gaddr;				/* групповой адрес терминала */
	uint8_t iaddr;				/* индивидуальный адрес терминала */
} __attribute__((__packed__));			/* должно быть 32 байта */

#define PING_DATA_LEN	sizeof(struct ping_data)

/* Все интервалы задаются в сотых долях секунды */
#define ICMP_PING_INTERVAL		100	/* 1 сек */
#define NR_PINGS			3

extern bool	init_ping();
extern void	release_ping(void);
extern bool	draw_ping(void);
extern bool	process_ping(struct kbd_event *e);

#if defined __cplusplus
}
#endif

#endif		/* GUI_PING_H */
