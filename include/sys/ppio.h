/*
	�㭪樨, ��騥 ��� �����஢ BSC �� �᭮�� ��580��55�.
	(c) gsr 2001-2003
*/

#if !defined SYS_PPIO_H
#define SYS_PPIO_H

#include <linux/ioctl.h>
#include "sysdefs.h"

/* �������� �����/�뢮�� */
#define PPIO_REGA	0
#define PPIO_REGB	1
#define PPIO_REGC	2
#define PPIO_CTL	3

/* �८�ࠧ������ ��� ��� ����� � ॣ���� �ࠢ����� */
#define PPIO_CTL_BITS(v,s) (uint8_t)(((v) << 1) | ((s) & 1))

/* ����⠭� ��� ���樠����樨 ��580��55� */
#define PPIO_MAGIC	0x98

#endif		/* SYS_PPIO_H */
