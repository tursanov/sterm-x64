/*
 * ������ ��砩��� �ᥫ ��� �ணࠬ�� ���� ��業��� ���.
 * ������ ������ � ⠡��� ��⮨� �� 3-� 32-ࠧ�來�� ��砩��� �ᥫ.
 * (c) gsr 2006, 2019
 */

#if !defined RNDTBL_H
#define RNDTBL_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

/* ������ � ⠡��� ��砩��� �ᥫ */
struct rnd_rec {
	uint32_t a;
	uint32_t b;
	uint32_t c;
};

/* ������⢮ ����ᥩ � ⠡��� ��砩��� �ᥫ */
#define NR_RND_REC		100

/* ������ ��砩��� �ᥫ ��� �ନ஢���� ���� 㤠����� ��業��� ��� */
extern struct rnd_rec rnd_tbl_bnk[NR_RND_REC];

/* ������ ��砩��� �ᥫ ��� �ନ஢���� ���� 㤠����� ��業��� ��� */
extern struct rnd_rec rnd_tbl_lprn[NR_RND_REC];

#if defined __cplusplus
}
#endif

#endif		/* RNDTBL_H */
