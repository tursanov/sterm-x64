/*
	sterm calculator
	(c) gsr 2000
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "kbd.h"
#include "paths.h"
#include "sterm.h"
#include "gui/scr.h"
#include "gui/options.h"
#include "gui/adv_calc.h"
#include "gui/exgdi.h"

/* Размеры кнопки и отступ м/у кнопками */
#define BCX	45
#define BCY	35
#define BO	2

/* Кол-во кнопок по горизонтали */
#define BTNW 6
/* Кол-во кнопок по вертикали */
#define BTNH 5

typedef struct {
	char *text;
	bool down;
} CALCBTN, *CalcBtnPtr;

#define CB_BKSPACE 	3
#define CB_CE 		4
#define CB_CLEAR	5
#define CB_MC		6
#define CB_7		7
#define CB_8		8
#define CB_9		9
#define CB_DIV		10
#define CB_PERCENT	11
#define CB_MR		12
#define CB_4		13
#define CB_5		14
#define CB_6		15
#define CB_MUL		16
#define CB_DZERO	17
#define CB_MPLUS	18
#define CB_1		19
#define CB_2		20
#define CB_3		21
#define CB_MINUS	22
#define CB_TZERO	23
#define CB_MMINUS	24
#define CB_0		25
#define CB_SIGN		26
#define CB_DOT		27
#define CB_PLUS		28
#define CB_EQUAL	29

static CALCBTN cbtns[] = { 
	{NULL, false}, {NULL, false}, {NULL, false},  {"<-", false}, {"CE", false}, {"C", false},
	{"MC", false}, {"7", false},  {"8", false},   {"9", false},  {"/", false},  {"%", false},
	{"MR", false}, {"4", false},  {"5", false},   {"6", false},  {"*", false},  {"00", false},
	{"M+", false}, {"1", false},  {"2", false},   {"3", false},  {"-", false},  {"000", false},
	{"M-", false}, {"0", false},  {"+/-", false}, {".", false},  {"+", false},  {"=", false},
};

static FontPtr pFont = NULL;
int wx=0, wy=0;

static double operands[2]={0,0};		/* операнды */
static int cur_operand=0;			/* текущий операнд */
static int action = op_none;			/* действие */
static char op_buf[128];			/* текстовое представление операнда */
static int op_ptr;				/* указатель текущей позиции op_buf */
static int op_sign;				/* знак операнда */
static bool can_op_destroy;			/* флаг перезаписи операнда */
static double memory=0.0;			/* память */
static bool memuse=false;			/* флаг использования памяти */

void RedrawBtn(int index)
{
	if (cbtns[index].text != NULL) {
		int col = index % BTNW, row=index / BTNW;
		GCPtr pGC = CreateGC(wx+col*(BCX+BO)-BO, wy+row*(BCY+BO)-BO, BCX+1, BCY+1);
		GCPtr pMemGC = CreateMemGC(BCX+1, BCY+1);
		SetFont(pMemGC, pFont);
		DrawButton(pMemGC, 0, 0, BCX, BCY, cbtns[index].text, cbtns[index].down);
		CopyGC(pGC, 0, 0, pMemGC, 0, 0, BCX+1, BCY+1);
		DeleteGC(pMemGC);
		DeleteGC(pGC);
	}	
}

#define HELP_HEIGHT	150
#define MAX_SYM		128

bool draw_calc_help(GCPtr pGC)
{
	FILE *hlp_f;
	
	DrawBorder(pGC, 0, 0, GetCX(pGC), GetCY(pGC), 1,
		clBtnShadow, clBtnHighlight);
	
	if ((hlp_f = fopen(_("readme.calc"), "r"))) {
		char buf[MAX_SYM];
		int i=0;
		
		SetFont(pGC, font_hints);
		while (fgets(buf, MAX_SYM, hlp_f)) {
			TextOut(pGC, 10, 2+i*font_hints->max_height, buf);
			i++;
		}		
		fclose(hlp_f);
		return true;
	}else
		return false;
}

bool adv_draw_calc()
{
	int CalcWidth  = BTNW*(BCX+BO)-BO + borderCX*2+20;
	int CalcHeight = (BTNW)*(BCY+BO)-BO + borderCX*2 + titleCY+30 + HELP_HEIGHT;
	int x = (DISCX-CalcWidth)/2, y = (DISCY-CalcHeight)/2-50;
	int i;
	GCPtr pGC = CreateGC(0, 0, DISCX, DISCY);
	FontPtr pTitleFont = CreateFont(_("fonts/andale8x15.fnt"), true);
	
	SetFont(pGC, pTitleFont);
	DrawWindow(pGC, x, y, CalcWidth, CalcHeight, "Калькулятор", pGC);
	ClearGC(pGC, clBtnFace);
	
	wx = x+borderCX*2+11;
	wy = y+borderCX*2+titleCY+BCY+2*BO+10;
	
	DrawBorder(pGC, 10, BCY+BO*2+10, BCX, BCY-2, 2, clBtnShadow, clBtnHighlight);
	DrawBorder(pGC, 10, 7, CalcWidth-20-BO, BCY, 2, clBtnShadow, clBtnHighlight);
	SetBrushColor(pGC, RGB(255, 255, 224));
	FillBox(pGC, 12, 9, CalcWidth-20-BO-3, BCY-3);
	
	SetGCBounds(pGC, pGC->box.x+10, pGC->box.y+(BTNW)*(BCY+BO)-BO+borderCX*2+titleCY,
		CalcWidth - 20, HELP_HEIGHT);
	draw_calc_help(pGC);
		
	DeleteFont(pTitleFont);
	DeleteGC(pGC);
	
	for (i=0; i<ASIZE(cbtns); i++)
		RedrawBtn(i);
	adv_draw_op();
	return true;
}

void ShowMemuse()
{
	GCPtr pGC = CreateGC(wx-BO+2, wy-BO+3, BCX-3, BCY-6);
	GCPtr pMemGC = CreateMemGC(BCX-3, BCY-6);
	SetFont(pMemGC, pFont);
	ClearGC(pMemGC, clBtnFace);
	if (memuse)
		DrawText(pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC), "M", 0);
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC));
	DeleteGC(pMemGC);
	DeleteGC(pGC);
}

bool adv_draw_op(void)		/* Вывод операнда на экран */
{
	GCPtr pGC = CreateGC(wx, wy-BCY-BO-4, BTNW*(BCX+BO)-BO*2-1, BCY-2);
	GCPtr pMemGC = CreateMemGC(GetCX(pGC), GetCY(pGC));
	char *buf = malloc(MAX_DIGITS+2);
	char *p = buf;
	
	memset(p, 0, MAX_DIGITS+2);
	if (op_sign < 0)
		*p++ = '-';
	memcpy(p, op_buf, strlen(op_buf));
	
	ClearGC(pMemGC, RGB(255, 255, 224));
	SetFont(pMemGC, pFont);
	DrawText(pMemGC, 0, 0, GetCX(pGC), GetCY(pGC), buf, DT_RIGHT | DT_VCENTER);
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC));
	free(buf);
	DeleteGC(pMemGC);
	DeleteGC(pGC);
	
	return true;
}

void adv_clear_calc(void)	/* Сброс калькулятора в начальное состояние */
{
	memset(operands,0,2*sizeof(double));
	cur_operand=0;
	action=op_none;
	adv_clear_op();
}

void adv_clear_op(void)	/* Сброс текущего операнда */
{
	sprintf(op_buf,"0");
	op_ptr=0;
	op_sign=1;
	can_op_destroy=true;
}

bool adv_get_op_val(double *v)
{
	char *eptr;
	double vv=strtod(op_buf,&eptr);
	if (*eptr != 0)
		return false;
	else{
		*v = (op_sign == 1) ? vv : -vv;
		return true;
	}
}

bool adv_set_op_val(double v)
{
	int n=0,m=0,k;
	double w=fabs(v);
	while (w >= 1){
		w /= 10;
		n++;
	}
	if (n > MAX_DIGITS)
		return false;
	m=MAX_DIGITS-(n+1);
	if (m < 0)
		m=0;
	sprintf(op_buf,"%*.*f",MAX_DIGITS,m,fabs(v));
/** calculator bug fix **/
	for (n=0; n < strlen(op_buf); n++)
		if (op_buf[n] == '.')
			break;
/** end of modifications **/
	for (k=n; k < strlen(op_buf); k++)
	 	if ((op_buf[k] != '.') && (op_buf[k] != '0'))
			n=k+1;
	if (n == 0) n=1;
	op_buf[n]=0;
	op_sign = (v < 0) ? -1 : 1;
	return true;
}

void adv_calc_error(void)
{
	adv_clear_calc();
	sprintf(op_buf,"Ошибка");
	adv_draw_op();
	/* err_beep(); */
}

bool adv_calculate(void)	/* Вычисление текущего результата */
{
	if (!adv_get_op_val(&operands[cur_operand]))
		return false;
	if ((cur_operand > 0) && !can_op_destroy){
		switch (action){
			case op_plus:
				operands[0] += operands[1];
				break;
			case op_minus:
				operands[0] -= operands[1];
				break;
			case op_mul:
				operands[0] *= operands[1];
				break;
			case op_div:
				if (operands[1] != 0)
					operands[0] /= operands[1];
				else
					return false;
				break;	
			case op_percent:
				operands[0] = operands[0]*operands[1]/100;
				break;
		}
		action=op_none;
	}
	return true;
}

bool adv_calc_right_char(int ch)
{
	return isdigit(ch) || (ch == '.' && !strstr(op_buf, "."));
}

bool adv_calc_right_op(int ch)
{
	return	(ch == '+') ||
		(ch == '-') ||
		(ch == '*') ||
		(ch == '/');
}

bool adv_calc_input_char(int ch)	/* Ввод очередного символа */
{
	if (can_op_destroy)
		adv_clear_op();
	if (!adv_calc_right_char(ch) || (op_ptr >= MAX_DIGITS))
		return false;
	op_buf[op_ptr++]=ch;
	op_buf[op_ptr]=0;
	can_op_destroy=false;
	adv_adjust_op();
	return true;
}

int adv_get_action(int index)
{
	switch (index){
		case CB_PLUS:
			return op_plus;
		case CB_MINUS:
			return op_minus;
		case CB_MUL:
			return op_mul;
		case CB_DIV:
			return op_div;
		case CB_PERCENT:
			return op_percent;
		default:
			return op_none;
	}
}

bool adv_calc_input_op(int op)
{
	action=op;
	return can_op_destroy=true;
}

bool adv_ch_op_sign(void)		/* Изменение знака */
{
	op_sign *= -1;
	return true;
}

bool adv_calc_bk_char(void)		/* Удаление предыдущего символа */
{
	if (op_ptr > 0){
		if (op_ptr > 1)
			op_buf[--op_ptr]=0;
		else
			adv_clear_op();
		adv_adjust_op();
		return true;
	}else
		return false;
}

bool adv_adjust_op(void)
{
	if (adv_get_op_val(&operands[cur_operand]))
		return true;
	else{
		adv_calc_error();
		return false;
	}
}

int adv_ch_to_idx(char ch)
{
	if (ch == '0') return CB_0;
	else if (ch == '1') return CB_1;
	else if (ch == '2') return CB_2;
	else if (ch == '3') return CB_3;
	else if (ch == '4') return CB_4;
	else if (ch == '5') return CB_5;
	else if (ch == '6') return CB_6;
	else if (ch == '7') return CB_7;
	else if (ch == '8') return CB_8;
	else if (ch == '9') return CB_9;
	else return -1;
}

bool adv_process_calc_key(int index, struct kbd_event *e)
{
	int act = op_none;
	
	switch(index) {
	case CB_0: case CB_1: case CB_2: case CB_3: case CB_4:
	case CB_5: case CB_6: case CB_7: case CB_8:	case CB_9:
	case CB_DOT:
		if (adv_calc_input_char(e->ch))
			adv_draw_op();
		return true;
	case CB_DZERO:
		adv_calc_input_char('0');
		adv_calc_input_char('0');
		adv_draw_op();
		return true;
	case CB_TZERO:
		adv_calc_input_char('0');
		adv_calc_input_char('0');
		adv_calc_input_char('0');
		adv_draw_op();
		return true;
	case CB_DIV: case CB_MUL: case CB_PLUS: case CB_MINUS: case CB_PERCENT:
		act=adv_get_action(index);
		if (cur_operand == 0)
			action=act;
	case CB_EQUAL:
		if (!adv_calculate())
			adv_calc_error();
		else{
			if (adv_set_op_val(operands[0])){
				if (cur_operand == 0)
					cur_operand=1;
				if (e->key == KEY_NUMENTER || e->key == KEY_ENTER)
					cur_operand=0;
				can_op_destroy=true;
				adv_draw_op();
				action=act;
			}else
				adv_calc_error();
		}
		return true;
	case CB_BKSPACE:
		if (adv_calc_bk_char())
			adv_draw_op();
		return true;
	case CB_CE:
		adv_clear_op();
		adv_draw_op();
		return true;
	case CB_CLEAR:
		adv_clear_calc();
		adv_draw_op();
		return true;
	case CB_SIGN:
		adv_ch_op_sign();
		adv_adjust_op();
		adv_draw_op();
		return true;
	case CB_MPLUS:
		memory += operands[cur_operand];
		memuse = true;
		ShowMemuse();
		return true;
	case CB_MMINUS:
		memory -= operands[cur_operand];
		memuse = true;
		ShowMemuse();
		return true;
	case CB_MC:
		memory = 0.0;
		memuse = false;
		ShowMemuse();
		return true;
	case CB_MR:
		if (memuse) {
			if (can_op_destroy)
				adv_clear_op();
			adv_set_op_val(memory);
			adv_draw_op();
			operands[cur_operand] = memory;
			can_op_destroy = false;
		}
		return true;
	default:
		return true;	
	}
}

void adv_init_calc(void)
{
	calc_active=true;
	set_term_busy(true);
	set_term_state(st_none);
	set_term_astate(ast_none);
	hide_cursor();
	scr_visible = false;
	ClearScreen(clBlack);
	
	pFont = CreateFont(_("fonts/terminal10x18.fnt"), true);
	adv_clear_calc();
	adv_draw_calc();
}

void adv_release_calc(void)
{
	DeleteFont(pFont);
	ClearScreen(clBtnFace);
	calc_active=false;
}

bool adv_process_calc(struct kbd_event *e)
{
	static int oldIdx = -1;
	int index=-1;
	bool down = (e->pressed || e->repeated);
	
	if (e->key == KEY_NONE)
		return true;
	
	if (e->key == KEY_TAB && e->pressed && !e->repeated)
		return false;
	
	if (oldIdx != -1) {
		cbtns[oldIdx].down = false;
		RedrawBtn(oldIdx);
		oldIdx = -1;
	}

	if (e->ch == '0' && e->shift_state & SHIFT_SHIFT)
		index = CB_DZERO;
	else if (e->ch == '0' && e->shift_state & SHIFT_CTRL)
		index = CB_TZERO;
	else if ((index = adv_ch_to_idx(e->ch)) == -1)
	switch (e->key) {
	case KEY_NUMLOCK:
		if (e->shift_state & SHIFT_CTRL)
			index = CB_MC;
		else	
			index=CB_CLEAR;
		break;
	case KEY_ESCAPE:
		index = CB_CE;
		break;
	case KEY_BACKSPACE:
		index=CB_BKSPACE;
		break;
	case KEY_MINUS:
		index=CB_SIGN;	
		break;
	case KEY_ENTER: 
	case KEY_NUMENTER:
		if (e->shift_state & SHIFT_CTRL)
			index = CB_MR;
		else	
			index=CB_EQUAL;
		break;
	case KEY_5:
		index = CB_PERCENT;
		break;
	default:	
		switch(e->ch) {
		case '/':
			index = CB_DIV;
			break;
		case '*':
			index = CB_MUL;
			break;
		case '+':
			if (e->shift_state & SHIFT_CTRL)
				index = CB_MPLUS;
			else	
				index = CB_PLUS;
			break;
		case '-':
			if (e->shift_state & SHIFT_CTRL)
				index = CB_MMINUS;
			else	
				index = CB_MINUS;
			break;
		case '.':
			index = CB_DOT;
			break;
		}	
	}
		
	if (index != -1) {
		if (down) adv_process_calc_key(index, e);
		cbtns[index].down = down;
		RedrawBtn(index);
		oldIdx = index;
	}
	return true;
}
