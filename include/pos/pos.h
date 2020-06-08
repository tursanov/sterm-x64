/* Основной модуль для работы с POS-эмулятором. (c) A.Popov, gsr 2004 */

#if !defined POS_H
#define POS_H

#include "sysdefs.h"

/* Блок данных для обмена с POS-эмулятором */
struct pos_message_block {
	uint16_t stream;		/* поток для вывода данных */
	uint16_t length;		/* длина данных без учета заголовка */
	uint8_t data[0];		/* данные */
} __attribute__((__packed__));

/* Коды потоков */
#define POS_STREAM_KEYBOARD	0x0000
#define POS_STREAM_SCREEN	0x0001
#define POS_STREAM_PRINTER	0x0002
#define POS_STREAM_TCP		0x0003
#define POS_STREAM_ERROR	0x0005
#define POS_STREAM_COMMAND	0x0099

/* Флаги потоков */
#define POS_HAS_STREAM_KEYBOARD	0x01
#define POS_HAS_STREAM_SCREEN	0x02
#define POS_HAS_STREAM_PRINTER	0x04
#define POS_HAS_STREAM_TCP	0x08
#define POS_HAS_STREAM_ERROR	0x10
#define POS_HAS_STREAM_COMMAND	0x20

extern uint32_t stream_flags;

/* Сообщение для обмена с POS-эмулятором */
struct pos_message {
	uint8_t nr_blocks;		/* число блоков в сообщении [0 ... POS_MAX_BLOCKS] */
	struct pos_message_block data[0];	/* блоки данных [0 ... nr_blocks] */
} __attribute__((__packed__));

/* Максимальный размер данных в блоке */
#define POS_MAX_BLOCK_DATA_LEN	8192
/* Максимальное число блоков данных для данного протокола */
#define POS_MAX_BLOCKS		4

/* Заголовок сообщения */
struct pos_message_header {
	uint8_t version[4];	/* версия протокола */
	uint32_t length;		/* длина сообщения без учета заголовка */
	struct pos_message msg;	/* сообщение */
} __attribute__((__packed__));

/* Текущая версия протокола */
#define POS_CURRENT_VERSION	0x31303030	/* 1000 */

/* Максимальная длина сообщения */
#define POS_MAX_MESSAGE_LEN	32793
/* Максимальная длина данных в сообщении */
#define POS_MAX_MESSAGE_DATA_LEN	(POS_MAX_MESSAGE_LEN - 8)

/* Структура для запроса и ответа */
struct pos_data_buf
{
	union {
		uint8_t data[POS_MAX_MESSAGE_LEN];	/* данные */
		struct pos_message_header hdr;	/* заголовок сообщения */
	} un;
	int data_len;		/* текущая длина данных */
	int data_index;		/* индекс при чтении/записи */
	int block_start;	/* начало текущего блока данных */
};

/* Используется для периодического опроса POS-эмулятора */
#define POS_TIMEOUT		100	/* в сотых секунды */
extern uint32_t pos_t0;
extern bool poll_ok;
#define MAX_POS_TIMEOUT		1000

/* Чтение данных из буфера */
extern bool pos_read(struct pos_data_buf *buf, uint8_t *data, int len);
/* Чтение байта из буфера */
extern bool pos_read_byte(struct pos_data_buf *buf, uint8_t *v);
/* Чтение слова из буфера */
extern bool pos_read_word(struct pos_data_buf *buf, uint16_t *v);
/* Чтение двойного слова из буфера */
extern bool pos_read_dword(struct pos_data_buf *buf, uint32_t *v);
/* Чтение массива байтов из буфера */
extern int pos_read_array(struct pos_data_buf *buf, uint8_t *data, int max_len);

/* Запись данных в буфер */
extern bool pos_write(struct pos_data_buf *buf, uint8_t *data, int len);
/* Запись байта в буфер */
extern bool pos_write_byte(struct pos_data_buf *buf, uint8_t v);
/* Запись слова в буфер */
extern bool pos_write_word(struct pos_data_buf *buf, uint16_t v);
/* Запись двойного слова в буфер */
extern bool pos_write_dword(struct pos_data_buf *buf, uint32_t v);
/* Запись массива байтов в запрос */ 
extern bool pos_write_array(struct pos_data_buf *buf, uint8_t *data, int len);

/* Начало формирования буфера */
extern bool pos_req_begin(struct pos_data_buf *buf);
/* Конец формирования буфера */
extern bool pos_req_end(struct pos_data_buf *buf);
/* Начало формирования потока */
extern bool pos_req_stream_begin(struct pos_data_buf *buf, uint16_t stream);
/* Конец формирования потока */
extern bool pos_req_stream_end(struct pos_data_buf *buf);

/* Пустой ответ */
#define pos_req_empty(req)	(pos_req_begin(req) && pos_req_end(req))

#if defined __POS_DEBUG__
/* Печать буфера */
extern bool pos_dump(struct pos_data_buf *buf);
#else
#define pos_dump(buf)
#endif

/* Состояния модуля работы с POS-эмулятором */
enum {
	pos_new,		/* требуется посылка команды INIT_CHECK */
	pos_init_check,		/* послана команда INIT_CHECK, ожидается ответ */
	pos_idle,		/* ожидание начала работы */
	pos_init,		/* начало работы, необходимо послать INIT */
	pos_ready,		/* API-модуль готов к работе */
	pos_enter,		/* пользователь нажал Enter */
	pos_print,		/* начало печати */
	pos_printing,		/* печать */
	pos_finish,		/* завершение работы по инициативе POS-эмулятора */
	pos_break,		/* завершение работы по инициативе пользователя */
	pos_err,		/* ошибка работы с POS-эмулятором */
	pos_err_out,		/* ошибка работы с POS-эмулятором */
	pos_wait,		/* ожидание после ошибки перед посылкой INIT_CHECK */
	pos_ewait,		/* ожидание после вывода на экран сообщения об ошибке */
};

extern bool pos_create(void);
extern void pos_release(void);
extern int  pos_get_state(void);
extern void pos_set_state(int st);
extern void pos_process(void);
extern bool pos_send_params(void);

#endif		/* POS_H */
