/* Различные типы, макросы и константы общего назначения. (c) gsr 2000-2020 */

/* NB: этот файл используется как в обычном режиме, так и в режиме ядра. */

#if !defined SYSDES_H
#define SYSDES_H

#if defined __KERNEL__
#include <linux/types.h>
#else
#include <sys/times.h>
#include <stdint.h>
#endif
#include <stddef.h>	/* offsetof */
#include "config.h"

/* Вариант sizeof, возвращающий int вместо size_t (используется в *printf) */
#define isizeof(v) (int)sizeof(v)
/* Определение количества элементов в массиве */
#define ASIZE(a) (int)(sizeof(a)/sizeof(a[0]))
/* Универсальный вариант offsetof (принимает как типы, так и переменные) */
#define xoffsetof(s, f) offsetof(typeof(s), f)
/* Определение размера структуры, начиная с заданного поля */
#define xsizefrom(s, f) (sizeof(s) - offsetof(typeof(s), f))

#if !defined __cplusplus && !defined __KERNEL__
typedef int bool;
#endif
#define false 0
#define true 1

#define ON	1
#define OFF	0

#define __LO_BYTE(w)	(uint8_t)((w) & 0xff)
#define __HI_BYTE(w)	(uint8_t)(((w) >> 8) & 0xff)

/* NB: при вызове данных функций происходит обнуление выделенной памяти */
#define __calloc(n, t) calloc(n, sizeof(t))
#define __new(t) (typeof(t) *)__calloc(1, typeof(t))
/* ВНИМАНИЕ: возможны side effects */
#define __delete(p)			\
{					\
	if (p != NULL){			\
		free(p);		\
		p=NULL;			\
	}				\
}

/* Преобразование значения макроса в строку. См. info cpp: Argument Prescan */
#define _s(s) __s(s)
#define __s(s) #s

#if !defined __KERNEL__
static inline uint32_t u_times(void)
{
	return times(NULL);
}
#endif

/* Преобразование беззнакового целого в struct in_addr (например, для inet_ntoa) */
#define dw2ip(dw) (struct in_addr){.s_addr = dw}

/* По аналогии с ядром Linux для "подсказки" компилятору при оптимизации */
#if !defined __KERNEL__
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#endif

/* В gcc 7.x и выше введён атрибут fallthrough */
#if defined __GNUC__ && (__GNUC__ > 6)
#define __fallthrough__	__attribute__((__fallthrough__))
#else
#define __fallthrough__	((void)0)
#endif

#endif		/* SYSDEFS_H */
