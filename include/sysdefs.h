/* ������� ⨯�, ������ � ����⠭�� ��饣� �����祭��. (c) gsr 2000-2020 */

/* NB: ��� 䠩� �ᯮ������ ��� � ���筮� ०���, ⠪ � � ०��� ��. */

#if !defined SYSDES_H
#define SYSDES_H

#if defined __KERNEL__
#include <linux/types.h>
#else
#include <sys/times.h>
#include <stdint.h>
#endif
#include <stddef.h>	/* offsetof */
#include "config.h"

/* ��ਠ�� sizeof, �������騩 int ����� size_t (�ᯮ������ � *printf) */
#define isizeof(v) (int)sizeof(v)
/* ��।������ ������⢠ ����⮢ � ���ᨢ� */
#define ASIZE(a) (int)(sizeof(a)/sizeof(a[0]))
/* ������ᠫ�� ��ਠ�� offsetof (�ਭ����� ��� ⨯�, ⠪ � ��६����) */
#define xoffsetof(s, f) offsetof(typeof(s), f)
/* ��।������ ࠧ��� ��������, ��稭�� � ��������� ���� */
#define xsizefrom(s, f) (sizeof(s) - offsetof(typeof(s), f))

#if !defined __cplusplus && !defined __KERNEL__
typedef int bool;
#endif
#define false 0
#define true 1

#define ON	1
#define OFF	0

#define __LO_BYTE(w)	(uint8_t)((w) & 0xff)
#define __HI_BYTE(w)	(uint8_t)(((w) >> 8) & 0xff)

/* NB: �� �맮�� ������ �㭪権 �ந�室�� ���㫥��� �뤥������ ����� */
#define __calloc(n, t) calloc(n, sizeof(t))
#define __new(t) (typeof(t) *)__calloc(1, typeof(t))
/* ��������: �������� side effects */
#define __delete(p)			\
{					\
	if (p != NULL){			\
		free(p);		\
		p=NULL;			\
	}				\
}

/* �८�ࠧ������ ���祭�� ����� � ��ப�. ��. info cpp: Argument Prescan */
#define _s(s) __s(s)
#define __s(s) #s

#if !defined __KERNEL__
static inline uint32_t u_times(void)
{
	return times(NULL);
}
#endif

/* �८�ࠧ������ ������������ 楫��� � struct in_addr (���ਬ��, ��� inet_ntoa) */
#define dw2ip(dw) (struct in_addr){.s_addr = dw}

/* �� �������� � �஬ Linux ��� "���᪠���" ���������� �� ��⨬���樨 */
#if !defined __KERNEL__
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#endif

/* � gcc 7.x � ��� ����� ��ਡ�� fallthrough */
#if defined __GNUC__ && (__GNUC__ > 6)
#define __fallthrough__	__attribute__((__fallthrough__))
#else
#define __fallthrough__	((void)0)
#endif

#endif		/* SYSDEFS_H */
