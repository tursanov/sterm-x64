/*
 * Проверка линии TCP/IP посылкой icmp-пакетов (ping).
 * (c) gsr, Alex P. Popov. 2002,2004.
 */

#if !defined GUI_PING_H
#define GUI_PING_H

#include "kbd.h"
#include "gui/scr.h"

#define ICMP_PROTO_NAME		"icmp"
#define RCV_BUF_LEN		49152		/* 48K */
#define PING_DATA_LEN		32
#define MAX_IP_LEN		60
#define MAX_ICMP_LEN		76
#define ICMP_DATA		"__sterm__"

/* Все интерваля задаются в сотых долях секунды */
#define ICMP_PING_INTERVAL		10	/* 0.1 с */
#define ICMP_WAIT_INTERVAL		300	/* 3 с */
#define NR_PINGS			3

extern bool	init_ping();
extern void	release_ping(void);
extern bool	draw_ping(void);
extern bool	process_ping(struct kbd_event *e);

#endif		/* GUI_PING_H */
