/* ����� � COM-���⮬. (c) gsr 2007, 2009 */

#if !defined SERIAL_H
#define SERIAL_H

#include <sys/types.h>
#include <termios.h>
#include "sysdefs.h"

/* ������⢮ ����㯭�� COM-���⮢ */
#define NR_SERIALS		4

/* ��䨪� ����� 䠩�� ���ன�⢠ COM-���� */
#define SERIAL_NAME_PREFIX	"/dev/ttyS"
/* ��䨪� ����� 䠩�� ���ன�⢠ ����㠫쭮�� COM-���� (USB-COM) */
#define VSERIAL_NAME_PREFIX	"/dev/ttyUSB"

/* ��� �ࠢ����� ��⮪�� */
enum {
	SERIAL_FLOW_NONE,		/* ��� */
	SERIAL_FLOW_RTSCTS,		/* RTS/CTS */
	SERIAL_FLOW_XONXOFF,		/* XON/XOFF */
};

/* ������⢮ �⮯-��� */
enum {
	SERIAL_STOPB_1,			/* 1 �⮯-��� */
	SERIAL_STOPB_2,			/* 2 �⮯-��� */
};

/* ��� ����஫� �⭮�� */
enum {
	SERIAL_PARITY_NONE,		/* ��� ����஫� �⭮�� */
	SERIAL_PARITY_ODD,		/* ����஫� ���⭮�� */
	SERIAL_PARITY_EVEN,		/* ����஫� �⭮�� */
};

/* ����ன�� COM-���� */
struct serial_settings {
	int csize;			/* ࠧ��� ᨬ���� (�. termios.h) */
	int parity;			/* ⨯ ����஫� �⭮�� */
	int stop_bits;			/* ������⢮ �⮯-��� */
	int control;			/* ⨯ �ࠢ����� ��⮪�� */
	int baud;			/* ᪮���� ������ (�. termios.h) */
};

/* ����⨥ ��������� COM-���� */
extern int serial_open(const char *name, const struct serial_settings *cfg, int flags);
/* �����⨥ COM-���� */
extern bool serial_close(int fd);
/* ��⠭���� ��ࠬ��஢ ࠡ��� COM-���� */
extern bool serial_configure(int dev, const struct serial_settings *cfg);
extern bool serial_configure2(int dev, const struct serial_settings *cfg);

/* ���⪠ ��।�� COM-���� */
extern bool serial_flush(int dev, int what);

/*
 * �⥭�� ��।���� ���� �� ��������� 䠩�� ���ன�⢠. �����頥� ��⠭��
 * ���� (0x100, �᫨ ������ ���) ��� UINT16_MAX � ��砥 �訡��.
 */
extern uint16_t serial_read_byte(int dev);
/*
 * �⥭�� ������ �� 䠩�� ���ன�⢠ � �祭�� ��������� ⠩����.
 * �����頥� ����� ���⠭��� ������ (0, �᫨ ������ ���) ��� -1 � ��砥
 * �訡��. ���祭�� ⠩���� ���४������. ������� 㪠�뢠���� � ����
 * ����� ᥪ㭤�.
 */
extern ssize_t serial_read(int dev, uint8_t *data, size_t len, uint32_t *timeout);
/*
 * ��।�� ������ � 䠩� ���ன�⢠ � �祭�� ��������� ⠩����.
 * �����頥� ����� ����ᠭ��� ������ ��� -1 � ��砥 �訡��. ���祭��
 * ⠩���� ���४������. ������� 㪠�뢠���� � ���� ����� ᥪ㭤�.
 */
extern ssize_t serial_write(int dev, const uint8_t *data, size_t len, uint32_t *timeout);

/* ����祭�� ���ﭨ� ����� COM-���� */
extern uint32_t serial_get_lines(int dev);
/* ��⠭���� ����� COM-���� */
extern bool serial_set_lines(int dev, uint32_t lines);
/* ��⠭����/��� �������� ����� COM-���� */
extern bool serial_ch_lines(int dev, uint32_t lines, bool set);

/* ��।������ ����� 䠩�� �� ��� ���ਯ��� */
extern const char *fd2name(int fd);

#endif		/* SERIAL_H */
