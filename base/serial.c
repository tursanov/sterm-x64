/* Работа с COM-портом. (c) gsr 2007, 2009, 2020 */

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

static const char *serial_csize_str(int csize)
{
	static char ret[8];
	int n = 0;
	switch (csize){
		case CS5:
			n = 5;
			break;
		case CS6:
			n = 6;
			break;
		case CS7:
			n = 7;
			break;
		case CS8:
			n = 8;
			break;
	}
	if (n > 0)
		snprintf(ret, sizeof(ret), "%d", n);
	else
		snprintf(ret, sizeof(ret), "??");
	return ret;
}

static const char *serial_parity_str(int parity)
{
	static char ret[3];
	switch (parity){
		case SERIAL_PARITY_NONE:
			strcpy(ret, "N");
			break;
		case SERIAL_PARITY_ODD:
			strcpy(ret, "O");
			break;
		case SERIAL_PARITY_EVEN:
			strcpy(ret, "E");
			break;
		default:
			strcpy(ret, "*");
	}
	return ret;
}

static const char *serial_stopb_str(int stop_bits)
{
	static char ret[3];
	switch (stop_bits){
		case SERIAL_STOPB_1:
			strcpy(ret, "1");
			break;
		case SERIAL_STOPB_2:
			strcpy(ret, "2");
			break;
		default:
			strcpy(ret, "-");
	}
	return ret;
}

static const char *serial_control_str(int flow_ctl)
{
	static char ret[10];
	switch (flow_ctl){
		case SERIAL_FLOW_NONE:
			strcpy(ret, "none");
			break;
		case SERIAL_FLOW_RTSCTS:
			strcpy(ret, "RTS/CTS");
			break;
		case SERIAL_FLOW_XONXOFF:
			strcpy(ret, "XON/XOFF");
			break;
		default:
			strcpy(ret, "???");
	}
	return ret;
}

static const char *serial_baud_str(int baud)
{
	static const struct {
		int baud;
		int v;
	} map[] = {
		{B0,		0},
		{B50,		50},
		{B75,		75},
		{B110,		110},
		{B134,		134},
		{B150,		150},
		{B200,		200},
		{B300,		300},
		{B600,		600},
		{B1200,		1200},
		{B1800,		1800},
		{B2400,		2400},
		{B4800,		4800},
		{B9600,		9600},
		{B19200,	19200},
		{B38400,	38400},
		{B57600,	57600},
		{B115200,	115200},
		{B230400,	230400},
		{B460800,	460800},
		{B500000,	500000},
		{B576000,	576000},
		{B921600,	921600},
		{B1000000,	1000000},
		{B1152000,	1152000},
		{B1500000,	1500000},
		{B2000000,	2000000},
		{B2500000,	2500000},
		{B3000000,	3000000},
		{B3500000,	3500000},
		{B4000000,	4000000},
	};
	int cmp(const void *key, const void *v)
	{
		int baud = *(int *)(key);
		const typeof(*map) *p = (const typeof(*map) *)v;
		return baud - p->baud;
	}
	static char ret[10];
	const typeof(*map) *p = bsearch(&baud, map, ASIZE(map), sizeof(*map), cmp);
	if (p != NULL)
		snprintf(ret, sizeof(ret), "%d", p->v);
	else
		snprintf(ret, sizeof(ret), "0%.6o", baud);
	return ret;
}

/* Преобразование настроек COM-порта в строку */
const char *serial_settings_str(const struct serial_settings *ss)
{
	if (ss == NULL)
		return NULL;
	static char txt[10][32];
	static int idx = 0;
	char *ret = txt[idx++];
	idx %= ASIZE(txt);
	snprintf(ret, sizeof(*txt), "%s%s%s %s %s", serial_csize_str(ss->csize),
		serial_parity_str(ss->parity), serial_stopb_str(ss->stop_bits),
		serial_baud_str(ss->baud), serial_control_str(ss->control));
	return ret;
}

/* Определение имени файла по его дескриптору */
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

/* Открытие заданного COM-порта в неблокирующем режиме */
int serial_open(const char *name, const struct serial_settings *cfg, int flags)
{
	int dev = open(name, flags);
	bool failed = false;
	if (dev == -1)
		fprintf(stderr, "%s: ошибка открытия %s: %m.\n", __func__, name);
	else if (fcntl(dev, F_SETFL, O_NONBLOCK) == -1){
		fprintf(stderr, "%s: ошибка перевода %s в неблокирующий режим: %m.\n",
			__func__, name);
		failed = true;
	}else if (!serial_configure(dev, cfg))
		failed = true;
	if (failed){
		close(dev);
		dev = -1;
	}
	return dev;
}

/* Закрытие COM-порта */
bool serial_close(int fd)
{
	bool ret = false;
	if (fd != -1){
		const char *name = fd2name(fd);
/* Отключаем управление потоком */
		struct termios tio;
		if (tcgetattr(fd, &tio) == 0){
			tio.c_cflag &= ~CRTSCTS;
			tio.c_cflag &= ~(IXON | IXOFF | IXANY);
			if (tcsetattr(fd, TCSANOW, &tio) != 0)
				fprintf(stderr, "%s: ошибка tcsetattr для %s: %m.\n",
					__func__, name);
		}else
			fprintf(stderr, "%s: ошибка tcgetattr для %s: %m.\n", __func__, name);
/* Перед закрытием необходимо очистить буферы данных порта */
		if (tcflush(fd, TCIOFLUSH) == -1)
			fprintf(stderr, "%s: ошибка tcflush для %s: %m.\n", __func__, name);
		else
			ret = true;
		if (close(fd) == -1){
			fprintf(stderr, "%s: ошибка close для %s: %m.\n", __func__, name);
			ret = false;
		}
	}
	return ret;
}

/* Установка параметров работы COM-порта */
bool serial_configure(int dev, const struct serial_settings *cfg)
{
	if (dev == -1){
		fprintf(stderr, "%s: COM-порт не открыт.\n", __func__);
		return false;
	}else if (cfg == NULL){
		fprintf(stderr, "%s: cfg == NULL.\n", __func__);
		return false;
	}
	struct termios tio;
	memset(&tio, 0 , sizeof(tio));
	tio.c_iflag = IGNBRK;
/* Скорость обмена и размер символа */
	cfsetspeed(&tio, cfg->baud);
	tio.c_cflag |= cfg->csize | CLOCAL | CREAD;
/* Контроль четности */
	if (cfg->parity != SERIAL_PARITY_NONE){
		tio.c_cflag |= PARENB;
		if (cfg->parity == SERIAL_PARITY_ODD)
			tio.c_cflag |= PARODD;
	}
/* Количество стоп-бит */
	if (cfg->stop_bits != SERIAL_STOPB_1)
		tio.c_cflag |= CSTOPB;
/* Управление потоком */
	if (cfg->control == SERIAL_FLOW_RTSCTS)
		tio.c_cflag |= CRTSCTS;
	else if (cfg->control == SERIAL_FLOW_XONXOFF)
		tio.c_iflag |= IXON | IXOFF;
	tio.c_cc[VMIN] = 1;	/* без этого не работает dsd */
	if (tcsetattr(dev, TCSANOW, &tio) == -1){
		fprintf(stderr, "%s: ошибка установки параметров %s: %m.\n",
			__func__, fd2name(dev));
		return false;
	}else
		return true;
}

bool serial_configure2(int dev, const struct serial_settings *cfg)
{
	if (dev == -1){
		fprintf(stderr, "%s: COM-порт не открыт.\n", __func__);
		return false;
	}else if (cfg == NULL){
		fprintf(stderr, "%s: cfg == NULL.\n", __func__);
		return false;
	}
	struct termios tio;
	memset(&tio, 0 , sizeof(tio));
	tio.c_iflag = IGNBRK;
/* Скорость обмена и размер символа */
	cfsetspeed(&tio, cfg->baud);
	tio.c_cflag |= cfg->csize | CLOCAL | CREAD;
/* Контроль четности */
	if (cfg->parity != SERIAL_PARITY_NONE){
		tio.c_cflag |= PARENB;
		if (cfg->parity == SERIAL_PARITY_ODD)
			tio.c_cflag |= PARODD;
	}
	struct serial_settings *ss = (struct serial_settings *)cfg;
	ss->parity = (rand() & 0x80) ? SERIAL_PARITY_ODD : SERIAL_PARITY_EVEN;
/* Количество стоп-бит */
	if (cfg->stop_bits != SERIAL_STOPB_1)
		tio.c_cflag |= CSTOPB;
/* Управление потоком */
	if (cfg->control == SERIAL_FLOW_RTSCTS)
		tio.c_cflag |= CRTSCTS;
	else if (cfg->control == SERIAL_FLOW_XONXOFF)
		tio.c_iflag |= IXON | IXOFF;
//	tio.c_cc[VMIN] = 1;	/* без этого не работает dsd */
	return true;
}

/* Очистка очередей COM-порта */
bool serial_flush(int dev, int what)
{
	return tcflush(dev, what) == 0;
}

/*
 * Чтение очередного байта из заданного файла устройства. Возвращает считанный
 * байт (0x100, если данных нет) или UINT16_MAX в случае ошибки.
 */
uint16_t serial_read_byte(int dev)
{
	uint16_t ret = UINT16_MAX;
	const char *name = fd2name(dev);
	uint8_t b;
	ssize_t v = read(dev, &b, 1);
	if (v == -1){
		if (errno == EWOULDBLOCK)
			ret = 0x100;
		else
			fprintf(stderr, "%s: ошибка чтения из %s: %m.\n", __func__, name);
	}else if (v == 1)
		ret = b;
	else
		fprintf(stderr, "%s: read для %s вернула %zd (errno = %d).\n",
			__func__, name, v, errno);
	return ret;
}

/*
 * Чтение данных из файла устройства в течение заданного таймаута.
 * Возвращает длину прочитанных данных (0, если данных нет) или -1 в случае
 * ошибки. Значение таймаута корректируется. Таймаут указывается в сотых
 * долях секунды.
 */
ssize_t serial_read(int dev, uint8_t *data, size_t len, uint32_t *timeout)
{
	const char *name = fd2name(dev);
	uint32_t t0 = u_times(), dt = 0;
	ssize_t ret = 0, rx_len = 0;
	while (rx_len < len){
		ret = read(dev, data + rx_len, len - rx_len);
		if (ret == -1){
			if (errno == EWOULDBLOCK)
				ret = 0;
			else{
				fprintf(stderr, "%s: ошибка чтения из %s: %m.\n", __func__, name);
				break;
			}
		}else if (ret > 0){
			rx_len += ret;
			if (rx_len >= len)
				break;
		}
		dt = u_times() - t0;
		if (dt > *timeout){
			fprintf(stderr, "%s: таймаут чтения из %s; считано %zd байт вместо %zu.\n",
				__func__, name, rx_len, len);
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
 * Передача данных в файл устройства в течение заданного таймаута.
 * Возвращает длину записанных данных или -1 в случае ошибки. Значение
 * таймаута корректируется. Таймаут указывается в сотых долях секунды.
 */
ssize_t serial_write(int dev, const uint8_t *data, size_t len, uint32_t *timeout)
{
	const char *name = fd2name(dev);
	uint32_t t0 = u_times(), dt = 0;
	ssize_t ret = 0, tx_len = 0;
	while (tx_len < len){
		ret = write(dev, data + tx_len, len - tx_len);
		if (ret == -1){
			if (errno == EWOULDBLOCK)
				ret = 0;
			else{
				fprintf(stderr, "%s: ошибка записи в %s: %m.\n", __func__, name);
				break;
			}
		}else if (ret > 0){
			tx_len += ret;
			if (tx_len >= len)
				break;
		}
		dt = u_times() - t0;
		if (dt > *timeout){
			fprintf(stderr, "%s: таймаут записи в %s.\n", __func__, name);
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

/* Получение состояния линий COM-порта */
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

/* Установка линий COM-порта */
bool serial_set_lines(int dev, uint32_t lines)
{
	if (dev == -1)
		return false;
	else
		return ioctl(dev, TIOCMSET, &lines) == 0;
}

/* Установка/сброс заданных линий COM-порта */
bool serial_ch_lines(int dev, uint32_t lines, bool set)
{
	uint32_t v = serial_get_lines(dev);
	if (set)
		v |= lines;
	else
		v &= ~lines;
	return serial_set_lines(dev, v);
}
