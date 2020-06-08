/* Вывод на экран справочной информации. (c) gsr 2000 */

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

struct help_line *hlp_lines;
int n_hlp_lines;
int cur_hlp_line;
int hlp_w;
int hlp_h;
int max_str_w;
bool help_printing = false;
PixmapFontPtr help_font = NULL;


void init_help(char *fname)
{
	help_active=true;
	set_term_busy(true);
	
	hide_cursor();
	set_term_state(st_help);
	set_term_astate(ast_none);
	set_scr_mode(m80x20,false,false);
	clear_text_field();
	draw_scr_bevel();
	scr_show_pgnum(false);
	scr_show_mode(false);
	scr_show_language(false);
	scr_show_log(false);
/*	scr_visible=false;*/

	help_printing=false;
	hlp_w=78;
	hlp_h=21;
	cur_hlp_line=0;
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
	help_active=false;
}

bool read_hlp_lines(char *fname)
{
	FILE *f;
	char buf[128];
	struct help_line *p,*pp=NULL;
	int strl;
	
	max_str_w = 0;
	if (fname == NULL) return false;
	if ((f=fopen(fname,"rt")) == NULL)
		return false;
        n_hlp_lines=0;
	while(fgets(buf,128,f)){
		p=malloc(sizeof(struct help_line));
		p->next=NULL;
		strl = strlen(buf);
		if (strl > max_str_w) max_str_w = strl;
		strcpy(p->str=malloc(strl+1),chop(buf));
		if (pp)
			pp->next=p;
		else
			hlp_lines=p;
		pp=p;
		n_hlp_lines++;
	}
	fclose(f);
	return true;
}

bool dispose_hlp_lines()
{
	struct help_line *p=hlp_lines,*pp;
	while (p){
		free(p->str);
		pp=p->next;
		free(p);
		p=pp;
	}
	return true;
}

char *get_hlp_line(int n)
{
	struct help_line *p=hlp_lines;
	int i;
	if ((n < 0) || (n >= n_hlp_lines))
		return false;
	for (i=0; i < n; i++)
		p=p->next;
	return p->str;
}

char *chop(char *s)		/* Perl like chop */
{
	if (s){
		int l=strlen(s);
		if (l > 0)
			s[l-1]=0;
	}
	return s;
}

bool hv_up(void)
{
	if (cur_hlp_line > 0){
		cur_hlp_line--;
		return true;
	}else
		return false;
}

bool hv_dn(void)
{
	if (cur_hlp_line < (n_hlp_lines-1)){
		cur_hlp_line++;
		return true;
	}else
		return false;
}

bool hv_pg_up(void)
{
	if (cur_hlp_line > 0){
		cur_hlp_line -= hlp_h;
		if (cur_hlp_line < 0)
			cur_hlp_line=0;
		return true;
	}else
		return false;
}

bool hv_pg_dn(void)
{
	cur_hlp_line += hlp_h;
	if (cur_hlp_line >= n_hlp_lines){
		cur_hlp_line -= hlp_h;
		return false;
	}else
		return true;
}

bool hv_home(void)
{
	if (cur_hlp_line > 0){
		cur_hlp_line=0;
		return true;
	}else
		return false;
}

bool hv_end(void)
{
	int ln = n_hlp_lines - hlp_h;
	if (cur_hlp_line < ln){
		cur_hlp_line = ln;
		return true;
	}else
		return false;
}

bool draw_hlp_lines(void)
{
	int cx = DISCX-2, cy = DISCY-42;
	GCPtr pGC = CreateGC(20, 35, cx, cy);
	GCPtr pMemGC = CreateMemGC(hlp_w*help_font->max_width, help_font->max_height);
	int n, k;
	
	for (n=0; n < hlp_h ; n++) {
		ClearGC(pMemGC, cfg.bg_color);	 	
		k = n+cur_hlp_line;
		if (k < n_hlp_lines){
			PixmapTextOut(pMemGC, help_font, 0, 0, get_hlp_line(k));
		}
		CopyGC(pGC, 0, help_font->max_height*(k-cur_hlp_line), pMemGC,
			0, 0, GetCX(pMemGC), GetCY(pMemGC));	
	}
	
	DeleteGC(pMemGC);
	DeleteGC(pGC);
	
	kbd_flush_queue();
	
	return true;
}

bool draw_hlp_hints(void)
{
	static char *norm_hints="Используйте \x18\x19, PgUp/PgDn для перемещения. F2 -- печать. Esc -- выход";
	static char *prn_hints="Идет печать справки...";
	char *p = help_printing ? prn_hints : norm_hints;
	const int hint_h = 34;
	int cx = DISCX - 5;
	int cy = hint_h*2+4;
	GCPtr pGC = CreateGC(4, DISCY - hint_h*2-8, cx, cy);
	GCPtr pMemGC = CreateMemGC(cx, cy);
	FontPtr pFont = CreateFont(_("fonts/terminal10x18.fnt"), false);
	
	SetFont(pMemGC, pFont);
	ClearGC(pMemGC, clBtnFace);
	DrawBorder(pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC), 1,
		clBtnShadow, clBtnHighlight);
	
	TextOut(pMemGC, 20, 30, p);
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, cx, cy);
	DeleteFont(pFont);
	DeleteGC(pMemGC);
	DeleteGC(pGC);

	return true;
}

bool draw_help(void)
{
	draw_hlp_lines();
	draw_hlp_hints();
	return true;
}

void print_help(void)
{
	int i;
	char buf[128];
	help_printing=true;
	draw_hlp_hints();
	for (i=0; i < n_hlp_lines; i++){
		strcpy(buf,get_hlp_line(i));
		strcat(buf,"\n\r");
		if (!xprn_print(recode_str(buf,-1),strlen(buf)))
			break;
	}
	help_printing=false;
	if (help_active)
		draw_hlp_hints();
}

bool process_help(struct kbd_event *e)
{
	bool flag = false;
	if (help_printing){
		if (e->pressed && !e->repeated)
			err_beep();
		return true;
	}else if (!e->pressed || (e->key == KEY_NONE))
		return true;
	switch (e->key){
		case KEY_F2:
			if (cfg.has_xprn)
				print_help();
			else{
				set_term_astate(ast_noxprn);
				err_beep();
			}
			break;
		case KEY_UP:
		case KEY_NUMUP:
			if ((flag = hv_up()))
				draw_hlp_lines();
			break;
		case KEY_DOWN:
		case KEY_NUMDOWN:
			if ((flag = hv_dn()))
				draw_hlp_lines();
			break;
		case KEY_PGUP:
		case KEY_NUMPGUP:
			if ((flag = hv_pg_up()))
				draw_hlp_lines();
			break;
		case KEY_PGDN:
		case KEY_NUMPGDN:
			if ((flag = hv_pg_dn()))
				draw_hlp_lines();
			break;
		case KEY_HOME:
		case KEY_NUMHOME:
			if ((flag = hv_home()))
				draw_hlp_lines();
			break;
		case KEY_END:
		case KEY_NUMEND:
			if ((flag = hv_end()))
				draw_hlp_lines();
			break;
		case KEY_ESCAPE:
		case KEY_F10:
			return false;
	}
	if (flag && (_term_aux_state != ast_none))
		set_term_astate(ast_none);
	return true;
}
