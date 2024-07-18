/* Работа с потоком передачи команд POS-эмулятора. (c) A.Popov, gsr 2004-2005, 2024 */

#if !defined POS_COMMAND_H
#define POS_COMMAND_H

#if defined __cplusplus
extern "C" {
#endif

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
	uint8_t required;		/* флаг обязательности параметра */
} pos_request_param_t;

/* Список запрашиваемых параметров */
typedef struct {
	uint8_t count;			/* число параметров */
	pos_request_param_t *params;	/* параметры */
} pos_request_param_list_t;

/* Названия параметров */
#define POS_PARAM_AMOUNT_STR	"AMOUNT"	/* сумма заказа (коп) */
#define POS_PARAM_INVOICE	"INVOICE"	/* номер квитанции */
#define POA_PARAM_ORDS_STR	"ORDS"		/* список номеров и стоимостей документов */
#define POS_PARAM_TIME_STR	"TIME"		/* время заказа */
#define POS_PARAM_ID_STR	"ORDERID"	/* идентификатор заказа в системе */
#define POS_PARAM_TERMID_STR	"TERMID"	/* идентификатор терминала */
#define POS_PARAM_CLERKID_STR	"CLERKID"	/* идентификатор кассира */
#define POS_PARAM_CLERKTYPE_STR	"CLERKTYPE"	/* тип жетона кассира */
#define POS_PARAM_UBT_STR	"EBT"		/* поддержка единой банковской транзакции (ЕБТ) */
#define POS_PARAM_VERSION_STR	"VERSION"	/* версия ПО ИПТ */
#define POS_PARAM_MTYPE_STR	"MTYPE"		/* пункт меню */
#define POS_PARAM_EDIT_STR	"EDIT"		/* возможность редактирования суммы и номера квитанции */
#define POS_PARAM_FMENU_STR	"FINISHMENU"	/* завершение работы банковского приложения */
#define POS_PARAM_RES_CODE_STR	"RESULT_CODE"	/* код возврата */
#define POS_PARAM_RESP_CODE_STR	"RESPONSE_CODE"	/* код ответа */
#define POS_PARAM_ID_POS_STR	"ID_POS"	/* идентификатор ИПТ */
#define POS_PARAM_NMTYPE_STR	"NEXT_MTYPE"	/* следующий пункт меню */
#define POS_PARAM_NPARAMS_STR	"NPARAMS"	/* количество параметров */

/* Типы параметров */
enum {
	POS_PARAM_UNKNOWN	= -1,
	POS_PARAM_AMOUNT,
	POS_PARAM_INVOICE,
	POS_PARAM_ORDS,
	POS_PARAM_TIME,
	POS_PARAM_ID,
	POS_PARAM_TERMID,
	POS_PARAM_CLERKID,
	POS_PARAM_CLERKTYPE,
	POS_PARAM_UBT,
	POS_PARAM_VERSION,
	POS_PARAM_MTYPE,
	POS_PARAM_EDIT,
	POS_PARAM_FMENU,
	POS_PARAM_RES_CODE,
	POS_PARAM_RESP_CODE,
	POS_PARAM_ID_POS,
	POS_PARAM_NMTYPE,
	POS_PARAM_NPARAMS,
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

/* Поддержка ЕБТ в ИПТ */
extern bool ubt_supported;

/* Выбранные пункты меню */
#define MTYPE_PROCESS_INCOMPLETE	0xa0
#define MTYPE_DAY_OPEN			0xa1
#define MTYPE_DAY_CLOSE			0xa2
#define MTYPE_PAYMENT			0xa3
#define MTYPE_REFUND			0xa4
#define MTYPE_CANCEL			0xa5
#define MTYPE_SERVICE			0xa6
#define MTYPE_UNKNOWN			0xff

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

#if defined __cplusplus
}
#endif

#endif		/* POS_COMMAND_H */
