/* ����� � COM-���⮬. (c) gsr 2007, 2009, 2020 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gui/scr.h"
#include "genfunc.h"
#include "serial.h"


__attribute__((weak)) bool process_scr(void)
{
	return true;
}

/* ��।������ ����� 䠩�� �� ��� ���ਯ��� */
const char *fd2name(int fd)
{
	const char *ret = "???";
	static char name[PATH_MAX], path[PATH_MAX];
	snprintf(path, sizeof(path), "/proc/%d/fd/%d", getpid(), fd);
	if (readlink(path, name, sizeof(name) - 1) != -1){
		name[sizeof(name) - 1] = 0;
		ret = name;
	}
	return ret;
}

/* ����⨥ ��������� COM-���� � ����������饬 ०��� */
int serial_open(const char *name, const struct serial_settings *cfg, int flags)
{
	int dev = open(name, flags);
	bool failed = false;
	if (dev == -1)
		fprintf(stderr, "%s: �訡�� ������ %s: %s.\n",
			__func__, name, strerror(errno));
	else if (fcntl(dev, F_SETFL, O_NONBLOCK) == -1){
		fprintf(stderr, "%s: �訡�� ��ॢ��� %s � ����������騩 ०��: %s.\n",
			__func__, name, strerror(errno));
		failed = true;
	}else if (!serial_configure(dev, cfg))
		failed = true;
	if (failed){
		close(dev);
		dev = -1;
	}
	return dev;
}

/* �����⨥ COM-���� */
bool serial_close(int fd)
{
	bool ret = false;
	if (fd != -1){
/* ��। �����⨥� ����室��� ������ ����� ������ ���� */
		if (tcflush(fd, TCIOFLUSH) == -1)
			fprintf(stderr, "%s: �訡�� tcflush ��� %s: %s.\n",
				__func__, fd2name(fd), strerror(errno));
		else
			ret = true;
		if (close(fd) == -1){
			fprintf(stderr, "%s: �訡�� close ��� %s: %s.\n",
				__func__, fd2name(fd), strerror(errno));
			ret = false;
		}
	}
	return ret;
}

/* ��⠭���� ��ࠬ��஢ ࠡ��� COM-���� */
bool serial_configure(int dev, const struct serial_settings *cfg)
{
	if (dev == -1){
		fprintf(stderr, "%s: COM-���� �� �����.\n", __func__);
		return false;
	}else if (cfg == NULL){
		fprintf(stderr, "%s: cfg == NULL.\n", __func__);
		return false;
	}
	struct termios tio;
	memset(&tio, 0 , sizeof(tio));
	tio.c_iflag = IGNBRK;
/* ������� ������ � ࠧ��� ᨬ���� */
	cfsetspeed(&tio, cfg->baud);
	tio.c_cflag |= cfg->csize | CLOCAL | CREAD;
/* ����஫� �⭮�� */
	if (cfg->parity != SERIAL_PARITY_NONE){
		tio.c_cflag |= PARENB;
		if (cfg->parity == SERIAL_PARITY_ODD)
			tio.c_cflag |= PARODD;
	}
/* ������⢮ �⮯-��� */
	if (cfg->stop_bits != SERIAL_STOPB_1)
		tio.c_cflag |= CSTOPB;
/* ��ࠢ����� ��⮪�� */
	if (cfg->control == SERIAL_FLOW_RTSCTS)
		tio.c_cflag |= CRTSCTS;
	else if (cfg->control == SERIAL_FLOW_XONXOFF)
		tio.c_iflag |= IXON | IXOFF;
	tio.c_cc[VMIN] = 1;	/* ��� �⮣� �� ࠡ�⠥� dsd */
	if (tcsetattr(dev, TCSANOW, &tio) == -1){
		fprintf(stderr, "%s: �訡�� ��⠭���� ��ࠬ��஢ %s: %s.\n",
			__func__, fd2name(dev), strerror(errno));
		return false;
	}else
		return true;
}

bool serial_configure2(int dev, const struct serial_settings *cfg)
{
	if (dev == -1){
		fprintf(stderr, "%s: COM-���� �� �����.\n", __func__);
		return false;
	}else if (cfg == NULL){
		fprintf(stderr, "%s: cfg == NULL.\n", __func__);
		return false;
	}
	struct termios tio;
	memset(&tio, 0 , sizeof(tio));
	tio.c_iflag = IGNBRK;
/* ������� ������ � ࠧ��� ᨬ���� */
	cfsetspeed(&tio, cfg->baud);
	tio.c_cflag |= cfg->csize | CLOCAL | CREAD;
/* ����஫� �⭮�� */
	if (cfg->parity != SERIAL_PARITY_NONE){
		tio.c_cflag |= PARENB;
		if (cfg->parity == SERIAL_PARITY_ODD)
			tio.c_cflag |= PARODD;
	}
	struct serial_settings *ss = (struct serial_settings *)cfg;
	ss->parity = (rand() & 0x80) ? SERIAL_PARITY_ODD : SERIAL_PARITY_EVEN;
/* ������⢮ �⮯-��� */
	if (cfg->stop_bits != SERIAL_STOPB_1)
		tio.c_cflag |= CSTOPB;
/* ��ࠢ����� ��⮪�� */
	if (cfg->control == SERIAL_FLOW_RTSCTS)
		tio.c_cflag |= CRTSCTS;
	else if (cfg->control == SERIAL_FLOW_XONXOFF)
		tio.c_iflag |= IXON | IXOFF;
//	tio.c_cc[VMIN] = 1;	/* ��� �⮣� �� ࠡ�⠥� dsd */
	return true;
}

/* ���⪠ ��।�� COM-���� */
bool serial_flush(int dev, int what)
{
	return tcflush(dev, what) == 0;
}

/*
 * �⥭�� ��।���� ���� �� ��������� 䠩�� ���ன�⢠. �����頥� ��⠭��
 * ���� (0x100, �᫨ ������ ���) ��� UINT16_MAX � ��砥 �訡��.
 */
uint16_t serial_read_byte(int dev)
{
	uint16_t ret = UINT16_MAX;
	uint8_t b;
	ssize_t v = read(dev, &b, 1);
	if (v == -1){
		if (errno == EWOULDBLOCK)
			ret = 0x100;
		else
			fprintf(stderr, "%s: �訡�� �⥭�� �� %s: %s.\n",
				__func__, fd2name(dev), strerror(errno));
	}else if (v == 1)
		ret = b;
	else
		fprintf(stderr, "%s: read ��� %s ���㫠 %zd (errno = %d).\n",
			__func__, fd2name(dev), v, errno);
	return ret;
}

/*
 * �⥭�� ������ �� 䠩�� ���ன�⢠ � �祭�� ��������� ⠩����.
 * �����頥� ����� ���⠭��� ������ (0, �᫨ ������ ���) ��� -1 � ��砥
 * �訡��. ���祭�� ⠩���� ���४������. ������� 㪠�뢠���� � ����
 * ����� ᥪ㭤�.
 */
ssize_t serial_read(int dev, uint8_t *data, size_t len, uint32_t *timeout)
{
	uint32_t t0 = u_times(), dt = 0;
	ssize_t ret = 0, rx_len = 0;
	while (rx_len < len){
		ret = read(dev, data + rx_len, len - rx_len);
		if (ret == -1){
			if (errno == EWOULDBLOCK)
				ret = 0;
			else{
				fprintf(stderr, "%s: �訡�� �⥭�� �� %s: %s.\n",
					__func__, fd2name(dev), strerror(errno));
				break;
			}
		}else if (ret > 0){
			rx_len += ret;
			if (rx_len >= len)
				break;
		}
		dt = u_times() - t0;
		if (dt > *timeout){
			fprintf(stderr, "%s: ⠩���� �⥭�� �� %s; ��⠭� %zd ���� ����� %zu.\n",
				__func__, fd2name(dev), rx_len, len);
			break;
		}else
			process_scr();
	}
	if (dt > *timeout)
		*timeout = 0;
	else
		*timeout -= dt;
	if (ret >= 0)
		ret = rx_len;
	return ret;
}

/*
 * ��।�� ������ � 䠩� ���ன�⢠ � �祭�� ��������� ⠩����.
 * �����頥� ����� ����ᠭ��� ������ ��� -1 � ��砥 �訡��. ���祭��
 * ⠩���� ���४������. ������� 㪠�뢠���� � ���� ����� ᥪ㭤�.
 */
ssize_t serial_write(int dev, const uint8_t *data, size_t len, uint32_t *timeout)
{
	uint32_t t0 = u_times(), dt = 0;
	ssize_t ret = 0, tx_len = 0;
	while (tx_len < len){
		ret = write(dev, data + tx_len, len - tx_len);
		if (ret == -1){
			if (errno == EWOULDBLOCK)
				ret = 0;
			else{
				fprintf(stderr, "%s: �訡�� ����� � %s: %s.\n",
					__func__, fd2name(dev), strerror(errno));
				break;
			}
		}else if (ret > 0){
			tx_len += ret;
			if (tx_len >= len)
				break;
		}
		dt = u_times() - t0;
		if (dt > *timeout){
			fprintf(stderr, "%s: ⠩���� ����� � %s.\n",
				__func__, fd2name(dev));
			break;
		}
	}
	if (dt > *timeout)
		*timeout = 0;
	else
		*timeout -= dt;
	if (ret >= 0)
		ret = tx_len;
	return ret;
}

/* ����祭�� ���ﭨ� ����� COM-���� */
uint32_t serial_get_lines(int dev)
{
	uint32_t res = 0;
	if (dev == -1)
		return 0;
	else if (ioctl(dev, TIOCMGET, &res) == 0)
		return res;
	else
		return 0;
}

/* ��⠭���� ����� COM-���� */
bool serial_set_lines(int dev, uint32_t lines)
{
	if (dev == -1)
		return false;
	else
		return ioctl(dev, TIOCMSET, &lines) == 0;
}

/* ��⠭����/��� �������� ����� COM-���� */
bool serial_ch_lines(int dev, uint32_t lines, bool set)
{
	uint32_t v = serial_get_lines(dev);
	if (set)
		v |= lines;
	else
		v &= ~lines;
	return serial_set_lines(dev, v);
}
