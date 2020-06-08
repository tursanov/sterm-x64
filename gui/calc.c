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
#include "sterm.h"
#include "gui/scr.h"
#include "gui/options.h"
#include "gui/calc.h"
#include "gui/exgdi.h"

double operands[2]={0,0};		/* операнды */
int cur_operand=0;			/* текущий операнд */
int action = op_none;			/* действие */
char op_buf[128];			/* текстовое представление операнда */
int op_ptr;				/* указатель текущей позиции op_buf */
int op_sign;				/* знак операнда */
bool can_op_destroy;			/* флаг перезаписи операнда */
int op_x0;				/* начало операнда (вместе со знаком) */

#define BEVEL_LEFT		0
#define BEVEL_TOP		490
#define BEVEL_WIDTH		800
#define BEVEL_HEIGHT	(600-BEVEL_TOP)

#define DEL_Y			34

#define CALC_LEFT	201	
#define CALC_TOP	1
#define CALC_WIDTH	(BEVEL_WIDTH-402)
#define CALC_HEIGHT	(DEL_Y-10)

GCPtr pGC = NULL;

void init_calc(void)
{
	pGC = CreateGC(BEVEL_LEFT, BEVEL_TOP, BEVEL_WIDTH, BEVEL_HEIGHT);

	calc_active=true;
	set_term_busy(true);
	set_term_state(st_none);
	set_term_astate(ast_none);
	hide_cursor();
	scr_visible = false;
	
	SetBrushColor(pGC, clBlack);
	FillBox(pGC, 0, DEL_Y, BEVEL_WIDTH, BEVEL_HEIGHT-DEL_Y);
	SetBrushColor(pGC, clSilver);
	FillBox(pGC, 2, 0, BEVEL_WIDTH-4, DEL_Y-6);
	DrawBorder(pGC, CALC_LEFT-1, CALC_TOP-1, CALC_WIDTH+2, CALC_HEIGHT+2,
		1, clGray, clWhite);
	
	SetFont(pGC, font_status);
	SetTextColor(pGC, clBlack);
	DrawText(pGC, 40, 0, CALC_LEFT-1, CALC_HEIGHT, "Калькулятор:", 0);
		
	clear_calc();
	draw_calc();
}

void release_calc(void)
{
	SetBrushColor(pGC, clSilver);
	ClearGC(pGC, clSilver);
	DeleteGC(pGC);
	
	calc_active=false;
}

void clear_calc(void)	//Сброс калькулятора в начальное состояние
{
	memset(operands,0,2*sizeof(double));
	cur_operand=0;
	action=op_none;
	clear_op();
}

void clear_op(void)	//Сброс текущего операнда
{
	sprintf(op_buf,"0");
	op_ptr=0;
	op_sign=1;
	can_op_destroy=true;
}

bool get_op_val(double *v)
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

bool set_op_val(double v)
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
/** calculator bug resolve **/
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

void calc_error(void)
{
	clear_calc();
	sprintf(op_buf,"Ошибка !");
	draw_op();
	err_beep();
}

bool calculate(void)	//Вычисление текущего результата
{
	if (!get_op_val(&operands[cur_operand]))
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
		}
		action=op_none;
	}
	return true;
}

bool calc_right_char(int ch)
{
	return isdigit(ch) || (ch == '.');
}

bool calc_right_op(int ch)
{
	return	(ch == '+') ||
		(ch == '-') ||
		(ch == '*') ||
		(ch == '/');
}

bool calc_input_char(int ch)	//Ввод очередного символа
{
	if (can_op_destroy)
		clear_op();
	if (!calc_right_char(ch) || (op_ptr >= MAX_DIGITS))
		return false;
	op_buf[op_ptr++]=ch;
	op_buf[op_ptr]=0;
	can_op_destroy=false;
	adjust_op();
	return true;
}

int get_action(uint16_t key)
{
	switch (key){
		case KEY_NUMPLUS:
			return op_plus;
		case KEY_NUMMINUS:
			return op_minus;
		case KEY_NUMMUL:
			return op_mul;
		case KEY_NUMSLASH:
			return op_div;
		default:
			return op_none;
	}
}

bool calc_input_op(int op)
{
	action=op;
	return can_op_destroy=true;
}

bool ch_op_sign(void)		//Изменение знака
{
	op_sign *= -1;
	return true;
}

bool calc_bk_char(void)		//Удаление предыдущего символа
{
	if (op_ptr > 0){
		if (op_ptr > 1)
			op_buf[--op_ptr]=0;
		else
			clear_op();
		adjust_op();
		return true;
	}else
		return false;
}

bool adjust_op(void)
{
	if (get_op_val(&operands[cur_operand]))
		return true;
	else{
		calc_error();
		return false;
	}
}

bool draw_op(void)		//Вывод операнда на экран
{
	char *buf = malloc(MAX_DIGITS+2);
	char *p = buf;
	
	memset(p, 0, MAX_DIGITS+2);
	if (op_sign < 0)
		*p++ = '-';
	memcpy(p, op_buf, strlen(op_buf));
	
	SetBrushColor(pGC, clSilver);
	FillBox(pGC, CALC_LEFT, CALC_TOP, CALC_WIDTH, CALC_HEIGHT);
	SetFont(pGC, font32x8);
	SetTextColor(pGC, clRed);
	DrawText(pGC, CALC_LEFT, CALC_TOP+2, CALC_WIDTH, CALC_HEIGHT,	buf, 0);
	free(buf);
	
	return true;
}

bool draw_calc(void)		//Рисование калькулятора
{
	return draw_op();
}

bool process_calc(struct kbd_event *e)
{
	int act = op_none;
	if ((e == NULL) || !e->pressed)
		return true;
	switch (e->key){
		case KEY_NUMDEL:
		case KEY_NUMINS:
		case KEY_NUMEND:
		case KEY_NUMDOWN:
		case KEY_NUMPGDN:
		case KEY_NUMLEFT:
		case KEY_NUMDOT:
		case KEY_NUMRIGHT:
		case KEY_NUMHOME:
		case KEY_NUMUP:
		case KEY_NUMPGUP:
			if (calc_input_char(e->ch))
				draw_op();
			break;
		case KEY_NUMPLUS:
		case KEY_NUMMINUS:
		case KEY_NUMMUL:
		case KEY_NUMSLASH:
			act=get_action(e->key);
			if (cur_operand == 0)
				action=act;
		case KEY_NUMENTER:
			if (!calculate())
				calc_error();
			else{
				if (set_op_val(operands[0])){
					if (cur_operand == 0)
						cur_operand=1;
					if (e->key == KEY_NUMENTER)
						cur_operand=0;
					can_op_destroy=true;
					draw_op();
					action=act;
				}else
					calc_error();
			}
			break;
		case KEY_MINUS:
			ch_op_sign();
			adjust_op();
			draw_op();
			break;
		case KEY_NUMLOCK:
			clear_calc();
			draw_op();
			break;
		case KEY_ESCAPE:
			clear_op();
			draw_op();
			break;
		case KEY_BACKSPACE:
			if (calc_bk_char())
				draw_op();
			break;
		case KEY_TAB:
			if (!e->repeated)
				return false;
	}
	return true;
}

