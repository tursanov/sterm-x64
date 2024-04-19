/* ����� � �ਭ�஬ "������". (c) gsr 2000, 2004, 2009 */

#ifndef PRN_EXPRESS_H
#define PRN_EXPRESS_H

#if defined __cplusplus
extern "C" {
#endif

#include "prn/generic.h"
#include "sys/ioctls.h"
#include "sys/xprn.h"
#include "sysdefs.h"

/* ������� �ਭ�� "������" */

/* �������⮢� ������� */
#define XPRN_NUL		0x00	/* ����� ������� */
#define XPRN_WIDE_FNT		0x07	/* �ப�� ���� */
#define XPRN_BS			0x08	/* 蠣 ����� */
#define XPRN_ITALIC		0x09	/* �������� ���� */
#define XPRN_LF			0x0a	/* ��ॢ�� ��ப� */
#define XPRN_NORM_FNT		0x0b	/* ��ଠ��� ���� */
#define XPRN_FORM_FEED		0x0c	/* ���� ������ */
#define XPRN_CR			0x0d	/* ������ ���⪨ */
#define XPRN_DLE1		0x10	/* ��砫� ������� */
#define XPRN_UNDERLINE		0x14	/* �����ભ��� ���� */
#define XPRN_CPI12		0x19	/* ���⭮��� ���� 12 cpi */
#define XPRN_DLE		0x1b	/* ��砫� ������� */
#define XPRN_CPI10		0x1c	/* ���⭮��� ���� 10 cpi */
#define XPRN_CPI16		0x1e	/* ���⭮��� ���� 16 cpi */
#define XPRN_RST		0x7f	/* ��� ���ன�⢠ */

/* ��������⮢� ������� */
#define XPRN_AUXLNG		0x09	/* ���室 �� �������⥫��� ���� ᨬ����� */
#define XPRN_MAINLNG		0x0b	/* ���室 �� �᭮���� ���� ᨬ����� */
#define XPRN_FONT		0x41	/* �롮� ���� */
#define XPRN_VPOS		0x45	/* ���⨪��쭮� ����樮��஢���� */
#define XPRN_WR_BCODE		0x4c	/* ����� ����-���� */
#define XPRN_PRNOP		0x5b	/* ������� �ਭ�� */
#define XPRN_NO_BCODE		0x73	/* ࠡ���� ��� ����-���� */
#define XPRN_RD_BCODE		0x76	/* ���� ����-��� */

/* ����砭�� ������� XPRN_PRNOP */
#define XPRN_PRNOP_HPOS_RIGHT	0x61	/* �⭮�⥫쭮� ��ਧ��⠫쭮� ����樮��஢���� ��ࠢ� */
#define XPRN_PRNOP_HPOS_LEFT	0x71	/* �⭮�⥫쭮� ��ਧ��⠫쭮� ����樮��஢���� ����� */
#define XPRN_PRNOP_HPOS_ABS	0x60	/* ��᮫�⭮� ��ਧ��⠫쭮� ����樮��஢���� */
#define XPRN_PRNOP_VPOS_FW	0x65	/* �⭮�⥫쭮� ���⨪��쭮� ����樮��஢���� ����� */
#define XPRN_PRNOP_VPOS_BK	0x75	/* �⭮�⥫쭮� ���⨪��쭮� ����樮��஢���� ����� */
#define XPRN_PRNOP_VPOS_ABS	0x64	/* ��᮫�⭮� ���⨪��쭮� ����樮��஢���� */
#define XPRN_PRNOP_INP_TICKET	0x74	/* ���� ������ */
#define XPRN_PRNOP_LINE_FW	0x76	/* ��४��祭�� ��ப ����� */

/* ��⠭���������� �� �६� ���� �� �ਭ�� */
extern bool xprn_printing;

extern void xprn_init(void);
extern void xprn_release(void);
extern void xprn_flush(void);
extern bool xprn_print(const char *txt, int l);

#if defined __cplusplus
}
#endif

#endif		/* PRN_EXPRESS_H */
