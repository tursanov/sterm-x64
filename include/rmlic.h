/*
 * Удаление лицензий ИПТ/ППУ с терминала и формирование кода удаления.
 * При удалении в MBR записывается специальный флаг, а пользователю
 * выдается 32-значное шестнадцатеричное число. Число формируется
 * на основе текущей даты, таблицы случайных чисел (rndtbl.c) и
 * хэша заводского номера терминала.
 * (c) gsr 2006, 2011, 2019.
 */

#if !defined RMLIC_H
#define RMLIC_H

#include "md5.h"
#include "rndtbl.h"

/* Удаление файла лицензии */
extern bool rm_lic_file(const char *name, const char *lic_name);
/* Формирование кода удаления лицензии для заданного заводского номера терминала */
extern bool mk_del_hash_for_tnumber(uint8_t *uint8_t, const struct md5_hash *number,
	const struct rnd_rec *rnd_tbl, int n);
/* Формирование кода удаления лицензии */
extern bool mk_del_hash(uint8_t *buf, const struct rnd_rec *rnd_tbl, int n);
/* Вывод кода удаления лицензии на экран */
extern void print_del_hash(const uint8_t *del_hash, const char *lic_name);
/* Запрос подтверждения удаления лицензии */
extern bool ask_rmlic_confirmation(const char *lic_name);

#endif		/* RMLIC_H */
