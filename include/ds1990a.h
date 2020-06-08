/* Работа с ключами DS1990A. (c) gsr 2000-2004. */

#ifndef DS1990A_H
#define DS1990A_H

#include "md5.h"
#include "sysdefs.h"

/* Число ключей различного типа */
#define NR_SRV_KEYS		2
#define NR_DBG_KEYS		20

/* Номер ключа DS1990A */
#define DS_NUMBER_LEN		8
typedef uint8_t ds_number[DS_NUMBER_LEN];

/* Тип ключа DS1990A */
enum {
	key_none = -1,		/* нет ключа */
	key_dbg,		/* отладочный ключ */
	key_srv,		/* настроечный ключ */
	key_reg,		/* рабочий ключ */
};

extern bool ds_init(void);
extern bool ds_read(ds_number dsn);
extern bool ds_hash(ds_number dsn, struct md5_hash *md5);
extern char ds_key_char(int kt);

/* Управление динамиком через COM-порт */
#define SPEAKER_DEV	"/dev/speaker"

#endif		/* DS1990A_H */
