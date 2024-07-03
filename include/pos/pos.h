/* �᭮���� ����� ��� ࠡ��� � POS-����஬. (c) A.Popov, gsr 2004 */

#if !defined POS_H
#define POS_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

/* ���� ������ ��� ������ � POS-����஬ */
struct pos_message_block {
	uint16_t stream;		/* ��⮪ ��� �뢮�� ������ */
	uint16_t length;		/* ����� ������ ��� ��� ��������� */
	uint8_t data[0];		/* ����� */
} __attribute__((__packed__));

/* ���� ��⮪�� */
#define POS_STREAM_KEYBOARD	0x0000
#define POS_STREAM_SCREEN	0x0001
#define POS_STREAM_PRINTER	0x0002
#define POS_STREAM_TCP		0x0003
#define POS_STREAM_ERROR	0x0005
#define POS_STREAM_COMMAND	0x0099

/* ����� ��⮪�� */
#define POS_HAS_STREAM_KEYBOARD	0x01
#define POS_HAS_STREAM_SCREEN	0x02
#define POS_HAS_STREAM_PRINTER	0x04
#define POS_HAS_STREAM_TCP	0x08
#define POS_HAS_STREAM_ERROR	0x10
#define POS_HAS_STREAM_COMMAND	0x20

extern uint32_t stream_flags;

/* ����饭�� ��� ������ � POS-����஬ */
struct pos_message {
	uint8_t nr_blocks;		/* �᫮ ������ � ᮮ�饭�� [0 ... POS_MAX_BLOCKS] */
	struct pos_message_block data[0];	/* ����� ������ [0 ... nr_blocks] */
} __attribute__((__packed__));

/* ���ᨬ���� ࠧ��� ������ � ����� */
#define POS_MAX_BLOCK_DATA_LEN	8192
/* ���ᨬ��쭮� �᫮ ������ ������ ��� ������� ��⮪��� */
#define POS_MAX_BLOCKS		4

/* ��������� ᮮ�饭�� */
struct pos_message_header {
	uint8_t version[4];	/* ����� ��⮪��� */
	uint32_t length;	/* ����� ᮮ�饭�� ��� ��� ��������� */
	struct pos_message msg;	/* ᮮ�饭�� */
} __attribute__((__packed__));

/* ������ ����� ��⮪��� */
#define POS_CURRENT_VERSION	0x31303030	/* 1000 */

/* ���ᨬ��쭠� ����� ᮮ�饭�� */
#define POS_MAX_MESSAGE_LEN	32793
/* ���ᨬ��쭠� ����� ������ � ᮮ�饭�� */
#define POS_MAX_MESSAGE_DATA_LEN	(POS_MAX_MESSAGE_LEN - 8)

/* ������� ��� ����� � �⢥� */
struct pos_data_buf
{
	union {
		uint8_t data[POS_MAX_MESSAGE_LEN];	/* ����� */
		struct pos_message_header hdr;	/* ��������� ᮮ�饭�� */
	} un;
	int data_len;		/* ⥪��� ����� ������ */
	int data_index;		/* ������ �� �⥭��/����� */
	int block_start;	/* ��砫� ⥪�饣� ����� ������ */
};

/* �ᯮ������ ��� ��ਮ���᪮�� ���� POS-����� */
#define POS_TIMEOUT		100	/* � ���� ᥪ㭤� */
extern uint32_t pos_t0;
extern bool poll_ok;
#define MAX_POS_TIMEOUT		1000

/* �⥭�� ������ �� ���� */
extern bool pos_read(struct pos_data_buf *buf, uint8_t *data, int len);
/* �⥭�� ���� �� ���� */
extern bool pos_read_byte(struct pos_data_buf *buf, uint8_t *v);
/* �⥭�� ᫮�� �� ���� */
extern bool pos_read_word(struct pos_data_buf *buf, uint16_t *v);
/* �⥭�� �������� ᫮�� �� ���� */
extern bool pos_read_dword(struct pos_data_buf *buf, uint32_t *v);
/* �⥭�� ���ᨢ� ���⮢ �� ���� */
extern int pos_read_array(struct pos_data_buf *buf, uint8_t *data, int max_len);

/* ������ ������ � ���� */
extern bool pos_write(struct pos_data_buf *buf, uint8_t *data, int len);
/* ������ ���� � ���� */
extern bool pos_write_byte(struct pos_data_buf *buf, uint8_t v);
/* ������ ᫮�� � ���� */
extern bool pos_write_word(struct pos_data_buf *buf, uint16_t v);
/* ������ �������� ᫮�� � ���� */
extern bool pos_write_dword(struct pos_data_buf *buf, uint32_t v);
/* ������ ���ᨢ� ���⮢ � ����� */ 
extern bool pos_write_array(struct pos_data_buf *buf, uint8_t *data, int len);

/* ��砫� �ନ஢���� ���� */
extern bool pos_req_begin(struct pos_data_buf *buf);
/* ����� �ନ஢���� ���� */
extern bool pos_req_end(struct pos_data_buf *buf);
/* ��砫� �ନ஢���� ��⮪� */
extern bool pos_req_stream_begin(struct pos_data_buf *buf, uint16_t stream);
/* ����� �ନ஢���� ��⮪� */
extern bool pos_req_stream_end(struct pos_data_buf *buf);

/* ���⮩ �⢥� */
#define pos_req_empty(req)	(pos_req_begin(req) && pos_req_end(req))

#if defined __POS_DEBUG__
/* ����� ���� */
extern bool pos_dump(struct pos_data_buf *buf);
#else
#define pos_dump(buf)
#endif

/* ����ﭨ� ����� ࠡ��� � POS-����஬ */
enum {
	pos_new,		/* �ॡ���� ���뫪� ������� INIT_CHECK */
	pos_init_check,		/* ��᫠�� ������� INIT_CHECK, ��������� �⢥� */
	pos_idle,		/* �������� ��砫� ࠡ��� */
	pos_init,		/* ��砫� ࠡ���, ����室��� ��᫠�� INIT */
	pos_ready,		/* API-����� ��⮢ � ࠡ�� */
	pos_enter,		/* ���짮��⥫� ����� Enter */
	pos_print,		/* ��砫� ���� */
	pos_printing,		/* ����� */
	pos_finish,		/* �����襭�� ࠡ��� �� ���樠⨢� POS-����� */
	pos_break,		/* �����襭�� ࠡ��� �� ���樠⨢� ���짮��⥫� */
	pos_err,		/* �訡�� ࠡ��� � POS-����஬ */
	pos_err_out,		/* �訡�� ࠡ��� � POS-����஬ */
	pos_wait,		/* �������� ��᫥ �訡�� ��। ���뫪�� INIT_CHECK */
	pos_ewait,		/* �������� ��᫥ �뢮�� �� �࠭ ᮮ�饭�� �� �訡�� */
};

extern bool pos_create(void);
extern void pos_release(void);
extern int  pos_get_state(void);
extern void pos_set_state(int st);
extern void pos_process(void);
extern bool pos_send_params(void);

#if defined __cplusplus
}
#endif

#endif		/* POS_H */
