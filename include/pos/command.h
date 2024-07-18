/* ����� � ��⮪�� ��।�� ������ POS-�����. (c) A.Popov, gsr 2004-2005, 2024 */

#if !defined POS_COMMAND_H
#define POS_COMMAND_H

#if defined __cplusplus
extern "C" {
#endif

#include "pos/pos.h"

/* ���� ������ */
#define POS_COMMAND_INIT			0x01
#define POS_COMMAND_FINISH			0x02
#define POS_COMMAND_REQUEST_PARAMETERS		0x03
#define POS_COMMAND_RESPONSE_PARAMETERS		0x04
#define POS_COMMAND_INIT_REQUIRED		0x05
#define POS_COMMAND_INIT_CHECK			0x06

/* ����訢���� ��ࠬ���� */
typedef struct {
	char *name;			/* ��� ��ࠬ��� */
	int type;			/* ⨯ ��ࠬ��� */
	uint8_t required;		/* 䫠� ��易⥫쭮�� ��ࠬ��� */
} pos_request_param_t;

/* ���᮪ ����訢����� ��ࠬ��஢ */
typedef struct {
	uint8_t count;			/* �᫮ ��ࠬ��஢ */
	pos_request_param_t *params;	/* ��ࠬ���� */
} pos_request_param_list_t;

/* �������� ��ࠬ��஢ */
#define POS_PARAM_AMOUNT_STR	"AMOUNT"	/* �㬬� ������ (���) */
#define POS_PARAM_INVOICE	"INVOICE"	/* ����� ���⠭樨 */
#define POA_PARAM_ORDS_STR	"ORDS"		/* ᯨ᮪ ����஢ � �⮨���⥩ ���㬥�⮢ */
#define POS_PARAM_TIME_STR	"TIME"		/* �६� ������ */
#define POS_PARAM_ID_STR	"ORDERID"	/* �����䨪��� ������ � ��⥬� */
#define POS_PARAM_TERMID_STR	"TERMID"	/* �����䨪��� �ନ���� */
#define POS_PARAM_CLERKID_STR	"CLERKID"	/* �����䨪��� ����� */
#define POS_PARAM_CLERKTYPE_STR	"CLERKTYPE"	/* ⨯ ��⮭� ����� */
#define POS_PARAM_UBT_STR	"EBT"		/* �����প� ������ ������᪮� �࠭���樨 (���) */
#define POS_PARAM_VERSION_STR	"VERSION"	/* ����� �� ��� */
#define POS_PARAM_MTYPE_STR	"MTYPE"		/* �㭪� ���� */
#define POS_PARAM_EDIT_STR	"EDIT"		/* ����������� ।���஢���� �㬬� � ����� ���⠭樨 */
#define POS_PARAM_FMENU_STR	"FINISHMENU"	/* �����襭�� ࠡ��� ������᪮�� �ਫ������ */
#define POS_PARAM_RES_CODE_STR	"RESULT_CODE"	/* ��� ������ */
#define POS_PARAM_RESP_CODE_STR	"RESPONSE_CODE"	/* ��� �⢥� */
#define POS_PARAM_ID_POS_STR	"ID_POS"	/* �����䨪��� ��� */
#define POS_PARAM_NMTYPE_STR	"NEXT_MTYPE"	/* ᫥���騩 �㭪� ���� */
#define POS_PARAM_NPARAMS_STR	"NPARAMS"	/* ������⢮ ��ࠬ��஢ */

/* ���� ��ࠬ��஢ */
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

/* ��ࠬ���� �⢥� */
typedef struct {
	char *name;			/* ��� ��ࠬ��� */
	char *value;			/* ���祭�� ��ࠬ��� */
} pos_response_param_t;

/* ���᮪ ��ࠬ��஢ �⢥� */
typedef struct {
	uint8_t count;			/* �᫮ ��ࠬ��஢ */
	pos_response_param_t *params;	/* ��ࠬ���� */
} pos_response_param_list_t;

extern pos_request_param_list_t req_param_list;
extern pos_response_param_list_t resp_param_list;

/* �����প� ��� � ��� */
extern bool ubt_supported;

/* ��࠭�� �㭪�� ���� */
#define MTYPE_PROCESS_INCOMPLETE	0xa0
#define MTYPE_DAY_OPEN			0xa1
#define MTYPE_DAY_CLOSE			0xa2
#define MTYPE_PAYMENT			0xa3
#define MTYPE_REFUND			0xa4
#define MTYPE_CANCEL			0xa5
#define MTYPE_SERVICE			0xa6
#define MTYPE_UNKNOWN			0xff

/* ������ ��⮪� ������ */
extern bool pos_parse_command_stream(struct pos_data_buf *buf, bool check_only);

/* ������ ������� INIT */
extern bool pos_req_save_command_init(struct pos_data_buf *buf);
/* ������ ������� FINISH */
extern bool pos_req_save_command_finish(struct pos_data_buf *buf);
/* ������ ������� REQUEST_PARAMETERS */
extern bool pos_req_save_command_request_parameters(struct pos_data_buf *buf);
/* ������ ������� RESPONSE_PARAMETERS */
extern bool pos_req_save_command_response_parameters(struct pos_data_buf *buf);
/* ������ ������� INIT_CHECK */
extern bool pos_req_save_command_init_check(struct pos_data_buf *buf);

#if defined __cplusplus
}
#endif

#endif		/* POS_COMMAND_H */
