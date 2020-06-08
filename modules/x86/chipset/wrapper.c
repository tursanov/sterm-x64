/*
 * Функции общего назначения для работы с различными адаптерами BSC.
 * (c) gsr 2003
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/io.h>		/* I/O port macros */
#include <linux/console.h>
#include <linux/delay.h>	/* for udelay() declaration */
#include <linux/ioport.h>
#include <linux/sched.h>	/* for process scheduling */
#include "sysdefs.h"
#include "sys/chipset.h"
#include "sys/ioctls.h"
#include "chip/cave.h"
#include "chip/ppio.h"
#include "chip/xilinx.h"
#include "chip/pci.h"

/* Описание модуля */
MODULE_AUTHOR("Sergey Tursanov");
MODULE_DESCRIPTION("generic support for eXpress adapters");
MODULE_SUPPORTED_DEVICE("all Spektr eXpress adapters");
MODULE_LICENSE("GPL");
#define DEVICE_NAME	"chipset"
/* Флаг использования линейки на основе XILINX для ККМ. */
int xilinx_for_kkm = 0;
module_param(xilinx_for_kkm, int, 0);

int chipset_type = chipsetUNKNOWN;
struct chipset_metrics cm = {
	name:		NULL,
	io_base:	0,
	io_extent:	0,
	bsc_irq:	-1,
	bsc_read_dta:	NULL,
	bsc_write_dta:	NULL,
	bsc_read_ctl:	NULL,
	bsc_write_ctl:	NULL,
	bsc_GOT:	NULL,
	bsc_ZAK:	NULL,
	prn_write_data:	NULL,
	prn_BUSY:	NULL,
	prn_DRQ:	NULL,
	prn_RPR:	NULL,
	prn_SPR:	NULL,
	dallas_DLSO:	NULL,
	dallas_DLSK:	NULL,
};

EXPORT_SYMBOL(cm);

/* Установка структуры для соответствующего адаптера */
bool set_chipset_metrics(int chipset)
{
	static struct chipset_entry {
		int chipset;
		chipset_set_metrics_t set_metrics;
	} chipset_tbl[] = {
		{chipsetCAVE,	cave_set_metrics},
		{chipsetPPIO,	ppio_set_metrics},
		{chipsetXILINX,	xilinx_set_metrics},
		{chipsetPCI,	pci_set_metrics},
	};
	int i;
	chipset_type = chipset;
	for (i = 0; i < ASIZE(chipset_tbl); i++){
		if (chipset_tbl[i].chipset == chipset){
			chipset_tbl[i].set_metrics(&cm);
			return true;
		}
	}
	return false;
}

static int detect_chipset(void)
{
	static struct probe_entry {
		chipset_probe_t probe;
		int chipset;
	} probe_tbl[] = {
		{pci_probe,	chipsetPCI},
		{cave_probe,	chipsetCAVE},
		{ppio_probe,	chipsetPPIO},
		{xilinx_probe,	chipsetXILINX},
	};
	int i, chipset = chipsetUNKNOWN;
	for (i = 0; i < ASIZE(probe_tbl); i++){
		if (probe_tbl[i].probe()){
			chipset = probe_tbl[i].chipset;
			set_chipset_metrics(chipset);
			break;
		}
	}
	return chipset;
}

/* IOCTL */
/* Номер IRQ для BSC */
static int chipset_io_bsc_irq(int param)
{
	return cm.bsc_irq;
}

/* Чтение байта данных */
static int chipset_io_bsc_rdta(int param)
{
	if (cm.bsc_read_dta != NULL)
		return cm.bsc_read_dta();
	else
		return -1;
}

/* Запись байта данных */
static int chipset_io_bsc_wdta(int param)
{
	if (cm.bsc_write_dta != NULL){
		cm.bsc_write_dta((uint8_t)param);
		return 0;
	}else
		return -1;
}

/* Чтение регистра состояния */
static int chipset_io_bsc_rctl(int param)
{
	if (cm.bsc_read_ctl != NULL)
		return cm.bsc_read_ctl();
	else
		return -1;
}

/* Запись регистра состояния */
static int chipset_io_bsc_wctl(int param)
{
	if (cm.bsc_write_ctl != NULL){
		cm.bsc_write_ctl((uint8_t)param);
		return 0;
	}else
		return -1;
}

/* Чтение состояния линии ГОТОВНОСТЬ коммутатора */
static int chipset_io_bsc_got(int param)
{
	if (cm.bsc_GOT != NULL)
		return cm.bsc_GOT();
	else
		return -1;
}

/* Установка линии ЗАКАЗ коммутатора */
static int chipset_io_bsc_zak(int param)
{
	if (cm.bsc_ZAK != NULL){
		cm.bsc_ZAK((bool)param);
		return 0;
	}else
		return -1;
}

/* Принтер */
/* Запись байта данных */
static int chipset_io_prn_wdata(int param)
{
	if (cm.prn_write_data != NULL){
		cm.prn_write_data((uint8_t)param);
		return 0;
	}else
		return -1;
}

/* Чтение шины BUSY */
static int chipset_io_prn_busy(int param)
{
	if (cm.prn_BUSY != NULL)
		return cm.prn_BUSY();
	else
		return -1;
}

/* Чтение шины DRQ (ACK) */
static int chipset_io_prn_drq(int param)
{
	if (cm.prn_DRQ != NULL)
		return cm.prn_DRQ();
	else
		return -1;
}

/* Установка шины RPR */
static int chipset_io_prn_rpr(int param)
{
	if (cm.prn_RPR != NULL){
		cm.prn_RPR((uint8_t)param);
		return 0;
	}else
		return -1;
}

/* Установка шины SPR */
static int chipset_io_prn_spr(int param)
{
	if (cm.prn_SPR != NULL){
		cm.prn_SPR((uint8_t)param);
		return 0;
	}else
		return -1;
}

/* Ключи DS1990A */
/* Установка линии DLSO */
static int chipset_io_dallas_dlso(int param)
{
	if (cm.dallas_DLSO != NULL){
		cm.dallas_DLSO((uint8_t)param);
		return 0;
	}else
		return -1;
}

/* Чтение линии DLSK */
static int chipset_io_dallas_dlsk(int param)
{
	if (cm.dallas_DLSK != NULL)
		return cm.dallas_DLSK();
	else
		return -1;
}

/* Определение типа адаптера */
static int chipset_io_chip_type(int param)
{
	return chipset_type;
}

static int chipset_ioctl(struct inode *inode, struct file *file,
		unsigned int ioctl_num, unsigned long param)
{
	static struct ioctl_proc{
		int ioctl_num;
		int (*ioctl_fn)(int);
	} ioctls[]={
		{CHIPSET_IO_BSC_IRQ,	chipset_io_bsc_irq},
		{CHIPSET_IO_BSC_RDTA,	chipset_io_bsc_rdta},
		{CHIPSET_IO_BSC_WDTA,	chipset_io_bsc_wdta},
		{CHIPSET_IO_BSC_RCTL,	chipset_io_bsc_rctl},
		{CHIPSET_IO_BSC_WCTL,	chipset_io_bsc_wctl},
		{CHIPSET_IO_BSC_GOT,	chipset_io_bsc_got},
		{CHIPSET_IO_BSC_ZAK,	chipset_io_bsc_zak},
		{CHIPSET_IO_PRN_WDATA,	chipset_io_prn_wdata},
		{CHIPSET_IO_PRN_BUSY,	chipset_io_prn_busy},
		{CHIPSET_IO_PRN_DRQ,	chipset_io_prn_drq},
		{CHIPSET_IO_PRN_RPR,	chipset_io_prn_rpr},
		{CHIPSET_IO_PRN_SPR,	chipset_io_prn_spr},
		{CHIPSET_IO_DALLAS_DLSO,chipset_io_dallas_dlso},
		{CHIPSET_IO_DALLAS_DLSK,chipset_io_dallas_dlsk},
		{CHIPSET_IO_CHIP_TYPE,	chipset_io_chip_type},
	};
	int i;
	for (i=0; i < ASIZE(ioctls); i++)
		if (ioctls[i].ioctl_num == ioctl_num)
			return ioctls[i].ioctl_fn(param);
	return -ENOSYS;
}

static int chipset_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int chipset_close(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations chipset_fops = {
	owner:			THIS_MODULE,
	ioctl:			chipset_ioctl,
	open:			chipset_open,
	release:		chipset_close,
};

int init_module(void)
{
	int chipset = detect_chipset();
	if (chipset == chipsetUNKNOWN){
		printk("failed to detect chipset\n");
		return -1;
	}else{
		printk("%s detected\n", cm.name);
		if (register_chrdev(CHIPSET_MAJOR, DEVICE_NAME, &chipset_fops) < 0)
			return -1;
		else if (!request_region(cm.io_base, cm.io_extent, cm.name))
			return -1;
		else
			return 0;
	}
}

void cleanup_module(void)
{
	if (cm.io_base != 0)
		release_region(cm.io_base, cm.io_extent);
	unregister_chrdev(CHIPSET_MAJOR, DEVICE_NAME);
}
