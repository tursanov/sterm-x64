/* Гашение экрана терминала. (c) Alex Popov 2002 */

#include <sys/times.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kbd.h"
#include "sterm.h"
#include "gui/exgdi.h"
#include "gui/options.h"
#include "gui/scr.h"
#include "gui/ssaver.h"

static int x, y, tw, th, dx;
static int r, g, b;
static GCPtr pGC;

static void init_rand_rgb()
{
	r = 255*((double)rand()/(double)RAND_MAX);
	g = 255*((double)rand()/(double)RAND_MAX);
	b = 255*((double)rand()/(double)RAND_MAX);
}

#define SSAVER_TEXT	"ТМ \"Экспресс-2А-К\""

void init_ssaver(void)
{
	ssaver_active=true;
	set_scr_mode(m80x20,false,false);
	hide_cursor();
	scr_visible = false;
	
	pGC = CreateGC(0,0,800,600);
	SetFont(pGC, font32x8);
	SetBrushColor(pGC, clBlack);
	
	tw = TextWidth(font32x8, SSAVER_TEXT);
	th = TextHeight(font32x8);
	x = 0;
	y = (600-th)/2;
	dx = 1;
	init_rand_rgb();

	draw_ssaver();
}

void release_ssaver(void)
{
	DeleteGC(pGC);
	ClearScreen(clBtnFace);
	ssaver_active=false;
}

bool draw_ssaver(void)
{
	ClearScreen(clBlack);
	return true;
}

#define MOVE_TIME	1UL	/* ссек */
#define RGB_THRESHOLD	100

bool move_text(void)
{
	static uint32_t prev_t=0;
	uint32_t current_t = u_times();
	
	if (current_t-prev_t > MOVE_TIME) {
		FillBox(pGC, x, y, tw, th);
		
		x += dx;
		if (x > 800-tw)
		{
			x = 800-tw;
			dx = -dx;
		}
		else if (x < 0)
		{
			dx = -dx;
			x = 0;
		}
		if (r >= RGB_THRESHOLD) r--;
		if (g >= RGB_THRESHOLD) g--;
		if (b >= RGB_THRESHOLD) b--;
		if (r < RGB_THRESHOLD && g < RGB_THRESHOLD && b < RGB_THRESHOLD)
			init_rand_rgb();
		
		SetTextColor(pGC, RGB(r, g, b));
		TextOut(pGC, x, y, SSAVER_TEXT);
			
		prev_t = current_t;
		return true;
	}

	return false;
}

bool process_ssaver(struct kbd_event *e)
{
	if (e->key == KEY_NONE)
	{
		move_text();
		return true;
	}
	return false;
}
