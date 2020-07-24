/* Константы для IOCTL. (c) gsr 2001-2003, 2020 */

#ifndef SYS_IOCTLS_H
#define SYS_IOCTLS_H

#define STERM_MAJOR_BASE	200
#define XPRN_MAJOR		STERM_MAJOR_BASE + 2 

#define STERM_IO_SEQ_BASE	0
#define XPRN_IO_SEQ_BASE	STERM_IO_SEQ_BASE

/* xprn.o */
#define XPRN_IOCTL_CODE		0xc2
#define XPRN_DEV_NAME		"/dev/xprn"
/* Сброс принтера */
#define XPRN_IO_RESET		_IO(XPRN_IOCTL_CODE, XPRN_IO_SEQ_BASE)
/* Вывод символа на принтер */
#define XPRN_IO_OUTCHAR		_IOW(XPRN_IOCTL_CODE, XPRN_IO_SEQ_BASE + 1, char)

#endif		/* SYS_IOCTLS_H */
