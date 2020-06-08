/*
 * ��⮬���᪮� ��।������ ⨯� ������ BSC.
 * � ���㫥 chipset ��室���� �� �㭪樨, ᯥ����
 * ��� �����⭮�� ������.
 * (c) gsr & �.�.�䠭��쥢 2003
 */

#if !defined SYS_CHIPSET_H
#define SYS_CHIPSET_H

#include "sysdefs.h"

/* ��� �ᯮ��㥬��� ������ BSC */
enum {
	chipsetUNKNOWN = 0,	/* ��������� */
	chipsetCAVE,		/* �� �᭮�� ��580��55� (���� ��ਠ��) */
	chipsetPPIO,		/* �� �᭮�� ��580��55� */
	chipsetXILINX,		/* �� �᭮�� XILINX */
	chipsetPCI,		/* ������ PCI */
	chipsetCARDLESS,	/* ��� ������ BSC */
	chipsetSTRIDER,		/* Win32 */
};

/*
 * �ᯮ��㥬� ������ ����뢠���� ᯥ樠�쭮� ������ன,
 * � ���ன ᮤ�ঠ��� �� �����, ����室��� ��� ࠡ��� � ���.
 * � ���㫥 �� ᮮ⢥�����騥 ���� �������� �ᯮ������� �������,
 * � � ���짮��⥫�᪨� �ணࠬ��� -- �१ IOCTL.
 */

/* ���⨯� �㭪権 ��� ࠡ��� � �����஬ */
/* BSC */
/* �⥭�� ���� ������ */
typedef uint8_t (*bsc_read_dta_t)(void);
/* ������ ���� ������ */
typedef void (*bsc_write_dta_t)(uint8_t);
/* �⥭�� ॣ���� ���ﭨ� */
typedef uint8_t (*bsc_read_ctl_t)(void);
/* ������ ॣ���� ���ﭨ� */
typedef void (*bsc_write_ctl_t)(uint8_t);
/* �⥭�� ���ﭨ� ����� ���������� �������� */
typedef bool (*bsc_GOT_t)(void);
/* ��⠭���� ����� ����� �������� */
typedef void (*bsc_ZAK_t)(bool);
/* �ਭ�� */
/* ������ ���� ������ */
typedef void (*prn_write_data_t)(uint8_t);
/* �⥭�� 設� BUSY */
typedef bool (*prn_BUSY_t)(void);
/* �⥭�� 設� DRQ (ACK) */
typedef bool (*prn_DRQ_t)(void);
/* ��⠭���� 設� RPR */
typedef void (*prn_RPR_t)(bool);
/* ��⠭���� 設� SPR */
typedef void (*prn_SPR_t)(bool);
/* ���� DS1990A */
/* ��⠭���� ����� DLSO */
typedef void (*dallas_DLSO_t)(bool);
/* �⥭�� ����� DLSK */
typedef bool (*dallas_DLSK_t)(void);

/* ���ᠭ�� ������ */
struct chipset_metrics {
/* ��騥 �ࠪ���⨪� */
	char *name;		/* �������� ������ */
	uint32_t io_base;	/* ��砫� ��������� �����/�뢮�� */
	uint32_t io_extent;	/* ࠧ��� �������� �����/�뢮�� */
/* BSC */
	int bsc_irq;
	bsc_read_dta_t bsc_read_dta;
	bsc_write_dta_t bsc_write_dta;
	bsc_read_ctl_t bsc_read_ctl;
	bsc_write_ctl_t bsc_write_ctl;
	bsc_GOT_t bsc_GOT;
	bsc_ZAK_t bsc_ZAK;
/* �ਭ�� */
	prn_write_data_t prn_write_data;
	prn_BUSY_t prn_BUSY;
	prn_DRQ_t prn_DRQ;
	prn_RPR_t prn_RPR;
	prn_SPR_t prn_SPR;
/* ���� DS1990A */
	dallas_DLSO_t dallas_DLSO;
	dallas_DLSK_t dallas_DLSK;
};

extern int chipset_type;
extern struct chipset_metrics cm;

/* �㭪�� �஢�ન ᮮ⢥�����饣� ������ */
typedef bool (*chipset_probe_t)(void);
/* �㭪�� ��⠭���� ��ࠬ��஢ ᮮ⢥�����饣� ������ */
typedef void (*chipset_set_metrics_t)(struct chipset_metrics *);

#endif		/* SYS_CHIPSET_H */
