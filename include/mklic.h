/* ��騥 �㭪樨 ��� ᮧ����� 䠩�� ᯨ᪠ ��業��� ���. (c) gsr 2011 */

#if !defined MKLIC_H
#define MKLIC_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

/*
 * �⥭�� ��।���� �����᪮�� ����� �� 䠩��. �����頥� �᫮
 * ��⠭��� ����஢ (0/1) ��� -1 � ��砥 �訡��.
 */
extern int read_number(int fd, uint8_t *number);

/* ����⨥ 䠩�� ᯨ᪠ ����஢ */
extern int open_number_file(const char *name);

#if defined __cplusplus
}
#endif

#endif		/* MKLIC_H */
