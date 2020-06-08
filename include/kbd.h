/* Работа с клавиатурой терминала и динамиком. (c) gsr 2000-2001, 2010, 2018 */

#if !defined KBD_H
#define KBD_H

#include <time.h>
#include "sysdefs.h"

/* Коды клавиатуры в режиме MEDIUMRAW */

#define KEY_NONE	0xff
#define KEY_ESCAPE	0x01
#define KEY_F1		0x3b
#define KEY_F2		0x3c
#define KEY_F3		0x3d
#define KEY_F4		0x3e
#define KEY_F5		0x3f
#define KEY_F6		0x40
#define KEY_F7		0x41
#define KEY_F8		0x42
#define KEY_F9		0x43
#define KEY_F10		0x44
#define KEY_F11		0x57
#define KEY_F12		0x58
#define KEY_TILDE	0x29
#define KEY_1		0x02
#define KEY_2		0x03
#define KEY_3		0x04
#define KEY_4		0x05
#define KEY_5		0x06
#define KEY_6		0x07
#define KEY_7		0x08
#define KEY_8		0x09
#define KEY_9		0x0a
#define KEY_0		0x0b
#define KEY_MINUS	0x0c
#define KEY_PLUS	0x0d
#define KEY_BACKSPACE	0x0e
#define KEY_PRNSCR	0x63
#define KEY_SCRLOCK	0x46
#define KEY_BREAK	0x77
#define KEY_NUMLOCK	0x45
#define KEY_NUMSLASH	0x62
#define KEY_NUMMUL	0x37
#define KEY_NUMMINUS	0x4a
#define KEY_TAB		0x0f
#define KEY_Q		0x10
#define KEY_W		0x11
#define KEY_E		0x12
#define KEY_R		0x13
#define KEY_T		0x14
#define KEY_Y		0x15
#define KEY_U		0x16
#define KEY_I		0x17
#define KEY_O		0x18
#define KEY_P		0x19
#define KEY_LFBRACE	0x1a
#define KEY_RFBRACE	0x1b
#define KEY_ENTER	0x1c
#define KEY_INS		0x6e
#define KEY_HOME	0x66
#define KEY_PGUP	0x68
#define KEY_NUMHOME	0x47
#define KEY_NUMUP	0x48
#define KEY_NUMPGUP	0x49
#define KEY_NUMPLUS	0x4e
#define KEY_CAPS	0x3a
#define KEY_A		0x1e
#define KEY_S		0x1f
#define KEY_D		0x20
#define KEY_F		0x21
#define KEY_G		0x22
#define KEY_H		0x23
#define KEY_J		0x24
#define KEY_K		0x25
#define KEY_L		0x26
#define KEY_COLON	0x27
#define KEY_QUOTE	0x28
#define KEY_DEL		0x6f
#define KEY_END		0x6b
#define KEY_PGDN	0x6d
#define KEY_NUMLEFT	0x4b
#define KEY_NUMDOT	0x4c
#define KEY_NUMRIGHT	0x4d
#define KEY_LSHIFT	0x2a
#define KEY_Z		0x2c
#define KEY_X		0x2d
#define KEY_C		0x2e
#define KEY_V		0x2f
#define KEY_B		0x30
#define KEY_N		0x31
#define KEY_M		0x32
#define KEY_COMMA	0x33
#define KEY_DOT		0x34
#define KEY_SLASH	0x35
#define KEY_RSHIFT	0x36
#define KEY_BKSLASH	0x2b
#define KEY_UP		0x67
#define KEY_NUMEND	0x4f
#define KEY_NUMDOWN	0x50
#define KEY_NUMPGDN	0x51
#define KEY_NUMENTER	0x60
#define KEY_LCTRL	0x1d
#define KEY_LWFLAG	0x7d
#define KEY_LALT	0x38
#define KEY_SPACE	0x39
#define KEY_RALT	0x64
#define KEY_RWFLAG	0x7e
#define KEY_WPOPUP	0x7f
#define KEY_RCTRL	0x61
#define KEY_LEFT	0x69
#define KEY_DOWN	0x6c
#define KEY_RIGHT	0x6a
#define KEY_NUMINS	0x52
#define KEY_NUMDEL	0x53
#define KEY_LWIN	0x7d
#define KEY_RWIN	0x7e

/* Состояние клавиш-модификаторов */
#define SHIFT_LSHIFT	0x0001
#define SHIFT_RSHIFT	0x0002
#define SHIFT_LCTRL	0x0004
#define SHIFT_RCTRL	0x0008
#define SHIFT_LALT	0x0010
#define SHIFT_RALT	0x0020

#define SHIFT_CTRL	(SHIFT_LCTRL | SHIFT_RCTRL)
#define SHIFT_ALT	(SHIFT_LALT | SHIFT_RALT)
#define SHIFT_SHIFT	(SHIFT_LSHIFT | SHIFT_RSHIFT)
#define SHIFT_CT_AL	(SHIFT_CTRL | SHIFT_ALT)

extern int kbd_shift_state;

struct kbd_event{
	uint8_t key;
	uint8_t ch;
	bool pressed;
	bool repeated;
	int shift_state;
};

extern int kbd;

#define N_CHAR_KEYS	64

/* Язык клавиатуры */
enum {
	lng_rus,
	lng_lat,
};

extern int kbd_lang;

extern bool init_kbd(void);
extern void release_kbd(void);
extern uint32_t kbd_idle_interval(void);
extern void kbd_reset_idle_interval(void);
extern bool kbd_set_rate(int rate,int delay);
extern bool kbd_get_event(struct kbd_event *e);
extern bool kbd_wait_event(struct kbd_event *e);
extern void kbd_flush_queue(void);
extern bool kbd_exact_shift_state(struct kbd_event *e, int state);

#if defined __CONSOLE_SWITCHING__
extern int strip_ctrl(void);
#endif

/* Управление звуком */

extern void sound(uint32_t freq);
extern void nosound(void);
extern void beep(uint32_t freq, uint32_t ms);
extern void beep_sync(uint32_t freq, uint32_t ms);

#define delay(ms) usleep((ms)*1000)

#endif		/* KBD_H */
