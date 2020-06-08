/* ����� � ��⮪�� ��।�� �訡�� POS-�����. (c) A.Popov, gsr 2004 */

#if !defined POS_ERROR_H
#define POS_ERROR_H

#include "pos/pos.h"

/* ������ �訡�� */
#define POS_ERROR_CLASS_SYSTEM		0x00
#define POS_ERROR_CLASS_SCREEN		0x01
#define POS_ERROR_CLASS_KEYBOARD	0x02
#define POS_ERROR_CLASS_PRINTER		0x03
#define POS_ERROR_CLASS_TCP		0x04
#define POS_ERROR_CLASS_COMMANDS	0x09

/* ���� �訡�� */
/* ���⥬� */
#define POS_ERR_LOW_MEM		0x0012	/* �� 墠⠥� ����� */
#define POS_ERR_MSG_FMT		0x0030	/* �ਭ��� ������ �� ����� ���� ࠧ��࠭� */
#define POS_ERR_SYSTEM		0x0031	/* �⠫쭠� �訡�� ��⥬� */
#define POS_ERR_TIMEOUT		0x0068	/* ⠩���� ���� POS-����� */
#define POS_ERR_CRYPTO		0x0092	/* �訡�� �ਯ⮣�䨨 */
#define POS_ERR_NOT_INIT	0x1000	/* POS-����� �� ���樠����஢�� */
/* ��࠭ */
#define POS_ERR_SCR_NOT_READY	0x0001	/* ���ன�⢮ �� ����㯭� */
#define POS_ERR_DEPICT		0x0002	/* ���������� �⮡ࠧ��� ����� */
/* ��������� */
#define POS_ERR_KEY_MSG_FMT	0x0003	/* ���������� ࠧ����� ᮮ�饭�� */
#define POS_ERR_KEY_DBL_OBJ	0x0004	/* �������騥�� ��६���� */
#define POS_ERR_UNKNOWN_OBJ	0x0005	/* ����।������� ��६����� */
/* �ਭ�� */
#define POS_ERR_PRN		0x0008	/* �訡�� �ਭ�� */
/* TCP/IP */
#define POS_ERR_TCP_INIT	0x0002	/* �訡�� ���樠����樨 �ࠩ��஢ TCP/IP */
#define POS_ERR_HOST_NOT_CNCT	0x0010	/* �������⨬� ����� �ࢥ� */
#define POS_ERR_CONNECT		0x0012	/* �訡�� ᮥ������� � �ࢥ஬ */
#define POS_ERR_DATA_TX		0x0018	/* �訡�� ��।�� ������ */
#define POS_ERR_TIMEOUT_RX	0x0068	/* ⠩���� �ਥ�� ������ */
#define POS_ERR_TIMEOUT_TX	0x0069	/* ⠩���� ��।�� ������ */
/* ������� */
#define POS_ERR_FMT		0x0003	/* �訡�� �ଠ� ������� */
#define POS_ERR_CMD		0x0005	/* ������� �� ����� ���� ��ࠡ�⠭� */

/* ���ଠ�� �� �訡�� */
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

/* ������ �⢥� �� POS-����� �� �訡�� */
extern bool pos_parse_error_stream(struct pos_data_buf *buf, bool check_only);

/* ������ �訡�� � ��⮪ */
extern bool pos_req_save_error_stream(struct pos_data_buf *buf,
		pos_error_t *err);

/* ��⠭���� �訡�� */
extern void pos_set_error(int class, int code, intptr_t param);
/* ���� �訡�� */
extern void pos_error_clear(void);
		
#endif		/* POS_ERROR_H */
