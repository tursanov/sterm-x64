/*
 * Функции, специфичные для PCI адаптера BSC на основе XILINX.
 * (c) gsr 2003
 */

#if !defined SYS_PCI_H
#define SYS_PCI_H

#include "sys/chipset.h"

extern bool pci_probe(void);
extern void pci_set_metrics(struct chipset_metrics *cm);

#endif		/* SYS_PCI_H */
