/* Работа с потоком TCP/IP POS-эмулятора. (c) A.Popov, gsr 2004 */

#if !defined POS_TCP_H
#define POS_TCP_H

#include "pos/pos.h"

/* Базовый номер TCP-порта процессингового центра */
#define TCP_POS_PORT_BASE		8001
/* Максимальное число транзакций с процессинговым центром */
#define MAX_POS_TCP_TRANSACTIONS	8
/* Максимальный размер буфера транзакции */
#define MAX_POS_TCP_DATA_LEN		8188	/* 8192 - 4 */

/* Коды команд */
#define POS_TCP_CONNECT		0x01
#define POS_TCP_DISCONNECT	0x02
#define POS_TCP_DATA		0x03

/* Установка массива транзакций в начальное состояние */
extern void pos_init_transactions(void);
/* Освобождение массива транзакций */
extern void pos_release_transactions(void);
/* Завершение всех транзакций */
extern void pos_stop_transactions(void);
/* Обработка всех транзакций */
extern bool pos_tcp_process(void);
/* Разбор потока TCP-IP */
extern bool pos_parse_tcp_stream(struct pos_data_buf *buf, bool check_only);

/* Определение числа событий для записи в поток TCP/IP */
extern int pos_count_tcp_events(void);
/* Запись потока TCP/IP */
extern bool pos_req_save_tcp(struct pos_data_buf *buf);

#endif		/* POS_TCP_H */
