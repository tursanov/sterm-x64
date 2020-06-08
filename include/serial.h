/* Работа с COM-портом. (c) gsr 2007, 2009 */

#if !defined SERIAL_H
#define SERIAL_H

#include <sys/types.h>
#include <termios.h>
#include "sysdefs.h"

/* Количество доступных COM-портов */
#define NR_SERIALS		4

/* Префикс имени файла устройства COM-порта */
#define SERIAL_NAME_PREFIX	"/dev/ttyS"
/* Префикс имени файла устройства виртуального COM-порта (USB-COM) */
#define VSERIAL_NAME_PREFIX	"/dev/ttyUSB"

/* Тип управление потоком */
enum {
	SERIAL_FLOW_NONE,		/* нет */
	SERIAL_FLOW_RTSCTS,		/* RTS/CTS */
	SERIAL_FLOW_XONXOFF,		/* XON/XOFF */
};

/* Количество стоп-бит */
enum {
	SERIAL_STOPB_1,			/* 1 стоп-бит */
	SERIAL_STOPB_2,			/* 2 стоп-бита */
};

/* Тип контроля четности */
enum {
	SERIAL_PARITY_NONE,		/* нет контроля четности */
	SERIAL_PARITY_ODD,		/* контроль нечетности */
	SERIAL_PARITY_EVEN,		/* контроль четности */
};

/* Настройки COM-порта */
struct serial_settings {
	int csize;			/* размер символа (см. termios.h) */
	int parity;			/* тип контроля четности */
	int stop_bits;			/* количество стоп-бит */
	int control;			/* тип управления потоком */
	int baud;			/* скорость обмена (см. termios.h) */
};

/* Открытие заданного COM-порта */
extern int serial_open(const char *name, const struct serial_settings *cfg, int flags);
/* Закрытие COM-порта */
extern bool serial_close(int fd);
/* Установка параметров работы COM-порта */
extern bool serial_configure(int dev, const struct serial_settings *cfg);
extern bool serial_configure2(int dev, const struct serial_settings *cfg);

/* Очистка очередей COM-порта */
extern bool serial_flush(int dev, int what);

/*
 * Чтение очередного байта из заданного файла устройства. Возвращает считанный
 * байт (0x100, если данных нет) или UINT16_MAX в случае ошибки.
 */
extern uint16_t serial_read_byte(int dev);
/*
 * Чтение данных из файла устройства в течение заданного таймаута.
 * Возвращает длину прочитанных данных (0, если данных нет) или -1 в случае
 * ошибки. Значение таймаута корректируется. Таймаут указывается в сотых
 * долях секунды.
 */
extern ssize_t serial_read(int dev, uint8_t *data, size_t len, uint32_t *timeout);
/*
 * Передача данных в файл устройства в течение заданного таймаута.
 * Возвращает длину записанных данных или -1 в случае ошибки. Значение
 * таймаута корректируется. Таймаут указывается в сотых долях секунды.
 */
extern ssize_t serial_write(int dev, const uint8_t *data, size_t len, uint32_t *timeout);

/* Получение состояния линий COM-порта */
extern uint32_t serial_get_lines(int dev);
/* Установка линий COM-порта */
extern bool serial_set_lines(int dev, uint32_t lines);
/* Установка/сброс заданных линий COM-порта */
extern bool serial_ch_lines(int dev, uint32_t lines, bool set);

/* Определение имени файла по его дескриптору */
extern const char *fd2name(int fd);

#endif		/* SERIAL_H */
