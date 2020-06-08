/* Строка статуса терминала. (c) gsr, А.Попов 2001, 2009 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gui/exgdi.h"
#include "gui/options.h"
#include "gui/scr.h"
#include "gd.h"
#include "genfunc.h"
#include "keys.h"
#include "sterm.h"

bool scr_draw_bevel(int x, int y, int w, int h, int th, const char *txt,
		Color txt_color, Color bg_color)
{
	int bh = (h - th) / 2;
	GCPtr pGC = CreateGC(x, y, w, h);
	GCPtr pMemGC = CreateMemGC(w, th);
	SetFont(pMemGC, font_status);
	ClearGC(pMemGC, bg_color);
	SetTextColor(pMemGC, txt_color);
	if (txt != NULL)
		DrawText(pMemGC, 0, 0, w, th, txt, 0);
	DrawBorder(pMemGC, 0, 0, w, th, borderCX, clBtnShadow, clBtnHighlight);
	CopyGC(pGC, 0, bh, pMemGC, 0, 0, w, th);
	DeleteGC(pMemGC);
	DeleteGC(pGC);
	return true;
}

bool scr_set_lstatus(const char *s)
{
	if ((s != NULL) && scr_visible)
		return scr_draw_bevel(LSTATUS_LEFT,LSTATUS_TOP,
			LSTATUS_WIDTH,LSTATUS_HEIGHT,LSTATUS_TXT_HEIGHT,
			s,
			LSTATUS_TXT_COLOR,LSTATUS_BG_COLOR);
	else
		return false;
}

bool scr_set_led(char c)
{
	if (scr_visible){
		char s[2] = { c, 0 };
		scr_draw_bevel(HBYTE_LEFT,HBYTE_TOP,
			HBYTE_WIDTH,HBYTE_HEIGHT,HBYTE_TXT_HEIGHT,
			s,
			HBYTE_TXT_COLOR,HBYTE_BG_COLOR);

	}
	return scr_show_key(c != ' ');
}

bool scr_show_key(bool show)
{
	if (scr_visible){
		char s[2] = " ";
	
		if (show)
			s[0] = ds_key_char(kt);
		return scr_draw_bevel(KT_LEFT,KT_TOP,
			KT_WIDTH,KT_HEIGHT,KT_TXT_HEIGHT,
			s,
			KT_TXT_COLOR,KT_BG_COLOR);
	}
	return true;
}

bool scr_set_rstatus(const char *s)
{
	if ((s != NULL) && scr_visible){
		Color fg = RSTATUS_TXT_COLOR, bg = RSTATUS_BG_COLOR_A;
		int w = cfg.has_hint ? RSTATUS_WIDTH : RSTATUS_WIDTH1;
		if (*s == 0x01){
			s++;
			if (resp_executing){
				fg = RSTATUS_TXT_COLOR_ALRM;
				bg = RSTATUS_BG_COLOR_ALRM;
			}
		}else if (*s == 0)
			bg = RSTATUS_BG_COLOR;
		return scr_draw_bevel(RSTATUS_LEFT, RSTATUS_TOP,
			w, RSTATUS_HEIGHT, RSTATUS_TXT_HEIGHT,
			s, fg, bg);
	}else
		return false;
}

bool scr_show_log(bool show __attribute__((unused)))
{
	char s[4]="   ";
	Color bg = LOG_BG_COLOR;
	if (!scr_visible)
		return false;
	return scr_draw_bevel(LOG_LEFT, LOG_TOP, LOG_WIDTH, LOG_HEIGHT,
		LOG_TXT_HEIGHT, s, LOG_TXT_COLOR, bg);
}

bool scr_show_pgnum(bool show)
{
	if (scr_visible){
		char s[9];
	
		if (show)
			sprintf(s,"стр %.3d",page+1);
		else
			sprintf(s,"        ");
		return scr_draw_bevel(PGNUM_LEFT,PGNUM_TOP,
			PGNUM_WIDTH,PGNUM_HEIGHT,PGNUM_TXT_HEIGHT,
			s,
			PGNUM_TXT_COLOR,PGNUM_BG_COLOR);
	}
	return true;
}

bool scr_show_mode(bool show)
{
	char s[4]="   ";
	if (!scr_visible)
		return false;
	if (show){
		if (hex_input)
			strcpy(s,"hex");
		else if (key_input || wait_key)
			strcpy(s,"клч");
		else if (kbd_shift_state & SHIFT_CTRL)
			strcpy(s,"доп");
		else
			strcpy(s,"осн");
	}
	return scr_draw_bevel(MODE_LEFT,MODE_TOP,
		MODE_WIDTH,MODE_HEIGHT,MODE_TXT_HEIGHT,
		s,
		MODE_TXT_COLOR,MODE_BG_COLOR);
}

bool scr_show_language(bool show)
{
	char s[4]="   ";
	Color bg;
	if (!scr_visible)
		return false;
	bg = show ? LNG_BG_COLOR_A : LNG_BG_COLOR;
	if (show)
		strcpy(s,(kbd_lang == lng_rus ? "рус" : "лат"));
	
	
	return scr_draw_bevel(LNG_LEFT,LNG_TOP,
		LNG_WIDTH,LNG_HEIGHT,LNG_TXT_HEIGHT,
		s,
		LNG_TXT_COLOR,bg);
}

bool draw_clock(bool nowait)
{
	static time_t t0=0;
	time_t t = time(NULL) + time_delta;
	if (nowait || (t != t0)){
		struct tm *tt = localtime(&t);
		char s[32];
		GCPtr pGC = CreateGC(CLOCK_LEFT, CLOCK_TOP,
				CLOCK_WIDTH, CLOCK_HEIGHT);
		GCPtr pMemGC;
		sprintf(s,"%.2d.%.2d.%.2d %.2d:%.2d:%.2d",
			tt->tm_mday,tt->tm_mon+1,tt->tm_year%100,tt->tm_hour,tt->tm_min,tt->tm_sec);
		t0=t;

		SetFont(pGC, font_status);
		
		pMemGC = CreateMemGC(CLOCK_WIDTH, CLOCK_TXT_HEIGHT);
		SetFont(pMemGC, font_status);
		ClearGC(pMemGC, CLOCK_BG_COLOR);
		DrawBorder(pMemGC, 0, 0, CLOCK_WIDTH, CLOCK_TXT_HEIGHT,
				1, clBtnShadow, clBtnHighlight);
		SetTextColor(pMemGC, CLOCK_TXT_COLOR);
		DrawText(pMemGC, 0, 0, CLOCK_WIDTH, CLOCK_TXT_HEIGHT, s, 0);
		
		SetPenColor(pMemGC, clBtnShadow);
		Line(pMemGC, CLOCK_SEP, 2, CLOCK_SEP, CLOCK_TXT_HEIGHT-4);
		SetPenColor(pMemGC, clBtnHighlight);
		Line(pMemGC, CLOCK_SEP+1, 2, CLOCK_SEP+1, CLOCK_TXT_HEIGHT-4);
		
		CopyGC(pGC, 0, CLOCK_BEVEL_HEIGHT, pMemGC,
				0, 0, CLOCK_WIDTH, CLOCK_TXT_HEIGHT);
		DeleteGC(pMemGC);
		DeleteGC(pGC);
		return true;
	}else
		return false;
}
