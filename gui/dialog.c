/* Диалоговые окна. (c) gsr, Alex Popov 2000-2004 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/dialog.h"
#include "gui/exgdi.h"
#include "gui/options.h"
#include "gui/scr.h"
#include "kbd.h"
#include "paths.h"
#include "sterm.h"

/* Все размеры задаются в символах */

/* Максимальная длина текста кнопки */
#define DLG_BTN_MAX_TXT_LEN	6
/* Максимальная длина пользовательского текста кнопки */
#define DLG_CBTN_MAX_TXT_LEN	20
/* Горизонтальный зазор между границей кнопки и текстом */
#define DLG_BTN_HMARGIN		1
/* Горизонтальный размер кнопки диалога */
#define DLG_BTN_WIDTH		(DLG_BTN_MAX_TXT_LEN + 2 * DLG_BTN_HMARGIN)
/* Вертикальный размер кнопки окна диалога */
#define DLG_BTN_HEIGHT		2
/* Максимальное количество кнопок в окне диалога */
#define DLG_MAX_BTNS		3
/* Расстояние между соседними кнопками по горизонтали */
#define DLG_BTN_HGAP		5
/* Горизонтальный зазор между границей окна и текстом */
#define DLG_HMARGIN		2
/* Вертикальный зазор между границей окна и текстом */
#define DLG_VMARGIN		1
/* Максимальная длина строки текста в окне */
#define DLG_MAX_STR_LEN		80	//60
/* Максимальное число строк в диалоговом окне */
#define DLG_MAX_LINES		35
/* Горизонтальный размер диалогового окна */
#define DLG_WIDTH		(DLG_MAX_STR_LEN + 2 * DLG_HMARGIN)
/* Минимальная высота диалогового окна */
#define DLG_MIN_HEIGHT		5
/* Максимальная высота диалогового окна */
#define DLG_MAX_HEIGHT		(DLG_MIN_HEIGHT + DLG_MAX_LINES)

/* А в этой структуре все размеры указываются в пикселах */
struct dlg_win {
	int top;
	int left;
	int width;
	int height;
	char *title;
	char *message;
	intptr_t buttons;
	int n_buttons;
	int active_btn;
	int cmd;		/* команда в случае пользовательских кнопок */
	int align;
};

/* Надписи на кнопках в окне диалога */
#define BTN_YES_TXT		"Да"
#define BTN_NO_TXT		"Нет"

/* Шрифт для вывода текста в диалоговом окне */
static FontPtr pFont = NULL;

/* Определение максимальной длины слов, не превышающей заданную */
static int get_words_len(char *s, int n)
{
	int i, m;
	bool space = false;
	if (s == NULL)
		return 0;
	for (i = m = 0; s[i] && (i < n); i++){
		if (s[i] == ' '){
			if (!space){
				space = true;
				m = i;
			}
		}else if (s[i] == '\n')
			return i;
		else
			space = false;
	}
	return s[i] ? m : i;
}

/* Определение числа строк при заданной максимальной длине строки */
static int count_lines(char *txt, int max_l)
{
	int n = 0;
	if (txt == NULL)
		return n;
	while (*txt){
		n++;
		txt += get_words_len(txt, max_l);
		if (*txt == '\n')
			txt++;
		else
			for (; *txt && (*txt == ' '); txt++){}
	}
	return n;
}

/* Подсчёт количества кнопок */
static int count_buttons(intptr_t buttons)
{
	int ret = 0;
	if (buttons == dlg_yes)
		ret = 1;
	else if (buttons == dlg_yes_no)
		ret = 2;
	else if (buttons > dlg_custom){
		struct custom_btn *btn = (struct custom_btn *)buttons;
		while (btn->text != NULL){
			ret++;
			btn++;
		}
	}
	if (ret > DLG_MAX_BTNS)
		ret = DLG_MAX_BTNS;
	return ret;
}

/* Определение максимальной длины надписи на кнопках */
static int get_max_btn_txt_len(intptr_t buttons)
{
	return (buttons < dlg_custom) ? DLG_BTN_MAX_TXT_LEN : DLG_CBTN_MAX_TXT_LEN;
}

/* Определение команды, соответствующей выбранной кнопке */
static int get_dlg_cmd(struct dlg_win *dlg)
{
	int ret = dlg->active_btn;
	if ((dlg->n_buttons > 0) && (dlg->buttons > dlg_custom) &&
			(dlg->active_btn >= 0) &&
			(dlg->active_btn < dlg->n_buttons))
		ret = ((struct custom_btn *)dlg->buttons)[dlg->active_btn].cmd;
	return ret;
}

/* Определение геометрии окна диалога */
static void make_dlg_geom(struct dlg_win *dlg)
{
	if ((dlg == NULL) || (pFont == NULL))
		return;
	dlg->width = DLG_WIDTH;
	dlg->height = DLG_MIN_HEIGHT +
		count_lines(dlg->message, DLG_MAX_STR_LEN);
	if (dlg->buttons == dlg_none)
		dlg->height -= DLG_BTN_HEIGHT;
	dlg->width *= pFont->max_width;
	dlg->height *= pFont->max_height;
	dlg->height += titleCY;
	dlg->top = (DISCY - dlg->height) / 2;
	dlg->left = (DISCX - dlg->width) / 2;
	dlg->n_buttons = count_buttons(dlg->buttons);
}

/* Освобождение ресурсов окна диалога */
static void release_dlg(struct dlg_win *dlg)
{
	if (pFont != NULL){
		DeleteFont(pFont);
		pFont = NULL;
	}
	if (dlg != NULL){
		if (dlg->title != NULL)
			free(dlg->title);
		if (dlg->message != NULL)
			free(dlg->message);
		free(dlg);
	}
}

/* Создание окна диалога */
static void draw_dlg(struct dlg_win *dlg);
static struct dlg_win *new_dlg(const char *title, const char *message,
	intptr_t buttons, int active_btn, int align)
{
	struct dlg_win *dlg;
	if ((title == NULL) || (message == NULL))
		return NULL;
	pFont = CreateFont(_("fonts/fixedsys8x16.fnt"), false);
	if (pFont == NULL)
		return NULL;
	dlg = __new(struct dlg_win);
	if (dlg == NULL)
		return NULL;
	dlg->title = strdup(title);
	dlg->message = strdup(message);
	if ((dlg->title == NULL) || (dlg->message == NULL)){
		release_dlg(dlg);
		return NULL;
	}
	dlg->buttons = buttons;
	dlg->active_btn = active_btn;
	dlg->cmd = cmd_none;
	dlg->align = align;
	make_dlg_geom(dlg);
	draw_dlg(dlg);
	return dlg;
}

#define BTNW 60
#define BTNH 30
#define BTNOFS 10

/* Рисование кнопок окна диалога */
static void draw_dlg_buttons(struct dlg_win *dlg)
{
	int x, y, w = 0, bw, h;
	GCPtr pGC;
	if ((pFont == NULL) || (dlg == NULL) || (dlg->n_buttons <= 0))
		return;
	bw = get_max_btn_txt_len(dlg->buttons) + 2 * DLG_BTN_HMARGIN;
	w = dlg->n_buttons * bw + (dlg->n_buttons - 1) * DLG_BTN_HGAP;
	w *= pFont->max_width;
	h = DLG_BTN_HEIGHT * pFont->max_height;
	x = dlg->left + (dlg->width - w) / 2;
	y = dlg->top + dlg->height - (DLG_BTN_HEIGHT + 1) * pFont->max_height; 
	pGC = CreateGC(x, y, w, h);
	SetFont(pGC, pFont);
	bw *= pFont->max_width;
	int dx = bw + DLG_BTN_HGAP * pFont->max_width;
	if (dlg->buttons < dlg_custom){
		DrawButton(pGC, 0, 0, bw, h, BTN_YES_TXT, false);
		if (dlg->buttons == dlg_yes_no)
			DrawButton(pGC, w - bw, 0, bw, h, BTN_NO_TXT, false);
	}else{
		x = 0;
		for (struct custom_btn *btn = (struct custom_btn *)dlg->buttons;
				btn->text != NULL; btn++, x += dx)
			DrawButton(pGC, x, 0, bw, h, btn->text, false);
	}
	SetPenColor(pGC, clNavy);
	DrawRect(pGC, dlg->active_btn * dx + 4, 4, bw - 8, h - 8);
	DeleteGC(pGC);
}

/* Рисование окна диалога */
static void draw_dlg(struct dlg_win *dlg)
{
	GCPtr pGC;
	int x, y, n, line = 0;
	char *p = NULL, str[DLG_MAX_STR_LEN + 1];
	if ((dlg == NULL) || (pFont == NULL))
		return;
	pGC = CreateGC(dlg->left, dlg->top, dlg->width, dlg->height);
	SetFont(pGC, pFont);
	DrawWindow(pGC, 0, 0, dlg->width, dlg->height, dlg->title, NULL);
	SetGCBounds(pGC, pGC->box.x + borderCX,
			pGC->box.y + titleCY,
			pGC->box.width - 2 * borderCX,
			pGC->box.height - titleCY - borderCX);
	ClearGC(pGC, clBtnFace);
	p = dlg->message;
	y = pFont->max_height;
	while (*p && (line < DLG_MAX_LINES)){
		n = get_words_len(p, DLG_MAX_STR_LEN);
		strncpy(str, p, n);
		str[n] = 0;
		p += n;
		if (*p == '\n')
			p++;
		else
			for (; *p && (*p == ' '); p++){}
		if (dlg->align == al_left)
			x = 0;
		else if (dlg->align == al_right)
			x = pGC->box.width - n * pFont->max_width;
		else	/* al_center */
			x = (pGC->box.width - n * pFont->max_width) / 2;
		TextOut(pGC, x, y, str);
		y += pFont->max_height;
		line++;
	}	
	DeleteGC(pGC);
	draw_dlg_buttons(dlg);
}

/* Переход к следующей кнопке диалогового окна */
static bool next_btn(struct dlg_win *dlg)
{
	if (dlg->n_buttons > 1){
		dlg->active_btn++;
		dlg->active_btn %= dlg->n_buttons;
		return true;
	}else
		return false;
}

/* Переход к предыдущей кнопке диалогового окна */
static bool prev_btn(struct dlg_win *dlg)
{
	if (dlg->n_buttons > 1){
		dlg->active_btn += dlg->n_buttons - 1;
		dlg->active_btn %= dlg->n_buttons;
		return true;
	}else
		return false;
}

/* Выбор первой кнопки диалогового окна */
static bool first_btn(struct dlg_win *dlg)
{
	bool ret = false;
	if (dlg->n_buttons > 1){
		if (dlg->active_btn != 0){
			dlg->active_btn = 0;
			ret = true;
		}
	}
	return ret;
}

/* Выбор последней кнопки диалогового окна */
static bool last_btn(struct dlg_win *dlg)
{
	bool ret = false;
	if (dlg->n_buttons > 1){
		if (dlg->active_btn != (dlg->n_buttons - 1)){
			dlg->active_btn = dlg->n_buttons - 1;
			ret = true;
		}
	}
	return ret;
}

bool process_dlg(struct dlg_win *dlg, struct kbd_event *e)
{
	bool ret = true;
	if ((e != NULL) && e->pressed){
		bool flag = false;
		if (e->shift_state == 0){
			switch (e->key){
				case KEY_TAB:
				case KEY_RIGHT:
					flag = next_btn(dlg);
					break;
				case KEY_LEFT:
					flag = prev_btn(dlg);
					break;
				case KEY_HOME:
					flag = first_btn(dlg);
					break;
				case KEY_END:
					flag = last_btn(dlg);
					break;
				case KEY_ESCAPE:
					if (dlg->buttons < dlg_custom){
						if (dlg->buttons == dlg_yes_no)
							flag = dlg->active_btn == DLG_BTN_NO;
						dlg->active_btn = DLG_BTN_NO;
						dlg->cmd = get_dlg_cmd(dlg);
						ret = false;
					}
					break;
				case KEY_SPACE:
				case KEY_ENTER:
				case KEY_NUMENTER:
					dlg->cmd = get_dlg_cmd(dlg);
					ret = false;
					break;
				case KEY_F9:
					if ((kt == key_srv) || (kt == key_dbg)){
						ClearScreen(clBtnFace);
						reset_term(false);
						dlg->cmd = cmd_reset;
						ret = false;
					}
					break;
			}
		}else if (kbd_exact_shift_state(e, SHIFT_SHIFT)){
			switch (e->key){
				case KEY_TAB:
					flag = prev_btn(dlg);
					break;
			}
		}
		if (flag)
			draw_dlg_buttons(dlg);
	}else
		process_scr();
	return ret;
}

int execute_dlg(struct dlg_win *dlg)
{
	struct kbd_event e;
	if (dlg == NULL)
		return -1;
	do {
		if (get_key_type() == key_none)
			return -1;
		kbd_get_event(&e);
	} while (process_dlg(dlg,&e));
	return dlg->cmd;
}

int message_box(const char *title, const char *message,
	intptr_t buttons, int active_btn, int align)
{
	struct dlg_win *dlg = NULL;
	int cmd = DLG_BTN_NO;
	if (message == NULL)
		return DLG_BTN_NO;
	if (title == NULL)
		title = "ВНИМАНИЕ";
	if ((dlg = new_dlg(title, message, buttons, active_btn, align)) == NULL)
		return DLG_BTN_NO;
	cmd = execute_dlg(dlg);
	release_dlg(dlg);
	return cmd;
}

/* Вывод диалогового окна без ожидания завершения работы с ним */
static struct dlg_win *nonmodal_dlg;

bool begin_message_box(const char *title, const char *message, int align)
{
	if ((nonmodal_dlg != NULL) || (message == NULL))
		return false;
	if (title == NULL)
		title = "ВНИМАНИЕ";
	nonmodal_dlg = new_dlg(title, message, dlg_none, 0, align);
	return nonmodal_dlg != NULL;
}

bool end_message_box(void)
{
	if (nonmodal_dlg == NULL)
		return false;
	else{
		release_dlg(nonmodal_dlg);
		nonmodal_dlg = NULL;
		return true;
	}
}

////////////////////////////////////////////////////////////////////////////////

void draw_hint(GCPtr pGC, char *szHintText,
		int x, int y, int w, int h)
{
	int left, top, width, height;

	left = pGC->box.x;
	top = pGC->box.y;
	width = pGC->box.width;
	height = pGC->box.height;

	SetGCBounds(pGC, x, y, w, h);
	ClearGC(pGC, clBtnFace);
	DrawBorder(pGC, 0, 0, GetCX(pGC), GetCY(pGC), 1,
			clBtnHighlight, clBtnShadow);
	DrawBorder(pGC, 4, 4, GetCX(pGC)-8, GetCY(pGC)-8, 1,
			clBtnShadow, clBtnHighlight);
	DrawText(pGC, 4, 4, GetCX(pGC)-8, GetCY(pGC)-8,
			szHintText, DT_CENTER | DT_VCENTER);
	SetGCBounds(pGC, left, top, width, height);
}

////////////////////////////////////////////////////////////////////////////////

#define MAX_VALUE	20

typedef struct {
	int l,t,w,h;
	int min_v, max_v;
	char value[MAX_VALUE];
	int cursor_pos;
} spin_edit_t;


#define AE_MIN_SPIN_EDIT	0x0
#define AE_MAX_SPIN_EDIT	0x1
#define AE_OK_BUTTON		0x2
#define AE_CANCEL_BUTTON	0x3

typedef struct {
	int l,t,w,h;
	GCPtr pGC;
	char *title;
	int active_elem;
	spin_edit_t min, max;
} log_range_dlg_t;


#define TR_DLG_LEFT	200
#define TR_DLG_TOP	200
#define TR_DLG_WIDTH	400
#define TR_DLG_HEIGHT	150

#define FONT_NAME	_("fonts/fixedsys8x16.fnt")

#define SE_WIDTH	140
#define SE_HEIGHT	30

#define MIN_SE_LEFT	((TR_DLG_WIDTH/4)-50)
#define MIN_SE_TOP	25

#define MAX_SE_LEFT	(MIN_SE_LEFT+SE_WIDTH+20)
#define MAX_SE_TOP	25

#define TR_TITLE	"Диапазон записей для печати"

#define TR_BUTTON_OFFSET2	10
#define TR_BUTTON_OFFSET	(TR_BUTTON_OFFSET2*2)
#define TR_BUTTON_WIDTH		100
#define TR_BUTTON_HEIGHT	30
#define TR_BUTTON_LEFT		((TR_DLG_WIDTH/4)-TR_BUTTON_OFFSET2)
#define TR_BUTTON_TOP		(TR_DLG_HEIGHT-TR_BUTTON_HEIGHT-30)

#define TR_HINT_TEXT		"Перемещение между элементами по Tab"

void init_spin_edit(spin_edit_t *se, int l, int t, int w, int h, int value,
		int min_val, int max_val)
{
	se->l = l;
	se->t = t;
	se->w = w;
	se->h = h;
	se->min_v = min_val;
	se->max_v = max_val;
	memset(se->value, 0, sizeof(se->value));
	sprintf(se->value, "%d", value);
	se->cursor_pos = 0;
}

/* Корректировка значения spin_edit */
void adjust_spin_edit(spin_edit_t *spin)
{
	int v;
	if (spin == NULL)
		return;
	v = atoi(spin->value);
	if (v < spin->min_v)
		v = spin->min_v;
	else if (v > spin->max_v)
		v = spin->max_v;
	sprintf(spin->value, "%d", v);
}

void draw_log_range_dlg(log_range_dlg_t *dlg);
	
log_range_dlg_t create_log_range_dlg(uint32_t min, uint32_t max, uint32_t minv, uint32_t maxv)
{
	log_range_dlg_t dlg;
	dlg.l = TR_DLG_LEFT;
	dlg.t = TR_DLG_TOP;
	dlg.w = TR_DLG_WIDTH;
	dlg.h = TR_DLG_HEIGHT;
	
	dlg.active_elem = AE_MIN_SPIN_EDIT;
	
	dlg.title = TR_TITLE;
	init_spin_edit(&dlg.min, MIN_SE_LEFT, MIN_SE_TOP, SE_WIDTH, SE_HEIGHT,
			min, minv, maxv);
	init_spin_edit(&dlg.max, MAX_SE_LEFT, MAX_SE_TOP, SE_WIDTH, SE_HEIGHT, 
			max, minv, maxv);
	draw_log_range_dlg(&dlg);
	return dlg;
}

void release_log_range_dlg(log_range_dlg_t *dlg)
{
	DeleteFont(dlg->pGC->pFont);
	dlg->pGC->pFont = NULL;
	DeleteGC(dlg->pGC);
}

void draw_log_range_dlg_buttons(log_range_dlg_t *dlg)
{
	DrawButton(dlg->pGC,TR_BUTTON_LEFT, TR_BUTTON_TOP, TR_BUTTON_WIDTH, 
			TR_BUTTON_HEIGHT, "Печать", false);
	DrawButton(dlg->pGC,TR_BUTTON_LEFT+TR_BUTTON_WIDTH+TR_BUTTON_OFFSET, TR_BUTTON_TOP,
			TR_BUTTON_WIDTH, TR_BUTTON_HEIGHT, "Отмена", false);
	SetPenColor(dlg->pGC, clNavy);
	if (dlg->active_elem == AE_OK_BUTTON)
		DrawRect(dlg->pGC, TR_BUTTON_LEFT+4, TR_BUTTON_TOP+4, 
				TR_BUTTON_WIDTH-8, TR_BUTTON_HEIGHT-8);
	else if (dlg->active_elem == AE_CANCEL_BUTTON)
		DrawRect(dlg->pGC, TR_BUTTON_LEFT+TR_BUTTON_WIDTH+TR_BUTTON_OFFSET+4,
				TR_BUTTON_TOP+4,TR_BUTTON_WIDTH-8, TR_BUTTON_HEIGHT-8);
}

void draw_spin_edit(GCPtr pGC, spin_edit_t *se, bool selected)
{
	DrawBorder(pGC, se->l, se->t, se->w, se->h, 2, clBtnShadow, clBtnHighlight);
	if (selected) {
		SetPenColor(pGC, clNavy);
		DrawRect(pGC, se->l-1, se->t-1, se->w+2, se->h+2);
	} else {
		SetPenColor(pGC, clBtnFace);
		DrawRect(pGC, se->l-1, se->t-1, se->w+2, se->h+2);
	}
	SetBrushColor(pGC, RGB(255, 255, 224));
	FillBox(pGC, se->l+2, se->t+2, se->w-4, se->h-4);
	DrawText(pGC, se->l+3, se->t+2, se->w-6, se->h-4, se->value,
			DT_LEFT | DT_VCENTER);
	if (selected) {
		int cursor_x;
		char *s;
		
		/* Drawing the cursor */
		SetRop2Mode(pGC, R2_XOR);
		SetPenColor(pGC, clBtnFace);
	
		s = strdup(se->value);
		s[se->cursor_pos] = 0;
		cursor_x = TextWidth(pGC->pFont, s);
		free(s);
		Line(pGC, se->l+3+cursor_x, se->t+4, se->l+3+cursor_x, se->t+se->h-4);
		Line(pGC, se->l+4+cursor_x, se->t+4, se->l+4+cursor_x, se->t+se->h-4);
		SetRop2Mode(pGC, R2_COPY);
	}
}

void draw_log_range_spin_edits(log_range_dlg_t *dlg)
{
	draw_spin_edit(dlg->pGC, &(dlg->min), dlg->active_elem == AE_MIN_SPIN_EDIT);
	draw_spin_edit(dlg->pGC, &(dlg->max), dlg->active_elem == AE_MAX_SPIN_EDIT);
}

void draw_log_range_dlg(log_range_dlg_t *dlg)
{
	GCPtr pGC = CreateGC(dlg->l, dlg->t, dlg->w, dlg->h);
	char s[128];
	FontPtr pFont = CreateFont(FONT_NAME, false);

	SetFont(pGC, pFont);
	DrawWindow(pGC, 0, 0, GetCX(pGC), GetCY(pGC), dlg->title, NULL);
	SetGCBounds(pGC,pGC->box.x+1,pGC->box.y+21,pGC->box.width-2,pGC->box.height-22);
	ClearGC(pGC, clBtnFace);
	
	dlg->pGC = pGC;
	draw_log_range_dlg_buttons(dlg);
	
	sprintf(s,"От [%d - %d]:",dlg->min.min_v, dlg->min.max_v);
	DrawText(pGC, dlg->min.l, dlg->min.t-dlg->min.h, dlg->min.w, dlg->min.h,
			s, 0);
	sprintf(s,"До [%d - %d]:",dlg->max.min_v, dlg->max.max_v);
	DrawText(pGC, dlg->max.l, dlg->max.t-dlg->max.h, dlg->max.w, dlg->max.h,
			s, 0);
	draw_log_range_spin_edits(dlg);
	
	/* drawing the hint */
	draw_hint(pGC, TR_HINT_TEXT, dlg->l, dlg->t+dlg->h, dlg->w, 30);
}

int get_num_key(int key)
{
	switch (key) {
	case KEY_NUMINS:
		return 0;
	case KEY_NUMEND:
		return 1;
	case KEY_NUMDOWN:
		return 2;
	case KEY_NUMPGDN:
		return 3;
	case KEY_NUMLEFT:
		return 4;
	case KEY_NUMDOT:
		return 5;
	case KEY_NUMRIGHT:
		return 6;
	case KEY_NUMHOME:
		return 7;
	case KEY_NUMUP:
		return 8;
	case KEY_NUMPGUP:
		return 9;
	default:
		return -1;
	}
}

bool process_log_range_dlg(log_range_dlg_t *dlg,struct kbd_event *e)
{
	if ((e != NULL) && e->pressed)
		switch (e->key){
			case KEY_UP: {
				spin_edit_t *se = NULL;
				if (dlg->active_elem == AE_MIN_SPIN_EDIT)
					se = &dlg->min;
				else if (dlg->active_elem == AE_MAX_SPIN_EDIT)
					se = &dlg->max;
				if (se) {
					int val=atoi(se->value);
					if (val != se->max_v) {
						val++;
						sprintf(se->value, "%d", val);	
						se->cursor_pos = strlen(se->value);
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_DOWN: {
				spin_edit_t *se = NULL;
				if (dlg->active_elem == AE_MIN_SPIN_EDIT)
					se = &dlg->min;
				else if (dlg->active_elem == AE_MAX_SPIN_EDIT)
					se = &dlg->max;
				if (se) {
					int val=atoi(se->value);
					if (val != se->min_v) {
						val--;
						sprintf(se->value, "%d", val);	
						se->cursor_pos = strlen(se->value);
						draw_spin_edit(dlg->pGC, se, true);
					}	
				}
				}
				break;
			case KEY_LEFT: {
				spin_edit_t *se = NULL;
				if (dlg->active_elem == AE_MIN_SPIN_EDIT)
					se = &dlg->min;
				else if (dlg->active_elem == AE_MAX_SPIN_EDIT)
					se = &dlg->max;
				if (se) {
					if (se->cursor_pos > 0) {
						se->cursor_pos--;
						draw_spin_edit(dlg->pGC, se, true);
					}	
				}
				}
				break;
			case KEY_RIGHT: {
				spin_edit_t *se = NULL;
				if (dlg->active_elem == AE_MIN_SPIN_EDIT)
					se = &dlg->min;
				else if (dlg->active_elem == AE_MAX_SPIN_EDIT)
					se = &dlg->max;
				if (se) {
					if (se->cursor_pos < strlen(se->value)) {
						se->cursor_pos++;
						draw_spin_edit(dlg->pGC, se, true);
					}	
				}
				}
				break;
			case KEY_BACKSPACE: {
				spin_edit_t *se = NULL;
				if (dlg->active_elem == AE_MIN_SPIN_EDIT)
					se = &dlg->min;
				else if (dlg->active_elem == AE_MAX_SPIN_EDIT)
					se = &dlg->max;
				if (se) {
					if (se->cursor_pos > 0) {
						memcpy(se->value+se->cursor_pos-1, se->value+se->cursor_pos,
								strlen(se->value)-se->cursor_pos+1);
						se->cursor_pos--;
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_HOME: {
				spin_edit_t *se = NULL;
				if (dlg->active_elem == AE_MIN_SPIN_EDIT)
					se = &dlg->min;
				else if (dlg->active_elem == AE_MAX_SPIN_EDIT)
					se = &dlg->max;
				if (se) {
					if (se->cursor_pos != 0) {
						se->cursor_pos=0;
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_END: {
				spin_edit_t *se = NULL;
				if (dlg->active_elem == AE_MIN_SPIN_EDIT)
					se = &dlg->min;
				else if (dlg->active_elem == AE_MAX_SPIN_EDIT)
					se = &dlg->max;
				if (se) {
					if (se->cursor_pos != strlen(se->value)) {
						se->cursor_pos=strlen(se->value);
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_DEL: {
				spin_edit_t *se = NULL;
				if (dlg->active_elem == AE_MIN_SPIN_EDIT)
					se = &dlg->min;
				else if (dlg->active_elem == AE_MAX_SPIN_EDIT)
					se = &dlg->max;
				if (se) {
					if (se->cursor_pos < strlen(se->value)) {
						memcpy(se->value+se->cursor_pos, se->value+se->cursor_pos+1,
								strlen(se->value)-se->cursor_pos+1);
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_TAB:
				if (dlg->active_elem == AE_CANCEL_BUTTON) {
					dlg->active_elem = AE_MIN_SPIN_EDIT;
					draw_log_range_dlg_buttons(dlg);
				} else if (dlg->active_elem++ == AE_MAX_SPIN_EDIT)
					draw_log_range_spin_edits(dlg);
				
				if (dlg->active_elem == AE_OK_BUTTON ||
					dlg->active_elem == AE_CANCEL_BUTTON)
					draw_log_range_dlg_buttons(dlg);
				else
					draw_log_range_spin_edits(dlg);
				break;
			case KEY_ESCAPE:
				dlg->active_elem = AE_CANCEL_BUTTON;
				return false;
			case KEY_SPACE:
			case KEY_ENTER:
			case KEY_NUMENTER:
				if (dlg->active_elem != AE_OK_BUTTON &&
						dlg->active_elem != AE_CANCEL_BUTTON)
					dlg->active_elem = AE_OK_BUTTON;
				return false;
			default: {
				int n;
				if ( (n=get_num_key(e->key)) != -1) {
					spin_edit_t *se = NULL;
					if (dlg->active_elem == AE_MIN_SPIN_EDIT)
						se = &dlg->min;
					else if (dlg->active_elem == AE_MAX_SPIN_EDIT)
						se = &dlg->max;
					if (se) {
						char sn[2];
						char buf1[64];
						char buf2[64];
						int val;
						
						
						sprintf(sn, "%d", n);
						if (se->cursor_pos == strlen(se->value)) {
							sprintf(buf1, "%d", se->min_v);
							sprintf(buf2, "%d", se->max_v);
							val = (strlen(buf1) > strlen(buf2))?strlen(buf1):strlen(buf2);
							if (strlen(se->value)+1>val)
								break;
							se->value[strlen(se->value)+1] = 0;
							se->value[strlen(se->value)] = sn[0];
						} else
							se->value[se->cursor_pos] = sn[0];
						se->cursor_pos++;
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
			 }
		}
	return true;
}

int execute_log_range_dlg(log_range_dlg_t *dlg)
{
	struct kbd_event e;
	if (dlg == NULL)
		return -1;
	do
		kbd_get_event(&e);
	while (process_log_range_dlg(dlg,&e));
	return (dlg->active_elem == AE_OK_BUTTON)?DLG_BTN_YES:DLG_BTN_NO;
}

int log_range_dlg(uint32_t *min, uint32_t *max, uint32_t min_val, uint32_t max_val)
{
	log_range_dlg_t dlg = 	create_log_range_dlg(*min, *max,
			min_val, max_val);
	int cmd;
	
	cmd=execute_log_range_dlg(&dlg);
	*min = atoi(dlg.min.value);
	*max = atoi(dlg.max.value);
	release_log_range_dlg(&dlg);

	return cmd;
}

/******************************************************************************/

#define TF_DLG_LEFT	TR_DLG_LEFT
#define TF_DLG_TOP	TR_DLG_TOP
#define TF_DLG_WIDTH	TR_DLG_WIDTH
#define TF_DLG_HEIGHT	TR_DLG_HEIGHT	

#define TF_TITLE	"Поиск записей по дате"

#define TF_BUTTON_LEFT		TR_BUTTON_LEFT
#define TF_BUTTON_TOP		TR_BUTTON_TOP
#define TF_BUTTON_WIDTH		TR_BUTTON_WIDTH
#define TF_BUTTON_HEIGHT	TR_BUTTON_HEIGHT
#define TF_BUTTON_OFFSET	TR_BUTTON_OFFSET

#undef AE_OK_BUTTON
#undef AE_CANCEL_BUTTON

#define AE_HOUR_SPIN_EDIT	0x0
#define AE_DAY_SPIN_EDIT	0x1
#define AE_MON_SPIN_EDIT	0x2
#define AE_YEAR_SPIN_EDIT	0x3
#define AE_OK_BUTTON		0x4
#define AE_CANCEL_BUTTON	0x5

#undef SE_WIDTH
#define SE_WIDTH	65

#define HOUR_SE_LEFT	(30)
#define HOUR_SE_TOP	25

#define DAY_SE_LEFT	(HOUR_SE_LEFT+SE_WIDTH+25)
#define DAY_SE_TOP	25

#define MON_SE_LEFT	(DAY_SE_LEFT+SE_WIDTH+25)
#define MON_SE_TOP	25

#define YEAR_SE_LEFT	(MON_SE_LEFT+SE_WIDTH+25)
#define YEAR_SE_TOP	25


typedef struct {
	int l,t,w,h;
	GCPtr pGC;
	char *title;
	int active_elem;
	spin_edit_t hour;
	spin_edit_t day;
	spin_edit_t mon;
	spin_edit_t year;
} find_log_date_dlg_t;


void draw_find_log_date_dlg(find_log_date_dlg_t *dlg);
	
find_log_date_dlg_t create_find_log_date_dlg(int hour, int day, int mon, int year)
{
	find_log_date_dlg_t dlg;
	
	dlg.l = TF_DLG_LEFT;
	dlg.t = TF_DLG_TOP;
	dlg.w = TF_DLG_WIDTH;
	dlg.h = TF_DLG_HEIGHT;
	
	dlg.active_elem = AE_HOUR_SPIN_EDIT;
	
	dlg.title = TF_TITLE;

	init_spin_edit(&dlg.hour, HOUR_SE_LEFT, HOUR_SE_TOP, SE_WIDTH, SE_HEIGHT, 
			hour, 0, 23);
	init_spin_edit(&dlg.day, DAY_SE_LEFT, DAY_SE_TOP, SE_WIDTH, SE_HEIGHT,
			day, 1, 31);
	init_spin_edit(&dlg.mon, MON_SE_LEFT, MON_SE_TOP, SE_WIDTH, SE_HEIGHT, 
			mon, 1, 12);
	init_spin_edit(&dlg.year, YEAR_SE_LEFT, YEAR_SE_TOP, SE_WIDTH, SE_HEIGHT, 
			year, 1900, 2032);
	draw_find_log_date_dlg(&dlg);
	return dlg;
}

void release_find_log_date_dlg(find_log_date_dlg_t *dlg)
{
	DeleteFont(dlg->pGC->pFont);
	dlg->pGC->pFont = NULL;
	DeleteGC(dlg->pGC);
}

void draw_find_log_date_dlg_buttons(find_log_date_dlg_t *dlg)
{
	DrawButton(dlg->pGC,TF_BUTTON_LEFT, TF_BUTTON_TOP, TF_BUTTON_WIDTH, 
			TF_BUTTON_HEIGHT, "Поиск", false);
	DrawButton(dlg->pGC,TF_BUTTON_LEFT+TF_BUTTON_WIDTH+TF_BUTTON_OFFSET, TF_BUTTON_TOP,
			TF_BUTTON_WIDTH, TF_BUTTON_HEIGHT, "Отмена", false);
	SetPenColor(dlg->pGC, clNavy);
	if (dlg->active_elem == AE_OK_BUTTON)
		DrawRect(dlg->pGC, TF_BUTTON_LEFT+4, TF_BUTTON_TOP+4, 
				TF_BUTTON_WIDTH-8, TF_BUTTON_HEIGHT-8);
	else if (dlg->active_elem == AE_CANCEL_BUTTON)
		DrawRect(dlg->pGC, TF_BUTTON_LEFT+TF_BUTTON_WIDTH+TF_BUTTON_OFFSET+4,
				TF_BUTTON_TOP+4,TF_BUTTON_WIDTH-8, TF_BUTTON_HEIGHT-8);
}

void draw_find_log_date_spin_edits(find_log_date_dlg_t *dlg)
{
	draw_spin_edit(dlg->pGC, &(dlg->day), dlg->active_elem == AE_DAY_SPIN_EDIT);
	draw_spin_edit(dlg->pGC, &(dlg->mon), dlg->active_elem == AE_MON_SPIN_EDIT);
	draw_spin_edit(dlg->pGC, &(dlg->year), dlg->active_elem == AE_YEAR_SPIN_EDIT);
	draw_spin_edit(dlg->pGC, &(dlg->hour), dlg->active_elem == AE_HOUR_SPIN_EDIT);
}

void draw_find_log_date_dlg(find_log_date_dlg_t *dlg)
{
	GCPtr pGC = CreateGC(dlg->l, dlg->t, dlg->w, dlg->h);
	FontPtr pFont = CreateFont(FONT_NAME, false);
	char s[128];

	SetFont(pGC, pFont);
	DrawWindow(pGC, 0, 0, GetCX(pGC), GetCY(pGC), dlg->title, NULL);
	SetGCBounds(pGC,pGC->box.x+1,pGC->box.y+21,pGC->box.width-2,pGC->box.height-22);
	ClearGC(pGC, clBtnFace);
	
	dlg->pGC = pGC;
	draw_find_log_date_dlg_buttons(dlg);

	sprintf(s,"Час");
	DrawText(pGC, dlg->hour.l, dlg->hour.t-dlg->hour.h, dlg->hour.w, dlg->hour.h,
			s, 0);
	sprintf(s,"Число");
	DrawText(pGC, dlg->day.l, dlg->day.t-dlg->day.h, dlg->day.w, dlg->day.h,
			s, 0);
	sprintf(s,"Месяц");
	DrawText(pGC, dlg->mon.l, dlg->mon.t-dlg->mon.h, dlg->mon.w, dlg->mon.h,
			s, 0);
	sprintf(s,"Год");
	DrawText(pGC, dlg->year.l, dlg->year.t-dlg->year.h, dlg->year.w, dlg->year.h,
			s, 0);
	
	draw_find_log_date_spin_edits(dlg);
	/* drawing the hint */
	draw_hint(pGC, TR_HINT_TEXT, dlg->l, dlg->t+dlg->h, dlg->w, 30);
}

#define INIT_CODE \
	spin_edit_t *se = NULL; \
	if (dlg->active_elem == AE_HOUR_SPIN_EDIT) \
		se = &dlg->hour; \
	if (dlg->active_elem == AE_DAY_SPIN_EDIT) \
		se = &dlg->day; \
	else if (dlg->active_elem == AE_MON_SPIN_EDIT) \
		se = &dlg->mon; \
	else if (dlg->active_elem == AE_YEAR_SPIN_EDIT) \
		se = &dlg->year;

bool process_find_log_date_dlg(find_log_date_dlg_t *dlg,struct kbd_event *e)
{
/* Корректировка введенных значений */
	void adjust_spin_edits(void)
	{
		adjust_spin_edit(&dlg->hour);
		adjust_spin_edit(&dlg->day);
		adjust_spin_edit(&dlg->mon);
		adjust_spin_edit(&dlg->year);
	}
	if ((e != NULL) && e->pressed)
		switch (e->key){
			case KEY_UP: {
				INIT_CODE	
				if (se) {
					int val=atoi(se->value);
					if (val != se->max_v) {
						val++;
						sprintf(se->value, "%d", val);	
						se->cursor_pos = strlen(se->value);
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_DOWN: {
				INIT_CODE
				if (se) {
					int val=atoi(se->value);
					if (val != se->min_v) {
						val--;
						sprintf(se->value, "%d", val);	
						se->cursor_pos = strlen(se->value);
						draw_spin_edit(dlg->pGC, se, true);
					}	
				}
				}
				break;
			case KEY_LEFT: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos > 0) {
						se->cursor_pos--;
						draw_spin_edit(dlg->pGC, se, true);
					}	
				}
				}
				break;
			case KEY_RIGHT: {
				INIT_CODE	
				if (se) {
					if (se->cursor_pos < strlen(se->value)) {
						se->cursor_pos++;
						draw_spin_edit(dlg->pGC, se, true);
					}	
				}
				}
				break;
			case KEY_BACKSPACE: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos > 0) {
						memcpy(se->value+se->cursor_pos-1, se->value+se->cursor_pos,
								strlen(se->value)-se->cursor_pos+1);
						se->cursor_pos--;
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_HOME: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos != 0) {
						se->cursor_pos=0;
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_END: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos != strlen(se->value)) {
						se->cursor_pos=strlen(se->value);
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_DEL: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos < strlen(se->value)) {
						memcpy(se->value+se->cursor_pos, se->value+se->cursor_pos+1,
								strlen(se->value)-se->cursor_pos+1);
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_TAB:
				adjust_spin_edits();
				if (dlg->active_elem == AE_CANCEL_BUTTON) {
					dlg->active_elem = AE_HOUR_SPIN_EDIT;
					draw_find_log_date_dlg_buttons(dlg);
				} else if (dlg->active_elem++ == AE_YEAR_SPIN_EDIT)
					draw_find_log_date_spin_edits(dlg);
				
				if (dlg->active_elem == AE_OK_BUTTON ||
					dlg->active_elem == AE_CANCEL_BUTTON)
					draw_find_log_date_dlg_buttons(dlg);
				else
					draw_find_log_date_spin_edits(dlg);
				break;
			case KEY_ESCAPE:
				dlg->active_elem = AE_CANCEL_BUTTON;
				return false;
			case KEY_SPACE:
			case KEY_ENTER:
			case KEY_NUMENTER:
				adjust_spin_edits();
				if (dlg->active_elem != AE_OK_BUTTON &&
						dlg->active_elem != AE_CANCEL_BUTTON)
					dlg->active_elem = AE_OK_BUTTON;
				return false;
			default: {
				int n;
				if ( (n=get_num_key(e->key)) != -1) {
					INIT_CODE
					if (se) {
						char sn[2];
						char buf1[64];
						char buf2[64];
						int val;
						
						
						sprintf(sn, "%d", n);
						if (se->cursor_pos == strlen(se->value)) {
							sprintf(buf1, "%d", se->min_v);
							sprintf(buf2, "%d", se->max_v);
							val = (strlen(buf1) > strlen(buf2))?strlen(buf1):strlen(buf2);
							if (strlen(se->value)+1>val)
								break;
							se->value[strlen(se->value)+1] = 0;
							se->value[strlen(se->value)] = sn[0];
						} else
							se->value[se->cursor_pos] = sn[0];
						se->cursor_pos++;
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
			 }
		}
	return true;
}

int execute_find_log_date_dlg(find_log_date_dlg_t *dlg)
{
	struct kbd_event e;
	if (dlg == NULL)
		return -1;
	do
		kbd_get_event(&e);
	while (process_find_log_date_dlg(dlg,&e));
	return (dlg->active_elem == AE_OK_BUTTON)?DLG_BTN_YES:DLG_BTN_NO;
}

int find_log_date_dlg(int *hour, int *day, int *mon, int *year)
{
	find_log_date_dlg_t dlg = create_find_log_date_dlg(*hour, *day, *mon, *year);
	int cmd;
	
	cmd=execute_find_log_date_dlg(&dlg);

	*hour = atoi(dlg.hour.value);
	*day = atoi(dlg.day.value);
	*mon = atoi(dlg.mon.value);
	*year = atoi(dlg.year.value);

	release_find_log_date_dlg(&dlg);

	return cmd;
}



/******************************************************************************/

#undef AE_OK_BUTTON
#undef AE_CANCEL_BUTTON

#define AE_NUMBER_SPIN_EDIT	0x1
#define AE_OK_BUTTON		0x2
#define AE_CANCEL_BUTTON	0x3

#undef SE_WIDTH
#define SE_WIDTH			100

#define NUMBER_SE_LEFT		(TF_DLG_WIDTH - SE_WIDTH)/2
#define NUMBER_SE_TOP		30

typedef struct {
	int l,t,w,h;
	GCPtr pGC;
	char *title;
	int active_elem;
	spin_edit_t number;
} find_log_number_dlg_t;

extern void draw_find_log_number_dlg(find_log_number_dlg_t *dlg);
	
find_log_number_dlg_t create_find_log_number_dlg(int number, int min_number,
		int max_number)
{
	find_log_number_dlg_t dlg;
	
	dlg.l = TF_DLG_LEFT;
	dlg.t = TF_DLG_TOP;
	dlg.w = TF_DLG_WIDTH;
	dlg.h = TF_DLG_HEIGHT;
	
	dlg.active_elem = AE_NUMBER_SPIN_EDIT;
	
	dlg.title = "Поиск записей по номеру";

	init_spin_edit(&dlg.number, NUMBER_SE_LEFT, NUMBER_SE_TOP,
			SE_WIDTH, SE_HEIGHT, number, min_number, max_number);
	draw_find_log_number_dlg(&dlg);
	return dlg;
}

void release_find_log_number_dlg(find_log_number_dlg_t *dlg)
{
	DeleteFont(dlg->pGC->pFont);
	dlg->pGC->pFont = NULL;
	DeleteGC(dlg->pGC);
}

void draw_find_log_number_dlg_buttons(find_log_number_dlg_t *dlg)
{
	DrawButton(dlg->pGC,TF_BUTTON_LEFT, TF_BUTTON_TOP, TF_BUTTON_WIDTH, 
			TF_BUTTON_HEIGHT, "Поиск", false);
	DrawButton(dlg->pGC,TF_BUTTON_LEFT+TF_BUTTON_WIDTH+TF_BUTTON_OFFSET, TF_BUTTON_TOP,
			TF_BUTTON_WIDTH, TF_BUTTON_HEIGHT, "Отмена", false);
	SetPenColor(dlg->pGC, clNavy);
	if (dlg->active_elem == AE_OK_BUTTON)
		DrawRect(dlg->pGC, TF_BUTTON_LEFT+4, TF_BUTTON_TOP+4, 
				TF_BUTTON_WIDTH-8, TF_BUTTON_HEIGHT-8);
	else if (dlg->active_elem == AE_CANCEL_BUTTON)
		DrawRect(dlg->pGC, TF_BUTTON_LEFT+TF_BUTTON_WIDTH+TF_BUTTON_OFFSET+4,
				TF_BUTTON_TOP+4,TF_BUTTON_WIDTH-8, TF_BUTTON_HEIGHT-8);
}

void draw_find_log_number_spin_edits(find_log_number_dlg_t *dlg)
{
	draw_spin_edit(dlg->pGC, &(dlg->number), dlg->active_elem == AE_NUMBER_SPIN_EDIT);
}

void draw_find_log_number_dlg(find_log_number_dlg_t *dlg)
{
	GCPtr pGC = CreateGC(dlg->l, dlg->t, dlg->w, dlg->h);
	FontPtr pFont = CreateFont(FONT_NAME, false);
	char s[128];

	SetFont(pGC, pFont);
	DrawWindow(pGC, 0, 0, GetCX(pGC), GetCY(pGC), dlg->title, NULL);
	SetGCBounds(pGC,pGC->box.x+1,pGC->box.y+21,pGC->box.width-2,pGC->box.height-22);
	ClearGC(pGC, clBtnFace);
	
	dlg->pGC = pGC;
	draw_find_log_number_dlg_buttons(dlg);

	sprintf(s,"Номер");
	DrawText(pGC, dlg->number.l, dlg->number.t-dlg->number.h,
			dlg->number.w, dlg->number.h, s, 0);
	
	draw_find_log_number_spin_edits(dlg);

	/* drawing the hint */
	draw_hint(pGC, TR_HINT_TEXT, dlg->l, dlg->t+dlg->h, dlg->w, 30);
}

#undef INIT_CODE
#define INIT_CODE \
	spin_edit_t *se = NULL; \
	if (dlg->active_elem == AE_NUMBER_SPIN_EDIT) \
		se = &dlg->number; \

bool process_find_log_number_dlg(find_log_number_dlg_t *dlg,struct kbd_event *e)
{
	if ((e != NULL) && e->pressed)
		switch (e->key){
			case KEY_UP: {
				INIT_CODE	
				if (se) {
					int val=atoi(se->value);
					if (val != se->max_v) {
						val++;
						sprintf(se->value, "%d", val);	
						se->cursor_pos = strlen(se->value);
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_DOWN: {
				INIT_CODE
				if (se) {
					int val=atoi(se->value);
					if (val != se->min_v) {
						val--;
						sprintf(se->value, "%d", val);	
						se->cursor_pos = strlen(se->value);
						draw_spin_edit(dlg->pGC, se, true);
					}	
				}
				}
				break;
			case KEY_LEFT: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos > 0) {
						se->cursor_pos--;
						draw_spin_edit(dlg->pGC, se, true);
					}	
				}
				}
				break;
			case KEY_RIGHT: {
				INIT_CODE	
				if (se) {
					if (se->cursor_pos < strlen(se->value)) {
						se->cursor_pos++;
						draw_spin_edit(dlg->pGC, se, true);
					}	
				}
				}
				break;
			case KEY_BACKSPACE: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos > 0) {
						memcpy(se->value+se->cursor_pos-1, se->value+se->cursor_pos,
								strlen(se->value)-se->cursor_pos+1);
						se->cursor_pos--;
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_HOME: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos != 0) {
						se->cursor_pos=0;
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_END: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos != strlen(se->value)) {
						se->cursor_pos=strlen(se->value);
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_DEL: {
				INIT_CODE
				if (se) {
					if (se->cursor_pos < strlen(se->value)) {
						memcpy(se->value+se->cursor_pos, se->value+se->cursor_pos+1,
								strlen(se->value)-se->cursor_pos+1);
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
				}
				break;
			case KEY_TAB:
				if (dlg->active_elem == AE_CANCEL_BUTTON) {
					dlg->active_elem = AE_NUMBER_SPIN_EDIT;
					draw_find_log_number_dlg_buttons(dlg);
				} else if (dlg->active_elem++ == AE_NUMBER_SPIN_EDIT)
					draw_find_log_number_spin_edits(dlg);
				
				if (dlg->active_elem == AE_OK_BUTTON ||
					dlg->active_elem == AE_CANCEL_BUTTON)
					draw_find_log_number_dlg_buttons(dlg);
				else
					draw_find_log_number_spin_edits(dlg);
				break;
			case KEY_ESCAPE:
				dlg->active_elem = AE_CANCEL_BUTTON;
				return false;
			case KEY_SPACE:
			case KEY_ENTER:
			case KEY_NUMENTER:
				if (dlg->active_elem != AE_OK_BUTTON &&
						dlg->active_elem != AE_CANCEL_BUTTON)
					dlg->active_elem = AE_OK_BUTTON;
				return false;
			default: {
				int n;
				if ( (n=get_num_key(e->key)) != -1) {
					INIT_CODE
					if (se) {
						char sn[2];
						char buf1[64];
						char buf2[64];
						int val;
						
						
						sprintf(sn, "%d", n);
						if (se->cursor_pos == strlen(se->value)) {
							sprintf(buf1, "%d", se->min_v);
							sprintf(buf2, "%d", se->max_v);
							val = (strlen(buf1) > strlen(buf2))?strlen(buf1):strlen(buf2);
							if (strlen(se->value)+1>val)
								break;
							se->value[strlen(se->value)+1] = 0;
							se->value[strlen(se->value)] = sn[0];
						} else
							se->value[se->cursor_pos] = sn[0];
						se->cursor_pos++;
						draw_spin_edit(dlg->pGC, se, true);
					}
				}
			 }
		}
	return true;
}

int execute_find_log_number_dlg(find_log_number_dlg_t *dlg)
{
	struct kbd_event e;
	if (dlg == NULL)
		return -1;
	do
		kbd_get_event(&e);
	while (process_find_log_number_dlg(dlg,&e));
	return (dlg->active_elem == AE_OK_BUTTON)?DLG_BTN_YES:DLG_BTN_NO;
}

int find_log_number_dlg(uint32_t *number, uint32_t min_number, uint32_t max_number)
{
	find_log_number_dlg_t dlg = create_find_log_number_dlg(*number,
			min_number, max_number);
	int cmd;
	char *p = NULL;
	cmd = execute_find_log_number_dlg(&dlg);
	*number = strtoul(dlg.number.value, &p, 10);
	if (*p)
		*number = 0;
	release_find_log_number_dlg(&dlg);
	return cmd;
}
