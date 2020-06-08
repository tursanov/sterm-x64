/* ����� � ���祢� USB-��᪮�. (c) gsr, Alex Popov 2004 */

#if !defined USB_KEY_H
#define USB_KEY_H

#include "md5.h"

/* �������� ��������� USB-���� */
#define USB_KEY_MAGIC		0x53464b55	/* UKFS */

/* ��������� USB-���� */
struct key_header {
	int magic;		/* ᨣ����� */
	int data_size;		/* ࠧ��� ������, ������ ��������� */
	int n_chunks;		/* �᫮ ������ ������ */
} __attribute__((__packed__));

/* ��������� ����� ������ */
struct key_data_header {
	int len;		/* ����� ������ ����� � ���������� */
	struct md5_hash crc;	/* ����஫쭠� �㬬� MD5 ������ ��� ��������� */
} __attribute__((__packed__));

/* �⥭�� ����� ������᭮�� � ᮧ����� ����室���� 䠩��� */
extern bool read_usbkey(void);

/* �⥭�� 䠩�� �ਢ離� �� ����� ������᭮�� */
extern bool read_usbkey_bind(void);

/* �������� �������⥫��� ������ ����� ������᭮�� */
#define USB_AUX_MAGIC		0x5855413e	/* >AUX */

/* �������⥫쭠� ���ଠ�� ����� ������᭮�� */
struct key_aux_data {
	int magic;		/* ᨣ����� */
	struct md5_hash sig;	/* 㭨���쭠� ᨣ����� */
	uint8_t gaddr;		/* ��㯯���� ���� �ନ���� */
	uint8_t iaddr;		/* �������㠫�� ���� �ନ���� */
	uint32_t host_ip;	/* ip-���� ���-��� */
	uint32_t bank_ip;	/* ip-���� ����ᨭ������ 業�� */
} __attribute__((__packed__));

/* �⥭�� �������⥫쭮� ���ଠ樨 �� ����� ������᭮�� */
extern bool read_usbkey_aux(struct key_aux_data *aux_data);

#endif		/* USB_KEY_H */
