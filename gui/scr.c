/* Основные функции для работы с экраном. (c) gsr, Alex Popov 2000-2004 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <linux/kd.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "gui/exgdi.h"
#include "gui/options.h"
#include "gui/status.h"
#include "gui/scr.h"
#include "prn/aux.h"
#include "prn/express.h"
#include "prn/local.h"
#include "kkt/kkt.h"
#include "kkt/fd/ad.h"
#include "gd.h"
#include "hex.h"
#include "iplir.h"
#include "kbd.h"
#include "keys.h"
#include "paths.h"
#include "ppp.h"
#include "sterm.h"
#include "transport.h"

/* Режим ввода текста */
enum {
	im_overwrite,
	im_insert
};

/* Индексы в буфере ввода непрерывной области выделения */
struct sel_area {
	int start;		/* -1, если ничего не выбрано */
	int end;		/* -1, если ничего не выбрано */
};

struct input_window{
	int line;		/* Позиция курсора */
	int col;
	int page;
	uint8_t buf[OUT_BUF_LEN];	/* Данные */
	int lang;
	int cur_lang;
	int input_mode;
	bool first_key;
	bool hex_input;
	char hex_val[2];
	char hex_ptr;
	bool key_input;
	bool wait_key;
	struct sel_area sel;
};

bool scr_visible;		/* флаг видимости основного окна терминала */
static int main_lang;		/* основной язык клавиатуры */
static int line;		/* текущая строка в текущем окне */
static int col;			/* текущий столбец в текущем окне */
static int cursor_line;
static int cursor_col;
static bool cursor_shown=false;	/* флаг отображения курсора */
bool cursor_visible = true; 	/* для отображения курсора */
int page;			/* текущая страница */
int cur_window;			/* текущее окно ввода */
int cur_mode;			/* текущее разрешение экрана */
static int main_mode;		/* основное разрешение экрана после Ctrl+Ф */
static int resp_mode = m_undef;	/* разрешение экрана при просмотре ответа */
int input_mode;			/* режим вставки/замены */
/* Массив окон ввода */
static struct input_window inp_buf[MAX_WINDOWS];
/* ОЗУ ответа */
static uint16_t scr_resp_buf[SCR_RESP_BUF_LEN];
uint8_t *cur_buf;			/* текущий буфер для отображения на экране */
static int cur_buf_len;		/* длина текущего буфера */
static int txt_type;		/* тип текста в буфере (txt_plain/txt_rich) */
bool scr_text_modified;		/* флаг модификации текста в буфере */
bool hex_input;			/* флаг шестнадцатеричного ввода */
static char hex_val[2];		/* буфер шестнадцатеричного числа */
static int hex_ptr;		/* индекс в буфере hex_val */
bool key_input;			/* флаг ввода ключа (по F4) */
bool wait_key;			/* флаг ожидания ключа по F3 */
static bool scr_first_key;	/* флаг ввода первого ключа */

/* Шрифты */
FontPtr font32x8 = NULL;
FontPtr font80x20 = NULL;
FontPtr font_hints = NULL;
FontPtr font_status = NULL;

/* Подсказки */
#define NR_HINTS		15
#define NR_HINTS1		8
#define NR_HINTS2		(NR_HINTS - NR_HINTS1)

/* Подсказки основного режима */
static struct hint_entry main_hints[NR_HINTS]={
/* Первая стока (8 ячеек) */
	{"F1",		"Помощь", 		_("pict/help.bmp")},
	{"F2",		"Печать", 		_("pict/print.bmp")},
	{"F5",		"Отказ", 		_("pict/reject.bmp")},
	{"F6",		"Очистка", 		_("pict/clear.bmp")},
	{"F7",		"Просм. зак.",		_("pict/f7.bmp")},
	{"F8",		"Просм. отв.",		_("pict/resp.bmp")},
	{"Alt",		"След. ключ", 		_("pict/key.bmp")},
	{"Enter",	"Заказ", 		_("pict/request.bmp")},
/* Вторая строка (7 ячеек) */
	{"Shift",	"Рус/Лат", 		_("pict/rus-lat.bmp")},
	{"CapsLock",	"Рус/Лат",		_("pict/rus-lat.bmp")},
	{"Ctrl+S",	"Приг. реж.",		_("pict/local/exit.bmp")},
	{"Ctrl+Л",	"КЛ Экспр.",		_("pict/log.bmp")},
	{"Ctrl+Ч",	"КЛ ИПТ",		_("pict/log.bmp")},
	{"Ctrl+Б",	"ИПТ",			_("pict/bank.bmp")},
	{"Ctrl+Щ",		"ККТ",		_("pict/kkt.bmp")},
};

/* Подсказки пригородного режима */
static struct hint_entry local_hints[NR_HINTS]={
/* Первая стока (8 ячеек) */
	{"F1",		"Помощь", 		_("pict/help.bmp")},
	{"F2",		"Печать", 		_("pict/print.bmp")},
	{"F5",		"Отказ", 		_("pict/reject.bmp")},
	{"F6",		"Очистка", 		_("pict/clear.bmp")},
	{"F7",		"Просм. зак.",		_("pict/f7.bmp")},
	{"F8",		"Просм. отв.",		_("pict/resp.bmp")},
	{"Alt",		"След. ключ", 		_("pict/key.bmp")},
	{"Enter",	"Заказ", 		_("pict/request.bmp")},
/* Вторая строка (7 ячеек) */
	{"Shift",	"Рус/Лат", 		_("pict/rus-lat.bmp")},
	{"CapsLock",	"Рус/Лат",		_("pict/rus-lat.bmp")},
	{"Ctrl+S",	"Осн. реж.",		_("pict/local/exit.bmp")},
	{"Ctrl+Ь",	"КЛ приг.",		_("pict/log.bmp")},
	{"Ctrl+Г",	"Печ. обр.",		_("pict/local/images.bmp")},
	{"Ctrl+Я",	"Номер",		_("pict/local/number.bmp")},
	{"Ctrl+Щ",		"ККТ",		_("pict/kkt.bmp")},
};

/* Геометрия экрана 80x20 */
static struct scr_geom sg80x20 = {
	.width	= 80,
	.height	= 25,
	.w	= 80,
	.lines	= 20
};

/* Геометрия экрана 32x8 */
struct scr_geom sg32x8 = {
	.width	= 40,
	.height	= 25,
	.w	= 32,
	.lines	= 8
};

/* Текущая геометрия */
struct scr_geom *sg = &sg80x20;

/* Вертикальное смещение рабочей области */
#define OFS_Y_32x8		44
#define OFS_Y_80x20		44

#define ERR_FREQ		500
#define RUS_FREQ		500
#define LAT_FREQ		1000

/* Звуковые сигналы */
int err_beep(void)
{
	beep_sync(ERR_FREQ, 250);
	return cmd_none;
}

static int def_beep_rus(void)
{
	if (cfg.kbd_beeps)
		beep_sync(RUS_FREQ, 10);
	return cmd_none;
}

static int def_beep_lat(void)
{
	if (cfg.kbd_beeps)
		beep_sync(LAT_FREQ, 10);
	return cmd_none;
}

static int def_beep(void)
{
	return (kbd_lang == lng_rus) ? def_beep_rus() : def_beep_lat();
}

/* Создание шрифтов терминала */
static void make_term_font(void)
{
	font80x20 = CreateFont(_("fonts/sterm/80x20/courier10x21.fnt"), false);
	font32x8 = CreateFont(_("fonts/sterm/32x8/courier22x53.fnt"), false);
	font_hints = CreateFont(_("fonts/serif11x13.fnt"), true);
	font_status = CreateFont(_("fonts/fixedsys8x16.fnt"), false);
}

/* Удаление шрифтов терминала */
static void free_term_font(void)
{
	DeleteFont(font80x20);
	DeleteFont(font32x8);
	DeleteFont(font_hints);
	DeleteFont(font_status);
}

/* Символ-заполнитель текущего буфера (0/' ') */
static uint8_t padding_char(void)
{
	return (txt_type == txt_rich) || scr_is_req() ? 0x20 : 0;
}

/* Буфер обмена */
static uint8_t clipboard[OUT_BUF_LEN];
static int clip_len;

/* Копирование текста в буфер обмена */
static bool do_copy(void)
{
	bool ret = false;
	struct input_window *win = inp_buf + cur_window;
	if (scr_is_req() && (win->sel.start != -1)){
		int len = win->sel.end - win->sel.start + 1;
		if (len <= sizeof(clipboard)){
			memcpy(clipboard, win->buf + win->sel.start, len);
			clip_len = len;
			ret = true;
		}
	}
	return ret;
}

/* Вставка текста из буфера обмена. Возвращает количество вставленных символов */
static int do_paste(void)
{
	int len = 0;
	struct input_window *win = inp_buf + cur_window;
	if (scr_is_req() && (clip_len > 0)){
		int idx = page * sg->lines * sg->w + line * sg->w + col;
		if (idx < cur_buf_len){
			if (input_mode == im_insert){
				for (len = cur_buf_len; len > 0; len--){
					if (win->buf[len - 1] != padding_char())
						break;
				}
				len = cur_buf_len - len;
				if (len > clip_len)
					len = clip_len;
				if (len > 0){
					memmove(win->buf + idx + len,
						win->buf + idx,
						cur_buf_len - idx - len);
					memcpy(win->buf + idx, clipboard, len);
				}
			}else{		/* im_overwrite */
				len = cur_buf_len - idx;
				if (len > clip_len)
					len = clip_len;
				if (len > 0)
					memcpy(win->buf + idx, clipboard, len);
			}
		}
	}
	return len;
}

/* Сброс области выделения в начальное состояние */
static void reset_sel_area(struct sel_area *sel)
{
	sel->start = sel->end = -1;
}

/* Поменять местами start и end */
static void xchg_sel(struct sel_area *sel)
{
	int tmp = sel->start;
	sel->start = sel->end;
	sel->end = tmp;
}

/* Преобразовать sel_area так, чтобы start был меньше, чем end */
static void adjust_sel(struct sel_area *sel)
{
	if (sel->start > sel->end){
		sel->start--;
		sel->end++;
		if (sel->start < sel->end)
			reset_sel_area(sel);
		else if (sel->start > sel->end)
			xchg_sel(sel);
	}
}

static bool draw_scr_text(void);

/* Отмена выделения */
static void cancel_selection(void)
{
	struct input_window *win = inp_buf + cur_window;
	if (win->sel.start != -1){
		reset_sel_area(&win->sel);
		draw_scr_text();
	}
}

static void set_cursor(int _line, int _col);

/* Удаление выделенного текста */
static bool do_delete(void)
{
	bool ret = false;
	struct input_window *win = inp_buf + cur_window;
	if (scr_is_req() && (win->sel.start != -1)){
		int l1 = sizeof(win->buf) - win->sel.end + 1;
		if (l1 > 0){
			int l2 = win->sel.end - win->sel.start + 1;
			int scr_sz = sg->lines * sg->w;
			div_t d;
			memcpy(win->buf + win->sel.start,
				win->buf + win->sel.end + 1, l1);
			if (l2 > 0)
				memset(win->buf + sizeof(win->buf) - l2, 0x20, l2);
			d = div(win->sel.start, scr_sz);
			page = d.quot;
			d = div(d.rem, sg->w);
			line = d.quot;
			col = d.rem;
			reset_sel_area(&win->sel);
			draw_scr_text();
			set_cursor(line, col);
			ret = true;
		}
	}
	return ret;
}

/* Инициализация массива окон ввода */
void init_input_windows(void)
{
	int i;
	for (i = 0; i < ASIZE(inp_buf); i++){
		struct input_window *p = inp_buf + i;
		p->line = p->col = 0;
		memset(p->buf, ' ', OUT_BUF_LEN);
		p->lang = p->cur_lang = kbd_lang;
		p->input_mode = im_overwrite;
		p->first_key = true;
		p->hex_input = p->key_input = p->wait_key = false;
		p->hex_val[0] = p->hex_val[1] = 0x30;
		p->hex_ptr = 0;
		reset_sel_area(&p->sel);
	}
}

/* Перерисовка строки статуса */
static void RedrawStatus(bool all __attribute__((unused)))
{
	GCPtr pGC = CreateGC(6, DISCY-82, DISCX-12, 4);
	DrawBorder(pGC, 0, 0, GetCX(pGC), GetCY(pGC), borderCX, 
		clBtnShadow, clBtnHighlight);
	DeleteGC(pGC);
	
	scr_show_language(true);
	scr_show_mode(true);
	scr_show_log(true);
}

/* Перерисовка экрана */
bool RedrawScr(bool all, const char *title)
{
	const char *bold = NULL;
	char title1[80], title2[80], c;
	int i, l1, l2, w, h, cw, x;
	for (i = l1 = l2 = 0; title[i] && ((l1 + 1) < sizeof(title1)) &&
			((l2 + 1) < sizeof(title2)); i++){
		c = title[i];
		if (c == '\x01'){
			if (bold == NULL)
				bold = title + i + 1;
		}else if (bold == NULL)
			title1[l1++] = c;
		else
			title2[l2++] = c;
	}
	title1[l1] = title2[l2] = 0;
	GCPtr pGC = CreateGC(0, 0, DISCX, DISCY);
	GCPtr pMemGC = CreateGC(0, 0, DISCX, titleCY);
	SetFont(pMemGC, font_status);
	w = GetCX(pMemGC);
	h = GetCY(pMemGC);
	cw = font_status->max_width;
	x = (w - (l1 + l2) * cw) / 2;
	DrawBorder(pMemGC, 0, 0, w, h, borderCX, clBtnShadow, clBtnHighlight);
	DrawHorzGrad(pMemGC, 1, 1, w - 2, h - 2, clBlue, clNavy);
	SetTextColor(pMemGC, clNavy);
	DrawText(pMemGC, x + 2, 2, w, h, title1, DT_LEFT | DT_VCENTER);
	SetTextColor(pMemGC, clWhite);
	DrawText(pMemGC, x, 0, w, h, title1, DT_LEFT | DT_VCENTER);
	if (l2 != 0){
		x += l1 * cw;
		SetTextColor(pMemGC, clNavy);
		DrawText(pMemGC, x + 2, 2, w, h, title2, DT_LEFT | DT_VCENTER);
		SetTextColor(pMemGC, clYellow);
		DrawText(pMemGC, x, 0, w, h, title2, DT_LEFT | DT_VCENTER);
	}
	
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, w, h);
	
	if (all)
		DrawBorder(pGC, 0, 0, GetCX(pGC), GetCY(pGC), borderCX, 
			clBtnShadow, clBtnHighlight);
	
	/* DeleteFont(title_font); */
	DeleteGC(pMemGC);
	DeleteGC(pGC);

	RedrawStatus(all);

	return true;
}

/* Очистка всего экрана */
void ClearScreen(Color c)
{
	GCPtr pGC = CreateGC(0, 0, DISCX, DISCY);
	if (c != clBtnFace)
		ClearGC(pGC, c);
	else {
		SetBrushColor(pGC, clBtnFace);
		FillBox(pGC, 0, 0, DISCX, 30);
		FillBox(pGC, 0, DISCY-115, DISCX, 115);
		
		FillBox(pGC, 0, 0, 9, DISCY-115);
		FillBox(pGC, DISCX-9, 0, 9, DISCY-115);
	}	
	DeleteGC(pGC);
}

/* Очистка поля ввода текста терминала */
void clear_text_field(void)
{
	GCPtr pGC;

	pGC = CreateGC(8, 30, DISCX-16, DISCY-144);
	ClearGC(pGC, cfg.bg_color);
	DeleteGC(pGC);
}

/* Инициализация экрана (вызывается из create_term) */
void create_scr(void)
{
	if (!InitVGA()){
		printf("cannot initialize graphics\n");
		exit(-1);
	}
	sg = &sg80x20;
	init_input_windows();
	/* reset_scr();*/	/* FIXME */
	InitExGDI();

	scr_visible = true;
	make_term_font();

	ClearScreen(clBtnFace);
}

/* Освобождение экрана */
void release_scr(void)
{
	free_term_font();
	ReleaseVGA();
}

static bool set_input_window(int n);
/* Сброс экрана */
void reset_scr(void)
{
	kbd_lang=lng_rus;
	hex_input=false;
	hex_ptr=2;
	key_input=false;
	wait_key=false;
	line=0;
	col=0;
	page=0;
	cur_window=0;
	main_mode = resp_mode = cur_mode = cfg.scr_mode;
	input_mode=im_overwrite;
	txt_type=txt_plain;
	scr_text_modified=false;
	flush_resp_mode();
	set_input_window(0);
}

/* Запомнить состояние текущего окна ввода */
static bool sync_input_window(int n)
{
	struct input_window *p;
	if ((n < 0) || (n >= MAX_WINDOWS))
		return false;
	p=&inp_buf[n];
	p->line=line; p->col=col;
	p->page=page;
	p->lang=main_lang;
	p->cur_lang=kbd_lang;
	p->input_mode=input_mode;
	p->first_key=scr_first_key;
	p->hex_input=hex_input;
	memcpy(p->hex_val,hex_val,sizeof(hex_val));
	p->hex_ptr=hex_ptr;
	p->key_input=key_input;
	p->wait_key=wait_key;
	return true;
}

/* Установка текущего окна ввода */
static bool set_input_window(int n)
{
	struct input_window *p;
	if ((n < 0) || (n >= MAX_WINDOWS) || (inp_buf[n].buf == NULL))
		return false;
	if (scr_is_req())
		sync_input_window(cur_window);
	cur_window=n;
	p=&inp_buf[n];
	cur_buf=p->buf;
	cur_buf_len = sizeof(p->buf);
	txt_type=txt_plain;
	line=p->line; col=p->col;
	page=p->page;
	main_lang=p->lang;
	kbd_lang=p->cur_lang;
	input_mode=p->input_mode;
	scr_first_key=p->first_key;
	hex_input=p->hex_input;
	memcpy(hex_val,p->hex_val,sizeof(hex_val));
	hex_ptr=p->hex_ptr;
	key_input=p->key_input;
	wait_key=p->wait_key;
	switch_key_set(n);
/*	scr_show_language(true);*/
	return true;
}

/* Пересчитать позиции курсора перед изменением разрешения экрана */
static void adjust_input_cursors(void)
{
	int i,w=sg->lines*sg->w,prev_lines,prev_cols;
	struct input_window *win;
	bool is_req = scr_is_req();
	if (is_req)
		sync_input_window(cur_window);
	if (cur_mode == m32x8){
		prev_lines = sg80x20.lines;
		prev_cols = sg80x20.w;
	}else{
		prev_lines = sg32x8.lines;
		prev_cols = sg32x8.w;
	}
	for (i = 0; i < MAX_WINDOWS; i++){
		int n,k;
		if ((win = &inp_buf[i]) != NULL){
			n = win->page*prev_lines*prev_cols + win->line*prev_cols + win->col;
			if (n >= sizeof(win->buf))
				n = sizeof(win->buf) - 1;
			win->page = n / w;
			k = n % w;
			win->line = k / sg->w;
			win->col = k % sg->w;
			if (is_req && (i == cur_window)){
				page = win->page;
				line = win->line;
				col = win->col;
			}
		}
	}
}

/* Установка режима экрана (80x20/32x8) */
int set_scr_mode(int m, bool need_redraw, bool is_main)
{
	int _m=cur_mode;
	cur_mode=m;
	if (is_main)
		main_mode=cur_mode;
	if (m == m80x20)
		sg=&sg80x20;
	else
		sg=&sg32x8;
	if (m != _m){
		adjust_input_cursors();
		if (!scr_is_req())
			line=col=page=0;
	}
	
	if (need_redraw)
		draw_scr(true, main_title);
	else
		draw_scr_bevel();
	return _m;
}

/* Переключение режима экрана */
void switch_scr_mode(bool need_redraw)
{
	if (cur_mode == m80x20)
		set_scr_mode(m32x8, need_redraw, true);
	else
		set_scr_mode(m80x20, need_redraw, true);
	set_resp_mode(cur_mode);
}

/* Корректировка режима экрана */
static bool adjust_scr_mode(void)
{
	if (scr_is_resp()){
		if ((resp_mode != m_undef) && (cur_mode != resp_mode)){
			set_scr_mode(resp_mode,false,false);
			return true;
		}
	}else if (cur_mode != main_mode){
		set_scr_mode(main_mode,false,false);
		return true;
	}
	return false;
}

/* Установка режима просмотра ответа */
void set_resp_mode(int m)
{
	resp_mode = m;
}

/* Сброс режима просмотра ответа в неопределенное состояние */
void flush_resp_mode(void)
{
	set_resp_mode(m_undef);
}

/* Активен ли просмотр ответа */
bool scr_is_resp(void)
{
	return cur_buf == (uint8_t *)scr_resp_buf;
}

/* Активно ли окно заказа */
bool scr_is_req(void)
{
	return (bool)(cur_buf == inp_buf[cur_window].buf);
}

/* Проверка заданного индекса в буфере на принадлежность области выделения */
static bool is_idx_selected(int idx)
{
	struct input_window *win = inp_buf + cur_window;
	return	(idx >= win->sel.start) && (idx <= win->sel.end);
}

/* Множитель для работы с буферами текста (txt_plain/txt_rich) */
#define mul() ((txt_type == txt_plain) ? 1 : 2)

/* Получить текущий элемент массива по индексу */
static uint16_t get_elem(int index)
{
	uint16_t ret = 0;
	if (index >= cur_buf_len)
		return 0;
	if (txt_type == txt_plain){
		if (scr_is_req() && is_idx_selected(index))
			ret = (uint16_t)cur_buf[index] | 0xff00;
		else
			ret = (uint16_t)(cur_buf[index]);
	}else
		ret = *(uint16_t *)&cur_buf[index * 2];
	return ret;
}

/* Установить элемент массива по индексу */
static uint16_t set_elem(int index, uint16_t w)
{
	if (index >= cur_buf_len)
		return 0;
	scr_text_modified = true;
	if (txt_type == txt_plain)
		return (uint16_t)(cur_buf[index] = (uint8_t)w);
	else
		return *(uint16_t *)&cur_buf[index * 2] = w;
}

/* Индекс массива по указателю */
static int index_of(uint8_t *p)
{
	return (p - cur_buf) / mul();
}

static bool is_low_lat(uint16_t c)
{
	return (__LO_BYTE(c) >= 'a') && (__LO_BYTE(c) <= 'z');
}

static bool is_up_lat(uint16_t c)
{
	return (__LO_BYTE(c) >= '@') && (__LO_BYTE(c) <= '_');
}

/* Вызывается из gdi.c */
bool is_lat(uint16_t c)
{
	return is_low_lat(c) || is_up_lat(c);
}

/* Индекс последнего символа в строке line */
static int last_char(int line)
{
	int n = -1, i;
	int k = page * sg->lines * sg->w + line*sg->w;
	int w = cur_buf_len - k;
	if (w > sg->w)
		w = sg->w;
	for (i = 0; i < w; i++, k++)
		if (__LO_BYTE(get_elem(k)) != padding_char())
			n = i;
	return n;
}

/* Максимальное число строк текущего буфера */
static int max_lines(void)
{
	int n=cur_buf_len / sg->w;
	if (cur_buf_len % sg->w)
		n++;
	return n;
}

/* Индекс последней заполненной строки */
static int last_line(void)
{
	int n=0,i;
	int m=max_lines();
	for (i=0; i < m; i++)
		if (last_char(i) >= 0)
			n=i;
	return n % sg->lines;
}

/* Функции для работы с курсором */
/* Время мигания курсора (ссек) */
#define CURSOR_BLINK_TIME	50UL

/* Рисует или стирает курсор в заданной позиции */
static void DrawCursor(bool switch_shown)
{
	FontPtr curfont = (cur_mode == m80x20) ? font80x20 : font32x8;
	int tw = curfont->max_width;
	int th = curfont->max_height;
	int ww = curfont->max_width*sg->w;
	int max_width = curfont->max_width, max_height = th;
	int i;
	int yofs = (cur_mode == m80x20) ? OFS_Y_80x20 : OFS_Y_32x8;
	GCPtr pGC = CreateGC(((DISCX-ww)>>1)+tw*cursor_col, yofs+(th*cursor_line), max_width, max_height);
	
	SetRop2Mode(pGC, R2_XOR);
	
	if (input_mode == im_insert){
		SetPenColor(pGC, cfg.lat_color ^ cfg.bg_color);
		for (i=0; i<INS_MODE_CURSOR_HEIGHT; i++)
			Line(pGC, 1, max_height-2-i, max_width-1, max_height-2-i);
	}else{
		SetPenColor(pGC, cfg.lat_color ^ cfg.bg_color);
		for (i=0; i < OVR_MODE_CURSOR_WIDTH-1; i++)
			Line(pGC, i, 1, i, max_height-3);
	}	
	
	DeleteGC(pGC);
	cursor_shown = switch_shown ? !cursor_shown : true;
}

static void draw_blink_cursor(void)
{
	static uint32_t cbt = CURSOR_BLINK_TIME;
	static uint32_t prev_t = 0;
	uint32_t current_t = u_times();
	
	if (current_t - prev_t > cbt) {
		cbt = cursor_shown ? CURSOR_BLINK_TIME/2 : CURSOR_BLINK_TIME;
		DrawCursor(true);
		prev_t = current_t;
	}
}

static void clear_cursor(void)
{
	if (cursor_shown)
		DrawCursor(true);
}

void hide_cursor(void)
{
	if (cursor_visible)
		clear_cursor();
	cursor_visible=false;
}

void show_cursor(void)
{
	cursor_visible=true;
}

void set_cursor_visibility(bool visible)
{
	visible ? show_cursor() : hide_cursor();
}

/* Установка курсора */
static void set_cursor(int _line, int _col)
{
	if (cursor_visible)
		clear_cursor();
	cursor_line=_line;
	cursor_col=_col;
	if (cursor_visible)
		DrawCursor(true);
}

/* Указатель на позицию буфера, в которой находится курсор */
static uint8_t *buf_ptr(void)
{
	return cur_buf + ((page * sg->lines * sg->w) + line * sg->w + col) * mul();
}

/* Показать подсказки */
bool show_hints(void)
{
	struct hint_entry *hints = (wm == wm_main) ? main_hints : local_hints;
	if (!scr_visible)
		return false;
	if (cfg.has_hint) {
		const int hint_h = 34;
		int hint_w;
		int max_width = DISCX - 5;
		int max_height = hint_h*2+4;
		int i, ofs, x, j;
		GCPtr pGC = CreateGC(4, DISCY - hint_h*2-8, max_width, max_height);
		GCPtr pMemGC = CreateMemGC(max_width, max_height);
		
		BitmapPtr glyphs[NR_HINTS];
		BitmapPtr logo;
	
		SetFont(pMemGC, font_hints);
		ClearGC(pMemGC, clBtnFace);
		SetPenColor(pMemGC, clBlack);
		
		for (i = 0; i < NR_HINTS; i++){
			const char *glyph = hints[i].glyph;
			if ((i == (NR_HINTS - 1))) {
				if (!cfg.has_kkt)
					glyph = NULL;
				else if (kkt == NULL)
					glyph = _("pict/kkt_err.bmp");
			}
			glyphs[i] = glyph ? CreateBitmap(glyph) : NULL;
		}
		for (i = j = x = 0, hint_w = (DISCX-2)/8; i < NR_HINTS1; i++, j++, x += hint_w) {
			DrawBorder(pMemGC, x, 0, hint_w - 1, hint_h, 1, 
				clBtnShadow, clBtnHighlight);
			if (glyphs[j] == NULL)
				continue;
			SetTextColor(pMemGC, clWhite);
			DrawText(pMemGC, x+34, 1, hint_w-33, hint_h/2, 
				hints[j].key, DT_LEFT | DT_VCENTER);
			SetTextColor(pMemGC, clMaroon);
			DrawText(pMemGC, x+33, 0, hint_w-33, hint_h/2, 
				hints[j].key, DT_LEFT | DT_VCENTER);
			SetTextColor(pMemGC, clBlack);

			DrawText(pMemGC, x+33, hint_h/2, hint_w-33, hint_h/2, 
				hints[j].descr, DT_LEFT | DT_VCENTER);
			DrawBitmap(pMemGC, glyphs[j], x, 1, 32, 32, true, FROM_BMP);
		}
		ofs = (DISCX-2 - hint_w*7)/2;
		for (i = x = 0; i < NR_HINTS2; i++, j++, x += hint_w) {
			DrawBorder(pMemGC, ofs+x, hint_h+3, hint_w-1, hint_h, 1, 
				clBtnShadow, clBtnHighlight);
			if (glyphs[j] == NULL)
				continue;
			SetTextColor(pMemGC, clWhite);
			DrawText(pMemGC, ofs+x+34, hint_h+4, hint_w-33, hint_h/2, 
				hints[j].key, DT_LEFT | DT_VCENTER);
			SetTextColor(pMemGC, clMaroon);
			DrawText(pMemGC, ofs+x+33, hint_h+4, hint_w-33, hint_h/2, 
				hints[j].key, DT_LEFT | DT_VCENTER);
				
			const char *descr = hints[j].descr;			
			char descr_tmp[256];
			
			if (i == (NR_HINTS2 - 1)) {
			    if (_ad != NULL && AD_doc_count() > 0) {
					sprintf(descr_tmp, "ККТ (%zu)", AD_doc_count());
					descr = descr_tmp;
				}
			}
				
			SetTextColor(pMemGC, clBlack);
			DrawText(pMemGC, ofs+x+33, hint_h+hint_h/2+4, hint_w-33, hint_h/2, 
				descr, DT_LEFT | DT_VCENTER);
			DrawBitmap(pMemGC, glyphs[j], ofs+x, hint_h+4, 32, 32, true, FROM_BMP);	

			if (i == (NR_HINTS2 - 1)) {
				const char *text = cfg.fiscal_mode ? "Ф" : "Н/Ф";

				int tw = GetTextWidth(pMemGC, text);

				SetTextColor(pMemGC, clWhite);
				DrawText(pMemGC, ofs+x+94 - tw + 1, hint_h + 2, hint_w-33, hint_h/2, 
					text, DT_LEFT | DT_VCENTER);

				SetTextColor(pMemGC, kkt ? clGreen : clRed);
				DrawText(pMemGC, ofs+x+94 - tw, hint_h + 2, hint_w-33, hint_h/2, 
					text, DT_LEFT | DT_VCENTER);

				if (cfg.kkt_apc) {
					text = "АПЧ";
					tw = GetTextWidth(pMemGC, text);

					SetTextColor(pMemGC, clWhite);
					DrawText(pMemGC, ofs+x+94 - tw + 1, hint_h + 23, hint_w-33, hint_h/2, 
						text, DT_LEFT | DT_TOP);

					SetTextColor(pMemGC, kkt ? clGreen : clRed);
					DrawText(pMemGC, ofs+x+94 - tw, hint_h + 23, hint_w-33, hint_h/2, 
						text, DT_LEFT | DT_TOP);
				}
			}
		}
		
		for (i = 0; i < NR_HINTS; i++){
			if (glyphs[i] != NULL)
				DeleteBitmap(glyphs[i]);
		}
			
/* Логотип "Спектра" */
		logo = CreateBitmap(_("pict/logo.bmp"));
		if (logo != NULL) {
			DrawBitmap(pMemGC, logo, 9, hint_h + 4, logo->width, logo->height, 
				false, FROM_BMP);
			SetPenColor(pMemGC, clNavy);
			DrawRect(pMemGC, 8, hint_h + 3, logo->width + 1,
				logo->height + 1);
		
			DeleteBitmap(logo);
		}	
/* Логотип "Инфотекса" */
		if (cfg.use_iplir && iplir_is_active())
			logo = CreateBitmap(_("pict/iplir.bmp"));
		else
			logo = CreateBitmap(_("pict/iplird.bmp"));
		if (logo != NULL){
			DrawBitmap(pMemGC, logo, 753, hint_h + 4,
				logo->width, logo->height, true, FROM_BMP);
			DeleteBitmap(logo);
		}	
		CopyGC(pGC, 0, 0, pMemGC, 0, 0, max_width, max_height);
		DeleteGC(pMemGC);
		DeleteGC(pGC);
	} else {
		GCPtr pGC = CreateGC(HINTS_LEFT, HINTS_TOP, HINTS_WIDTH, HINTS_HEIGHT);
		ClearGC(pGC, clBlack);
		SetPenColor(pGC, clBtnShadow);
		Line(pGC, 0, 0, GetMaxX(pGC), 0);
		DeleteGC(pGC);
	}
	
	return true;
}

/* Последняя заполненная строка независимо от страницы */
static bool draw_scr_void(GCPtr pGC, int l, int c, int tw, int th)
{
	if ((pGC != NULL) && (l < sg->lines) && (c < sg->w)){
		SetBrushColor(pGC, cfg.bg_color);
		if (c < sg->w)
			FillBox(pGC, c*tw, l*th, (sg->w-c)*tw, th);
		if (l < (sg->lines-1))
			FillBox(pGC, 0, (l+1)*th, sg->w*tw, (sg->lines-l-1)*th);
		return true;
	}else
		return false;
}

/* Отобразить текст на экране */
static bool draw_scr_text(void)
{
	if (scr_visible){
		FontPtr curfont = (cur_mode == m80x20) ? font80x20 : font32x8;
		int th = curfont->max_height;
		int tw = curfont->max_width;
		int ww = tw*sg->w;
		int hh = th*sg->lines;
		int yofs = (cur_mode == m80x20) ? OFS_Y_80x20 : OFS_Y_32x8;
		GCPtr pGC = CreateGC((DISCX - ww) / 2, yofs, ww, hh);
		GCPtr pMemGC;
		int i, j, n = page * sg->w * sg->lines;
		uint16_t *buf = __calloc(sg->w + 1, uint16_t);
		
		memset(buf, 0, (sg->w + 1) * 2);
		pMemGC = CreateMemGC(ww, hh);
		pMemGC->pFont = curfont;

		for (i = 0; i < sg->lines; i++) {
			for (j = 0; (j < sg->w) && (n < cur_buf_len); j++, n++)
				buf[j] = get_elem(n);
			RichTextOut(pMemGC, 0, i * th, buf, j);
			if (n >= cur_buf_len - 1){
				draw_scr_void(pMemGC, i, j, tw, th);
				break;
			}	
		}	
		CopyGC(pGC, 0, 0, pMemGC, 0, 0, ww, hh);
		if (cursor_visible){
			DrawCursor(false);
			cursor_shown = true;
		}
		set_cursor(line, col);
		scr_show_pgnum(true);
		DeleteGC(pGC);
		DeleteGC(pMemGC);
		free(buf);
		return true;
	}else
		return false;
}

bool draw_scr_bevel(void)
{
	GCPtr pGC;
	if (cur_mode == m32x8)
	{
		pGC = CreateGC(0, 28, DISCX, DISCY-140);
		SetBrushColor(pGC, cfg.bg_color);
		FillBox(pGC, 8, 2, GetCX(pGC)-16, GetCY(pGC)-4);
		DrawBorder(pGC, 6, 0, GetCX(pGC)-12, GetCY(pGC),
				2, clBtnShadow, clBtnHighlight);
		SetPenColor(pGC, clBtnShadow);
		Line(pGC, 0, 0, 0, GetMaxY(pGC));
		SetPenColor(pGC, clBtnHighlight);
		Line(pGC, GetMaxX(pGC), 0, GetMaxX(pGC), GetMaxY(pGC));

		SetBrushColor(pGC, clBtnFace);
		FillBox(pGC, 1, 0, 5, GetCY(pGC));
		FillBox(pGC, GetMaxX(pGC)-5, 0, 5, GetCY(pGC));
	}
	else
	{
		pGC = CreateGC(0, 28, DISCX, DISCY-140);
		SetBrushColor(pGC, cfg.bg_color);
		FillBox(pGC, 0, 2, GetCX(pGC), GetCY(pGC)-4);
		SetPenColor(pGC, clBtnShadow);
		Line(pGC, 0, 0, GetMaxX(pGC), 0);
		Line(pGC, 0, 1, GetMaxX(pGC), 1);
		SetPenColor(pGC, clBtnHighlight);
		Line(pGC, 0, GetMaxY(pGC)-1, GetMaxX(pGC), GetMaxY(pGC)-1);
		Line(pGC, 0, GetMaxY(pGC), GetMaxX(pGC), GetMaxY(pGC));
	}

/*	else
	{
		pGC = CreateGC(6, 28, DISCX-12, DISCY-140);
		SetBrushColor(pGC, cfg.bg_color);
		FillBox(pGC, 2, 2, GetCX(pGC)-4, GetCY(pGC)-4);
		DrawBorder(pGC, 0, 0, GetCX(pGC), GetCY(pGC),
				2, clBtnShadow, clBtnHighlight);
	}*/
	
	DeleteGC(pGC);
	return true;
}

/* Нарисовать весь экран */
bool draw_scr(bool show_text, const char *title)
{
	if (!scr_visible)
		return false;
	RedrawScr(false, title);
	draw_scr_bevel();
	if (show_text)
		draw_scr_text();
	show_hints();
	scr_show_mode(true);
	scr_show_pgnum(true);
	scr_show_language(true);
	scr_show_log(true);
	
	draw_clock(true);
	set_cursor(line, col);
	return true;
}

/* Вывести строку */
static bool draw_line(int _line,int _col)
{
	int n=page*sg->w*sg->lines + _line*sg->w + _col;/* Текущая позиция */
	FontPtr curfont = (cur_mode == m80x20) ? font80x20 : font32x8;
	int tw = curfont->max_width;
	int th = curfont->max_height;
	int ww = tw*(sg->w);
	int hh = th;
	int xofs = (DISCX-ww)>>1;
	int yofs = (cur_mode == m80x20) ? OFS_Y_80x20 : OFS_Y_32x8;
	GCPtr pGC = CreateGC(xofs, yofs+_line*hh, ww, hh);
	GCPtr pMemGC;
	int i, j;
	int l=sg->w-col;
	int mcx, mcy;
	uint16_t *buf = __calloc(l + 1, uint16_t);
		
	memset(buf, 0, (l + 1) * 2);
		
	mcx = (sg->w-col)*tw; mcy = th;
	pMemGC = CreateMemGC(mcx, mcy);
	SetFont(pMemGC, curfont);
	ClearGC(pMemGC,cfg.bg_color);	
	
	for (i = _col, j = 0; (i < sg->w) && (n < cur_buf_len); i++, j++, n++)
		((uint16_t *)buf)[i-_col] = get_elem(n);
			
	RichTextOut(pMemGC, 0, 0, buf, j);
	CopyGC(pGC, tw*col, 0, pMemGC, 0, 0, mcx, mcy);
		
	DeleteGC(pGC);
	DeleteGC(pMemGC);
	free(buf);
	
	if (cursor_visible && (_line == line) && (_col <= col))
		DrawCursor(false);
	return true;
}

/* Перейти к символу по указателю */
bool scr_goto_char(uint8_t *p)
{
	int n=index_of(p);
	if (n < 0)
		return false;
	else{
		div_t d;
		page = n/(sg->lines*sg->w);
		d = div(n - page*sg->lines*sg->w,sg->w);
		line = d.quot;
		col = d.rem;
		draw_scr_text();
		set_cursor(line,col);
		return true;
	}
}

/* Создание пустой позиции */
static bool make_gap(int line,int col)
{
	uint8_t *p=buf_ptr();
	int n=index_of(p),l=0;
	if (n >= cur_buf_len){
		err_beep();
		return false;
	}else if (input_mode == im_insert){		/* Режим вставки */
		if (__LO_BYTE(get_elem(cur_buf_len - 1)) == padding_char()){
			l = cur_buf_len - n - 1;
			memmove(p + 1, p, l * mul());
			set_elem(n, padding_char());
			return draw_scr_text();
		}
	}else{
		int lc = last_char(line);
		if ((lc < (sg->w - 1)) && (col <= lc)){
			l = (sg->w - col - 1);
			memmove(p + 1, p, l * mul());
			set_elem(n, padding_char());
			return draw_line(line, col);
		}
	}
	return false;
}

static bool next_page(void)
{
	int l=page*sg->lines+line;
	if (l < (max_lines()-1)){
		page++;
		line=0;
		return true;
	}else
		return false;
}

static bool prev_page(void)
{
	if (page > 0){
		page--;
		line=sg->lines-1;
		return true;
	}else
		return false;
}

/* Очистка текущего буфера */
static void clear_scr_buf(bool show)
{
	int i;
	for (i=0; i < cur_buf_len; set_elem(i++,padding_char()));
	if (show){
		page = line = col = 0;
		draw_scr_text();
	}
}

/* Команды, вызываемые из scr_handle_kbd */

static int cm_up(bool set)
{
	if (hex_input)
		return err_beep();
	if ((input_mode == im_insert) && (line == 0) && prev_page()){
		if (set){
			draw_scr_text();
			scr_show_pgnum(true);
		}
	}else{
		line += (sg->lines-1);
		line %= sg->lines;
	}
	if (set)
		set_cursor(line,col);
	return cmd_none;
}

static int cm_up2(struct kbd_event *e __attribute__((unused)))
{
	return cm_up(true);
}

static bool sel_up(void)
{
	bool ret = true;
	struct input_window *win = inp_buf + cur_window;
	int idx = page * sg->lines * sg->w + line * sg->w + col;
	if ((line == 0) && ((input_mode == im_overwrite) || (page == 0)))
		ret = false;
	else if (win->sel.start == -1){
		if ((idx - sg->w) < cur_buf_len){
			win->sel.start = idx - sg->w;
			win->sel.end = idx - 1;
			if (win->sel.end >= cur_buf_len)
				win->sel.end = cur_buf_len - 1;
		}
	}else if (idx == win->sel.start)
		win->sel.start -= sg->w;
	else if (idx == (win->sel.end + 1)){
		win->sel.end -= sg->w;
		adjust_sel(&win->sel);
	}else
		ret = false;
	return ret;
}

static void cm_shift_up(struct kbd_event *e __attribute__((unused)))
{
	if (sel_up()){
		int pg = page;
		cm_up(true);
		if (pg == page)
			draw_scr_text();
	}
}

static int cm_left(bool set)
{
	if (hex_input)
		return err_beep();
	col--;
	if (col < 0){
		col=sg->w-1;
		return cm_up(set);
	}
	if (set)
		set_cursor(line,col);
	return cmd_none;
}

static int cm_left2(struct kbd_event *e __attribute__((unused)))
{
	return cm_left(true);
}

/* При движении влево сначала перемещается курсор, а потом происходит выделение */
static bool sel_left(void)
{
	bool ret = true;
	struct input_window *win = inp_buf + cur_window;
	int idx = page * sg->lines * sg->w + line * sg->w + col;
	if (win->sel.start == -1){
		if (idx < cur_buf_len)
			win->sel.start = win->sel.end = idx;
	}else if ((idx + 1) == win->sel.start)
		win->sel.start--;
	else if (idx == win->sel.end){
		win->sel.end--;
		if (win->sel.end < win->sel.start)
			reset_sel_area(&win->sel);
	}else
		ret = false;
	return ret;
}

static int cm_right(bool set);

static void cm_shift_left(struct kbd_event *e __attribute__((unused)))
{
	cm_left(false);
	if (sel_left()){
		draw_scr_text();
		set_cursor(line, col);
	}else
		cm_right(false);
}

static int cm_down(bool set)
{
	if (hex_input)
		return err_beep();
	if ((input_mode == im_insert) && (line == sg->lines-1) && next_page()){
		if (set){
			draw_scr_text();
			scr_show_pgnum(true);
		}
	}else{
		line++;
		line %= sg->lines;
	}
	if (set)
		set_cursor(line, col);
	return cmd_none;
}

static int cm_down2(struct kbd_event *e __attribute__((unused)))
{
	return cm_down(true);
}

static bool sel_down(void)
{
	bool ret = true;
	struct input_window *win = inp_buf + cur_window;
	int idx = page * sg->lines * sg->w + line * sg->w + col;
	if (((idx + sg->w) > cur_buf_len) ||
			((line == (sg->lines - 1)) &&
			 (input_mode == im_overwrite)))
		ret = false;
	else if (win->sel.start == -1){
		if (idx < cur_buf_len){
			win->sel.start = idx;
			win->sel.end = idx +sg->w - 1;
			if (win->sel.end >= cur_buf_len)
				win->sel.end = cur_buf_len;
		}
	}else if (idx == win->sel.start){
		win->sel.start += sg->w;
		adjust_sel(&win->sel);
	}else if (idx == (win->sel.end + 1))
		win->sel.end += sg->w;
	else
		ret = false;
	return ret;
}

static void cm_shift_down(struct kbd_event *e __attribute__((unused)))
{
	if (sel_down()){
		int pg = page;
		cm_down(true);
		if (pg == page)
			draw_scr_text();
	}
}

static int cm_right(bool set)
{
	if (hex_input)
		return err_beep();
	col++;
	if (col >= sg->w){
		col=0;
		return cm_down(set);
	}
	if (set)
		set_cursor(line,col);
	return cmd_none;
}

static int cm_right2(struct kbd_event *e __attribute__((unused)))
{
	return cm_right(true);
}

/* При движении вправо сначала происходит выделение, а потом перемещается курсор */
static bool sel_right(void)
{
	bool ret = true;
	struct input_window *win = inp_buf + cur_window;
	int idx = page * sg->lines * sg->w + line * sg->w + col;
	if (idx >= cur_buf_len)
		ret = false;
	else if (win->sel.start == -1){
		if (idx < cur_buf_len)
			win->sel.start = win->sel.end = idx;
	}else if (idx == win->sel.start){
		win->sel.start++;
		if (win->sel.start > win->sel.end)
			win->sel.start = win->sel.end = -1;
	}else if (idx == (win->sel.end + 1))
		win->sel.end++;
	else
		ret = false;
	return ret;
}

static void cm_shift_right(struct kbd_event *e __attribute__((unused)))
{
	if (sel_right()){
		cm_right(true);
		draw_scr_text();
	}
}

static int cm_home(struct kbd_event *e)
{
	if (hex_input)
		return err_beep();
	col=0;
	if (e->shift_state & SHIFT_CTRL)
		line=0;
	set_cursor(line,col);
	return cmd_none;
}

static int sel_home(void)
{
	bool ret = true;
	struct input_window *win = inp_buf + cur_window;
	int sidx = page * sg->lines * sg->w + line * sg->w;
	int idx = sidx + col;
	if ((idx == sidx) || (sidx >= cur_buf_len))
		ret = false;
	else if (win->sel.start == -1){
		win->sel.start = sidx;
		win->sel.end = idx - 1;
		if (win->sel.end >= cur_buf_len)
			win->sel.end = cur_buf_len - 1;
	}else if (idx == win->sel.start)
		win->sel.start = sidx;
	else if (idx == (win->sel.end + 1)){
		win->sel.end = sidx - 1;
		adjust_sel(&win->sel);
	}else
		ret = false;
	return ret;
}

static void cm_shift_home(struct kbd_event *e)
{
	if (sel_home()){
		cm_home(e);
		draw_scr_text();
	}
}

static int cm_end(struct kbd_event *e)
{
	if (hex_input)
		return err_beep();
	if (e->shift_state & SHIFT_CTRL){
		line = sg->lines - 1;
		col = sg->w - 1;
	}else{
		col = last_char(line);	/* -1, если строка пуста */
		if (col < (sg->w - 1))
			col++;
	}
	set_cursor(line,col);
	return cmd_none;
}

static int sel_end(void)
{
	bool ret = true;
	struct input_window *win = inp_buf + cur_window;
	int sidx = page * sg->lines * sg->w + line * sg->w;
	int idx = sidx + col, eidx, n;
	if (sidx >= cur_buf_len)
		ret = false;
	else{
		n = last_char(line);
		if (n >= (sg->w - 1))
			n = sg->w - 2;
		eidx = sidx + n;
		if (win->sel.start == -1){
			win->sel.start = idx;
			if (win->sel.start >= cur_buf_len)
				win->sel.start = cur_buf_len - 1;
			win->sel.end = eidx;
			if (win->sel.end >= cur_buf_len)
				win->sel.end = cur_buf_len - 1;
			adjust_sel(&win->sel);
		}else if ((idx == win->sel.start) || (idx == (win->sel.end + 1))){
			win->sel.end = eidx;
			if (win->sel.end >= cur_buf_len)
				win->sel.end = cur_buf_len - 1;
			adjust_sel(&win->sel);
		}else
			ret = false;
	}
	return ret;
}

static void cm_shift_end(struct kbd_event *e)
{
	if (sel_end()){
		cm_end(e);
		draw_scr_text();
	}
}

int cm_pgup(struct kbd_event *e)
{
	if (hex_input)
		return err_beep();
	if (page > 0){
		if ((e != NULL) && (e->shift_state & SHIFT_CTRL))
			page=0;
		else
			page--;
		line=col=0;
		draw_scr_text();
		scr_show_pgnum(true);
	}
	return cmd_none;
}

int cm_pgdn(struct kbd_event *e)
{
	int m;
	if (hex_input)
		return err_beep();
	m=max_lines();
	if (m > (page+1)*sg->lines){
		if ((e != NULL) && (e->shift_state & SHIFT_CTRL))
			page = (m-1)/sg->lines;
		else
			page++;
		line=col=0;
		draw_scr_text();
		scr_show_pgnum(true);
	}
	return cmd_none;
}

static int cm_del(void)
{
	uint8_t *p;
	int n;
	if (txt_type == txt_rich)
		return cmd_view_req;
	else if (hex_input)
		return err_beep();
	else if ((cur_buf == resp_buf) && (kt != key_dbg))
		return err_beep();
	p=buf_ptr();
	n=index_of(p);
	if (n >= cur_buf_len)
		return err_beep();
/*	if (input_mode == im_insert){		*//* Режим вставки */
		memmove(p,p+1,(cur_buf_len-n-1)*mul());
		set_elem(cur_buf_len-1,padding_char());
		draw_scr_text();
/*	}else{				*//* Режим замены */
#if 0
		size_t l = cur_buf_len-(p-cur_buf);
		if (l > sg->w)
			l=sg->w-col;
		l--;
		if (l > 0)
			memmove(p,p+1,l*mul());
		set_elem(n+l,padding_char());
		draw_line(line,col);
		set_cursor(line,col);
	}
#endif
	scr_show_pgnum(true);
	return cmd_none;
}

static int cm_bkspace(void)
{
	if (txt_type == txt_rich)
		return cmd_view_req;
	else if (hex_input)
		return err_beep();
	else if ((cur_buf == resp_buf) && (kt != key_dbg))
		return err_beep();
	cm_left(false);
	clear_cursor();
	return cm_del();
}

static int cm_ins(struct kbd_event *e)
{
	if (txt_type == txt_rich)
		return cmd_view_req;
	else if ((cur_buf == resp_buf) && (kt != key_dbg) &&
			!(e->shift_state & SHIFT_CTRL))
		return err_beep();
	else if (e->shift_state & SHIFT_CTRL){
		clear_cursor();
		input_mode=(input_mode == im_insert) ? im_overwrite : im_insert;
	}else{
		make_gap(line,col);
		draw_scr_text();
		scr_show_pgnum(true);
	}
	return cmd_none;
}

static void cm_ctrl_c(struct kbd_event *e)
{
	if (!scr_is_req() || hex_input || key_input || wait_key || !do_copy()){
		if (!e->repeated)
			err_beep();
	}else
		cancel_selection();
}

static void cm_ctrl_v(struct kbd_event *e)
{
	if (!scr_is_req() || hex_input || key_input || wait_key){
		if (!e->repeated)
			err_beep();
	}else{
		int len = do_paste();
		if (len > 0){
			int pg = page;
			int scr_sz = sg->lines * sg->w;
			int idx = page * scr_sz + line * sg->w + col + len;
			div_t d = div(idx, scr_sz);
			page = d.quot;
			d = div(d.rem, sg->w);
			line = d.quot;
			col = d.rem;
			draw_scr_text();
			set_cursor(line, col);
			if (page != pg)
				scr_show_pgnum(true);
		}else if (!e->repeated)
			err_beep();
	}
}

static int cm_break_ln(struct kbd_event *e)
{
	uint16_t i,l,ll;
	if (txt_type == txt_rich)
		return cmd_view_req;
	else if (hex_input)
		return err_beep();
	else if ((cur_buf == resp_buf) && (kt != key_dbg))
		return err_beep();
	ll=last_line();
	if ((input_mode == im_insert) && (ll < sg->lines-1) && (line <= ll)){
		uint8_t *p=buf_ptr();
		uint8_t *pp=cur_buf + ((page*sg->lines*sg->w) + (line+1)*sg->w)*mul();
		if (index_of(pp) < cur_buf_len)
			memmove(pp,p,(cur_buf+cur_buf_len-pp)*mul());
		l=pp-p;
		for (i = 0; i < l; i++)
			set_elem(index_of(p++), padding_char());
		scr_show_pgnum(true);
		draw_scr_text();
	}
	cm_down(true);
	e->shift_state &= ~SHIFT_CTRL;
	cm_home(e);
	return cmd_none;
}

static int cm_new_ln(struct kbd_event *e)
{
	int _im=input_mode;
	if (txt_type == txt_rich)
		return cmd_view_req;
	else if ((cur_buf == resp_buf) && (kt != key_dbg))
		return err_beep();
	hide_cursor();
	input_mode=im_insert;
	col=0;
	cm_break_ln(e);
	cm_up(true);
	input_mode=_im;
	show_cursor();
	return cmd_none;
}

static int cm_del_chars(struct kbd_event *e)
{
	if (txt_type == txt_rich)
		return cmd_view_req;
	else if (hex_input)
		return err_beep();
	else if ((cur_buf == resp_buf) && (kt != key_dbg))
		return err_beep();
	if (col < sg->w-1){
		int i, n = index_of(buf_ptr());
		for (i = 0; i < (sg->w - col); i++)
			set_elem(n + i, padding_char());
		draw_line(line, col);
	}
	if (e->shift_state & SHIFT_CTRL){
		col = 0;
		cm_down(true);
	}
	return cmd_none;
}

/* Найти ключ по F3 */
static bool find_key(int key)
{
	int _page = page, _line = line, _col = col, n, m;
	if (input_mode == im_insert)
		n = cur_buf_len;
	else
		n = sg->lines * sg->w;
	for (; n > 0; n--){
		m = page * sg->lines * sg->w + line * sg->w + col;
		if (m >= cur_buf_len)
			page = line = col = 0;
		if (*buf_ptr() == key){
			cm_right(true);
			if (page != _page){
				draw_scr_text();
				scr_show_pgnum(true);
			}
			return true;
		}
		cm_right(false);
	}
	page = _page;
	line = _line;
	col = _col;
	err_beep();
	return false;
}

/* Ввод следующего ключа по Alt */
static int cm_next_key(struct kbd_event *e)
{
	int orig_im=input_mode;
	bool fwd=(bool)((e->shift_state & SHIFT_CTRL) == 0);
	bool flag=true;
	int _line=line, _col=col, n=0;
	uint8_t *p=buf_ptr();
	uint8_t *pp=p;
	bool empty_par=true;
	if (!scr_is_req())
		return cmd_view_req;
	else if (wait_key || hex_input || (__LO_BYTE(get_elem(cur_buf_len-1)) != ' '))
		return err_beep();
	input_mode = im_overwrite;
	while (!is_lat(*p)){
		cm_left(false);
		n++;
		p=buf_ptr();
		if (p == pp)
			break;
		empty_par = (*p == ' ') || is_lat(*p);
	}
	input_mode = orig_im;
	empty_par &= (n > 1);
	if (is_lat(*p)){
		char buf[11];
		int l=0;
		while (l < 10){
			cm_right(false);
			p=buf_ptr();
			if ((l > 0) && (*p == ' '))
				break;
			else
				buf[l++]=recode(*p);
		}
		for (; (l > 0) && (buf[l-1] == ' '); l--);
		buf[l]=0;
/*		recode_str(buf,-1); */
		if (l > 0){
			mark_key();
			line=_line; col=_col;
		}else if (!empty_par){
			cm_left(false); cm_left(false); cm_left(false);
		}else
			mark_key();
		if (kset->key_arg_len == 0)
			flag=walk_keys(fwd);
		else if (!next_key_group(buf)){
			line=_line; col=_col;
			return err_beep();
		}
	}else
		flag=walk_keys(fwd);
	p=buf_ptr();
	if (!flag){
		err_beep();
		line=_line; col=_col;
	}else{
		char c=get_key();
		set_elem(index_of(buf_ptr()),' ');
		cm_right(true);
		set_elem(index_of(buf_ptr()),c);
		cm_right(true);
	}
	set_cursor(line,col);
	draw_scr_text();
	return cmd_none;
}

static int cm_ch_key(struct kbd_event *e __attribute__((unused)))
{
	if (txt_type == txt_rich)
		return cmd_view_req;
	if (key_input || hex_input)
		return err_beep();
	wait_key=!wait_key;
	if (wait_key){
		main_lang=kbd_lang;
		kbd_lang=lng_lat;
	}else
		kbd_lang=main_lang;
	scr_show_mode(true);
	scr_show_language(true);
	return cmd_none;
}

static int cm_key(struct kbd_event *e __attribute__((unused)))
{
	if (txt_type == txt_rich)
		return cmd_view_req;
	if (hex_input || wait_key)
		return err_beep();
	key_input=!key_input;
	if (key_input){
		main_lang=kbd_lang;
		kbd_lang=lng_lat;
	}else
		kbd_lang=main_lang;
	scr_show_mode(true);
	scr_show_language(true);
	return cmd_none;
}

#if 0
static void emulate_rfid(void)
{
	if (scr_is_req()){
		int i = 0;
		uint8_t b = 0x20;
		while (b < 0x7f){
			cur_buf[i++] = 0x20;
			cur_buf[i++] = recode(b++);
		}
	}
}
#endif

/*
 * Эта функция вызывается из sterm.c в для очистки ОЗУ заказа в случае
 * синтаксической ошибки при работе в пригородном режиме.
 */
int cm_clear(void)
{
	if (scr_is_resp())
		return cmd_view_req;
	else if (!scr_is_req() && (kt != key_dbg))
		return err_beep();
	clear_scr_buf(true);
	if (scr_is_req())
		rollback_keys(false);
	page=line=col=0;
	draw_scr_text();
	return cmd_none;
}

static int cm_hex(struct kbd_event *e __attribute__((unused)))
{
	if ((kt == key_dbg) && !wait_key && !key_input){
		hex_input=!hex_input;
		hex_ptr = 0;
		if (hex_input){
			main_lang=kbd_lang;
			kbd_lang=lng_lat;
		}else{
			kbd_lang=main_lang;
		}
		scr_show_mode(true);
		scr_show_language(true);
	}else
		err_beep();
	return cmd_none;
}

static bool switch_window(struct kbd_event *e)
{
	if (e->pressed && (e->shift_state & SHIFT_CTRL)){
		if ((e->key >= KEY_1) && (e->key <= KEY_0)){
			int w=(e->key-KEY_1+1)%MAX_WINDOWS;
			set_input_window(w);
			draw_scr_text();
			show_cursor();
			scr_show_language(true);
			scr_show_mode(true);
			set_term_state(st_input);
			return true;
		}
	}
	return false;
}

/* Ввод символа */
static bool input_char(struct kbd_event *e)
{
	int n;
	if ((cur_buf == resp_buf) && (kt != key_dbg)){
		err_beep();
		return false;
	}else if ((e->ch == '\x1b') && (kt != key_dbg)){
		err_beep();
		return false;
	}
	if ((e->ch == '\x1b') && (e->shift_state & SHIFT_CTRL))
		e->ch = '\x1d';
	if (!scr_is_req())
		e->ch=recode(e->ch);
	if (hex_input){		/* Режим ввода шестнадцатеричного кода */
		if (isxdigit(e->ch)){
			hex_val[hex_ptr++]=e->ch;
			if (hex_ptr < 2)
				return true;
		}else{
			err_beep();
			return false;
		}
		hex_ptr = 0;
		hex_input=false;
		kbd_lang=main_lang;
		scr_show_language(true);
		scr_show_mode(true);
		e->ch = read_hex_byte((uint8_t *)hex_val) & 0x7f;
		if (scr_is_req())
			e->ch = recode(e->ch);
	}
	if ((key_input || wait_key) && !is_lat(e->ch)){
		err_beep();	/* Режим ввода ключа. Введен неверный символ */
		return false;
	}
	if (wait_key){		/* Режим корректировки ключа */
		wait_key=false;
		find_key(e->ch);
		kbd_lang=main_lang;
		scr_show_language(true);
		scr_show_mode(true);
		return true;
	}
	n=index_of(buf_ptr());		/* Текущая позиция */
	if (n >= cur_buf_len){
		err_beep();		/* Превышение длины буфера */
		return false;
	}
	if (input_mode == im_insert)	/* Режим вставки */
		if ((__LO_BYTE(get_elem(cur_buf_len-1)) != padding_char()) ||
				!make_gap(line,col))
			return false;
	if (key_input){		/* Режим ввода ключа */
		set_elem(n,' ');
		draw_line(line,col);
		cm_right(true);
		n=index_of(buf_ptr());
	}
	set_elem(n,e->ch);
	draw_line(line,col);
	cm_right(true);		/* Сдвинуть курсор вправо */
	if (key_input){
		key_input=false;
		kbd_lang=main_lang;
		scr_show_language(true);
		scr_show_mode(true);
#if 0
		if (col == 0)	/* Переход на следующую строку при вводе ключа */
			draw_line(line,col);
#endif
	}
/*	set_cursor(line,col);*/
	def_beep();
	return true;
}

/*
 * Ввод массива символов в ОЗУ заказа. Возвращает true, если был введён
 * хотя бы один символ.
 */
bool input_chars(const uint8_t *chars, int len)
{
	bool ret = false;
	int i, n;
	if (!scr_is_req())
		return false;
	for (i = 0; i < len; i++){
		n = index_of(buf_ptr());
		if ((input_mode == im_insert) &&
				((__LO_BYTE(get_elem(cur_buf_len - 1)) != padding_char()) ||
				!make_gap(line, col)))
			break;
		set_elem(n, chars[i]);
		cm_right(true);
	}
	if (i > 0){
		draw_scr_text();
		ret = true;
	}
	return ret;
}

bool quick_astate(int ast)
{
	switch (ast){
		case ast_noxprn:
		case ast_nolprn:
		case ast_lprn_err:
		case ast_lprn_ch_media:
		case ast_lprn_sd_err:
		case ast_repeat:
		case ast_illegal:
		case ast_rejected:
		case ast_error:
		case ast_rkey:
		case ast_skey:
		case ast_dkey:
		case ast_no_log_item:
		case ast_pos_error:
		case ast_pos_need_init:
		case ast_no_kkt:
			return true;
		default:
			return false;
	}
}

/* При нажатии некоторых клавиш из просмотра ответа переходим к ОЗУ заказа */
static bool need_req(struct kbd_event *e)
{
	uint16_t req_keys[]={KEY_F3,KEY_F4,KEY_F12,
		KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,KEY_HOME,KEY_END,
		KEY_INS,KEY_DEL,KEY_BACKSPACE,
		KEY_NUMLOCK,KEY_NUMSLASH,KEY_NUMMUL,KEY_NUMMINUS,
		KEY_LALT,KEY_RALT,
		KEY_ESCAPE};
	int i;
	if (!scr_is_resp() || (e == NULL) || resp_executing)
		return false;
	for (i=0; i < ASIZE(req_keys); i++)
		if (e->key == req_keys[i])
			return true;
	return false;
}

static bool handle_clipboard(struct kbd_event *e)
{
	static struct event_handler{
		uint16_t key;
		void (*fn)();
	} handlers[] = {
		{KEY_C,		cm_ctrl_c},
		{KEY_V,		cm_ctrl_v},
	};
	if (!kbd_exact_shift_state(e, SHIFT_CTRL))
		return false;
	switch (e->key){
		case KEY_C:
		case KEY_V:
			break;
		default:
			return false;
	}
	if (!scr_is_req() || hex_input || key_input || wait_key){
		if (!e->repeated)
			err_beep();
	}else{
		int i;
		for (i = 0; i < ASIZE(handlers); i++){
			if (e->key == handlers[i].key){
				handlers[i].fn(e);
				break;
			}
		}
	}
	return true;
}

static bool handle_sel(struct kbd_event *e)
{
	static struct event_handler{
		uint16_t key;
		void (*fn)();
	} handlers[] = {
		{KEY_LEFT,	cm_shift_left},
		{KEY_RIGHT,	cm_shift_right},
		{KEY_UP,	cm_shift_up},
		{KEY_DOWN,	cm_shift_down},
		{KEY_HOME,	cm_shift_home},
		{KEY_END,	cm_shift_end},
	};
	if ((e->key == KEY_DEL) && (e->shift_state == 0) && do_delete())
		return true;
	else if (!kbd_exact_shift_state(e, SHIFT_SHIFT))
		return false;
	switch (e->key){
		case KEY_LEFT:
		case KEY_RIGHT:
		case KEY_UP:
		case KEY_DOWN:
		case KEY_HOME:
		case KEY_END:
			break;
		default:
			return false;
	}
	if (!scr_is_req() || hex_input || key_input || wait_key){
		if (!e->repeated)
			err_beep();
	}else{
		int i;
		for (i = 0; i < ASIZE(handlers); i++){
			if (e->key == handlers[i].key){
				handlers[i].fn(e);
				break;
			}
		}
	}
	return true;
}

/* Основной обработчик клавиатуры экрана */
int scr_handle_kbd(struct kbd_event *e)
{
	static struct event_handler{
		uint16_t key;
		int (*fn)();
	} handlers[] = {
		{KEY_LALT,cm_next_key},
		{KEY_RALT,cm_next_key},
		{KEY_F3,cm_ch_key},
		{KEY_F4,cm_key},
		{KEY_F6,cm_clear},
		{KEY_F12,cm_hex},
		{KEY_UP,cm_up2},
		{KEY_LEFT,cm_left2},
		{KEY_DOWN,cm_down2},
		{KEY_RIGHT,cm_right2},
		{KEY_HOME,cm_home},
		{KEY_END,cm_end},
		{KEY_PGUP,cm_pgup},
		{KEY_PGDN,cm_pgdn},
		{KEY_DEL,cm_del},
		{KEY_BACKSPACE,cm_bkspace},
		{KEY_INS,cm_ins},
		{KEY_NUMENTER,cm_break_ln},
		{KEY_NUMSLASH,cm_del_chars},
		{KEY_NUMLOCK,cm_new_ln},
	};
	int i;
	if ((e->key == KEY_LSHIFT) || (e->key == KEY_RSHIFT) || (e->key == KEY_CAPS)){
		scr_show_language(true);
		return cmd_none;
	}else if ((e->key == KEY_LCTRL) || (e->key == KEY_RCTRL)){
		scr_show_mode(true);
		return cmd_none;
	}
	if (!e->pressed)
		return cmd_none;
	if (e->repeated)
		kbd_flush_queue();
	if (switch_window(e))
		return cmd_none;
	if (quick_astate(_term_aux_state))
		set_term_astate(ast_none);
	if (need_req(e))
		return cmd_view_req;
	else if (handle_sel(e))
		return cmd_none;
	else if (handle_clipboard(e))
		return cmd_none;
	else if ((e->key != KEY_DEL) || (e->shift_state != 0)) 
		cancel_selection();
	for (i = 0; i < ASIZE(handlers); i++)
		if (e->key == handlers[i].key)
			return handlers[i].fn(e);
/* Ввод символа */
	if ((e->ch == '\x1b') ||
			(e->ch && !(e->shift_state & SHIFT_CT_AL))){
		if (scr_is_resp())
			return cmd_view_req;
		input_char(e);
		scr_show_pgnum(true);
	}
	return cmd_none;
}

/* Получение текста для ОЗУ заказа */
#define HBYTE_MARK	0x48
int get_scr_text(uint8_t *buf, int len)
{
	bool is_valid_char(uint8_t b)
	{
		bool ret = true;
		switch (b){
			case VERSION_INFO_MARK:
			case REACTION_INFO_MARK:
			case XGATE_INFO_MARK:
			case TERM_INFO_MARK:
			case SESSION_INFO_MARK:
			case PRN_INFO_MARK:
				ret = false;
				break;
		}
		return ret;
	}
	enum {
		st_start,
		st_hbyte,
		st_regular,
		st_raw,
	};
	int i, j, k, st = st_start;
	uint8_t b, *p = inp_buf[cur_window].buf;
	if (buf == NULL)
		return 0;
	for (i = k = 0; (i < OUT_BUF_LEN) && (k < len); i++){
		b = recode(p[i]);
		switch (st){
			case st_start:
				if (b == HBYTE_MARK)
					st = st_hbyte;
				else if (b != 0x20){
					i--;
					st = st_regular;
				}
				break;
			case st_hbyte:
				if (b == HB_WRESP)
					st = st_start;
				else{
					buf[k++] = HBYTE_MARK;
					i--;
					st = st_regular;
				}
				break;
			case st_regular:
				if ((b != 0x20) && is_valid_char(b))
					buf[k++] = b;
				if (b == '['){
					for (j = i; j < OUT_BUF_LEN; j++){
						if (p[j] == ']'){
							st = st_raw;
							break;
						}
					}
				}
				break;
			case st_raw:
				if (is_valid_char(b))
					buf[k++] = b;
				if (b == ']')
					st = st_regular;
				break;
		}
	}
	return k;
}

/* Получение первых l символов ОЗУ заказа */
/* NB: возможен выход за правую границу буфера */
int scr_get_24(char *buf, int l, bool strip)
{
	uint8_t *p = inp_buf[cur_window].buf;
	int i, n = 0;
	if (buf == NULL)
		return 0;
	memset(buf, 0, l);
	for (i = 0; i < l; i++){
		uint8_t c = p[i];
		if (((c == ' ') || (c == '#')) && strip)
			continue;
		else if (is_escape(c)){
			switch (recode(p[i + 1])){
				case XPRN_RD_BCODE:
				case XPRN_WR_BCODE:
					if (p[i + 2] == ';'){
						if (p[i + 3] == ';')
							i += 16;
						else
							i += 17;
					}else if (p[i + 4] == ';')
						i += 17;
					else
						i += 18;
					break;
				case XPRN_NO_BCODE:
				case LPRN_NO_BCODE:
					i++;
					break;
				default:
					buf[n++] = c;
			}
		}else
			buf[n++] = c;
	}
	if (buf[n - 1] == '\f')
		memcpy(buf + n - 1, "\n\r\f", 3);
	else
		memcpy(buf + n, "\n\r", 2);
	n += 2;
	return n;
}

/* Получение "снимка" строки статуса */
static char *scr_status_sshot(void)
{
	static char buf[TERM_STATUS_LEN+1];
	char *p = buf;
	const char *str;
/*	time_t t = time(NULL);
	struct tm *tt = localtime(&t);*/
	memset(buf, ' ', TERM_STATUS_LEN);
	buf[TERM_STATUS_LEN] = 0;
	memcpy(buf, "\n\r", 2);
	p += 2;
	str = find_term_state(_term_state);
	if (str != NULL)
		memcpy(p, str, strlen(str));
	p += MAX_TERM_STATE_LEN;
	*p = recode(hbyte);
	p += 2;
	if (kt != key_none)	/* если нет ключа, то кто нажал Ctrl+П ? */
		*p = ds_key_char(kt);
	p += 2;
	str = find_term_astate(_term_aux_state);
	if (str != NULL)
		memcpy(p, str, strlen(str));
	p += MAX_TERM_ASTATE_LEN;
	memcpy(p, "   ", 3);
	p += 4;
	sprintf(p, "СТР %.3d", page + 1);
	p[7] = ' ';
	p += 8;
	if (kbd_lang == lng_rus)
		memcpy(p, "РУС", 3);
	else
		memcpy(p, "ЛАТ", 3);
/*	p += 4;
	sprintf(p, "%.2d.%.2d.%.2d %.2d:%.2d:%.2d\n\r",
		tt->tm_mday, tt->tm_mon + 1, tt->tm_year % 100,
		tt->tm_hour, tt->tm_min, tt->tm_sec);*/
	return recode_str(buf, -1);
}

/* Получение "снимка" окна терминала для вывода на печать */
int scr_snap_shot(uint8_t *buf, int l)
{
	int win_size = sg->lines * (2 * sg->w + 2);
	int i0 = page * sg->lines * sg->w, j;
	bool need_recode = scr_is_req();
	if (l > (win_size + TERM_STATUS_LEN)){
		int i;
		for (i=0,j=0; i < win_size; j++){
			uint8_t b=__LO_BYTE(get_elem(i0+j));
			if (need_recode)
				b=recode(b);
			if (b < 0x20)
				b += 0x40;
			if ((j % sg->w) == 0){
				buf[i++]='\n';
				buf[i++]='\r';
			}
			buf[i++] = b;
			buf[i++]='_';
		}
		buf[i]=0;
		strcat((char *)buf, scr_status_sshot());
		return i+TERM_STATUS_LEN;
	}
	return 0;
}

/* Найти конец команды печати */
uint8_t *scr_find_eprnop(uint8_t *s)
{
	if (s != NULL)
/* FIXME: поиск в s будет прекращён при достижении 0 */
		return s + strcspn((char *)s, "\x60\x61\x64\x65\x71\x74\x75\x76");
	else
		return NULL;
}

/* Нахождение 13-символьного штрих-кода */
static uint8_t *bctl_code(uint8_t *p)
{
	if ((p != NULL) && ((*p == XPRN_RD_BCODE) || (*p == XPRN_WR_BCODE))){
		if (p[1] == ';'){
			if (p[2] == ';')
				return p + 3;
			else
				return p + 4;
		}else if (p[3] == ';')
			return p + 4;
		else
			return p + 5;
	}else
		return NULL;
}

/*
 * Пропуск штрих-кода типа LPRN_WR_BCODE2. Возвращает указатель на последний
 * допустимый символ штрих-кода или NULL в случае ошибки.
 */
static uint8_t *skip_bcode2(uint8_t *p, int l)
{
	enum {
		st_start,
		st_type,
		st_x,
		st_y,
		st_number,
		st_len,
		st_data,
		st_stop,
		st_err,
	};
	int st = st_start, i, n = 0, data_len = 0;
	uint8_t b;
	if ((p == NULL) || (l <= 0))
		return NULL;
	for (i = 0; (i < l) && (st != st_stop) && (st != st_err); i++){
		b = p[i];
		switch (st){
			case st_start:
				if (b == LPRN_WR_BCODE2)
					st = st_type;
				else
					return NULL;
				break;
			case st_type:
				if (isdigit(b)){
					n = 0;
					st = st_x;
				}else
					st = st_err;
				break;
			case st_x:
				if ((n == 0) && (b == 0x3b))
					st = st_number;
				else if (!isdigit(b))
					st = st_err;
				else if (++n == 3){
					n = 0;
					st = st_y;
				}
				break;
			case st_y:
				if (!isdigit(b))
					st = st_err;
				else if (++n == 3){
					n = 0;
					st = st_len;
				}
				break;
			case st_number:
				if ((b != 0x31) && (b != 0x32) && (b != 0x33))
					st = st_err;
				n = 0;
				st = st_len;
				break;
			case st_len:
				if (!isdigit(b))
					st = st_err;
				data_len *= 10;
				data_len += b - 0x30;
				if (++n == 3){
					if (data_len == 0)
						st = st_err;
					else{
						n = 0;
						st = st_data;
					}
				}
				break;
			case st_data:
				if (++n == data_len)
					st = st_stop;
				break;
		}
	}
	if (st == st_stop)
		return p + i - 1;
	else
		return p + i - 2;
}

/* Установка текста на экране */
int set_scr_text(uint8_t *s, int l, int t, bool need_redraw)
{
	int n;
	uint8_t *ss = s;
	if (scr_is_req())
		sync_input_window(cur_window);
	txt_type = t;
	line = col = page = 0;
/*	input_mode=im_overwrite;*/
	scr_text_modified = false;
	if (s == NULL){
		set_input_window(cur_window);
		show_cursor();
	}else if (t == txt_plain){
		show_cursor();
		cur_buf = s;
		cur_buf_len = l;
	}else{		/* Обработка ОЗУ ответа */
		uint8_t bg, fg, *p;
		uint16_t attr = 0;
		bool dle = false, dle2 = false;
		cur_buf = (uint8_t *)scr_resp_buf;
		cur_buf_len = SCR_RESP_BUF_LEN;
		clear_scr_buf(false);
		hide_cursor();
		adjust_scr_mode();
		for (n = 0; (s - ss) < l; s++){
			if (is_escape(*s))
				dle = true;
			else if (*s == APRN_DLE)
				dle2 = true;
			else if (dle){
				dle = false;
				switch (*s){
					case X_ATTR:
						bg = *++s;
						fg = *++s;
						if ((fg == 0x70) && (bg == 0x70))
							attr = 0;
						else
							attr = (bg << 4) | (fg & 0x0f);
						break;
					case XPRN_RD_BCODE:
					case XPRN_WR_BCODE:
						p = bctl_code(s);
						if (p != NULL)
							s = p + 12;
						break;
					case LPRN_WR_BCODE2:
						p = skip_bcode2(s, ss + l - s);
						if (p != NULL)
							s = p;
						break;
					case XPRN_NO_BCODE:
					case LPRN_NO_BCODE:
						break;
					case XPRN_PRNOP:
						s=scr_find_eprnop(s);
						break;
					case XPRN_AUXLNG:
					case XPRN_MAINLNG:
						break;
					case LPRN_INTERLINE:
						s += 2;
						__fallthrough__;
					default:
						s++;
				}
			}else if (dle2){
				dle2 = false;
				switch (*s){
					case APRN_AUXLNG:
					case APRN_MAINLNG:
					case APRN_UNDERLINE:
					case APRN_CUT:
					case APRN_LOGO:
					case APRN_INVERSE:
					case APRN_TAB:
						break;
					case APRN_FONT:
					case APRN_BCODECHARS:
					case APRN_BCODEH:
					case APRN_SRV:
					case APRN_DIR:
						s++;
						break;
					case APRN_INTERLINE:
					case APRN_FEED:
					case APRN_SCALE:
					case APRN_SPACING:
					case APRN_STAB:
						s += 2;
						break;
					case APRN_VPOS:
					case APRN_HPOS:
					case APRN_LBL_LEN:
						s += 3;
						break;
					case APRN_BCODEF:
						if ((s - ss + 5) <= l)
							s += 3;
						else
							break;
						__fallthrough__;
					case APRN_BCODE:
						if (((s - ss) + 3) <= l){
							int v = (s[1] - 0x30) * 10 + (s[2] - 0x30);
							if (v >= 0)
								s += v + 2;
						}
						break;
				}
			}else{
				switch (*s){
					case '\r':
						n = sg->w*(n/sg->w);
						break;
					case '\n':
						n += sg->w;
						break;
					default:
						if (*s >= 0x20)
							set_elem(n++,(attr << 8) | *s);
				}
			}
		}
	}
	adjust_scr_mode();
	if (need_redraw && scr_visible){
		clear_text_field();
		draw_scr_text();
	}
	return l;
}

/* Установка текста запроса (автозапрос) */
int set_scr_request(uint8_t *s,int req_len,bool show)
{
	uint8_t *_cur_buf=cur_buf;
	uint16_t _cur_buf_len=cur_buf_len;
	int t=txt_type;
	uint16_t i;
	if (s == NULL) return 0;
	set_input_window(cur_window);
	clear_scr_buf(show);
	for (i=0; i < req_len; i++)
		set_elem(i,recode(s[i]));
	adjust_scr_mode();
	if (show){
		draw_scr_text();
		set_term_state(st_input);
	}else{
		cur_buf=_cur_buf;
		cur_buf_len=_cur_buf_len;
		txt_type=t;
	}
	return req_len;
}

/* Фоновая обработка экрана */
bool process_scr(void)
{
	if (scr_visible){
		draw_clock(false);
		if (cursor_visible)
			draw_blink_cursor();
	}
	return true;
}
