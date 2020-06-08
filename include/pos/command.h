/* Работа с потоком передачи команд POS-эмулятора. (c) A.Popov, gsr 2004 */

#if !defined POS_COMMAND_H
#define POS_COMMAND_H

#include "pos/pos.h"

/* Коды команд */
#define POS_COMMAND_INIT			0x01
#define POS_COMMAND_FINISH			0x02
#define POS_COMMAND_REQUEST_PARAMETERS		0x03
#define POS_COMMAND_RESPONSE_PARAMETERS		0x04
#define POS_COMMAND_INIT_REQUIRED		0x05
#define POS_COMMAND_INIT_CHECK			0x06

/* Запрашиваемые параметры */
typedef struct {
	char *name;			/* имя параметра */
	int type;			/* тип параметра */
	uint8_t required;			/* флаг обязательности параметра */
} pos_request_param_t;

/* Список запрашиваемых параметров */
typedef struct {
	uint8_t count;			/* число параметров */
	pos_request_param_t *params;	/* параметры */
} pos_request_param_list_t;

/* Названия параметров */
#define POS_PARAM_AMOUNT_STR	"AMOUNT"	/* сумма заказа (коп) */
#define POS_PARAM_TIME_STR	"TIME"		/* время заказа */
#define POS_PARAM_ID_STR	"ORDERID"	/* идентификатор заказа в системе */
#define POS_PARAM_TERMID_STR	"TERMID"	/* идентификатор терминала */
#define POS_PARAM_CLERKID_STR	"CLERKID"	/* идентификатор кассира */
#define POS_PARAM_CLERKTYPE_STR	"CLERKTYPE"	/* тип жетона кассира */

/* Типы параметров */
enum {
	POS_PARAM_UNKNOWN	= -1,
	POS_PARAM_AMOUNT,
	POS_PARAM_TIME,
	POS_PARAM_ID,
	POS_PARAM_TERMID,
	POS_PARAM_CLERKID,
	POS_PARAM_CLERKTYPE,
};

/* Параметры ответа */
typedef struct {
	char *name;			/* имя параметра */
	char *value;			/* значение параметра */
} pos_response_param_t;

/* Список параметров ответа */
typedef struct {
	uint8_t count;			/* число параметров */
	pos_response_param_t *params;	/* параметры */
} pos_response_param_list_t;

extern pos_request_param_list_t req_param_list;
extern pos_response_param_list_t resp_param_list;

/* Разбор потока команд */
extern bool pos_parse_command_stream(struct pos_data_buf *buf, bool check_only);

/* Запись команды INIT */
extern bool pos_req_save_command_init(struct pos_data_buf *buf);
/* Запись команды FINISH */
extern bool pos_req_save_command_finish(struct pos_data_buf *buf);
/* Запись команды REQUEST_PARAMETERS */
extern bool pos_req_save_command_request_parameters(struct pos_data_buf *buf);
/* Запись команды RESPONSE_PARAMETERS */
extern bool pos_req_save_command_response_parameters(struct pos_data_buf *buf);
/* Запись команды INIT_CHECK */
extern bool pos_req_save_command_init_check(struct pos_data_buf *buf);

#endif		/* POS_COMMAND_H */
