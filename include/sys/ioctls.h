/* Константы для IOCTL. (c) gsr 2001-2003 */

#ifndef SYS_IOCTLS_H
#define SYS_IOCTLS_H

#define STERM_MAJOR_BASE	200
#define CHIPSET_MAJOR		STERM_MAJOR_BASE
#define XPRN_MAJOR		STERM_MAJOR_BASE + 2 
#define DALLAS_MAJOR		STERM_MAJOR_BASE + 3

#define STERM_IO_SEQ_BASE	0
#define CHIPSET_IO_SEQ_BASE	STERM_IO_SEQ_BASE
#define XPRN_IO_SEQ_BASE	STERM_IO_SEQ_BASE
#define DALLAS_IO_SEQ_BASE	STERM_IO_SEQ_BASE
#define USBKEY_IO_SEQ_BASE	STERM_IO_SEQ_BASE

/* chipset.o */
#define CHIPSET_IOCTL_CODE	0xc0
#define CHIPSET_DEV_NAME	"/dev/chipset"
/* BSC */
/* Номер IRQ для BSC */
#define CHIPSET_IO_BSC_IRQ	_IO(CHIPSET_IOCTL_CODE, CHIPSET_IO_SEQ_BASE)
/* Чтение байта данных */
#define CHIPSET_IO_BSC_RDTA	_IO(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 1)
/* Запись байта данных */
#define CHIPSET_IO_BSC_WDTA	_IOW(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 2, int)
/* Чтение регистра состояния */
#define CHIPSET_IO_BSC_RCTL	_IO(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 3)
/* Запись регистра состояния */
#define CHIPSET_IO_BSC_WCTL	_IOW(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 4, int)
/* Чтение состояния линии ГОТОВНОСТЬ коммутатора */
#define CHIPSET_IO_BSC_GOT	_IO(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 5)
/* Установка линии ЗАКАЗ коммутатора */
#define CHIPSET_IO_BSC_ZAK	_IOW(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 6, int)
/* Принтер */
/* Запись байта данных */
#define CHIPSET_IO_PRN_WDATA	_IOW(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 7, int)
/* Чтение шины BUSY */
#define CHIPSET_IO_PRN_BUSY	_IO(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 8)
/* Чтение шины DRQ (ACK) */
#define CHIPSET_IO_PRN_DRQ	_IO(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 9)
/* Установка шины RPR */
#define CHIPSET_IO_PRN_RPR	_IOW(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 10, int)
/* Установка шины SPR */
#define CHIPSET_IO_PRN_SPR	_IOW(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 11, int)
/* Ключи DS1990A */
/* Установка линии DLSO */
#define CHIPSET_IO_DALLAS_DLSO	_IOW(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 12, int)
/* Чтение линии DLSK */
#define CHIPSET_IO_DALLAS_DLSK	_IO(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 13)
/* Тип адаптера */
#define CHIPSET_IO_CHIP_TYPE	_IO(CHIPSET_IOCTL_CODE,\
		CHIPSET_IO_SEQ_BASE + 14)

/* xprn.o */
#define XPRN_IOCTL_CODE		0xc2
#define XPRN_DEV_NAME		"/dev/xprn"
/* Сброс принтера */
#define XPRN_IO_RESET		_IO(XPRN_IOCTL_CODE, XPRN_IO_SEQ_BASE)
/* Вывод символа на принтер */
#define XPRN_IO_OUTCHAR		_IOW(XPRN_IOCTL_CODE, XPRN_IO_SEQ_BASE + 1, char)

/* dallas.o */
#define DALLAS_IOCTL_CODE	0xc3
#define DALLAS_DEV_NAME		"/dev/dallas"
/* Прочитать идентификатор DS1990A. При отсутствии ключа возвращает -1 */
#define DALLAS_IO_READ		_IOR(DALLAS_IOCTL_CODE, DALLAS_IO_SEQ_BASE, char *)

/* usbkey.o */
#define USBKEY_IOCTL_CODE	0xc5
#define USBKEY_BASE_NAME	"/dev/usbkey"
/* Проверка присутствия USB-ключа */
#define USBKEY_IO_HASKEY	_IO(USBKEY_IOCTL_CODE, USBKEY_IO_SEQ_BASE)

#endif		/* SYS_IOCTLS_H */
