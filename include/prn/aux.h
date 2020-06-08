/* Работа с дополнительным принтером (ДПУ). (c) gsr 2004, 2009 */

#ifndef PRN_AUX_H
#define PRN_AUX_H

#include "prn/generic.h"
#include "sys/ioctls.h"
#include "sys/xprn.h"
#include "sysdefs.h"

/* Команды ДПУ */

/* Однобайтовые команды */
#define APRN_NUL	0x00
#define APRN_RST	0x7f
#define APRN_LF		0x0a
#define APRN_FFEED	0x0c
#define APRN_CR		0x0d

/* Многобайтовые команды */
#define APRN_DLE	0x1d
#define APRN_AUXLNG	0x09
#define APRN_MAINLNG	0x0b
#define APRN_VPOS	0x21
#define APRN_HPOS	0x22
#define APRN_UNDERLINE	0x24
#define APRN_INTERLINE	0x25
#define APRN_FEED	0x27
#define APRN_CUT	0x28
#define APRN_FONT	0x29
#define APRN_LOGO	0x2c
#define APRN_SCALE	0x2d
#define APRN_INVERSE	0x2e
#define APRN_BCODECHARS	0x2f
#define APRN_BCODEH	0x31
#define APRN_BCODEF	0x33
#define APRN_BCODE	0x34
#define APRN_TAB	0x35
#define APRN_SPACING	0x36
#define APRN_SRV	0x37
#define APRN_LBL_LEN	0x3a
#define APRN_DIR	0x3b
#define APRN_STAB	0x64

extern void aprn_init(void);
extern void aprn_release(void);
extern void aprn_flush(void);
extern bool aprn_print(const char *txt, int l);

#endif		/* APRN_H */
