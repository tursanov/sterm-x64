/* Основные функции для работы с экраном. (c) gsr, Alex Popov 2000-2004 */

#ifndef SCR_H
#define SCR_H

#include "blimits.h"
#include "kbd.h"
#include "sterm.h"
#include "gui/exgdi.h"
#include "gui/status.h"

#define MAIN_TITLE	"Экспресс-2А-К  АО НПЦ \"Спектр\""
extern char main_title[80];

/* Шрифты */
extern FontPtr font32x8;
extern FontPtr font80x20;
extern FontPtr font_hints;
extern FontPtr font_status;

/* Геометрия экрана. Изначально использовалась для текстового режима. */
struct scr_geom{
	int width;		/* ширина экрана в символах */
	int height;		/* высота экрана с строках */
	int w;			/* ширина области ввода в символах */
	int lines;		/* высота области ввода в строках */
};

extern struct scr_geom *sg;

/* Размеры курсора */
#define INS_MODE_CURSOR_HEIGHT	5
#define OVR_MODE_CURSOR_WIDTH	4

/* Режим экрана */
enum {
	m_undef=-1,
	m80x20,
	m32x8
};

/* Тип текста в буфере */
enum {
	txt_plain,
	txt_rich
};

struct hint_entry{
	const char *key;
	const char *descr;
	const char *glyph;
};

extern bool		scr_visible;
extern bool		cursor_visible;
extern int		cur_mode;
extern uint8_t		*cur_buf;
extern int		cur_window;
extern bool		scr_text_modified;
extern bool		hex_input;
extern bool		key_input;
extern bool		wait_key;
extern int		page;

extern int		err_beep(void);

extern void		init_input_windows(void);
extern void		create_scr(void);
extern void		release_scr(void);
extern void		reset_scr(void);
extern void		ClearScreen(Color c);
extern bool		RedrawScr(bool all, const char *title);
extern void		clear_text_field(void);

extern int		set_scr_mode(int m, bool need_redraw, bool is_main);
extern void		switch_scr_mode(bool need_redraw);
extern void		set_resp_mode(int m);
extern void		flush_resp_mode(void);
extern bool		scr_is_resp(void);
extern bool		scr_is_req(void);
extern bool		scr_goto_char(uint8_t *p);
extern bool		draw_scr_bevel(void);
extern bool		draw_scr(bool show_text, const char *title);
extern int		scr_handle_kbd(struct kbd_event *e);
extern int		get_scr_text(uint8_t *buf, int len);
extern int		scr_get_24(char *buf,int l,bool strip);
extern int		scr_snap_shot(uint8_t *buf,int l);
extern int		set_scr_text(uint8_t *s,int l,int t, bool need_redraw);
extern int		set_scr_request(uint8_t *s, int req_len, bool show);
extern bool		process_scr(void);
extern bool		is_lat(uint16_t c);
extern void		hide_cursor(void);
extern void		show_cursor(void);
extern void		set_cursor_visibility(bool visible);
extern bool		show_hints(void);
extern bool		draw_clock(bool nowait);
extern bool		scr_show_pgnum(bool show);

extern int		cm_pgup(struct kbd_event *e);
extern int		cm_pgdn(struct kbd_event *e);

extern bool		quick_astate(int ast);
/*
 * Ввод массива символов в ОЗУ заказа. Возвращает true, если был введён
 * хотя бы один символ.
 */
extern bool		input_chars(const uint8_t *chars, int len);

#endif		/* SCR_H */
