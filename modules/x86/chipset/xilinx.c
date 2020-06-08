/*
 * �㭪樨, ᯥ���� ��� ISA �����஢ BSC �� �᭮�� XILINX.
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

/* ��騥 �ࠪ���⨪� */
static char *chipset_name = "Express XILINX card";
#define IO_BASE		0x3a0
#define IO_EXTENT	16
extern int xilinx_for_kkm;

/* �������� �����/�뢮�� */
#define XILINX_IO_BASE	0x3a4
#define XILINX_CTLW	0	/* �ࠢ���騩 ॣ���� (������) */
#define XILINX_DATA	1	/* ����� ��� �ਭ�� */
/*
 * ����� ����� ⠪�� ��⠭����� �१ XILINX_CTLW. XILINX_AUX �� ������
 * � � 2000 �. �� �⫠��� �����⬠ ����.
 */
#define XILINX_AUX	4	/* ����� ��� �������� */
#define XILINX_CTLR	8	/* �ࠢ���騩 ॣ���� (�⥭��) */

/* ��⮢� ����⠭�� ��� ����� */
#define XILINX_REQ	0			/* ����� ��� �������� */
#define XILINX_SPR	1			/* SPR ��� �ਭ�� */
#define XILINX_RPR	2			/* RPR ��� �ਭ�� */
#define XILINX_DLSO	3			/* DLSO ��� DS1990A */
/*
 * � ������� �� �᭮�� ��580��55� �뢮� RST ��580��51� �� �ᯮ������.
 * ����� ���� �.�.�䠭��쥢 �����뢠� � ॣ���� �� �㫥��� ���� �����,
 * ��᫥ 祣� �����奬� ���室��� � ०�� �������� �������.
 */
#define XILINX_RST51	4			/* ��� ��580��51� */

/* ��⮢� ��᪨ ��� �⥭�� */
#define XILINX_DLSK	0x01			/* DLSK ��� DS1990A */
#define XILINX_READY	0x02			/* ���������� �� �������� */
#define XILINX_ACK	0x04			/* DRQ (ACK) �� �ਭ�� */
#define XILINX_BUSY	0x08			/* BUSY �� �ਭ�� */
#define XILINX_PE	0x10			/* PE �� �ਭ�� (�� �ᯮ������) */

/* ��580��51� */
#define USART_IO_BASE	0x3a0
#define USART_DTA	USART_IO_BASE + 0	/* ����� (�⥭��/������) */
#define USART_CTL	USART_IO_BASE + 1	/* �ࠢ���騩 ॣ���� */
#define USART_IRQ	5		/* ����� �� ���뢠��� */

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
/* �⥭�� ���� ������ */
uint8_t xilinx_bsc_read_dta(void)
{
	return inb_p(USART_DTA);
}

/* ������ ���� ������ */
void xilinx_bsc_write_dta(uint8_t b)
{
	outb_p(b, USART_DTA);
}

/* �⥭�� ॣ���� ���ﭨ� */
uint8_t xilinx_bsc_read_ctl(void)
{
	return inb_p(USART_CTL);
}

/* ������ ॣ���� ���ﭨ� */
void xilinx_bsc_write_ctl(uint8_t b)
{
	outb_p(b, USART_CTL);
}

/* �⥭�� ���ﭨ� ����� ���������� �������� */
bool xilinx_bsc_GOT(void)
{
	return (inb_p(XILINX_IO_BASE + XILINX_CTLR) & XILINX_READY) == 0;
}

/* ��⠭���� ����� ����� �������� */
void xilinx_bsc_ZAK(bool set)
{
	if (xilinx_for_kkm)
		xilinx_bit(XILINX_REQ, !set);
	else
		outb_p(!set, XILINX_IO_BASE + XILINX_AUX);
}

/* �ਭ�� */
/* ������ ���� ������ */
void xilinx_prn_write_data(uint8_t b)
{
	outb_p(b, XILINX_IO_BASE + XILINX_DATA);
}

/* �⥭�� 設� BUSY */
bool xilinx_prn_BUSY(void)
{
	return (inb_p(XILINX_IO_BASE + XILINX_CTLR) & XILINX_BUSY) == 0;
}

/* �⥭�� 設� DRQ (ACK) */
bool xilinx_prn_DRQ(void)
{
	return (inb_p(XILINX_IO_BASE + XILINX_CTLR) & XILINX_ACK) == 0;
}

/* ��⠭���� 設� RPR */
void xilinx_prn_RPR(bool set)
{
	xilinx_bit(XILINX_RPR, set);
}

/* ��⠭���� 設� SPR */
void xilinx_prn_SPR(bool set)
{
	xilinx_bit(XILINX_SPR, !set);
}

/* ���� DS1990A */
/* ��⠭���� ����� DLSO */
void xilinx_dallas_DLSO(bool set)
{
	xilinx_bit(XILINX_DLSO, !set);
}

/* �⥭�� ����� DLSK */
bool xilinx_dallas_DLSK(void)
{
	return (inb_p(XILINX_IO_BASE + XILINX_CTLR) & XILINX_DLSK) != 0;
}

/* ��⠭���� �������� � ���ᠭ��� ������ */
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

/* �஢�ઠ ������ ������ */
bool xilinx_probe(void)
{
/* ����뢠�� DLSO */
	xilinx_dallas_DLSO(false);
	mdelay(10);
/* DLSK ������ ����� ��᮪�� �஢��� */
	if (xilinx_dallas_DLSK()){
/* ���뢠�� DLSO */
		xilinx_dallas_DLSO(true);
		udelay(100);
/* DLSK ������ ����� ������ �஢��� */
		if (!xilinx_dallas_DLSK()){
/* ����� �����㦥�. ����⠭�������� ��室��� ���ﭨ� */
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
