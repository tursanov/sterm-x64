/*
 * Функции, специфичные для нового адаптера BSC на основе КР580ВВ55А.
 * (c) gsr 2003
 */

#if !defined SYS_PPIO_H
#define SYS_PPIO_H

#include "sys/chipset.h"

extern bool ppio_probe(void);
extern void ppio_set_metrics(struct chipset_metrics *cm);

#endif		/* SYS_PPIO_H */
