/*
 * Работа с принтером "Экспресс". (c) gsr 2003
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/io.h>		/* I/O port macros */
#include <linux/delay.h>	/* for udelay() declaration */

#include "sysdefs.h"
#include "sys/ioctls.h"
#include "sys/chipset.h"
#include "sys/xprn.h"

/* Описание модуля */
MODULE_AUTHOR("Sergey Tursanov");
MODULE_DESCRIPTION("eXpress printer operations");
MODULE_SUPPORTED_DEVICE("generic eXpress printers");
MODULE_LICENSE("GPL");
#define DEVICE_NAME	"xprn"

static bool waiting_nack = false;

#define N_ACK_HITS	10
#define PRN_WAIT_DELAY	10000	/* 0.1 с */

/* Сброс принтера */
void reset_prn(void)
{
	cm.prn_RPR(true);
	udelay(1000);
	cm.prn_RPR(false);
	cm.prn_SPR(false);
	waiting_nack = false;
}

/* Ожидание сброса BUSY принтера */
static bool wait_ready(void)
{
	int i, n_hits = 0;
	bool flag;
	for (i = 0; i < PRN_WAIT_DELAY && n_hits < N_ACK_HITS; i++){
		flag = !cm.prn_BUSY();
		if (flag)
			n_hits++;
		else
			n_hits = 0;
	}
	return n_hits == N_ACK_HITS;
}

/* Ожидание ACK от принтера */
static bool wait_ack(void)
{
	int i, n_hits = 0;
	bool flag;
	for (i = 0; i < PRN_WAIT_DELAY && n_hits < N_ACK_HITS; i++){
		flag = cm.prn_DRQ();
		if (flag)
			n_hits++;
		else
			n_hits = 0;
	}
	return n_hits == N_ACK_HITS;
}

/* Ожидание NAK от принтера */
static bool wait_nak(void)
{
	int i, n_hits = 0;
	bool flag;
	for (i = 0; i < PRN_WAIT_DELAY && n_hits < N_ACK_HITS; i++){
		flag = !cm.prn_DRQ();
		if (flag)
			n_hits++;
		else
			n_hits = 0;
	}
	return n_hits == N_ACK_HITS;
}

/* Вывод символа на принтера */
static int print_char(char c)
{
	if (wait_ready()){
		if (!waiting_nack){
			cm.prn_write_data(c);
			if (!wait_ack())
				return prn_pout;
			cm.prn_SPR(true);
			waiting_nack = true;
		}
		if (!wait_nak())
			return prn_pout;
		cm.prn_SPR(false);
		waiting_nack = false;
		return prn_ready;
	}else
		return prn_dead;
}

/* IOCTL */
static int xprn_io_reset(int param)
{
	reset_prn();
	return 0;
}

static int xprn_io_outchar(int param)
{
	return print_char((char)(param & 0x7f));
}

static int xprn_ioctl(struct inode *inode, struct file *file,
		unsigned int ioctl_num, unsigned long param)
{
	static struct ioctl_proc{
		int ioctl_num;
		int (*ioctl_fn)(int);
	} ioctls[] = {			/* reaction table */
		{XPRN_IO_RESET, xprn_io_reset},
		{XPRN_IO_OUTCHAR, xprn_io_outchar},
	};
	int i;
	for (i = 0; i < ASIZE(ioctls); i++)
		if (ioctls[i].ioctl_num == ioctl_num)
			return ioctls[i].ioctl_fn(param);
	return -ENOSYS;
}

static int open_count = 0;

static int xprn_open(struct inode *inode, struct file *file)
{
	if (open_count > 0)
		return -EBUSY;
	else{
		open_count++;
		return 0;
	}
}

static int xprn_close(struct inode *inode, struct file *file)
{
	if (open_count > 0)
		open_count--;
	return 0;
}

static struct file_operations xprn_fops = {
	owner:			THIS_MODULE,
	ioctl:			xprn_ioctl,
	open:			xprn_open,
	release:		xprn_close,
};

int init_module(void)
{
	if (register_chrdev(XPRN_MAJOR, DEVICE_NAME, &xprn_fops) < 0){
		printk("%s: module_register_chrdev failed\n", DEVICE_NAME);
		return -1;
	}else{
		reset_prn();
		return 0;
	}
}

void cleanup_module(void)
{
	unregister_chrdev(XPRN_MAJOR, DEVICE_NAME);
}
