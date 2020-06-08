/*
 * Таблица случайных чисел для программы снятия лицензий ИПТ.
 * Каждая запись в таблице состоит из 3-х 32-разрядных случайных чисел.
 * (c) gsr 2006, 2019
 */

#if !defined RNDTBL_H
#define RNDTBL_H

#include "sysdefs.h"

/* Запись в таблице случайных чисел */
struct rnd_rec {
	uint32_t a;
	uint32_t b;
	uint32_t c;
};

/* Количество записей в таблице случайных чисел */
#define NR_RND_REC		100

/* Таблица случайных чисел для формирования кода удаления лицензии ИПТ */
extern struct rnd_rec rnd_tbl_bnk[NR_RND_REC];

/* Таблица случайных чисел для формирования кода удаления лицензии ППУ */
extern struct rnd_rec rnd_tbl_lprn[NR_RND_REC];

#endif		/* RNDTBL_H */
