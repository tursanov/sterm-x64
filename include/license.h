/* Работа с лицензиями ИПТ и ППУ. (c) gsr 2005-2006, 2011, 2019 */

#if !defined LICENSE_H
#define LICENSE_H

#include "md5.h"

/* Информация о лицензии ИПТ */
struct bank_license_info {
	struct md5_hash number;		/* хеш заводского номера */
	struct md5_hash license;	/* лицензия */
};

/* Максимальное количество лицензий ИПТ в файле */
#define MAX_BANK_LICENSES		10000

/* Информация о лицензии ППУ */
struct lprn_license_info {
	struct md5_hash number;		/* хеш заводского номера */
	struct md5_hash license;	/* лицензия */
};

/* Максимальное количество лицензий ППУ в файле */
#define MAX_LPRN_LICENSES		MAX_BANK_LICENSES

/* Имя файла хеша заводского номера терминала (для проверки лицензии) */
#define TERM_NUMBER_FILE		"/sdata/disk.dat"

#endif		/* LICENSE_H */
