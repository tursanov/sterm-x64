/*
 * Функции, специфичные для ISA адаптера BSC на основе XILINX.
 * (c) gsr 2003
 */

#if !defined SYS_XILINX_H
#define SYS_XILINX_H

#include "sys/chipset.h"

extern bool xilinx_probe(void);
extern void xilinx_set_metrics(struct chipset_metrics *cm);

#endif		/* SYS_XILINX_H */
