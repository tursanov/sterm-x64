/* Просмотр обмена с "Экспресс" по TCP/IP. (c) gsr 2004 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prn/express.h"
#include "genfunc.h"
#include "kbd.h"
#include "xchange.h"
#include "gui/scr.h"
#include "gui/options.h"
#include "gui/exgdi.h"
#include "gui/xchange.h"

/* Номер первой выводимой на экране строки */
static int first_line_index;

extern bool xchg_active;

/* Определение числа строк (включая заголовок) в одной записи */
static int count_item_lines(int index)
{
	struct xlog_item *p = xlog_get_item(index);
	if (p == NULL)
		return 0;
	else
		return (p->len + SCR_LINE_ELEMS - 1) / SCR_LINE_ELEMS +
			NR_HEADER_LINES + NR_FOOTER_LINES;
}

/* Определение общего числа строк (включая заголовки) */
static int count_all_lines(void)
{
	int i, n = xlog_count_items(), m = 0;
	for (i = 0; i < n; i++)
		m += count_item_lines(i);
	return m ? m - 1 : 0;	/* без учета последней пустой строки */
}

/* Перемещение по журналу. Возвращают true, если изменились текущие индексы. */
static bool line_up(void)
{
	if (first_line_index > 0){
		first_line_index--;
		return true;
	}else
		return false;
}

static bool line_dn(void)
{
	int n = count_all_lines();
	if (first_line_index < (n - 1)){
		first_line_index++;
		return true;
	}else
		return false;
}

static bool page_up(void)
{
	if (first_line_index > 0){
		first_line_index -= SCR_LINES;
		if (first_line_index < 0)
			first_line_index = 0;
		return true;
	}else
		return false;
}

static bool page_dn(void)
{
	int n = count_all_lines();
	if (first_line_index < (n - SCR_LINES)){
		first_line_index += SCR_LINES;
		return true;
	}else
		return false;
}

static bool home(void)
{
	if (first_line_index > 0){
		first_line_index = 0;
		return true;
	}else
		return false;
}

static bool end(void)
{
	int n = count_all_lines();
	if (first_line_index < (n - SCR_LINES)){
		first_line_index = n - SCR_LINES;
		return true;
	}else
		return false;
}

__attribute__((format (printf, 2, 3))) static void xprintf(char *s, char *format, ...)
{
	va_list p;
	va_start(p, format);
	vsprintf(s + strlen(s), format, p);
	va_end(p);
}

/* Получение заданной строки записи */
static bool get_item_line(char *buf, int item_index, int line_index,
		int nr_lines, bool prn)
{
	int offs, i;
	struct xlog_item *p = xlog_get_item(item_index);
	if (p == NULL)
		return false;
	if (line_index == 0){
		sprintf(buf, "%.2d.%.2d.%.2d %.2d:%.2d:%.2d ",
			p->dt.day + 1, p->dt.mon + 1, p->dt.year + 2000,
			p->dt.hour, p->dt.min, p->dt.sec);
		if (p->dir == xlog_in)
			xprintf(buf, "\"ЭКСПРЕСС\"->ТМ");
		else
			xprintf(buf, "ТМ->\"ЭКСПРЕСС\"");
		xprintf(buf, " (%d) ", p->len);
		if (p->dir == xlog_in)
			xprintf(buf, "ONПО: %.3hX ONТЗ: %.3hX OБП: %.2hX",
				p->Npo, p->Ntz, (uint16_t)p->Bp);
		else
			xprintf(buf, "ZNТЗ: %.3hX ZNПО: %.3hX ZБП: %.2hX",
				p->Ntz, p->Npo, (uint16_t)p->Bp);
	}else if (line_index == (nr_lines - 1))
		*buf = 0;
	else{
		offs = (line_index - 1) * SCR_LINE_ELEMS;
		sprintf(buf, prn ? "%.4X   " : "%.4X    ", offs);
		for (i = 0; i < SCR_LINE_ELEMS; i++){
			if (i == SCR_DASH_POS)
				xprintf(buf, " --");
			if ((offs + i) < p->len)
				xprintf(buf, " %.2hX", (uint16_t)p->data[offs + i]);
			else
				xprintf(buf, "   ");
		}
		xprintf(buf, prn ? "    " : "     ");
		for (i = 0; i < SCR_LINE_ELEMS; i++){
			if ((offs + i) < p->len){
				if (p->data[offs + i] < 0x20)
					xprintf(buf, ".");
				else
					xprintf(buf, "%c", recode(p->data[offs + i]));
			}else
				xprintf(buf, " ");
		}
	}
	i = strlen(buf);
	if (!prn){
		for (; i < SCR_COLS; i++)
			buf[i] = ' ';
	}
	buf[i] = 0;
	return true;
}

/* Рисование журнала обмена данными */
bool draw_xchange(void)
{
	int i, n, m, k, l, x, y;
	int tw = XLOG_FONT->max_width;
	int th = XLOG_FONT->max_height;
	int ww = tw * SCR_COLS;
	int hh = th * SCR_LINES + 35;
	char buf[SCR_COLS + 1], buf1[65], cell[2] = "";
	GCPtr pGC = CreateGC(((DISCX - ww) / 2), 30, ww, hh);
	GCPtr pMemGC = CreateMemGC(ww, hh);
	SetFont(pMemGC, XLOG_FONT);
	ClearGC(pMemGC, XLOG_BG_COLOR);
/* Ищем первую выводимую запись и индекс первой выводимой строки в ней */
	n = xlog_count_items();
	for (m = k = 0; k < n; k++){
		l = count_item_lines(k);
		if ((m + l) > first_line_index)
			break;
		m += l;
	}
	if (k == n){	/* запись не найдена, переходим к первой */
		first_line_index = 0;
		m = k = 0;
	}
	for (y = 20; (m < (first_line_index + SCR_LINES)) && (k < n); k++){
		l = count_item_lines(k);
		for (i = 0; i < l; i++){
			if ((m + i) < first_line_index)
				continue;
			else if ((m + i) >= (first_line_index + SCR_LINES))
				break;
			else if (get_item_line(buf, k, i, l, false)){
				SetTextColor(pMemGC, i ? XLOG_DATA_COLOR : XLOG_HEADER_COLOR);
				SetBrushColor(pMemGC, XLOG_BG_COLOR);
				if ((i == 0) || (i == (l - 1)))
					TextOut(pMemGC, 0, y, buf);
				else{
					memcpy(buf1, buf, 64);
					buf1[64] = 0;
					TextOut(pMemGC, 0, y, buf1);
					for (x = 64; x < 80; x++){
						if (isalpha(buf[x]))
							SetTextColor(pMemGC, XLOG_LAT_COLOR);
						else
							SetTextColor(pMemGC, XLOG_RUS_COLOR);
						cell[0] = buf[x];
						TextOut(pMemGC, x * tw, y, cell);
					}
				}
			}
			y += th;
		}
		m += l;
	}
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC));
	DeleteGC(pGC);
	DeleteGC(pMemGC);
	kbd_flush_queue();
	return true;
}

static bool printing;

/* Печать текущего экрана обмена */
static void print_xchange(void)
{
	static char buf[(SCR_LINES + 1) * (SCR_COLS + 2) + 3];
	char *p;
	int i, n, m, k, l;
	sprintf(buf, "ТЕРМИНАЛ \"ЭКСПРЕСС-2А-К\" %.2hX:%.2hX. ОБМЕН С ХОСТ-ЭВМ ПО TCP/IP\n\r",
			(uint16_t)cfg.gaddr, (uint16_t)cfg.iaddr);
	p = buf + strlen(buf);
/* Ищем первую выводимую запись и индекс первой выводимой строки в ней */
	n = xlog_count_items();
	for (m = k = 0; k < n; k++){
		l = count_item_lines(k);
		if ((m + l) > first_line_index)
			break;
		m += l;
	}
	if (k == n){	/* запись не найдена, переходим к первой */
		first_line_index = 0;
		m = k = 0;
	}
	for (; (m < (first_line_index + SCR_LINES)) && (k < n); k++){
		l = count_item_lines(k);
		for (i = 0; i < l; i++){
			if ((m + i) < first_line_index)
				continue;
			else if ((m + i) >= (first_line_index + SCR_LINES))
				break;
			else if (get_item_line(p, k, i, l, true)){
				p += strlen(p);
				*p++ = '\n';
				*p++ = '\r';
			}
		}
		m += l;
	}
	*p++ = '\x1c';
	*p++ = '\x0b';
	l = p - buf;
/* Буфер готов, выводим его на печать */
	recode_str(buf, l);
	printing = true;
	xprn_print(buf, l);
	printing = false;
}

/* Вызывается при добавлении нового сообщения в журнал */
void on_new_xchg_item(void)
{
	end();
	draw_xchange();
}

/* Обработка клавиатуры */
int process_xchange(struct kbd_event *e)
{
	bool flag = false;
	if ((e == NULL) || !e->pressed)
		return cmd_none;
	else if (!printing && kbd_exact_shift_state(e, SHIFT_CTRL) &&
			(e->key == KEY_J))
		return cmd_exit;
	else if (e->shift_state & (SHIFT_SHIFT | SHIFT_ALT | SHIFT_CTRL))
		return cmd_none;
	else if (printing){
		if (!e->repeated)
			err_beep();
		return cmd_none;
	}
	switch (e->key){
		case KEY_UP:
		case KEY_NUMUP:
			flag = line_up();
			break;
		case KEY_DOWN:
		case KEY_NUMDOWN:
			flag = line_dn();
			break;
		case KEY_PGUP:
		case KEY_NUMPGUP:
			flag = page_up();
			break;
		case KEY_PGDN:
		case KEY_NUMPGDN:
			flag = page_dn();
			break;
		case KEY_HOME:
		case KEY_NUMHOME:
			flag = home();
			break;
		case KEY_END:
		case KEY_NUMEND:
			flag = end();
			break;
		case KEY_F2:
			if (cfg.has_xprn)
				print_xchange();
			else{
				set_term_astate(ast_noxprn);
				err_beep();
			}
			break;
		case KEY_ENTER:
			return cmd_enter;
		case KEY_ESCAPE:
			return cmd_exit;
	}
	if (flag){
		if (_term_aux_state != ast_none)
			set_term_astate(ast_none);
		draw_xchange();
	}
	return cmd_none;
}

void init_xchg_view(void)
{
	xchg_active = true;
	set_scr_mode(m80x20, false, false);
	set_term_state(st_xchange);
	set_term_astate(ast_none);
	hide_cursor();
	scr_show_log(false);
	scr_show_pgnum(false);
	scr_show_mode(false);
	scr_show_language(false);
/*	scr_visible = false;*/
	end();
	draw_scr_bevel();
	draw_xchange();
}

void release_xchg_view(void)
{
	xchg_active = false;
}
