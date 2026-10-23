/* Вывод на экран справочной информации. (c) gsr 2000, 2026 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/exgdi.h"
#include "gui/help.h"
#include "gui/options.h"
#include "prn/express.h"
#include "genfunc.h"
#include "kbd.h"
#include "paths.h"
#include "sterm.h"
#include "termlog.h"

static struct help_line {
	struct help_line *next;
	char *str;
} *hlp_lines = NULL;

#define MAX_HLP_LINES	65536

static size_t nr_hlp_lines = 0;
static int cur_hlp_line = 0;

static size_t hlp_w = 0;
static size_t hlp_h = 0;

static PixmapFontPtr help_font = NULL;

static char *chop(char *s)
{
	if (s != NULL){
		size_t l = strlen(s);
		if (l > 0)
			s[l - 1] = 0;
	}
	return s;
}

static bool read_hlp_lines(const char *path)
{
	if (path == NULL){
		log_err("path = NULL.");
		return false;
	}
	FILE *f = fopen(path, "r");
	if (f == NULL){
		log_sys_err("Ошибка открытия %s для чтения:", path);
		return false;
	}
        nr_hlp_lines = 0;
	char buf[1024];
	struct help_line *pp = NULL;
	while(fgets(buf, sizeof(buf), f) != NULL){
		size_t len = strlen(buf);
		if (len == 0)
			continue;
		struct help_line *p = malloc(sizeof(struct help_line));
		p->next = NULL;
		p->str = malloc(len);
		strcpy(p->str, chop(buf));
		if (pp == NULL)
			hlp_lines = p;
		else
			pp->next = p;
		pp = p;
		nr_hlp_lines++;
		if (nr_hlp_lines >= MAX_HLP_LINES)
			break;
	}
	if (fclose(f) == EOF)
		log_sys_err("Ошибка закрытия %s после чтения:", path);
	return true;
}

static void dispose_hlp_lines(void)
{
	for (struct help_line *p = hlp_lines, *pp = NULL; p != NULL; p = pp){
		pp = p->next;
		if (p->str != NULL)
			free(p->str);
		free(p);
	}
}

static const char *get_hlp_line(int n)
{
	if ((n < 0) || (n >= nr_hlp_lines))
		return NULL;
	const char *ret = NULL;
	const struct help_line *p = hlp_lines;
	for (int i = 0; i < n; i++)
		p = p->next;
	if (p != NULL)
		ret = p->str;
	return ret;
}

static bool up(void)
{
	bool ret = true;
	if (cur_hlp_line > 0)
		cur_hlp_line--;
	else
		ret = false;
	return ret;
}

static bool dn(void)
{
	bool ret = true;
	if ((cur_hlp_line + 1) < nr_hlp_lines)
		cur_hlp_line++;
	else
		ret = false;
	return ret;
}

static bool pg_up(void)
{
	bool ret = true;
	if (cur_hlp_line > 0){
		cur_hlp_line -= hlp_h;
		if (cur_hlp_line < 0)
			cur_hlp_line = 0;
	}else
		ret = false;
	return ret;
}

static bool pg_dn(void)
{
	bool ret = true;
	if ((cur_hlp_line + hlp_h) < nr_hlp_lines)
		cur_hlp_line += hlp_h;
	else
		ret = false;
	return ret;
}

static bool home(void)
{
	bool ret = true;
	if (cur_hlp_line > 0)
		cur_hlp_line = 0;
	else
		ret = false;
	return ret;
}

static bool end(void)
{
	bool ret = true;
	if ((cur_hlp_line + hlp_h) < nr_hlp_lines)
		cur_hlp_line = nr_hlp_lines - hlp_h;
	else
		ret = false;
	return ret;
}

static void draw_hlp_lines(void)
{
	int cx = DISCX - 2, cy = DISCY - 42;
	GCPtr pGC = CreateGC(20, 35, cx, cy);
	GCPtr pMemGC = CreateMemGC(hlp_w * help_font->max_width, help_font->max_height);
	for (int n = 0; n < hlp_h; n++){
		ClearGC(pMemGC, cfg.bg_color);	 	
		int k = n + cur_hlp_line;
		if (k < nr_hlp_lines)
			PixmapTextOut(pMemGC, help_font, 0, 0, get_hlp_line(k));
		CopyGC(pGC, 0, help_font->max_height * (k - cur_hlp_line), pMemGC,
			0, 0, GetCX(pMemGC), GetCY(pMemGC));	
	}
	DeleteGC(pMemGC);
	DeleteGC(pGC);
	kbd_flush_queue();
}

static void draw_hlp_hints(void)
{
	static const char *hints = "Используйте \x18\x19, PgUp/PgDn/Home/End для перемещения. "
		"Esc/F10 -- выход.";
	const int hint_h = 34;
	int cx = DISCX - 5;
	int cy = hint_h * 2 + 4;
	GCPtr pGC = CreateGC(4, DISCY - hint_h * 2 - 8, cx, cy);
	GCPtr pMemGC = CreateMemGC(cx, cy);
	FontPtr pFont = CreateFont(_("fonts/terminal10x18.fnt"), false);
	SetFont(pMemGC, pFont);
	ClearGC(pMemGC, clBtnFace);
	DrawBorder(pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC), 1,
		clBtnShadow, clBtnHighlight);
	TextOut(pMemGC, 20, 30, hints);
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, cx, cy);
	DeleteFont(pFont);
	DeleteGC(pMemGC);
	DeleteGC(pGC);
}

bool draw_help(void)
{
	draw_hlp_lines();
	draw_hlp_hints();
	return true;
}

bool process_help(struct kbd_event *e)
{
	if (!e->pressed || (e->shift_state != 0) || (e->key == KEY_NONE))
		return true;
	bool ret = true, flag = false;
	switch (e->key){
		case KEY_UP:
		case KEY_NUMUP:
			flag = up();
			break;
		case KEY_DOWN:
		case KEY_NUMDOWN:
			flag = dn();
			break;
		case KEY_PGUP:
		case KEY_NUMPGUP:
			flag = pg_up();
			break;
		case KEY_PGDN:
		case KEY_NUMPGDN:
			flag = pg_dn();
			break;
		case KEY_HOME:
		case KEY_NUMHOME:
			flag = home();
			break;
		case KEY_END:
		case KEY_NUMEND:
			flag = end();
			break;
		case KEY_ESCAPE:
		case KEY_F10:
			ret = false;
	}
	if (flag && ret)
		draw_hlp_lines();
	return ret;
}

void init_help(const char *fname)
{
	online = false;
	help_active = true;
	set_term_busy(true);
	hide_cursor();
	set_term_state(st_help);
	set_term_astate(ast_none);
	set_scr_mode(m80x20, false, false);
	clear_text_field();
	draw_scr_bevel();
	scr_show_pgnum(false);
	scr_show_mode(false);
	scr_show_language(false);
	scr_show_log(false);
	hlp_w = 78;
	hlp_h = 21;
	cur_hlp_line = 0;
	read_hlp_lines(fname);
	help_font = CreatePixmapFont(_("fonts/sterm/80x20/courier10x21.fnt"),
		cfg.rus_color, cfg.bg_color);
	draw_help();
	draw_clock(true);
}

void release_help(void)
{
	DeletePixmapFont(help_font);
	dispose_hlp_lines();
	help_active = false;
}
