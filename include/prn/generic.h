/* ��騥 �㭪樨 ��� ࠡ��� � �ਭ�ࠬ�. (c) gsr 2009 */

#if !defined PRN_GENERIC_H
#define PRN_GENERIC_H

#if defined __cplusplus
extern "C" {
#endif

#include "blimits.h"
#include "sysdefs.h"

/* ���� ���� */
extern uint8_t prn_buf[PRN_BUF_LEN];
/* ����� ������ � ���� ���� */
extern int prn_buf_len;

extern bool prn_number_correct(uint8_t *number);
extern bool prn_numbers_ok(void);

#if defined __cplusplus
}
#endif

#endif		/* PRN_GENERIC_H */
