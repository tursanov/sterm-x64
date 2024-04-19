/* ��ୠ� ������ ����묨. (c) gsr 2004 */

#if !defined XCHANGE_H
#define XCHANGE_H

#if defined __cplusplus
extern "C" {
#endif

#include "genfunc.h"

/* ���ᨬ��쭠� ����� ����� ������ � ��ୠ�� */
#define XLOG_MAX_DATA_LEN	65536
/* ���ᨬ��쭮� �᫮ ����⮢ � ��ୠ�� */
#define XLOG_NR_ITEMS		64
/* ���ࠢ����� ��।�� ������ (�⭮�⥫쭮 �ନ����) */
enum {
	xlog_in,
	xlog_out,
};
/* ������� ������ � ��ୠ�� */
struct xlog_item {
	struct date_time dt;	/* ���/�६� ᮧ����� */
	int dir;		/* ���ࠢ����� */
	int len;		/* ����� ������ */
	uint8_t *data;		/* ����� */
	uint16_t Ntz;		/* ZN�/ON� */
	uint16_t Npo;		/* ZN��/ON�� */
	uint16_t Bp;		/* Z��/O�� */
};

extern bool xlog_add_item(uint8_t *data, int len, int dir);
extern int xlog_count_items(void);
extern struct xlog_item *xlog_get_item(int index);

#if defined __cplusplus
}
#endif

#endif		/* XCHANGE_H */
