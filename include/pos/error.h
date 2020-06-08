/* Работа с потоком передачи ошибок POS-эмулятора. (c) A.Popov, gsr 2004 */

#if !defined POS_ERROR_H
#define POS_ERROR_H

#include "pos/pos.h"

/* Классы ошибок */
#define POS_ERROR_CLASS_SYSTEM		0x00
#define POS_ERROR_CLASS_SCREEN		0x01
#define POS_ERROR_CLASS_KEYBOARD	0x02
#define POS_ERROR_CLASS_PRINTER		0x03
#define POS_ERROR_CLASS_TCP		0x04
#define POS_ERROR_CLASS_COMMANDS	0x09

/* Коды ошибок */
/* Система */
#define POS_ERR_LOW_MEM		0x0012	/* не хватает памяти */
#define POS_ERR_MSG_FMT		0x0030	/* принятые пакеты не могут быть разобраны */
#define POS_ERR_SYSTEM		0x0031	/* фатальная ошибка системы */
#define POS_ERR_TIMEOUT		0x0068	/* таймаут опроса POS-эмулятора */
#define POS_ERR_CRYPTO		0x0092	/* ошибка криптографии */
#define POS_ERR_NOT_INIT	0x1000	/* POS-эмулятор не инициализирован */
/* Экран */
#define POS_ERR_SCR_NOT_READY	0x0001	/* устройство не доступно */
#define POS_ERR_DEPICT		0x0002	/* невозможно отобразить элемент */
/* Клавиатура */
#define POS_ERR_KEY_MSG_FMT	0x0003	/* невозможно разобрать сообщение */
#define POS_ERR_KEY_DBL_OBJ	0x0004	/* повторяющиеся переменные */
#define POS_ERR_UNKNOWN_OBJ	0x0005	/* неопределенная переменная */
/* Принтер */
#define POS_ERR_PRN		0x0008	/* ошибка принтера */
/* TCP/IP */
#define POS_ERR_TCP_INIT	0x0002	/* ошибка инициализации драйверов TCP/IP */
#define POS_ERR_HOST_NOT_CNCT	0x0010	/* недопустимый номер сервера */
#define POS_ERR_CONNECT		0x0012	/* ошибка соединения с сервером */
#define POS_ERR_DATA_TX		0x0018	/* ошибка передачи данных */
#define POS_ERR_TIMEOUT_RX	0x0068	/* таймаут приема данных */
#define POS_ERR_TIMEOUT_TX	0x0069	/* таймаут передачи данных */
/* Команды */
#define POS_ERR_FMT		0x0003	/* ошибка формата команды */
#define POS_ERR_CMD		0x0005	/* команда не может быть обработана */

/* Информация об ошибке */
typedef struct {
	uint8_t class;
	uint16_t code;
	char message[129];
	char parameters[1024];
	int param_len;
} pos_error_t;

extern pos_error_t pos_error;
extern char pos_err_desc[256];
extern char *pos_err_xdesc;

/* Разбор ответа от POS-эмулятора об ошибке */
extern bool pos_parse_error_stream(struct pos_data_buf *buf, bool check_only);

/* Запись ошибки в поток */
extern bool pos_req_save_error_stream(struct pos_data_buf *buf,
		pos_error_t *err);

/* Установка ошибки */
extern void pos_set_error(int class, int code, intptr_t param);
/* Сброс ошибки */
extern void pos_error_clear(void);
		
#endif		/* POS_ERROR_H */
