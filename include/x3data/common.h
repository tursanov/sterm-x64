/* ��騥 ��।������ ��� ��� 䠩���. (c) gsr, 2024 */

#if !defined X3DATA_COMMON_H
#define X3DATA_COMMON_H

#include "sysdefs.h"

#if defined __cplusplus
extern "C" {
#endif

typedef void (*x3_sync_callback_t)(bool done, const char *message);

/* ��������� ����ࠦ���� � ���/��� */
struct pic_header {
	uint8_t hdr_len;	/* ����� ��������� */
	uint8_t w;		/* �ਭ� ����ࠦ���� � �� */
	uint8_t h;		/* ���� ����ࠦ���� � �� */
	uint8_t id;		/* �����䨪��� "������" ����ࠦ���� */
	uint8_t name[SPRN_MAX_PIC_NAME_LEN];
	uint32_t data_len;	/* ����� ������ */
} __attribute__((__packed__));

#if defined __cplusplus
}
#endif

#endif		/* X3DATA_COMMON_H */
