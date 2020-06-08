/*
 * �㭪樨, ᯥ���� ��� �����஢ BSC �� �᭮�� ��580��55�.
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

/* ��騥 �ࠪ���⨪� */
static char *chipset_name = "Express PPIO card";
#define IO_BASE		0x3a0
#define IO_EXTENT	16

/* ������ ���� �����/�뢮�� ��580��55� */
#define PPIO_BASE	0x3a4

/* ��⮢� ����⠭�� */
/* ������� A (�⥭��) */
#define PPIO_COMM_RDY	0x01		/* ���������� �� �������� */
#define PPIO_PRN_DRQ	0x02		/* DRQ (ACK) �� �ਭ�� */
#define PPIO_PRN_BUSY	0x04		/* BUSY �� �ਭ�� */

/* ������� C (�⥭��) */
#define PPIO_WACK	0x10		/* WACK �� �ਭ�� (�� �ᯮ������) */
#define PPIO_DLSK	0x80		/* DLSK */

/* ��ࠢ���騩 ॣ���� (������) */
#define PPIO_REQ	0		/* ����� ��� �������� */
#define PPIO_SPR	1		/* SPR ��� �ਭ�� */
#define PPIO_RPR	2		/* RPR ��� �ਭ�� */
#define PPIO_DLSO	3		/* DLSO DS1990A */

/* ��580��51� */
#define USART_IO_BASE	0x3a0
#define USART_DTA	USART_IO_BASE + 0		/* ����� (�⥭��/������) */
#define USART_CTL	USART_IO_BASE + 1		/* �ࠢ���騩 ॣ���� */
#define USART_IRQ	5		/* ����� �� ���뢠��� */

/* BSC */
/* �⥭�� ���� ������ */
uint8_t ppio_bsc_read_dta(void)
{
	return inb_p(USART_DTA);
}

/* ������ ���� ������ */
void ppio_bsc_write_dta(uint8_t b)
{
	outb_p(b, USART_DTA);
}

/* �⥭�� ॣ���� ���ﭨ� */
uint8_t ppio_bsc_read_ctl(void)
{
	return inb_p(USART_CTL);
}

/* ������ ॣ���� ���ﭨ� */
void ppio_bsc_write_ctl(uint8_t b)
{
	outb_p(b, USART_CTL);
}

/* �⥭�� ���ﭨ� ����� ���������� �������� */
bool ppio_bsc_GOT(void)
{
	return (inb_p(PPIO_BASE + PPIO_REGA) & PPIO_COMM_RDY) == 0;
}

/* ��⠭���� ����� ����� �������� */
void ppio_bsc_ZAK(bool set)
{
	outb_p(PPIO_CTL_BITS(PPIO_REQ, !set), PPIO_BASE + PPIO_CTL);
}

/* �ਭ�� */
/* ������ ���� ������ */
void ppio_prn_write_data(uint8_t b)
{
	outb_p(b, PPIO_BASE + PPIO_REGB);
}

/* �⥭�� 設� BUSY */
bool ppio_prn_BUSY(void)
{
	return (inb_p(PPIO_BASE + PPIO_REGA) & PPIO_PRN_BUSY) == 0;
}

/* �⥭�� 設� DRQ (ACK) */
bool ppio_prn_DRQ(void)
{
	return (inb_p(PPIO_BASE + PPIO_REGA) & PPIO_PRN_DRQ) == 0;
}

/* ��⠭���� 設� RPR */
void ppio_prn_RPR(bool set)
{
	outb_p(PPIO_CTL_BITS(PPIO_RPR, !set), PPIO_BASE + PPIO_CTL);
}

/* ��⠭���� 設� SPR */
void ppio_prn_SPR(bool set)
{
	outb_p(PPIO_CTL_BITS(PPIO_SPR, set), PPIO_BASE + PPIO_CTL);
}

/* ���� DS1990A */
/* ��⠭���� ����� DLSO */
void ppio_dallas_DLSO(bool set)
{
	outb_p(PPIO_CTL_BITS(PPIO_DLSO, set), PPIO_BASE + PPIO_CTL);
}

/* �⥭�� ����� DLSK */
bool ppio_dallas_DLSK(void)
{
	return (inb_p(PPIO_BASE + PPIO_REGC) & PPIO_DLSK) == 0;
}

/* ��⠭���� �������� � ���ᠭ��� ������ */
void ppio_set_metrics(struct chipset_metrics *cm)
{
	if (cm != NULL){
		cm->name = chipset_name;
		cm->io_base = IO_BASE;
		cm->io_extent = IO_EXTENT;
		cm->bsc_irq = USART_IRQ;
		cm->bsc_read_dta = ppio_bsc_read_dta;
		cm->bsc_write_dta = ppio_bsc_write_dta;
		cm->bsc_read_ctl = ppio_bsc_read_ctl;
		cm->bsc_write_ctl = ppio_bsc_write_ctl;
		cm->bsc_GOT = ppio_bsc_GOT;
		cm->bsc_ZAK = ppio_bsc_ZAK;
		cm->prn_write_data = ppio_prn_write_data;
		cm->prn_BUSY = ppio_prn_BUSY;
		cm->prn_DRQ = ppio_prn_DRQ;
		cm->prn_RPR = ppio_prn_RPR;
		cm->prn_SPR = ppio_prn_SPR;
		cm->dallas_DLSO = ppio_dallas_DLSO;
		cm->dallas_DLSK = ppio_dallas_DLSK;
	}
}

/* �஢�ઠ ������ ������ */
bool ppio_probe(void)
{
/* �०�� �ᥣ� �� ������ �ந��樠����஢��� ��580��55� */
	outb_p(PPIO_MAGIC, PPIO_BASE + PPIO_CTL);
	udelay(100);
/* ����뢠�� DLSO */
	ppio_dallas_DLSO(false);
	mdelay(10);
/* DLSK ������ ����� ��᮪�� �஢��� */
	if (ppio_dallas_DLSK()){
/* ���뢠�� DLSO */
		ppio_dallas_DLSO(true);
		udelay(100);
/* DLSK ������ ����� ������ �஢��� */
		if (!ppio_dallas_DLSK()){
/* ����� �����㦥�. ����⠭�������� ��室��� ���ﭨ� */
			ppio_dallas_DLSO(false);
			mdelay(10);
			ppio_bsc_write_ctl(0);
			udelay(10);
			ppio_bsc_write_ctl(0);
			udelay(10);
			ppio_bsc_write_ctl(0);
			udelay(10);
			ppio_bsc_write_ctl(0x40);
			mdelay(10);
			return true;
		}
	}
	return false;
}
