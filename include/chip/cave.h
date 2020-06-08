/*
 * Функции, специфичные для старого адаптера BSC на основе КР580ВВ55А.
 * (c) gsr 2003
 */

#if !defined SYS_CAVE_H
#define SYS_CAVE_H

#include "sys/chipset.h"

extern bool cave_probe(void);
extern void cave_set_metrics(struct chipset_metrics *cm);

#endif		/* SYS_CAVE_H */
