/* Работа с лицензиями ИПТ. (c) gsr 2005-2006, 2011, 2019, 2026 */

#if !defined LICENSE_H
#define LICENSE_H

#include "md5.h"
#include "paths.h"

__BEGIN_DECLS

/* Информация о лицензии ИПТ */
struct bank_license_info {
	struct md5_hash number;		/* хеш заводского номера */
	struct md5_hash license;	/* лицензия */
};

/* Максимальное количество лицензий ИПТ в файле */
#define MAX_BANK_LICENSES		10000

/* Имя файла хеша заводского номера терминала (для проверки лицензии) */
#define TERM_NUMBER_FILE		SDATA_DIR "/disk.dat"

__END_DECLS

#endif		/* LICENSE_H */
