/* Работа с принтером "Экспресс" через LPT. (c) gsr 2004, 2020 */

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
#define XPRN_CLASS	"x3prn"

static struct class *xprn_class = NULL;

static int open_count = 0;

static bool waiting_nack = false;

#define N_ACK_HITS	10
#define PRN_WAIT_DELAY	10000	/* 0.1 с */

/* Работа с интерфейсом parport */
static struct parport *port = NULL;
static struct pardevice *dev = NULL;

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
static long xprn_io_reset(unsigned long param)
{
	int ret = parport_claim(dev);
	if (ret == 0){
		reset_prn();
		parport_release(dev);
	}else
		ret = prn_dead;
	return ret;
}

static long xprn_io_outchar(unsigned long param)
{
	int ret = parport_claim(dev);
	if (ret == 0){
		ret = print_char((char)(param & 0x7f));
		parport_release(dev);
	}else
		ret = prn_dead;
	return ret;
}

static long xprn_ioctl(struct file *file, unsigned int ioctl_num, unsigned long param)
{
	static struct ioctl_proc{
		unsigned int ioctl_num;
		long (*ioctl_fn)(unsigned long);
	} ioctls[] = {
		{XPRN_IO_RESET, xprn_io_reset},
		{XPRN_IO_OUTCHAR, xprn_io_outchar},
	};
	long ret = -ENOSYS;
	int i;
	printk("%s: file = 0x%p; ioctl_num = 0x%x; param = 0x%lx.\n",
		__func__, file, ioctl_num, param);
	for (i = 0; i < ASIZE(ioctls); i++){
		typeof(*ioctls) *p = ioctls + i;
		if (p->ioctl_num == ioctl_num){
			ret = p->ioctl_fn(param);
			break;
		}
	}
	return ret;
}

static int xprn_open(struct inode *inode, struct file *file)
{
	int ret = -EBUSY;
	printk("%s: inode = 0x%p; file = 0x%p.\n", __func__, inode, file);
	if (open_count == 0){
		open_count++;
		ret = 0;
	}
	return ret;
}

static int xprn_close(struct inode *inode, struct file *file)
{
	printk("%s: inode = 0x%p; file = 0x%p.\n", __func__, inode, file);
	if (open_count > 0)
		open_count--;
	return 0;
}

static struct file_operations xprn_fops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl		= xprn_ioctl,
	.open			= xprn_open,
	.release		= xprn_close,
};

static int xprn_preempt(void *handle)
{
	return open_count == 0;
}

static void xprn_attach(struct parport *pp)
{
	printk("%s: pp->number = %d.\n", __func__, pp->number);
	if (pp->number == 0){
		port = pp;
		dev = parport_register_device(port, DEVICE_NAME,
			xprn_preempt, NULL, NULL, 0, NULL);
		device_create(xprn_class, port->dev, MKDEV(XPRN_MAJOR, 0), NULL, DEVICE_NAME);
	}else
		printk("ignoring parport %d\n", pp->number);
}

static void xprn_detach(struct parport *pp)
{
}

static struct parport_driver xprn_driver = {
	.name			= DEVICE_NAME,
	.attach			= xprn_attach,
	.detach			= xprn_detach,
};

int init_module(void)
{
	int ret = register_chrdev(XPRN_MAJOR, DEVICE_NAME, &xprn_fops);
	if (ret != 0){
		printk("register_chrdev failed with code %d\n", ret);
		return ret;
	}
	xprn_class = class_create(THIS_MODULE, XPRN_CLASS);
	if (IS_ERR(xprn_class)){
		ret = PTR_ERR(xprn_class);
		printk("class_create failed with code %d\n", ret);
		goto out_reg;
	}
	ret = parport_register_driver(&xprn_driver);
	if (ret != 0){
		printk("parport_register_driver failed with code %d\n", ret);
		goto out_class;
	}
	reset_prn();
	return ret;
out_class:
	class_destroy(xprn_class);
out_reg:
	unregister_chrdev(XPRN_MAJOR, DEVICE_NAME);
	return ret;
}

void cleanup_module(void)
{
	parport_unregister_driver(&xprn_driver);
	unregister_chrdev(XPRN_MAJOR, DEVICE_NAME);
	if (dev != NULL)
		parport_unregister_device(dev);
	if (xprn_class != NULL){
		device_destroy(xprn_class, MKDEV(XPRN_MAJOR, 0));
		class_destroy(xprn_class);
	}
}
