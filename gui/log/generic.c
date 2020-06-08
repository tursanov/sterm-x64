/* Общие функции для работы с контрольными лентами. (c) gsr 2009 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "gui/log/generic.h"
#include "gui/scr.h"
#include "gui/status.h"
#include "prn/express.h"
#include "prn/local.h"
#include "cfg.h"
#include "kbd.h"
#include "paths.h"
#include "sterm.h"

/* Очистка экранного буфера */
void log_clear_ctx(struct log_gui_context *ctx)
{
	ctx->scr_data_len = 0;
	ctx->scr_data_lines = 0;
	ctx->first_line = 0;
}

/* Определение длины команды XPRN_PRNOP; index указывает на код команды */
static int log_get_prnop_len(const uint8_t *data __attribute__((unused)),
	uint32_t len, int index)
{
	uint32_t ret = len - index;
	if (ret > 4)
		ret = 4;
	return ret;
/*	int k;
	uint8_t b;
	for (k = index + 1; k < len; k++){
		b = data[k];
		switch (b){
			case XPRN_PRNOP_HPOS_RIGHT:
			case XPRN_PRNOP_HPOS_LEFT:
			case XPRN_PRNOP_HPOS_ABS:
			case XPRN_PRNOP_VPOS_FW:
			case XPRN_PRNOP_VPOS_BK:
			case XPRN_PRNOP_VPOS_ABS:
			case XPRN_PRNOP_INP_TICKET:
			case XPRN_PRNOP_LINE_FW:
				return k - index + 1;
		}
	}
	return k - index;*/
}

/* Определение длины команды ЧТ/ПЧ ШТРИХ; index указывает на код команды */
static int log_get_bcode_len(const uint8_t *data, uint32_t len, uint32_t index)
{
	enum {
		st_start,
		st_digit1,
		st_semi1,
		st_digit2,
		st_semi2,
		st_stop,
	};
	int st = st_start, k, n = 0;
	uint8_t b;
	for (k = index + 1; (k < len); k++){
		b = data[k];
		switch (st){
			case st_start:
				if (b == ';')
					st = st_semi1;
				else if (isdigit(b)){
					n = 2;
					st = st_digit1;
				}else
					st = st_stop;
				break;
			case st_semi1:
				if (b == ';')
					st = st_semi2;
				else if (isdigit(b)){
					n = 14;
					st = st_digit2;
				}else
					st = st_stop;
				break;
			case st_digit1:
				if (--n == 0){
					if (b == ';')
						st = st_semi2;
					else if (isdigit(b)){
						n = 14;
						st = st_digit2;
					}else
						st = st_stop;
				}else if (!isdigit(b))
					st = st_stop;
				break;
			case st_semi2:
				if (isdigit(b)){
					n = 12;
					st = st_digit2;
				}else
					st = st_stop;
				break;
			case st_digit2:
				if (!isdigit(b) || !--n)
					st = st_stop;
				break;
		}
		if (st == st_stop)
			break;
	}
	return k - index + 1;
}

/* Определение длины команды ПЧ ШТРИХ2 для ППУ; index указывает на код команды */
static int log_get_bcode2_len(const uint8_t *data, uint32_t len, uint32_t index)
{
	enum {
		st_type,
		st_x,
		st_y,
		st_number,
		st_len,
		st_data,
		st_stop,
	};
	int st = st_type, k, n = 0, data_len = 0;
	uint8_t b;
	for (k = index + 1; (k < len); k++){
		b = data[k];
		switch (st){
			case st_type:
				if (isdigit(b)){
					n = 0;
					st = st_x;
				}else
					st = st_stop;
				break;
			case st_x:
				if ((n == 0) && (b == 0x3b))
					st = st_number;
				else if (!isdigit(b))
					st = st_stop;
				else if (++n == 3){
					n = 0;
					st = st_y;
				}
				break;
			case st_y:
				if (!isdigit(b))
					st = st_stop;
				else if (++n == 3){
					n = 0;
					st = st_len;
				}
				break;
			case st_number:
				if ((b != 0x31) && (b != 0x32) && (b != 0x33))
					st = st_stop;
				else{
					n = 0;
					st = st_len;
				}
				break;
			case st_len:
				if (!isdigit(b))
					st = st_stop;
				else{
					data_len *= 10;
					data_len += b - 0x30;
					if (++n == 3){
						if (data_len == 0)
							st = st_stop;
						else{
							n = 0;
							st = st_data;
						}
					}
				}
				break;
			case st_data:
				if (++n == data_len)
					st = st_stop;
				break;
		}
		if (st == st_stop)
			break;
	}
	return k - index + 1;
}

/* Определение длины команды без учета Ар2 */
int log_get_cmd_len(const uint8_t *data, uint32_t len, int index)
{
	switch (data[index]){
		case XPRN_FONT:
		case XPRN_VPOS:
		case X_RD_PROM:
		case X_RD_ROM:
			return 2;
		case X_REPEAT:
			return 3;
		case LPRN_INTERLINE:
			return 4;
		case XPRN_WR_BCODE:
		case XPRN_RD_BCODE:
			return log_get_bcode_len(data, len, index);
		case LPRN_WR_BCODE2:
			return log_get_bcode2_len(data, len, index);
		case XPRN_PRNOP:
			return log_get_prnop_len(data, len, index);
		default:
			return 1;
	}
}

/* Запись в экранный буфер заданной строки */
void log_fill_scr_str(struct log_gui_context *ctx,
		const char *format, ...)
{
	static char buf[LOG_SCREEN_COLS + 1];
	int i;
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	ctx->scr_data_len = strlen(buf);
	if (ctx->scr_data_len == 0)
		ctx->scr_data_lines = 0;
	else{
		ctx->scr_data_len++;
		ctx->scr_data[0] = 0;
		for (i = 1; i < ctx->scr_data_len; i++)
			ctx->scr_data[i] = recode(buf[i - 1]);
		ctx->scr_data_lines = 1;
	}
}

/*
 * Добавление в экранный буфер строки. Возвращает false в случае переполнения
 * экранного буфера.
 */
bool log_add_scr_str(struct log_gui_context *ctx, bool need_recode, const char *format, ...)
{
	static char s[LOG_SCREEN_COLS + 1];
	int i, len;
	va_list ap;
	va_start(ap, format);
	vsnprintf(s, sizeof(s), format, ap);
	va_end(ap);
	len = strlen(s);
	if ((ctx->scr_data_len + len + 2) <= ctx->scr_data_size){
		ctx->scr_data[ctx->scr_data_len++] = 0;
		for (i = 0; i < len; i++){
			char ch = s[i];
			if (need_recode)
				ch = recode(ch);
			ctx->scr_data[ctx->scr_data_len++] = ch;
		}
		ctx->scr_data[ctx->scr_data_len++] = 0;
		ctx->scr_data_lines++;
		return true;
	}else
		return false;
}

/* Вывод заголовка записи контрольной ленты */
static void log_draw_rec_header(struct log_gui_context *ctx)
{
	int i;
	Color tmp = ctx->mem_gc->brushcolor;
	SetFont(ctx->mem_gc, font_status);
	ctx->mem_gc->brushcolor = clRopnetLightBlue;
	FillBox(ctx->mem_gc, 0, 0, GetCX(ctx->mem_gc),
		font80x20->max_height * ctx->nr_head_lines);
	ctx->mem_gc->brushcolor = tmp;
	SetTextColor(ctx->mem_gc, clRopnetDarkBlue);
	for (i = 0; i < ctx->nr_head_lines; i++)
		TextOut(ctx->mem_gc, 0, i * font80x20->max_height,
			ctx->get_head_line(i));
}

static bool log_is_lat(const struct log_gui_context *ctx, uint8_t c)
{
	bool ret = false;
	if (ctx->asis)
		ret = ((c >= 0x41) && (c <= 0x5a)) || ((c >= 0x61) && (c <= 0x7a));
	else
		ret = (c >= 0x40) && (c <= 0x5f);
	return ret;
}

/* Определение цвета выводимого символа и фона */
static void log_get_char_color(const struct log_gui_context *ctx, uint8_t c, Color *fg, Color *bg)
{
	bool inverse = !ctx->asis && (c & 0x80);
	if (!ctx->asis)
		c &= 0x7f;
	if (inverse){
		*fg = cfg.bg_color;
		*bg = cfg.rus_color;
	}else{
		*bg = cfg.bg_color;
		if (log_is_lat(ctx, c))
			*fg = cfg.lat_color;
		else
			*fg = cfg.rus_color;
	}
}

/* Вывод строки записи контрольной ленты. Возвращает длину выведенной строки. */
static int log_draw_rec_line(const struct log_gui_context *ctx, int line, int index)
{
	int n = ctx->scr_data[index++],
	    w = font80x20->max_width, h = font80x20->max_height;
	uint8_t s[2] = {0, 0};
	uint8_t b;
	bool underline;
	Color fg, bg, tmp;
	for (; (n < LOG_SCREEN_COLS) && (index < ctx->scr_data_len); n++, index++){
		b = ctx->scr_data[index];
		if (b == 0)
			break;
		log_get_char_color(ctx, b, &fg, &bg);
		if (!ctx->asis)
			b &= 0x7f;
		underline = !ctx->asis && log_is_lat(ctx, b);
		if (bg != cfg.bg_color){
			tmp = ctx->mem_gc->brushcolor;
			ctx->mem_gc->brushcolor = bg;
			FillBox(ctx->mem_gc, n * w, line * h, w, h);
			ctx->mem_gc->brushcolor = tmp;
		}	
		*s = ctx->asis ? b : recode(b);
		SetTextColor(ctx->mem_gc, fg);
		TextOut(ctx->mem_gc, n * w, line * h, (char *)s);
		if (underline){
			SetPenColor(ctx->mem_gc, fg);
			Line(ctx->mem_gc, n * w + 1, (line + 1) * h - 2,
				(n + 1) * w - 1, (line + 1) * h-2);
		}
	}
	return n;
}

/* Вывод записи контрольной ленты */
static void log_draw_rec_data(struct log_gui_context *ctx)
{
	int i, line;
	bool col = true;
	SetFont(ctx->mem_gc, font80x20);
	for (i = line = 0; (i < ctx->scr_data_len) && (line < ctx->first_line); i++){
		if (col)
			col = false;
		else if (ctx->scr_data[i] == 0){
			line++;
			col = true;
		}
	}
	if (line < ctx->first_line)
		return;
	for (line = 0; (i < ctx->scr_data_len) &&
			(line < (LOG_SCREEN_LINES - ctx->nr_head_lines));
			i++, line++){
		log_draw_rec_line(ctx, line + ctx->nr_head_lines, i);
		for (i++; ctx->scr_data[i] && (i < ctx->scr_data_len); i++);
	}
	kbd_flush_queue();
}

/* Геометрия поля справки */
#define HINT_XOFFS		4
#define HINT_YOFFS		4
#define HINT_FRAME_WIDTH	1
#define HINT_FIELD_WIDTH	(DISCX - 2 * HINT_XOFFS)
#define HINT_FIELD_HEIGHT	72
#define HINT_TEXT_WIDTH		(HINT_FIELD_WIDTH - 2 * HINT_HMARGIN)
#define HINT_TEXT_HEIGHT	(HINT_FIELD_HEIGHT - 2 * HINT_VMARGIN)
#define HINT_FONT		_("fonts/terminal10x18.fnt")
#define HINT_FIELD_COLOR	clRopnetBrown
#define HINT_FRAME_COLOR	clRopnetDarkBrown
#define HINT_TEXT_COLOR		clBlack
#define HINT_KEY_COLOR		clRed

/* Рисование поля справки */
static void log_draw_hints(struct log_gui_context *ctx)
{
	GCPtr gc = CreateGC(HINT_XOFFS,
		DISCY - (HINT_YOFFS + HINT_FIELD_HEIGHT),
		HINT_FIELD_WIDTH, HINT_FIELD_HEIGHT);
	GCPtr mgc = CreateMemGC(HINT_FIELD_WIDTH, HINT_FIELD_HEIGHT);
	FontPtr fnt = CreateFont(HINT_FONT, false);
	
	SetFont(mgc, fnt);
	int cx = fnt->max_width;
	int cy = fnt->max_height;
	int txt_h = ctx->nr_hint_lines * cy;
	int y = (HINT_FIELD_HEIGHT - txt_h) / 2;
	ClearGC(mgc, HINT_FIELD_COLOR);
	DrawBorder(mgc, 0, 0, HINT_FIELD_WIDTH, HINT_FIELD_HEIGHT,
		HINT_FRAME_WIDTH, HINT_FRAME_COLOR, HINT_FRAME_COLOR);
	
	for (int i = 0, n = 0; i < ctx->nr_hint_lines; i++){
		bool key = true, key_start = true;
		const char *s = ctx->get_hint_line(i);
		int l = strlen(s);
		int x = (HINT_FIELD_WIDTH - l * cx) / 2;
		for (int j = 0; s[j]; j++){
			char c = s[j];
			if (key){
				if (c == ' '){
					if (!key_start){
						key = false;
						n = 0;
					}
				}else
					key_start = false;
			}else if (c == ' '){
				n++;
				if (n == 2)
					key = key_start = true;
			}else
				n = 0;
			SetTextColor(mgc, key ? HINT_KEY_COLOR : HINT_TEXT_COLOR);
			char buf[] = {c, 0};
			TextOut(mgc, x + j * cx, y + i * cy, buf);
		}
	}
	CopyGC(gc, 0, 0, mgc, 0, 0, HINT_FIELD_WIDTH, HINT_FIELD_HEIGHT);
	DeleteFont(fnt);
	DeleteGC(mgc);
	DeleteGC(gc);
}

/* Можно ли показывать данную запись */
static bool log_can_show_rec(struct log_gui_context *ctx, uint32_t index)
{
	return (ctx->filter == NULL) || ctx->filter(ctx->hlog, index);
}

/* Рисование контрольной ленты */
bool log_draw(struct log_gui_context *ctx)
{
	ClearGC(ctx->mem_gc, cfg.bg_color);
	log_draw_rec_header(ctx);
	if ((ctx->hlog->hdr->n_recs > 0) && log_can_show_rec(ctx, ctx->cur_rec_index))
		log_draw_rec_data(ctx);
	CopyGC(ctx->gc, 0, 0, ctx->mem_gc, 0, 0,
			GetCX(ctx->mem_gc), GetCY(ctx->mem_gc));
	if (cfg.has_hint)
		log_draw_hints(ctx);
	return true;
}

/* Перерисовка всего экрана */
void log_redraw(struct log_gui_context *ctx)
{
	redraw_term(false, ctx->title);
	scr_show_pgnum(false);
	scr_show_mode(false);
	scr_show_language(false);
	scr_show_log(false);
}

/* Чтение и прорисовка заданной записи */
static bool log_seek_rec(struct log_gui_context *ctx, uint32_t index)
{
	if ((ctx->hlog->hdr->n_recs > 0) && ctx->read_rec(ctx->hlog, index)){
		ctx->cur_rec_index = index;
		ctx->fill_scr_data(ctx);
		ctx->first_line = 0;
		return log_draw(ctx);
	}else
		return false;
}

/* Перемещение между записями */
static bool log_next_rec(struct log_gui_context *ctx)
{
	bool ret = false;
	uint32_t idx = ctx->cur_rec_index;
	while (!ret && ((idx + 1) < ctx->hlog->hdr->n_recs)){
		idx++;
		ret = log_can_show_rec(ctx, idx);
	}
	if (ret)
		ctx->cur_rec_index = idx;
	return ret;
}

static bool log_prev_rec(struct log_gui_context *ctx)
{
	bool ret = false;
	uint32_t idx = ctx->cur_rec_index;
	while (!ret && (idx > 0)){
		idx--;
		ret = log_can_show_rec(ctx, idx);
	}
	if (ret)
		ctx->cur_rec_index = idx;
	return ret;
}

static bool log_next_group(struct log_gui_context *ctx, bool big)
{
	bool ret = false;
	uint32_t delta = big ? LOG_BGROUP_LEN : LOG_GROUP_LEN;
	uint32_t idx = ctx->cur_rec_index;
	while (!ret && ((idx + 1) < ctx->hlog->hdr->n_recs)){
		if (delta > 0){
			idx += delta;
			if (idx >= ctx->hlog->hdr->n_recs)
				idx = ctx->hlog->hdr->n_recs - 1;
			delta = 0;
		}else
			idx++;
		ret = log_can_show_rec(ctx, idx);
	}
	if (ret)
		ctx->cur_rec_index = idx;
	return ret;
}

static bool log_prev_group(struct log_gui_context *ctx, bool big)
{
	bool ret = false;
	uint32_t delta = big ? LOG_BGROUP_LEN : LOG_GROUP_LEN;
	uint32_t idx = ctx->cur_rec_index;
	while (!ret && (idx > 0)){
		if (delta > 0){
			if (idx < delta)
				idx = 0;
			else
				idx -= delta;
			delta = 0;
		}else
			idx--;
		ret = log_can_show_rec(ctx, idx);
	}
	if (ret)
		ctx->cur_rec_index = idx;
	return ret;
}

static bool log_first_rec(struct log_gui_context *ctx)
{
	bool ret = false;
	uint32_t idx0 = ctx->cur_rec_index, idx = idx0;
	if (idx > 0){
		idx = 0;
		ret = log_can_show_rec(ctx, idx);
		while (!ret && (idx < idx0)){
			idx++;
			ret = log_can_show_rec(ctx, idx);
		}
		if (ret)
			ctx->cur_rec_index = idx;
	}
	return ret;
}

static bool log_last_rec(struct log_gui_context *ctx)
{
	bool ret = false;
	uint32_t idx0 = ctx->cur_rec_index, idx = idx0;
	if ((idx + 1) < ctx->hlog->hdr->n_recs){
		idx = ctx->hlog->hdr->n_recs - 1;
		ret = log_can_show_rec(ctx, idx);
		while (!ret && (idx > idx0)){
			idx--;
			ret = log_can_show_rec(ctx, idx);
		}
		if (ret)
			ctx->cur_rec_index = idx;
	}
	return ret;
}

/* Перемещение внутри записи. Возвращает true, если надо перерисовать запись */
static bool log_line_up(struct log_gui_context *ctx)
{
	bool ret = false;
	if (ctx->first_line > 0){
		ctx->first_line--;
		ret = true;
	}
	return ret;
}

static bool log_line_down(struct log_gui_context *ctx)
{
	bool ret = false;
	if ((ctx->first_line + LOG_SCREEN_LINES - ctx->nr_head_lines) <
			ctx->scr_data_lines){
		ctx->first_line++;
		ret = true;
	}
	return ret;
}

static bool log_pg_up(struct log_gui_context *ctx)
{
	bool ret = false;
	if (ctx->first_line > 0){
		uint32_t delta = LOG_SCREEN_LINES - ctx->nr_head_lines;
		if (ctx->first_line < delta)
			ctx->first_line = 0;
		else
			ctx->first_line -= delta;
		ret = true;
	}
	return ret;
}

static bool log_pg_down(struct log_gui_context *ctx)
{
	bool ret = false;
	uint32_t delta = LOG_SCREEN_LINES - ctx->nr_head_lines;
	if ((ctx->first_line + delta) < ctx->scr_data_lines){
		ctx->first_line += delta;
		ret = true;
	}
	return ret;
}

static bool log_home(struct log_gui_context *ctx)
{
	bool ret = false;
	if (ctx->first_line > 0){
		ctx->first_line = 0;
		ret = true;
	}
	return ret;
}

static bool log_end(struct log_gui_context *ctx)
{
	bool ret = false;
	uint32_t n = LOG_SCREEN_LINES - ctx->nr_head_lines;
	if ((ctx->first_line + n) < ctx->scr_data_lines){
		ctx->first_line = ctx->scr_data_lines - n;
		ret = true;
	}
	return ret;
}

/* Поиск записи контрольной ленты по её номеру */
bool log_show_rec_by_number(struct log_gui_context *ctx, uint32_t number)
{
	bool ret = false;
	uint32_t index = log_find_rec_by_number(ctx->hlog, number - 1);
	if (index != -1)
		ret = log_seek_rec(ctx, index);
	else
		set_term_astate(ast_no_log_item);
	return ret;
}

/* Поиск записи контрольной ленты по дате создания */
bool log_show_rec_by_date(struct log_gui_context *ctx, struct date_time *dt)
{
	bool ret = false;
	uint32_t index = log_find_rec_by_date(ctx->hlog, dt);
	if (index != -1)
		ret = log_seek_rec(ctx, index);
	else
		set_term_astate(ast_no_log_item);
	return ret;
}

/* Обработчик событий клавиатуры в норммальном состоянии */
static int log_process_normal(struct log_gui_context *ctx,
		struct kbd_event *e)
{
	bool redraw = false, chrec = false;
	if (!e->pressed)
		return cmd_none;
	set_term_astate(ast_none);
	if (e->shift_state == 0){
		switch (e->key){
			case KEY_UP:
			case KEY_NUMUP:
				redraw = log_line_up(ctx);
				break;
			case KEY_DOWN:
			case KEY_NUMDOWN:
				redraw = log_line_down(ctx);
				break;
			case KEY_PGUP:
			case KEY_NUMPGUP:
				redraw = log_pg_up(ctx);
				break;
			case KEY_PGDN:
			case KEY_NUMPGDN:
				redraw = log_pg_down(ctx);
				break;
			case KEY_HOME:
			case KEY_NUMHOME:
				chrec = log_first_rec(ctx);
				break;
			case KEY_END:
			case KEY_NUMEND:
				chrec = log_last_rec(ctx);
				break;
			case KEY_LEFT:
			case KEY_NUMLEFT:
				chrec = log_prev_rec(ctx);
				break;
			case KEY_RIGHT:
			case KEY_NUMRIGHT:
				chrec = log_next_rec(ctx);
				break;
			case KEY_ESCAPE:
				return cmd_exit;
		}
	}else if (kbd_exact_shift_state(e, SHIFT_CTRL)){
		switch (e->key){
			case KEY_LEFT:
			case KEY_NUMLEFT:
				chrec = log_prev_group(ctx, false);
				break;
			case KEY_RIGHT:
			case KEY_NUMRIGHT:
				chrec = log_next_group(ctx, false);
				break;
			case KEY_HOME:
			case KEY_NUMHOME:
				redraw = log_home(ctx);
				break;
			case KEY_END:
			case KEY_NUMEND:
				redraw = log_end(ctx);
				break;
		}
	}else if (kbd_exact_shift_state(e, SHIFT_CTRL | SHIFT_ALT)){
		switch (e->key){
			case KEY_LEFT:
			case KEY_NUMLEFT:
				chrec = log_prev_group(ctx, true);
				break;
			case KEY_RIGHT:
			case KEY_NUMRIGHT:
				chrec = log_next_group(ctx, true);
				break;
		}
	}
	if (chrec){
		if (ctx->read_rec(ctx->hlog, ctx->cur_rec_index))
			ctx->fill_scr_data(ctx);
		ctx->first_line = 0;
		redraw = true;
	}
	if (redraw)
		log_draw(ctx);
	return cmd_none;
}

/* Обработчик событий клавиатуры */
int log_process(struct log_gui_context *ctx, struct kbd_event *e)
{
	int cmd = cmd_none;
	if (ctx->handle_kbd != NULL)
		cmd = ctx->handle_kbd(ctx, e);
	if (cmd == cmd_none){
		if (!ctx->modal)
			cmd = log_process_normal(ctx, e);
	}
	return cmd;
}

/* Инициализация интерфейса пользователя */
bool log_init_view(struct log_gui_context *ctx)
{
	int w = LOG_SCREEN_COLS * font80x20->max_width;
	int h = LOG_SCREEN_LINES * font80x20->max_height;
	if (ctx->init != NULL)
		ctx->init(ctx);
	*ctx->active = true;
	set_term_busy(true);
	set_scr_mode(m80x20, false, false);
	set_term_state(st_log);
	set_term_astate(ast_none);
	hide_cursor();
	ctx->modal = false;
	bool empty = true;
	uint32_t idx = 0;
	if (ctx->hlog->hdr->n_recs > 0){
		idx = ctx->hlog->hdr->n_recs - 1;
		empty = !log_can_show_rec(ctx, idx);
		while (empty && (idx > 0)){
			idx--;
			empty = !log_can_show_rec(ctx, idx);
		}
	}
	if (empty)
		ctx->cur_rec_index = 0;
	else{
		ctx->cur_rec_index = idx;
		ctx->read_rec(ctx->hlog, idx);
		ctx->fill_scr_data(ctx);
	}
	ctx->first_line = 0;
	ctx->mem_gc = CreateMemGC(w, h);
	ctx->gc = CreateGC((DISCX - w) / 2, titleCY, w, h);
	log_redraw(ctx);
	return true;
}

/* Освобождение интерфейса пользователя */
void log_release_view(struct log_gui_context *ctx)
{
	DeleteGC(ctx->mem_gc);
	ctx->mem_gc = NULL;
	DeleteGC(ctx->gc);
	ctx->gc = NULL;
	*ctx->active = false;
}
