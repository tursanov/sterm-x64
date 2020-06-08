/* Работа с принтером "Экспресс". (c) gsr 2000, 2004, 2009 */

#ifndef PRN_EXPRESS_H
#define PRN_EXPRESS_H

#include "prn/generic.h"
#include "sys/ioctls.h"
#include "sys/xprn.h"
#include "sysdefs.h"

/* Команды принтера "Экспресс" */

/* Однобайтовые команды */
#define XPRN_NUL		0x00	/* пустая команда */
#define XPRN_WIDE_FNT		0x07	/* широкий шрифт */
#define XPRN_BS			0x08	/* шаг назад */
#define XPRN_ITALIC		0x09	/* наклонный шрифт */
#define XPRN_LF			0x0a	/* перевод строки */
#define XPRN_NORM_FNT		0x0b	/* нормальный шрифт */
#define XPRN_FORM_FEED		0x0c	/* выброс бланка */
#define XPRN_CR			0x0d	/* возврат каретки */
#define XPRN_DLE1		0x10	/* начало команды */
#define XPRN_UNDERLINE		0x14	/* подчёркнутый шрифт */
#define XPRN_CPI12		0x19	/* плотность печати 12 cpi */
#define XPRN_DLE		0x1b	/* начало команды */
#define XPRN_CPI10		0x1c	/* плотность печати 10 cpi */
#define XPRN_CPI16		0x1e	/* плотность печати 16 cpi */
#define XPRN_RST		0x7f	/* сброс устройства */

/* Многобайтовые команды */
#define XPRN_AUXLNG		0x09	/* переход на дополнительную зону символов */
#define XPRN_MAINLNG		0x0b	/* переход на основную зону символов */
#define XPRN_FONT		0x41	/* выбор шрифта */
#define XPRN_VPOS		0x45	/* вертикальное позиционирование */
#define XPRN_WR_BCODE		0x4c	/* печать штрих-кода */
#define XPRN_PRNOP		0x5b	/* команда принтера */
#define XPRN_NO_BCODE		0x73	/* работать без штрих-кода */
#define XPRN_RD_BCODE		0x76	/* читать штрих-код */

/* Окончания команды XPRN_PRNOP */
#define XPRN_PRNOP_HPOS_RIGHT	0x61	/* относительное горизонтальное позиционирование вправо */
#define XPRN_PRNOP_HPOS_LEFT	0x71	/* относительное горизонтальное позиционирование влево */
#define XPRN_PRNOP_HPOS_ABS	0x60	/* абсолютное горизонтальное позиционирование */
#define XPRN_PRNOP_VPOS_FW	0x65	/* относительное вертикальное позиционирование вперёд */
#define XPRN_PRNOP_VPOS_BK	0x75	/* относительное вертикальное позиционирование назад */
#define XPRN_PRNOP_VPOS_ABS	0x64	/* абсолютное вертикальное позиционирование */
#define XPRN_PRNOP_INP_TICKET	0x74	/* ввод бланка */
#define XPRN_PRNOP_LINE_FW	0x76	/* переключение строк вперёд */

/* Устанавливается во время печати на принтере */
extern bool xprn_printing;

extern void xprn_init(void);
extern void xprn_release(void);
extern void xprn_flush(void);
extern bool xprn_print(const char *txt, int l);

#endif		/* PRN_EXPRESS_H */
