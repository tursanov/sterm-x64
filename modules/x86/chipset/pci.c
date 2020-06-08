/*
 * Функции, специфичные для PCI адаптеров BSC на основе XILINX.
 * (c) gsr 2003
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/io.h>		/* I/O port macros */
#include <linux/delay.h>	/* for udelay() declaration */
#include <linux/ioport.h>
#include <linux/pci.h>
#include "sysdefs.h"
#include "sys/chipset.h"
#include "sys/ioctls.h"

/* Общие характеристики */
static char *chipset_name = "Express PCI card";
/* Для адаптера PCI базовый адрес ввода/вывода определяется динамически */
static uint32_t io_base = 0;
#define IO_EXTENT	16

#define PCI_CLASS_EXPRESS	PCI_CLASS_NETWORK_OTHER
#define PCI_VENDOR_ID_SPECTRUM	1
#define PCI_DEVICE_ID_SPECTRUM	1

/* КР580ВВ51А */
#define USART_IO_BASE	0
static int usart_io_base = 0;
#define USART_DTA	0		/* данные (чтение/запись) */
#define USART_CTL	4		/* управляющий регистр */
static int usart_irq  = 0;		/* запрос на прерывание */
/* XILINX */
#define XILINX_IO_BASE	8
static int xilinx_io_base = 0;
static uint8_t xilinx_ctl = 0;
#define XILINX_CTL	0
#define XILINX_DATA	4

/* Битовые константы для записи */
#define XILINX_SPR	0		/* SPR для принтера */
#define XILINX_RPR	1		/* RPR для принтера */
#define XILINX_DLSO	2		/* DLSO для DS1990A */
#define XILINX_RST51	3		/* сброс КР580ВВ51А */
#define XILINX_REQ	4		/* ЗАКАЗ для коммутатора */

/* Битовые маски для чтения */
#define XILINX_READY	0x01		/* ГОТОВНОСТЬ от коммутатора */
#define XILINX_ACK	0x02		/* DRQ (ACK) от принтера */
#define XILINX_DLSK	0x04		/* DLSK для DS1990A */
#define XILINX_BUSY	0x08		/* BUSY от принтера */
#define XILINX_PE	0x10		/* PE от принтера (не используется) */

static void xilinx_bit(int bit, int v)
{
	if ((bit >= 0) && (bit < 8)){
		if (v)
			xilinx_ctl |= (1 << bit);
		else
			xilinx_ctl &= ~(1 << bit);
		outb_p(xilinx_ctl, xilinx_io_base + XILINX_CTL);
	}
}

#if 0
struct pci_dev *find_express_card(void)
{
	struct pci_dev *dev;
/*	if ((dev = pci_find_class(PCI_CLASS_EXPRESS << 8, NULL)) != NULL){*/
	if ((dev = pci_get_class(PCI_CLASS_EXPRESS << 8, NULL)) != NULL){
		if (dev->vendor == PCI_VENDOR_ID_SPECTRUM)
			return dev;
	}
	return NULL;
}
#endif

/* BSC */
/* Чтение байта данных */
uint8_t pci_bsc_read_dta(void)
{
	return inb_p(usart_io_base + USART_DTA);
}

/* Запись байта данных */
void pci_bsc_write_dta(uint8_t b)
{
	outb_p(b, usart_io_base + USART_DTA);
}

/* Чтение регистра состояния */
uint8_t pci_bsc_read_ctl(void)
{
	return inb_p(usart_io_base + USART_CTL);
}

/* Запись регистра состояния */
void pci_bsc_write_ctl(uint8_t b)
{
	outb_p(b, usart_io_base + USART_CTL);
}

/* Чтение состояния линии ГОТОВНОСТЬ коммутатора */
bool pci_bsc_GOT(void)
{
	return (inb_p(xilinx_io_base + XILINX_CTL) & XILINX_READY) == 0;
}

/* Установка линии ЗАКАЗ коммутатора */
void pci_bsc_ZAK(bool set)
{
	xilinx_bit(XILINX_REQ, !set);
}

/* Принтер */
/* Запись байта данных */
void pci_prn_write_data(uint8_t b)
{
	outb_p(b, xilinx_io_base + XILINX_DATA);
}

/* Чтение шины BUSY */
bool pci_prn_BUSY(void)
{
	return (inb_p(xilinx_io_base + XILINX_CTL) & XILINX_BUSY) != 0;
}

/* Чтение шины DRQ (ACK) */
bool pci_prn_DRQ(void)
{
	return (inb_p(xilinx_io_base + XILINX_CTL) & XILINX_ACK) != 0;
}

/* Установка шины RPR */
void pci_prn_RPR(bool set)
{
	xilinx_bit(XILINX_RPR, !set);
}

/* Установка шины SPR */
void pci_prn_SPR(bool set)
{
	xilinx_bit(XILINX_SPR, set);
}

/* Ключи DS1990A */
/* Установка линии DLSO */
void pci_dallas_DLSO(bool set)
{
	xilinx_bit(XILINX_DLSO, !set);
}

/* Чтение линии DLSK */
bool pci_dallas_DLSK(void)
{
	return (inb_p(xilinx_io_base + XILINX_CTL) & XILINX_DLSK) != 0;
}

/* Установка структуры с описанием адаптера */
void pci_set_metrics(struct chipset_metrics *cm)
{
	if (cm != NULL){
		cm->name = chipset_name;
		cm->io_base = io_base;
		cm->io_extent = IO_EXTENT;
		cm->bsc_irq = usart_irq;
		cm->bsc_read_dta = pci_bsc_read_dta;
		cm->bsc_write_dta = pci_bsc_write_dta;
		cm->bsc_read_ctl = pci_bsc_read_ctl;
		cm->bsc_write_ctl = pci_bsc_write_ctl;
		cm->bsc_GOT = pci_bsc_GOT;
		cm->bsc_ZAK = pci_bsc_ZAK;
		cm->prn_write_data = pci_prn_write_data;
		cm->prn_BUSY = pci_prn_BUSY;
		cm->prn_DRQ = pci_prn_DRQ;
		cm->prn_RPR = pci_prn_RPR;
		cm->prn_SPR = pci_prn_SPR;
		cm->dallas_DLSO = pci_dallas_DLSO;
		cm->dallas_DLSK = pci_dallas_DLSK;
	}
}

/* Проверка наличия адаптера */
bool pci_probe(void)
{
	bool flag = false;
	int err;
	struct pci_dev *dev = pci_get_device(PCI_VENDOR_ID_SPECTRUM,
		PCI_DEVICE_ID_SPECTRUM, NULL);
	if (dev != NULL){
		err = pci_enable_device(dev);
		if (err != 0)
			printk("%s: pci_enable_device_failed with code %d\n",
				__func__, err);
/*		printk("%s: found PCI device: devfn = %d; vendor = %hu; "
				"device = %hu; svendor = %hu; sdevice = %hu; "
				"class = %u; cfg_size = %d; irq = %u; name = %s; start = 0x%.8x; "
				"end = 0x%.8x; flags = 0x%.8lx\n",
			__func__, dev->devfn, dev->vendor, dev->device,
			dev->subsystem_vendor, dev->subsystem_device,
			dev->class, dev->cfg_size,
			dev->irq, dev->resource[0].name,
			dev->resource[0].start, dev->resource[0].end,
			dev->resource[0].flags);*/
		io_base = pci_resource_start(dev, 0);
		if (io_base != 0 &&
				(pci_resource_flags(dev, 0) & IORESOURCE_IO)){
			usart_io_base = io_base + USART_IO_BASE;
			xilinx_io_base = io_base + XILINX_IO_BASE;
			flag = true;
		} else
			printk("cannot find PCI IO address\n");
	}
	if (flag){
		usart_irq = dev->irq;
		xilinx_bit(XILINX_RST51, 1);
		udelay(10);
		xilinx_bit(XILINX_RST51, 0);
		mdelay(10);
		printk("PCI card found at 0x%.8x irq %d\n",
				io_base, usart_irq);
	}
	return flag;
}
