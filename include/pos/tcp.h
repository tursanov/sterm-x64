/* ����� � ��⮪�� TCP/IP POS-�����. (c) A.Popov, gsr 2004 */

#if !defined POS_TCP_H
#define POS_TCP_H

#include "pos/pos.h"

/* ������ ����� TCP-���� ����ᨭ������ 業�� */
#define TCP_POS_PORT_BASE		8001
/* ���ᨬ��쭮� �᫮ �࠭���権 � ����ᨭ���� 業�஬ */
#define MAX_POS_TCP_TRANSACTIONS	8
/* ���ᨬ���� ࠧ��� ���� �࠭���樨 */
#define MAX_POS_TCP_DATA_LEN		8188	/* 8192 - 4 */

/* ���� ������ */
#define POS_TCP_CONNECT		0x01
#define POS_TCP_DISCONNECT	0x02
#define POS_TCP_DATA		0x03

/* ��⠭���� ���ᨢ� �࠭���権 � ��砫쭮� ���ﭨ� */
extern void pos_init_transactions(void);
/* �᢮�������� ���ᨢ� �࠭���権 */
extern void pos_release_transactions(void);
/* �����襭�� ��� �࠭���権 */
extern void pos_stop_transactions(void);
/* ��ࠡ�⪠ ��� �࠭���権 */
extern bool pos_tcp_process(void);
/* ������ ��⮪� TCP-IP */
extern bool pos_parse_tcp_stream(struct pos_data_buf *buf, bool check_only);

/* ��।������ �᫠ ᮡ�⨩ ��� ����� � ��⮪ TCP/IP */
extern int pos_count_tcp_events(void);
/* ������ ��⮪� TCP/IP */
extern bool pos_req_save_tcp(struct pos_data_buf *buf);

#endif		/* POS_TCP_H */
