/*
 * Модуль для чтения ключей DALLAS DS1990A.
 * (c) gsr 2003
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/uaccess.h>
#include <linux/delay.h>	/* for udelay() declaration */
#include <linux/fs.h>
#include <linux/spinlock.h>
#include "sysdefs.h"
#include "sys/chipset.h"
#include "sys/ioctls.h"

/* Описание модуля */
MODULE_AUTHOR("Sergey Tursanov");
MODULE_DESCRIPTION("DS1990A key read");
MODULE_SUPPORTED_DEVICE("DS1990A ROMs");
MODULE_LICENSE("GPL");
#define DEVICE_NAME	"dallas"

#define DALLAS_ID_LEN			8
/* Задержки в мкс для работы с DS1990A */
/* Инициализация */
#define DALLAS_INIT_STROBE_DELAY	500
#define DALLAS_INIT_RELAX_DELAY		DALLAS_INIT_STROBE_DELAY
#define DALLAS_INIT_WRESP_DELAY		50
/* Чтение */
#define DALLAS_READ_STROBE_DELAY	1
#define DALLAS_READ_WRESP_DELAY		1
#define DALLAS_READ_RELAX_DELAY		50
/* Запись */
#define DALLAS_WRITE_STROBE_DELAY	DALLAS_READ_STROBE_DELAY
#define DALLAS_WRITE_DELAY		50
#define DALLAS_WRITE_RELAX_DELAY	5

DEFINE_SPINLOCK(dallas_spin_lock);
static unsigned long cpu_flags = 0;
#define dallas_cli() spin_lock_irqsave(&dallas_spin_lock, cpu_flags)
#define dallas_sti() spin_unlock_irqrestore(&dallas_spin_lock, cpu_flags)

static void open_DLSO(void)
{
	cm.dallas_DLSO(true);
}

static void close_DLSO(void)
{
	cm.dallas_DLSO(false);
}

/* Определение наличия ключа */
static bool dallas_has_key(void)
{
	bool flag;
	open_DLSO();
	udelay(DALLAS_INIT_STROBE_DELAY);
	dallas_cli();
	close_DLSO();
	udelay(DALLAS_INIT_WRESP_DELAY);
	flag = !cm.dallas_DLSK();
	dallas_sti();
	udelay(DALLAS_INIT_RELAX_DELAY);
	return flag;
}

/* Чтение байта DS1990A */
static uint8_t dallas_read_byte(void)
{
	uint8_t b=0;
	int i;
	for (i = 0; i < 8; i++){
		b >>= 1;
		dallas_cli();
		open_DLSO();
		udelay(DALLAS_READ_STROBE_DELAY);
		close_DLSO();
		udelay(DALLAS_READ_WRESP_DELAY);
		if (cm.dallas_DLSK())
			b |= 0x80;
		dallas_sti();
		udelay(DALLAS_READ_RELAX_DELAY);
	}
	return b;
}

/* Запись байта DS1990A */
static void dallas_write_byte(uint8_t b)
{
	int i;
	for (i=0; i < 8; i++){
		dallas_cli();
		open_DLSO();
		udelay(DALLAS_WRITE_STROBE_DELAY);
		if (b & 1)
			close_DLSO();
		dallas_sti();
		udelay(DALLAS_WRITE_DELAY);
		close_DLSO();
		b >>= 1;
		udelay(DALLAS_WRITE_RELAX_DELAY);
	}
}

/* Подсчет контрольной суммы DALLAS */
static uint8_t dallas_crc(uint8_t *p,int l)
{
#define DOW_B0	0
#define DOW_B1	4
#define DOW_B2	5
	uint8_t b, s = 0;
	uint8_t m0 = 1 << (7-DOW_B0);
	uint8_t m1 = 1 << (7-DOW_B1);
	uint8_t m2 = 1 << (7-DOW_B2);
	int i, j;
	if (p != NULL){
		for (i = 0; i < l; i++){
			b = p[i];
			for (j = 0; j < 8; j++){
				b ^= (s & 0x01);
				s >>= 1;
				if (b & 0x01)
					s ^= (m0 | m1 | m2);
				b >>= 1;
			}
		}
	}
	return s;
}

/*
 * Если в считанном номере DALLAS все байты равны 0, то CRC совпадет,
 * но это неверный результат.
 */
static bool dallas_zero_id(uint8_t id[DALLAS_ID_LEN])
{
	int i;
	for (i = 0; i < DALLAS_ID_LEN; i++)
		if (id[i] != 0)
			return false;
	return true;
}

#define DALLAS_READ_TRIES	10

/* Чтение ключа DALLAS. Возвращает true, если изменилась информация о ключе. */
static bool dallas_read_key(uint8_t id[DALLAS_ID_LEN])
{
	int i, j;
	for (i = 0; i < DALLAS_READ_TRIES; i++){
		if (dallas_has_key()){
//			dallas_write_byte(0x0f);
			dallas_write_byte(0x33);
			for (j = 0; j < DALLAS_ID_LEN; j++)
				id[j] = dallas_read_byte();
			if (!dallas_zero_id(id) &&
					!dallas_crc(id, DALLAS_ID_LEN))
				return true;
		}
	}
	memset(id, 0, DALLAS_ID_LEN);
	return false;
}

/* ioctl */
static int dallas_io_read(int param)
{
	int ret;
	uint8_t id[DALLAS_ID_LEN];
	ret = !dallas_read_key(id);
	if (copy_to_user((uint8_t *)param, id, DALLAS_ID_LEN) != 0)
		ret = -EIO;
	return ret;
}

static int dallas_ioctl(struct inode *inode, struct file *file,
		unsigned int ioctl_num, unsigned long param)
{
	static struct ioctl_proc{
		int ioctl_num;
		int (*ioctl_fn)(int);
	} ioctls[]={
		{DALLAS_IO_READ,	dallas_io_read},
	};
	int i;
	for (i=0; i < ASIZE(ioctls); i++)
		if (ioctls[i].ioctl_num == ioctl_num)
			return ioctls[i].ioctl_fn(param);
	return -ENOSYS;
}

static int dallas_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int dallas_close(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations dallas_fops = {
	owner:			THIS_MODULE,
	ioctl:			dallas_ioctl,
	open:			dallas_open,
	release:		dallas_close,
};

int init_module(void)
{
	if (register_chrdev(DALLAS_MAJOR, DEVICE_NAME, &dallas_fops) < 0){
		printk("%s: module_register_chrdev failed\n", DEVICE_NAME);
		return -1;
	}else
		return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(DALLAS_MAJOR, DEVICE_NAME);
}
