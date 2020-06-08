/* Работа с признаками удаления лицензий. (c) gsr 2011, 2019 */

#if !defined LICSIG_H
#define LICSIG_H

#include <stdlib.h>
#include "sysdefs.h"

/* Размер сектора жесткого диска */
#define SECTOR_SIZE			512

/*
 * NB: если в MBR обнаружен признак удаленной лицензии, то ее использование
 * или повторная установка не допускаются.
 */

/* Признак удаленной лицензии ИПТ */
#define BANK_LIC_SIGNATURE		0x8680
/* Смещение признака удаленной лицензии ИПТ в MBR */
#define BANK_LIC_SIGNATURE_OFFSET	0x1fa

/* Признак удаленной лицензии ППУ */
#define LPRN_LIC_SIGNATURE		0x8080
/* Смещение признака удаленной лицензии ППУ в MBR */
#define LPRN_LIC_SIGNATURE_OFFSET	0x1da

/* Проверка присутствия признака удаления лицензии */
extern bool check_lic_signature(off_t offs, uint16_t sig);
/* Запись признака удаления лицензии */
extern bool write_lic_signature(off_t offs, uint16_t sig);
/* Очистка признака удаления лицензии */
extern bool clear_lic_signature(off_t offs, uint16_t sig);

#endif		/* LICSIG_H */
