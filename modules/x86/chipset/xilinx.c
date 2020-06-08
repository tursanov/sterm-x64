/*
 * Функции, специфичные для ISA адаптеров BSC на основе XILINX.
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

/* Общие характеристики */
static char *chipset_name = "Express XILINX card";
#define IO_BASE		0x3a0
#define IO_EXTENT	16
extern int xilinx_for_kkm;

/* Регистры ввода/вывода */
#define XILINX_IO_BASE	0x3a4
#define XILINX_CTLW	0	/* управляющий регистр (запись) */
#define XILINX_DATA	1	/* данные для принтера */
/*
 * ЗАКАЗ можно также установить через XILINX_CTLW. XILINX_AUX был введен
 * в июне 2000 г. при отладке алгоритма печати.
 */
#define XILINX_AUX	4	/* ЗАКАЗ для коммутатора */
#define XILINX_CTLR	8	/* управляющий регистр (чтение) */

/* Битовые константы для записи */
#define XILINX_REQ	0			/* ЗАКАЗ для коммутатора */
#define XILINX_SPR	1			/* SPR для принтера */
#define XILINX_RPR	2			/* RPR для принтера */
#define XILINX_DLSO	3			/* DLSO для DS1990A */
/*
 * В адаптерах на основе КР580ВВ55А вывод RST КР580ВВ51А не используется.
 * Вместо него В.Г.Афанасьев записывал в регистр три нулевых байта подряд,
 * после чего микросхема переходила в режим ожидания команды.
 */
#define XILINX_RST51	4			/* сброс КР580ВВ51А */

/* Битовые маски для чтения */
#define XILINX_DLSK	0x01			/* DLSK для DS1990A */
#define XILINX_READY	0x02			/* ГОТОВНОСТЬ от коммутатора */
#define XILINX_ACK	0x04			/* DRQ (ACK) от принтера */
#define XILINX_BUSY	0x08			/* BUSY от принтера */
#define XILINX_PE	0x10			/* PE от принтера (не используется) */

/* КР580ВВ51А */
#define USART_IO_BASE	0x3a0
#define USART_DTA	USART_IO_BASE + 0	/* данные (чтение/запись) */
#define USART_CTL	USART_IO_BASE + 1	/* управляющий регистр */
#define USART_IRQ	5		/* запрос на прерывание */

/* XILINX */
static uint8_t xilinx_ctl = 0;

static void xilinx_bit(int bit, int v)
{
	if ((bit >= 0) && (bit < 8)){
		if (v)
			xilinx_ctl |= (1 << bit);
		else
			xilinx_ctl &= ~(1 << bit);
		outb_p(xilinx_ctl, XILINX_IO_BASE + XILINX_CTLW);
	}
}

/* BSC */
/* Чтение байта данных */
uint8_t xilinx_bsc_read_dta(void)
{
	return inb_p(USART_DTA);
}

/* Запись байта данных */
void xilinx_bsc_write_dta(uint8_t b)
{
	outb_p(b, USART_DTA);
}

/* Чтение регистра состояния */
uint8_t xilinx_bsc_read_ctl(void)
{
	return inb_p(USART_CTL);
}

/* Запись регистра состояния */
void xilinx_bsc_write_ctl(uint8_t b)
{
	outb_p(b, USART_CTL);
}

/* Чтение состояния линии ГОТОВНОСТЬ коммутатора */
bool xilinx_bsc_GOT(void)
{
	return (inb_p(XILINX_IO_BASE + XILINX_CTLR) & XILINX_READY) == 0;
}

/* Установка линии ЗАКАЗ коммутатора */
void xilinx_bsc_ZAK(bool set)
{
	if (xilinx_for_kkm)
		xilinx_bit(XILINX_REQ, !set);
	else
		outb_p(!set, XILINX_IO_BASE + XILINX_AUX);
}

/* Принтер */
/* Запись байта данных */
void xilinx_prn_write_data(uint8_t b)
{
	outb_p(b, XILINX_IO_BASE + XILINX_DATA);
}

/* Чтение шины BUSY */
bool xilinx_prn_BUSY(void)
{
	return (inb_p(XILINX_IO_BASE + XILINX_CTLR) & XILINX_BUSY) == 0;
}

/* Чтение шины DRQ (ACK) */
bool xilinx_prn_DRQ(void)
{
	return (inb_p(XILINX_IO_BASE + XILINX_CTLR) & XILINX_ACK) == 0;
}

/* Установка шины RPR */
void xilinx_prn_RPR(bool set)
{
	xilinx_bit(XILINX_RPR, set);
}

/* Установка шины SPR */
void xilinx_prn_SPR(bool set)
{
	xilinx_bit(XILINX_SPR, !set);
}

/* Ключи DS1990A */
/* Установка линии DLSO */
void xilinx_dallas_DLSO(bool set)
{
	xilinx_bit(XILINX_DLSO, !set);
}

/* Чтение линии DLSK */
bool xilinx_dallas_DLSK(void)
{
	return (inb_p(XILINX_IO_BASE + XILINX_CTLR) & XILINX_DLSK) != 0;
}

/* Установка структуры с описанием адаптера */
void xilinx_set_metrics(struct chipset_metrics *cm)
{
	if (cm != NULL){
		cm->name = chipset_name;
		cm->io_base = IO_BASE;
		cm->io_extent = IO_EXTENT;
		cm->bsc_irq = USART_IRQ;
		cm->bsc_read_dta = xilinx_bsc_read_dta;
		cm->bsc_write_dta = xilinx_bsc_write_dta;
		cm->bsc_read_ctl = xilinx_bsc_read_ctl;
		cm->bsc_write_ctl = xilinx_bsc_write_ctl;
		cm->bsc_GOT = xilinx_bsc_GOT;
		cm->bsc_ZAK = xilinx_bsc_ZAK;
		cm->prn_write_data = xilinx_prn_write_data;
		cm->prn_BUSY = xilinx_prn_BUSY;
		cm->prn_DRQ = xilinx_prn_DRQ;
		cm->prn_RPR = xilinx_prn_RPR;
		cm->prn_SPR = xilinx_prn_SPR;
		cm->dallas_DLSO = xilinx_dallas_DLSO;
		cm->dallas_DLSK = xilinx_dallas_DLSK;
	}
}

/* Проверка наличия адаптера */
bool xilinx_probe(void)
{
/* Закрываем DLSO */
	xilinx_dallas_DLSO(false);
	mdelay(10);
/* DLSK должен иметь высокий уровень */
	if (xilinx_dallas_DLSK()){
/* Открываем DLSO */
		xilinx_dallas_DLSO(true);
		udelay(100);
/* DLSK должен иметь низкий уровень */
		if (!xilinx_dallas_DLSK()){
/* Чипсет обнаружен. Восстанавливаем исходное состояние */
			xilinx_dallas_DLSO(false);
			mdelay(10);
			xilinx_bit(XILINX_RST51, 1);
			udelay(10);
			xilinx_bit(XILINX_RST51, 0);
			mdelay(10);
			return true;
		}
	}
	return false;
}
