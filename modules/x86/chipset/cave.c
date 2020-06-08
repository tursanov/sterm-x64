/*
 * Функции, специфичные для старых адаптеров BSC на основе КР580ВВ55А.
 * (c) gsr 2003
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/io.h>		/* I/O port macros */
#include <linux/delay.h>	/* for udelay() declaration */
#include <linux/ioport.h>
#include "sysdefs.h"
#include "sys/chipset.h"
#include "sys/ioctls.h"
#include "sys/ppio.h"

/* Общие характеристики */
static char *chipset_name = "Express old PPIO card";
#define IO_BASE		0x3a0
#define IO_EXTENT	16

/* Базовый адрес ввода/вывода КР580ВВ55А */
#define PPIO_BASE	0x3a0

/* Битовые константы */
/* Регистр A (чтение) */
#define PPIO_COMM_RDY	0x01		/* ГОТОВНОСТЬ от коммутатора */
#define PPIO_PRN_DRQ	0x02		/* DRQ (ACK) от принтера */
#define PPIO_PRN_BUSY	0x04		/* BUSY от принтера */

/* Регистр C (чтение) */
#define PPIO_WACK	0x10		/* WACK от принтера (не используется) */
#define PPIO_DLSK	0x80		/* DLSK */

/* Управляющий регистр (запись) */
#define PPIO_REQ	0		/* ЗАКАЗ для коммутатора */
#define PPIO_SPR	1		/* SPR для принтера */
#define PPIO_RPR	2		/* RPR для принтера */
#define PPIO_DLSO	3		/* DLSO DS1990A */

/* КР580ВВ51А */
#define USART_IO_BASE	0x3a8
#define	USART_DTA	USART_IO_BASE + 0		/* данные (чтение/запись) */
#define USART_CTL	USART_IO_BASE + 1		/* управляющий регистр */
#define USART_IRQ	5		/* запрос на прерывание */

/* BSC */
/* Чтение байта данных */
uint8_t cave_bsc_read_dta(void)
{
	return inb_p(USART_DTA);
}

/* Запись байта данных */
void cave_bsc_write_dta(uint8_t b)
{
	outb_p(b, USART_DTA);
}

/* Чтение регистра состояния */
uint8_t cave_bsc_read_ctl(void)
{
	return inb_p(USART_CTL);
}

/* Запись регистра состояния */
void cave_bsc_write_ctl(uint8_t b)
{
	outb_p(b, USART_CTL);
}

/* Чтение состояния линии ГОТОВНОСТЬ коммутатора */
bool cave_bsc_GOT(void)
{
	return (inb_p(PPIO_BASE + PPIO_REGA) & PPIO_COMM_RDY) == 0;
}

/* Установка линии ЗАКАЗ коммутатора */
void cave_bsc_ZAK(bool set)
{
	outb_p(PPIO_CTL_BITS(PPIO_REQ, !set), PPIO_BASE + PPIO_CTL);
}

/* Принтер */
/* Запись байта данных */
void cave_prn_write_data(uint8_t b)
{
	outb_p(b, PPIO_BASE + PPIO_REGB);
}

/* Чтение шины BUSY */
bool cave_prn_BUSY(void)
{
	return (inb_p(PPIO_BASE + PPIO_REGA) & PPIO_PRN_BUSY) == 0;
}

/* Чтение шины DRQ (ACK) */
bool cave_prn_DRQ(void)
{
	return (inb_p(PPIO_BASE + PPIO_REGA) & PPIO_PRN_DRQ) == 0;
}

/* Установка шины RPR */
void cave_prn_RPR(bool set)
{
	outb_p(PPIO_CTL_BITS(PPIO_RPR, !set), PPIO_BASE + PPIO_CTL);
}

/* Установка шины SPR */
void cave_prn_SPR(bool set)
{
	outb_p(PPIO_CTL_BITS(PPIO_SPR, set), PPIO_BASE + PPIO_CTL);
}

/* Ключи DS1990A */
/* Установка линии DLSO */
void cave_dallas_DLSO(bool set)
{
	outb_p(PPIO_CTL_BITS(PPIO_DLSO, set), PPIO_BASE + PPIO_CTL);
}

/* Чтение линии DLSK */
bool cave_dallas_DLSK(void)
{
	return (inb_p(PPIO_BASE + PPIO_REGC) & PPIO_DLSK) == 0;
}

/* Установка структуры с описанием адаптера */
void cave_set_metrics(struct chipset_metrics *cm)
{
	if (cm != NULL){
		cm->name = chipset_name;
		cm->io_base = IO_BASE;
		cm->io_extent = IO_EXTENT;
		cm->bsc_irq = USART_IRQ;
		cm->bsc_read_dta = cave_bsc_read_dta;
		cm->bsc_write_dta = cave_bsc_write_dta;
		cm->bsc_read_ctl = cave_bsc_read_ctl;
		cm->bsc_write_ctl = cave_bsc_write_ctl;
		cm->bsc_GOT = cave_bsc_GOT;
		cm->bsc_ZAK = cave_bsc_ZAK;
		cm->prn_write_data = cave_prn_write_data;
		cm->prn_BUSY = cave_prn_BUSY;
		cm->prn_DRQ = cave_prn_DRQ;
		cm->prn_RPR = cave_prn_RPR;
		cm->prn_SPR = cave_prn_SPR;
		cm->dallas_DLSO = cave_dallas_DLSO;
		cm->dallas_DLSK = cave_dallas_DLSK;
	}
}

/* Проверка наличия адаптера */
bool cave_probe(void)
{
/* Прежде всего мы должны проинициализировать КР580ВВ55А */
	outb_p(PPIO_MAGIC, PPIO_BASE + PPIO_CTL);
	udelay(100);
/* Закрываем DLSO */
	cave_dallas_DLSO(false);
	mdelay(10);
/* DLSK должен иметь высокий уровень */
	if (cave_dallas_DLSK()){
/* Открываем DLSO */
		cave_dallas_DLSO(true);
		udelay(100);
/* DLSK должен иметь низкий уровень */
		if (!cave_dallas_DLSK()){
/* Чипсет обнаружен. Восстанавливаем исходное состояние */
			cave_dallas_DLSO(false);
			mdelay(10);
			cave_bsc_write_ctl(0);
			udelay(10);
			cave_bsc_write_ctl(0);
			udelay(10);
			cave_bsc_write_ctl(0);
			udelay(10);
			cave_bsc_write_ctl(0x40);
			mdelay(10);
			return true;
		}
	}
	return false;
}
