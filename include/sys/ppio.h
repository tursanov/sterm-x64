/*
	Функции, общие для адаптеров BSC на основе КР580ВВ55А.
	(c) gsr 2001-2003
*/

#if !defined SYS_PPIO_H
#define SYS_PPIO_H

#include <linux/ioctl.h>
#include "sysdefs.h"

/* Регистры ввода/вывода */
#define PPIO_REGA	0
#define PPIO_REGB	1
#define PPIO_REGC	2
#define PPIO_CTL	3

/* Преобразование бита для записи в регистр управления */
#define PPIO_CTL_BITS(v,s) (uint8_t)(((v) << 1) | ((s) & 1))

/* Константа для инициализации КР580ВВ55А */
#define PPIO_MAGIC	0x98

#endif		/* SYS_PPIO_H */
