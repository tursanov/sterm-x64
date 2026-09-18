/* Работа с лицензиями ИПТ. (c) gsr 2005-2006, 2011, 2019, 2026 */

#if !defined LICENSE_H
#define LICENSE_H

#if defined __cplusplus
extern "C" {
#endif

#include "md5.h"

/* Информация о лицензии ИПТ */
struct bank_license_info {
	struct md5_hash number;		/* хеш заводского номера */
	struct md5_hash license;	/* лицензия */
};

/* Максимальное количество лицензий ИПТ в файле */
#define MAX_BANK_LICENSES		10000

/* Имя файла хеша заводского номера терминала (для проверки лицензии) */
#define TERM_NUMBER_FILE		"/sdata/disk.dat"

#if defined __cplusplus
}
#endif

#endif		/* LICENSE_H */
