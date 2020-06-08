/*
 * ��������஢�� �ନ���� ��᫥ 㤠����� � ���� ��業��� ���/���. (c) gsr 2011, 2019.
 */

#if !defined UNLOCK_H
#define UNLOCK_H

#include "colortty.h"
#include "licsig.h"
#include "md5.h"

/* ������� ��� �࠭���� �� �����᪮�� ����� �ନ���� */
struct fuzzy_md5 {
	uint32_t a;
	uint32_t aa;
	uint32_t b;
	uint32_t bb;
	uint32_t c;
	uint32_t cc;
	uint32_t d;
	uint32_t dd;
};

/* �⥭�� �� �����᪮�� ����� �ନ���� */
extern bool read_term_number(const char *name, struct md5_hash *number);
/* ���� ��������� �����᪮�� ����� � ᯨ᪥ */
extern bool check_term_number(const struct md5_hash *number,
	const struct fuzzy_md5 *known_number, int n);
/* �뢮� �� �࠭ ᮮ�饭�� �� �ᯥ譮� ࠧ�����஢�� �ନ���� */
extern void print_unlock_msg(const char *name);
#endif		/* UNLOCK_H */
