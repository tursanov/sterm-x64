/* Работа с принтером "Экспресс" через LPT. (c) gsr 2004 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/parport.h>

#include "sys/ioctls.h"
#include "sys/xprn.h"
#include "sysdefs.h"

/* Описание модуля */
MODULE_AUTHOR("Sergey Tursanov");
MODULE_DESCRIPTION("eXpress printer over LPT");
MODULE_SUPPORTED_DEVICE("generic eXpress printers");
MODULE_LICENSE("GPL");
#define DEVICE_NAME	"xprn"

static int open_count = 0;

static bool waiting_nack;

#define N_ACK_HITS	10
#define PRN_WAIT_DELAY	10000	/* 0.1 с */

/* Работа с интерфейсом parport */
static struct parport *port;
static struct pardevice *dev;

static int xprn_preempt(void *handle)
{
	return open_count == 0;
}

static void xprn_attach(struct parport *pp)
{
	if (pp->number == 0){
		port = pp;
		dev = parport_register_device(port, DEVICE_NAME,
			xprn_preempt, NULL, NULL, 0, NULL);
	}else
		printk("ignoring parport %d\n", pp->number);
}

static void xprn_detach(struct parport *pp)
{
}

/* Управление различными линиями принтера */
/* RPR */
static void set_rpr(bool on)
{
	if ((port != NULL) && (port->ops != NULL) && (port->ops->frob_control != NULL))
		port->ops->frob_control(port, PARPORT_CONTROL_INIT,
				on ? 0 : PARPORT_CONTROL_INIT);
}

#define rpr_on() set_rpr(true)
#define rpr_off() set_rpr(false)

/* SPR */
static void set_spr(bool on)
{
	if ((port != NULL) && (port->ops != NULL) && (port->ops->frob_control != NULL))
		port->ops->frob_control(port, PARPORT_CONTROL_STROBE,
				on ? 0 : PARPORT_CONTROL_STROBE);
}

#define spr_on() set_spr(true)
#define spr_off() set_spr(false)

/* BUSY */
static bool busy_active(void)
{
	if ((port != NULL) && (port->ops != NULL) && (port->ops->read_status != NULL))
		return (port->ops->read_status(port) & PARPORT_STATUS_BUSY) == 0;
	else
		return false;
}

/* DRQ */
static bool drq_active(void)
{
	if ((port != NULL) && (port->ops != NULL) && (port->ops->read_status != NULL))
		return (port->ops->read_status(port) & PARPORT_STATUS_ACK) != 0;
	else
		return false;
}

/* Сброс принтера */
static void reset_prn(void)
{
	rpr_on();
	udelay(1000);
	rpr_off();
	spr_off();
	waiting_nack = false;
}

/* Ожидание сброса BUSY принтера */
static bool wait_ready(void)
{
	int i, n_hits = 0;
	for (i = 0; i < PRN_WAIT_DELAY && n_hits < N_ACK_HITS; i++){
		if (!busy_active())
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
	for (i = 0; i < PRN_WAIT_DELAY && n_hits < N_ACK_HITS; i++){
		if (drq_active())
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
	for (i = 0; i < PRN_WAIT_DELAY && n_hits < N_ACK_HITS; i++){
		if (drq_active())
			n_hits = 0;
		else
			n_hits++;
	}
	return n_hits == N_ACK_HITS;
}

/* Вывод символа на принтера */
static int print_char(char c)
{
	if ((port == NULL) || (port->ops == NULL) || (port->ops->write_data == NULL))
		return prn_dead;
	if (wait_ready()){
		if (!waiting_nack){
			port->ops->write_data(port, c);
			if (!wait_ack())
				return prn_pout;
			spr_on();
			waiting_nack = true;
		}
		if (!wait_nak())
			return prn_pout;
		spr_off();
		waiting_nack = false;
		return prn_ready;
	}else
		return prn_dead;
}

/* IOCTL */
static int xprn_io_reset(int param)
{
	int retval = parport_claim(dev);
	if (retval == 0){
		reset_prn();
		parport_release(dev);
	}
	return retval;
}

static int xprn_io_outchar(int param)
{
	int retval = parport_claim(dev);
	if (retval == 0){
		retval = print_char((char)(param & 0x7f));
		parport_release(dev);
		return retval;
	}else
		return prn_dead;
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
	.owner			= THIS_MODULE,
	.ioctl			= xprn_ioctl,
	.open			= xprn_open,
	.release		= xprn_close,
};

static struct parport_driver xprn_driver = {
	.name			= DEVICE_NAME,
	.attach			= xprn_attach,
	.detach			= xprn_detach,
};

int init_module(void)
{
	int retval;
	retval = parport_register_driver(&xprn_driver);
	if (retval != 0){
		printk("parport_register_driver failed with code %d\n", retval);
		return retval;
	}
	retval = register_chrdev(XPRN_MAJOR, DEVICE_NAME, &xprn_fops);
	if (retval != 0){
		printk("register_chrdev failed with code %d\n", retval);
		return retval;
	}else{
		reset_prn();
		return 0;
	}
}

void cleanup_module(void)
{
	parport_unregister_driver(&xprn_driver);
	parport_unregister_device(dev);
	unregister_chrdev(XPRN_MAJOR, DEVICE_NAME);
}
