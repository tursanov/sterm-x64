/*
 * Разблокировка терминала после удаления с него лицензим ИПТ/ППУ. (c) gsr 2011, 2019.
 */

#if !defined UNLOCK_H
#define UNLOCK_H

#include "colortty.h"
#include "licsig.h"
#include "md5.h"

/* Структура для хранения хеша заводского номера терминала */
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

/* Чтение хеша заводского номера терминала */
extern bool read_term_number(const char *name, struct md5_hash *number);
/* Поиск заданного заводского номера в списке */
extern bool check_term_number(const struct md5_hash *number,
	const struct fuzzy_md5 *known_number, int n);
/* Вывод на экран сообщения об успешной разблокировке терминала */
extern void print_unlock_msg(const char *name);
#endif		/* UNLOCK_H */
