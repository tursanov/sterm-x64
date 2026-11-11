/* Общие функции для создания файла списка лицензий ИПТ. (c) gsr 2011 */

#if !defined MKLIC_H
#define MKLIC_H

#include "sysdefs.h"

__BEGIN_DECLS

/*
 * Чтение очередного заводского номера из файла. Возвращает число
 * считанных номеров (0/1) или -1 в случае ошибки.
 */
extern int read_number(int fd, uint8_t *number);

/* Открытие файла списка номеров */
extern int open_number_file(const char *name);

__END_DECLS

#endif		/* MKLIC_H */
