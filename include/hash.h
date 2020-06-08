/* Работа с ассоциативными массивами. (c) gsr 2000, 2003, 2009 */

#if !defined HASH_H
#define HASH_H

#include "sysdefs.h"
#include "sterm.h"

#define HASH_ID_MIN		0x30
#define HASH_ID_MAX		0x7e
#define HASH_SIZE		HASH_ID_MAX - HASH_ID_MIN + 1
#define HASH_DELIM		0x1c
#define HASH_IDELIM		0x00	/* внутренний разделитель записей */
#define IS_HASH_ID(c) (bool)(((c) >= HASH_ID_MIN) && ((c) <= HASH_ID_MAX))

/*
 * Ассоциативный массив. Запись находится по ее однобайтовуму идентификатору
 * Записи хранятся в виде <id> <data...> <0x00>
 */
struct hash{
	uint8_t *data;		/* область данных */
	int hash_len;		/* длина области данных */
	int data_len;		/* длина данных */
};

extern struct hash	*create_hash(int l);
extern void		release_hash(struct hash *h);
extern void		clear_hash(struct hash *h);
/* Функция используется для установки значений по умолчанию в ОЗУ констант */
extern void		set_hash_defaults(struct hash *h);
extern bool		in_hash(struct hash *h, uint8_t id);
extern int		hash_get_len(struct hash *h, uint8_t id);
extern int		hash_get(struct hash *h, uint8_t id, uint8_t *buf);
extern bool		hash_set(struct hash *h, uint8_t id, uint8_t *txt);
extern int		hash_set_all(struct hash *h, uint8_t *str);

#endif		/* HASH_H */
