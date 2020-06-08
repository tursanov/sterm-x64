/*
 * �ࠩ��� ��� ࠡ��� � ���箬 ������᭮��. (c) Alex Popov & gsr 2004.
 * �᭮��� �ࠩ��� ���㦨� 䠩� drivers/usb/usb-skeleton.c �� kernel-2.4.26.
 */
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/timer.h>
#include <linux/usb.h>
#include "sys/ioctls.h"
#include "sysdefs.h"

#define MODULE_NAME		"usbkey"

#define DRIVER_VERSION		"v1.1"
#define DRIVER_AUTHOR		"Alex P. Popov <alex_popov@sendmail.ru>"
#define DRIVER_DESC		"USB Secure Key Driver"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

/* ��ࠬ����, ��।������ ����� */
static int debug;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "Debug enabled or not");

/* �뢮� �⫠����� ᮮ�饭�� */
#if defined dbg
#undef dbg
#endif
#define dbg(format, arg...) do { if (debug) printk(KERN_DEBUG "%s: " format "\n" , __func__ , ## arg); } while (0)

#if defined info
#undef info
#endif
#define info(format, arg...) do { printk(KERN_INFO "%s: " format "\n" , __func__ , ## arg); } while (0)

#if defined err
#undef err
#endif
#define err(format, arg...) do { printk(KERN_ERR "%s: " format "\n" , __func__ , ## arg); } while (0)

#define USBKEY_VENDOR_ID	0x0c70
#define USBKEY_PRODUCT_ID	0x0000

/* ������ ������ ����� ������᭮�� */
#define USBKEY_DATA_SIZE	65536
/* ������ ����� ������ ��� �⥭��/����� */
#define USBKEY_BLOCK_SIZE	256
/* �������, ��।������ ����� ������᭮�� */
struct usbkey_cmd {
#define USBKEY_CMD_READ		0x66
#define USBKEY_CMD_WRITE	0x55
	uint8_t cmd;
	uint8_t first_block;
	uint8_t n;
};

/* ������� �⥭�� (� ���� ����� ᥪ㭤�) */
#define USBKEY_READ_TIMEOUT	10
/* ������� ����� (� ���� ����� ᥪ㭤�) */
#define USBKEY_WRITE_TIMEOUT	10
/* ������⢮ ����⮪ �⥭�� ���ன�⢠ USB */
#define USBKEY_READ_TRIES	2

/* Get a minor range for your devices from the usb maintainer */
#define USBKEY_MINOR_BASE	120

/* ���ᨬ��쭮� �᫮ ���ன��, ����� ����� ���� ����祭� �����६���� */
#define MAX_DEVICES		16

/* ���ଠ��, ᯥ��筠� ��� ��襣� ���ன�⢠ */
struct usbkey {
	struct mutex		mutex;		/* �����஢�� ����㯠 � ������� */
	struct usb_device 	*udev;
	struct usb_device	*dev;
	uint8_t			minor;
	uint8_t			*in_data;	/* �室��� ���� */
	int			in_size;	/* ࠧ��� �室���� ���� */
	uint8_t			in_endpointAddr;/* the address of the bulk in endpoint */
	struct 			urb *read_urb;	/* URB ��� �⥭�� ������ */
	uint8_t			*out_data;	/* ��室��� ���� */
	int			out_size;	/* ࠧ��� ��室���� ���� */
	uint8_t			out_endpointAddr;/* the address of the bulk out endpoint */
	struct 			urb *write_urb;	/* URB ��� ����� ������ */
	uint8_t 			*data;		/* ����� ��� �⥭��/����� */
	size_t			data_len;	/* ࠧ��� ������ ��� �⥭��/����� */
	size_t			data_index;	/* ������ ᫥���饣� ᨬ���� ��� �⥭��/����� */
	wait_queue_head_t	waitq;		/* ��� ��������饣� �⥭��/����� */
	int			open_count;	/* �᫮ ����⨩ ������� ���ன�⢠ */
	bool			present;	/* 䫠� ������⢨� ���ன�⢠ */
//	bool			wake_up_flag;	/* ��⠭���������� �� �맮�� wake_up_interruptible */
	struct timer_list	timer;		/* ⠩��� �⥭��/����� */
	int			rx_interval;
	int			tx_interval;
};

/* ���ᨢ 㪠��⥫�� �� ������祭�� ���ன�⢠ */
static struct usbkey *dev_tbl[MAX_DEVICES];
/* ��� ࠡ��� � �⮩ ⠡��楩 ��� ���ॡ���� ��⥪� */
static DEFINE_MUTEX(dev_tbl_mutex);

/* �������� ���ଠ樨 � USB-���� (��। �맮��� ����室��� ��墠��� dev_tbl_mutex) */
static void usbkey_delete(struct usbkey *dev)
{
	if (dev == NULL)
		return;
	dev_tbl[dev->minor] = NULL;
	if (dev->in_data != NULL)
		kfree(dev->in_data);
	if (dev->out_data != NULL)
		kfree(dev->out_data);
	if (dev->data != NULL)
		kfree(dev->data);
	if (dev->read_urb != NULL)
		usb_free_urb(dev->read_urb);
	if (dev->write_urb != NULL)
		usb_free_urb(dev->write_urb);
	del_timer(&dev->timer);
	if (mutex_is_locked(&dev->mutex))
		mutex_unlock(&dev->mutex);
	mutex_destroy(&dev->mutex);
	kfree(dev);
}

/* ����⨥ ���ன�⢠ */
static int usbkey_open(struct inode *inode, struct file *file)
{
	struct usbkey *dev;
	int subminor;
	dbg("");
	subminor = MINOR (inode->i_rdev) - USBKEY_MINOR_BASE;
	if ((subminor < 0) || (subminor >= MAX_DEVICES))
		return -ENODEV;
	mutex_lock(&dev_tbl_mutex);
	dev = dev_tbl[subminor];
	if (dev == NULL){
		mutex_unlock(&dev_tbl_mutex);
		return -ENODEV;
	}
	mutex_lock(&dev->mutex);
	++dev->open_count;
	file->private_data = dev;
	mutex_unlock(&dev->mutex);
	mutex_unlock(&dev_tbl_mutex);
	return 0;
}

/* �����⨥ ���ன�⢠ */
static int usbkey_release(struct inode *inode, struct file *file)
{
	struct usbkey *dev;
	dev = (struct usbkey *)file->private_data;
	if (dev == NULL){
		dbg ("object is NULL");
		return -ENODEV;
	}
	dbg("minor %d", dev->minor);
	mutex_lock(&dev_tbl_mutex);
	mutex_lock(&dev->mutex);
	if (dev->open_count <= 0){
		dbg("device not opened");
		mutex_unlock(&dev->mutex);
		mutex_unlock(&dev_tbl_mutex);
		return -ENODEV;
	}
	if (dev->udev == NULL){
/* the device was unplugged before the file was released */
		mutex_unlock(&dev->mutex);
		usbkey_delete(dev);
		mutex_unlock(&dev_tbl_mutex);
		return 0;
	}
	--dev->open_count;
	if (dev->open_count <= 0){
		del_timer(&dev->timer);
		usb_kill_urb(dev->read_urb);
		usb_kill_urb(dev->write_urb);
		dev->open_count = 0;
		if (dev->data != NULL){
			kfree(dev->data);
			dev->data = NULL;
		}
	}
	mutex_unlock(&dev->mutex);
	mutex_unlock(&dev_tbl_mutex);
	return 0;
}

/* ��뢠���� �� ⠩���� �⥭��/����� */
static void timer_fn(unsigned long param)
{
	struct usbkey *dev = (struct usbkey *)param;
	dbg("waking up");
	wake_up_interruptible(&dev->waitq);
}

/* �㭪�� ���⭮�� �맮�� ��� �⥭�� */
static void usbkey_read_irq(struct urb *urb)
{
	int ret;
	struct usbkey *dev = (struct usbkey *)urb->context;
	dbg("status = %d; actual_length = %d", urb->status, urb->actual_length);
/*	if ((urb->status == -ETIMEDOUT) || (urb->status == -EILSEQ)){
		if (!dev->wake_up_flag){
			del_timer(&dev->timer);
			dev->wake_up_flag = true;
			dbg("waking up");
			wake_up_interruptible(&dev->waitq);
		}
	}else */
	if ((urb->status == 0)/* && (urb->actual_length == dev->in_size)*/){
		int len = dev->data_len - dev->data_index;
		if (len > urb->actual_length)
			len = urb->actual_length;
		if (len > 0)
			memcpy(dev->data + dev->data_index, dev->in_data, len);
		dev->data_index += urb->actual_length;
		dbg("%d bytes read", dev->data_index);
		ret = usb_submit_urb(dev->read_urb, GFP_ATOMIC);
		if (ret == 0)
			mod_timer(&dev->timer, jiffies + USBKEY_READ_TIMEOUT);
		else
			printk("%s: usb_submit_urb failed with code %d\n",
				__func__, ret);
	}
}

/* �㭪�� ���⭮�� �맮�� �� ����� ������ */
static void usbkey_write_irq(struct urb *urb)
{
	int len, ret;
	struct usbkey *dev = (struct usbkey *)urb->context;
	dbg("status = %d; actual_length = %d", urb->status, urb->actual_length);
	udelay(100);
/*	if ((urb->status == -ETIMEDOUT) || (urb->status == -EILSEQ))
		flag = true;
	else */
	if ((urb->status == 0)/* && ((urb->actual_length == dev->out_size))*/){
		dev->data_index += urb->actual_length;
		len = dev->data_len - dev->data_index;
		if (len > dev->out_size)
			len = dev->out_size;
		if (len > 0){
			memcpy(dev->out_data, dev->data + dev->data_index, len);
			ret = usb_submit_urb(dev->write_urb, GFP_ATOMIC);
			if (ret == 0)
				mod_timer(&dev->timer, jiffies + USBKEY_WRITE_TIMEOUT);
			else
				printk("%s: usb_submit_urb failed with code %d\n",
					__func__, ret);
		}/*else
			flag = true;*/
		dbg("%d bytes wrote", dev->data_index);
	}
/*	if (flag && !dev->wake_up_flag){
		del_timer(&dev->timer);
		dev->wake_up_flag = true;
		dbg("waking up");
		wake_up_interruptible(&dev->waitq);
	}*/
}

/* ��।�� ������� � ����� ������᭮�� */
static bool usbkey_send_cmd(struct usbkey *dev, uint8_t cmd, uint8_t first_block,
		uint8_t n)
{
	int ret;
	struct usbkey_cmd *pcmd = (struct usbkey_cmd *)dev->out_data;
/*	printk("%s: cmd = 0x%.2hx; first_block = %hu; n = %hu\n",
		__func__, (uint16_t)cmd, (uint16_t)first_block, (uint16_t)n);*/
	dev->data_len = dev->out_size;
	dev->data_index = 0;
	memset(dev->out_data, 0, dev->out_size);
	pcmd->cmd = cmd;
	pcmd->first_block = first_block;
	pcmd->n = n;
	usb_fill_int_urb(dev->write_urb, dev->dev,
			usb_sndintpipe(dev->dev, dev->out_endpointAddr),
			dev->out_data, dev->out_size,
			usbkey_write_irq, dev, dev->tx_interval);
/*	dev->wake_up_flag = false;*/
	ret = usb_submit_urb(dev->write_urb, GFP_KERNEL);
	if (ret != 0){
		printk("%s: usb_submit_urb failed with code %d\n",
			__func__, ret);
		return false;
	}
	dev->timer.expires = jiffies + USBKEY_WRITE_TIMEOUT;
	add_timer(&dev->timer);
	interruptible_sleep_on(&dev->waitq);
	usb_kill_urb(dev->write_urb);
	del_timer(&dev->timer);
	mdelay(5);
	return dev->data_index == dev->data_len;
}

/* �⥭�� ������ */
static bool usbkey_do_read(struct usbkey *dev, size_t n)
{
	bool retval = false;
/*	printk("%s\n", __func__);*/
	dev->data_len = n;
	dev->data_index = 0;
	usb_fill_int_urb(dev->read_urb, dev->dev,
			usb_rcvintpipe(dev->dev, dev->in_endpointAddr),
			dev->in_data, dev->in_size,
			usbkey_read_irq, dev, dev->rx_interval);
/*	dev->wake_up_flag = false;*/
	if (usb_submit_urb(dev->read_urb, GFP_KERNEL) == 0){
		dev->timer.expires = jiffies + USBKEY_READ_TIMEOUT;
		add_timer(&dev->timer);
		interruptible_sleep_on(&dev->waitq);
		usb_kill_urb(dev->read_urb);
		del_timer(&dev->timer);
		retval = dev->data_index == dev->data_len;
		mdelay(5);
	}
	return retval;
}

/* ��뢠���� ��⥬�� �� �⥭�� �� ���ன�⢠ */
static ssize_t usbkey_read(struct file *file, char *data, size_t count, loff_t *ppos)
{
	struct usbkey *dev;
	int retval = 0, i;
	dbg("offs = %lld; count = %d", *ppos, count);
	if ((*ppos + count) > USBKEY_DATA_SIZE)
		return 0;
	dev = (struct usbkey *)file->private_data;
	if (dev == NULL)
		return -ENODEV;
	mutex_lock(&dev->mutex);
	if (dev->udev == NULL)
		retval = -ENODEV;
	else if (dev->read_urb->status == -EINPROGRESS)
		retval = -EINPROGRESS;
	if (retval != 0){
		mutex_unlock(&dev->mutex);
		return retval;
	}else if (dev->data != NULL)
		kfree(dev->data);
	dev->data = kmalloc(count, GFP_KERNEL);
	if (dev->data == NULL)
		retval = -ENOMEM;
	else{
		retval = -EFAULT;
		for (i = 0; i < USBKEY_READ_TRIES; i++){
			dbg("read try #%d", i + 1);
			if (!usbkey_send_cmd(dev, USBKEY_CMD_READ,
					*ppos / USBKEY_BLOCK_SIZE,
					count / USBKEY_BLOCK_SIZE - 1))
				break;
			else if	(usbkey_do_read(dev, count)){
				if (!copy_to_user(data, dev->data, count))
					retval = count;
				break;
			}
		}
	}
	if (dev->data != NULL){
		kfree(dev->data);
		dev->data = NULL;
	}
	if (retval > 0)
		*ppos = (*ppos / USBKEY_BLOCK_SIZE + count / USBKEY_BLOCK_SIZE ) *
			USBKEY_BLOCK_SIZE;
	mutex_unlock(&dev->mutex);
	return retval;
}

/* ������ ������ */
static bool usbkey_do_write(struct usbkey *dev, size_t n)
{
	bool retval = false;
/*	printk("%s\n", __func__);*/
	dev->data_len = n;
	dev->data_index = 0;
	usb_fill_int_urb(dev->write_urb, dev->dev,
			usb_sndintpipe(dev->dev, dev->out_endpointAddr),
			dev->out_data, dev->out_size,
			usbkey_write_irq, dev, dev->tx_interval);
	memcpy(dev->out_data, dev->data, dev->out_size);
/*	dev->wake_up_flag = false;*/
	if (usb_submit_urb(dev->write_urb, GFP_KERNEL) == 0){
		dev->timer.expires = jiffies + USBKEY_WRITE_TIMEOUT;
		add_timer(&dev->timer);
		interruptible_sleep_on(&dev->waitq);
		usb_kill_urb(dev->write_urb);
		del_timer(&dev->timer);
		retval = dev->data_index == dev->data_len;
		mdelay(5);
	}
	return retval;
}

/* ��뢠���� ��⥬�� �� ����� � ���ன�⢮ */
static ssize_t usbkey_write(struct file *file, const char *data, size_t count, loff_t *ppos)
{
	struct usbkey *dev = (struct usbkey *)file->private_data;
	int retval = 0;
	if (dev == NULL)
		return -ENODEV;
	else if ((*ppos + count) > USBKEY_DATA_SIZE)
		return -ESPIPE;
	mutex_lock(&dev->mutex);
	if (dev->udev == NULL)
		retval = -ENODEV;
	else if (dev->write_urb->status == -EINPROGRESS)
		retval = -EINPROGRESS;
	if (retval != 0){
		mutex_unlock(&dev->mutex);
		return retval;
	}else if (dev->data != NULL)
		kfree(dev->data);
	dev->data = kmalloc (count, GFP_KERNEL);
	if (dev->data == NULL)
		retval = -ENOMEM;
	else if (copy_from_user(dev->data, data, count) ||
			!usbkey_send_cmd(dev, USBKEY_CMD_WRITE,
				*ppos / USBKEY_BLOCK_SIZE,
				count / USBKEY_BLOCK_SIZE - 1) ||
			!usbkey_do_write(dev, count))
		retval = -EFAULT;
	else
		retval = count;
	if (dev->data != NULL){
		kfree(dev->data);
		dev->data = NULL;
	}
	if (count > 0)
		*ppos = (*ppos / USBKEY_BLOCK_SIZE + count / USBKEY_BLOCK_SIZE ) *
			USBKEY_BLOCK_SIZE;
	mutex_unlock(&dev->mutex);
	return retval;
}

/* �����頥� 䫠� ������⢨� USB-���� */
static int usbkey_io_haskey(struct usbkey *dev)
{
	return (dev == NULL) ? -ENODEV : dev->present;
}

/* ��ࠡ�⪠ IOCTL.
 * ����室��� ������� � ⮬, �� ����� IOCTL ����� ����㯨�� �� �६� ����樨
 * �⥭��/�����, ����� ��⥪� ���ன�⢠ �������஢��. ��� ���������
 * ���⥫��� ��� �� �������� ࠧ�����஢�� ��⥪� ����室��� �����
 * mutex_lock �ᯮ�짮���� mutex_trylock � � ��砥 ��㤠� ��������
 * EWOULDBLOCK.
 */
static int usbkey_ioctl(struct inode *inode, struct file *file,
		unsigned int ioctl_num, unsigned long param)
{
	static struct {
		int ioctl_num;
		int (*ioctl_fn)(struct usbkey *dev);
	} usbkey_ioctls[] = {
		{USBKEY_IO_HASKEY,	usbkey_io_haskey},
	};
	int i, n, retval = -ENOSYS;
	n = MINOR (inode->i_rdev) - USBKEY_MINOR_BASE;
	if ((n < 0) || (n >= MAX_DEVICES))
		return -ENODEV;
	mutex_lock(&dev_tbl_mutex);
	if (dev_tbl[n] != NULL){
		if (mutex_trylock(&dev_tbl[n]->mutex)){
			for (i = 0; i < ASIZE(usbkey_ioctls); i++){
				if (ioctl_num == usbkey_ioctls[i].ioctl_num){
					retval = usbkey_ioctls[i].ioctl_fn(dev_tbl[n]);
					break;
				}
			}
			mutex_unlock(&dev_tbl[n]->mutex);
		}else
			retval = -EWOULDBLOCK;
	}
	mutex_unlock(&dev_tbl_mutex);
	return retval;
}

/* �����⨬� ����樨 ��� /dev/usbkey */
static struct file_operations usbkey_fops = {
	.owner		= THIS_MODULE,
	.read		= usbkey_read,
	.write		= usbkey_write,
	.ioctl		= usbkey_ioctl,
	.open		= usbkey_open,
	.release	= usbkey_release,
};      

static struct usb_class_driver usbkey_class = {
	.name		= "usbkey%d",
	.fops		= &usbkey_fops,
	.minor_base	= USBKEY_MINOR_BASE
};

/* ��뢠���� �����⥬�� USB �� ������祭�� ������ ���ன�⢠ */
static int usbkey_probe(struct usb_interface *interface,
		const struct usb_device_id *id)
{
	struct usbkey *dev = NULL;
	struct usb_host_interface *iface;
	struct usb_host_endpoint *endpoint;
	int minor, i, ret;
	mutex_lock(&dev_tbl_mutex);
	for (minor = 0; minor < MAX_DEVICES; ++minor) {
		if (dev_tbl[minor] == NULL)
			break;
	}
	if (minor >= MAX_DEVICES){
		err("Too many devices plugged in, can not handle this device.");
		mutex_unlock(&dev_tbl_mutex);
		return -ENOMEM;
	}
	dev = kzalloc(sizeof(struct usbkey), GFP_KERNEL);
	if (dev == NULL){
		err("Out of memory");
		mutex_unlock(&dev_tbl_mutex);
		return -ENOMEM;
	}
	dev_tbl[minor] = dev;
	mutex_init(&dev->mutex);
	dev->dev = interface_to_usbdev(interface);
	dev->udev = usb_get_dev(dev->dev);
	dev->minor = minor;
/*	usb_driver_set_configuration(dev->udev, 1);*/
	iface = interface->cur_altsetting;
	for (i = 0; i < iface->desc.bNumEndpoints; ++i){
		endpoint = iface->endpoint + i;
		if (((endpoint->desc.bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) &&
		    ((endpoint->desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)){
			dev->read_urb = usb_alloc_urb(0, GFP_KERNEL);
			if (dev->read_urb == NULL){
				err("No free URBs available");
				usbkey_delete (dev);
				dev = NULL;
				mutex_unlock(&dev_tbl_mutex);
				return -ENOMEM;
			}		
			dev->in_size = endpoint->desc.wMaxPacketSize;
			dev->in_endpointAddr = endpoint->desc.bEndpointAddress;
			dev->in_data = kmalloc(dev->in_size, GFP_KERNEL);
			if (dev->in_data == NULL){
				err("Cannot allocate in_data");
				usbkey_delete (dev);
				dev = NULL;
				mutex_unlock(&dev_tbl_mutex);
				return -ENOMEM;
			}
			dev->rx_interval = endpoint->desc.bInterval;
		}else if (((endpoint->desc.bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT) &&
				((endpoint->desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)){
			dev->write_urb = usb_alloc_urb(0, GFP_KERNEL);
			if (dev->write_urb == NULL){
				err("No free URBs available");
				usbkey_delete (dev);
				dev = NULL;
				mutex_unlock(&dev_tbl_mutex);
				return -ENOMEM;
			}
			dev->out_size = endpoint->desc.wMaxPacketSize;
			dev->out_endpointAddr = endpoint->desc.bEndpointAddress;
			dev->out_data = kmalloc(dev->out_size, GFP_KERNEL);
			if (dev->out_data == NULL){
				err("Cannot allocate out_data");
				usbkey_delete (dev);
				dev = NULL;
				mutex_unlock(&dev_tbl_mutex);
				return -ENOMEM;
			}
			dev->tx_interval = endpoint->desc.bInterval;
		}
	}
	init_waitqueue_head(&dev->waitq);
	dev->present = true;
	init_timer(&dev->timer);
	dev->timer.data = (unsigned long)dev;
	dev->timer.function = timer_fn;
	usb_set_intfdata(interface, dev);
	ret = usb_register_dev(interface, &usbkey_class);
	if (ret == 0)
		info("USB Key device now attached to /dev/usbkey%d", dev->minor);
	else{
		err("usb_register_dev failed with code %d", ret);
		usbkey_delete(dev);
	}
	mutex_unlock(&dev_tbl_mutex);
	return ret;
}

/* ��뢠���� �����⥬�� USB �� �⪫�祭�� ���ன�⢠ */
static void usbkey_disconnect(struct usb_interface *interface)
{
	int minor;
	struct usbkey *dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);
	usb_deregister_dev(interface, &usbkey_class);
	mutex_lock(&dev_tbl_mutex);
	mutex_lock(&dev->mutex);
	minor = dev->minor;
	dev->present = false;
	usb_put_dev(dev->udev);
	dev->udev = NULL;
	mutex_unlock(&dev->mutex);
	if (dev->open_count <= 0)
		usbkey_delete(dev);
	mutex_unlock(&dev_tbl_mutex);
	info("USB Key #%d now disconnected", minor);
}

/* ������ ���ன��, ���㦨������ �ࠩ��஬ */
static struct usb_device_id usbkey_table [] = {
	{ USB_DEVICE(USBKEY_VENDOR_ID, USBKEY_PRODUCT_ID) },
	{ }
};

MODULE_DEVICE_TABLE(usb, usbkey_table);

/* ����室��� ��� ॣ����樨 ����� �����⥬�� USB */
static struct usb_driver usbkey_driver = {
	.name		= MODULE_NAME,
	.probe		= usbkey_probe,
	.disconnect	= usbkey_disconnect,
	.id_table	= usbkey_table,
};

/* ���樠������ ����� */
int init_module(void)
{
	int ret = usb_register(&usbkey_driver);
	if (ret < 0){
		err("usb_register failed for the "__FILE__" driver. "
				"Error number %d", ret);
		return -EPERM;
	}else{
		mutex_init(&dev_tbl_mutex);
		info(DRIVER_DESC " " DRIVER_VERSION);
		return 0;
	}
}


/* ���㧪� ����� */
void cleanup_module(void)
{
	usb_deregister(&usbkey_driver);
	mutex_destroy(&dev_tbl_mutex);
}
