/* Калькулятор терминала. (c) gsr, А.Попов 2000-2001, 2026 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "gui/calc.h"
#include "gui/exgdi.h"
#include "gui/options.h"
#include "gui/scr.h"
#include "sterm.h"

static double operands[2] = {0, 0};	/* операнды */
static int cur_operand = 0;		/* текущий операнд */
static int action = op_none;		/* действие */
static char op_buf[128];		/* текстовое представление операнда */
static int op_ptr = 0;			/* указатель текущей позиции op_buf */
static int op_sign = 1;			/* знак операнда */
static bool can_op_destroy = true;	/* флаг перезаписи операнда */

#define BEVEL_LEFT	0
#define BEVEL_TOP	490
#define BEVEL_WIDTH	800
#define BEVEL_HEIGHT	(600-BEVEL_TOP)

#define DEL_Y		34

#define CALC_LEFT	201	
#define CALC_TOP	1
#define CALC_WIDTH	(BEVEL_WIDTH-402)
#define CALC_HEIGHT	(DEL_Y-10)

static GCPtr pGC = NULL;

static void clear_op(void)
{
	snprintf(op_buf, sizeof(op_buf), "0");
	op_ptr = 0;
	op_sign = 1;
	can_op_destroy = true;
}

static void clear_calc(void)
{
	memset(operands, 0, sizeof(operands));
	cur_operand = 0;
	action=op_none;
	clear_op();
}

static bool get_op_val(double *v)
{
	bool ret = true;
	char *eptr = NULL;
	double vv = strtod(op_buf, &eptr);
	if (*eptr == 0)
		*v = (op_sign == 1) ? vv : -vv;
	else
		ret = false;
	return ret;
}

static bool set_op_val(double v)
{
	int n = 0;
	double w = fabs(v), vv = w;
	while (w >= 1){
		w /= 10;
		n++;
	}
	if (n > MAX_DIGITS)
		return false;
	int m = MAX_DIGITS - (n + 1);
	if (m < 0)
		m = 0;
	snprintf(op_buf, sizeof(op_buf), "%*.*f", MAX_DIGITS, m, vv);
	size_t l = strlen(op_buf);
/** calculator bug resolve **/
	for (n = 0; n < l; n++){
		if (op_buf[n] == '.')
			break;
	}
/** end of modifications **/
	for (int k = n + 1; k < l; k++){
		if (op_buf[k] != '0')
			n = k + 1;
	}
	if (n == 0)
		n = 1;
	op_buf[n] = 0;
	op_sign = (v < 0) ? -1 : 1;
	return true;
}

static bool draw_op(void)
{
	char buf[MAX_DIGITS + 2];
	char *p = buf;
	memset(p, 0, MAX_DIGITS + 2);
	if (op_sign < 0)
		*p++ = '-';
	memcpy(p, op_buf, strlen(op_buf));
	SetBrushColor(pGC, clSilver);
	FillBox(pGC, CALC_LEFT, CALC_TOP, CALC_WIDTH, CALC_HEIGHT);
	SetFont(pGC, font32x8);
	SetTextColor(pGC, clRed);
	DrawText(pGC, CALC_LEFT, CALC_TOP + 2, CALC_WIDTH, CALC_HEIGHT, buf, 0);
	return true;
}

bool draw_calc(void)
{
	return draw_op();
}

static void calc_error(void)
{
	clear_calc();
	snprintf(op_buf, sizeof(op_buf), "Ошибка !");
	draw_op();
	err_beep();
}

static bool adjust_op(void)
{
	bool ret = true;
	if (!get_op_val(operands + cur_operand)){
		calc_error();
		ret = false;
	}
	return ret;
}

static bool calculate(void)
{
	if (!get_op_val(operands + cur_operand))
		return false;
	bool ret = true;
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
			default:
				ret = false;
		}
		action = op_none;
	}
	return ret;
}

static bool is_correct_char(int ch)
{
	return isdigit(ch) || (ch == '.');
}

static bool calc_input_char(int ch)
{
	if (can_op_destroy)
		clear_op();
	if (!is_correct_char(ch) || (op_ptr >= MAX_DIGITS))
		return false;
	op_buf[op_ptr++] = ch;
	op_buf[op_ptr] = 0;
	can_op_destroy = false;
	adjust_op();
	return true;
}

static int get_action(uint8_t key)
{
	int ret = op_none;
	switch (key){
		case KEY_NUMPLUS:
			ret = op_plus;
			break;
		case KEY_NUMMINUS:
			ret = op_minus;
			break;
		case KEY_NUMMUL:
			ret = op_mul;
			break;
		case KEY_NUMSLASH:
			ret = op_div;
			break;
	}
	return ret;
}

static bool ch_op_sign(void)
{
	op_sign *= -1;
	return true;
}

static bool calc_bk_char(void)
{
	bool ret = true;
	if (op_ptr > 0){
		if (op_ptr > 1)
			op_buf[--op_ptr] = 0;
		else
			clear_op();
		adjust_op();
	}else
		ret = false;
	return ret;
}

bool process_calc(struct kbd_event *e)
{
	if ((e == NULL) || !e->pressed || (e->shift_state != 0))
		return true;
	bool ret = true;
	int act = op_none;
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
			act = get_action(e->key);
			if (cur_operand == 0)
				action = act;
			__fallthrough__;
		case KEY_NUMENTER:
			if (!calculate())
				calc_error();
			else{
				if (set_op_val(operands[0])){
					if (cur_operand == 0)
						cur_operand = 1;
					if (e->key == KEY_NUMENTER)
						cur_operand = 0;
					can_op_destroy = true;
					draw_op();
					action = act;
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
				ret = false;
	}
	return ret;
}

void init_calc(void)
{
	online = false;
	pGC = CreateGC(BEVEL_LEFT, BEVEL_TOP, BEVEL_WIDTH, BEVEL_HEIGHT);
	calc_active = true;
	set_term_busy(true);
	set_term_state(st_none);
	set_term_astate(ast_none);
	hide_cursor();
	scr_visible = false;
	SetBrushColor(pGC, clBlack);
	FillBox(pGC, 0, DEL_Y, BEVEL_WIDTH, BEVEL_HEIGHT-DEL_Y);
	SetBrushColor(pGC, clSilver);
	FillBox(pGC, 2, 0, BEVEL_WIDTH-4, DEL_Y-6);
	DrawBorder(pGC, CALC_LEFT-1, CALC_TOP-1, CALC_WIDTH+2, CALC_HEIGHT+2, 1, clGray, clWhite);
	SetFont(pGC, font_status);
	SetTextColor(pGC, clBlack);
	DrawText(pGC, 40, 0, CALC_LEFT - 1, CALC_HEIGHT, "Калькулятор:", 0);
	clear_calc();
	draw_calc();
}

void release_calc(void)
{
	SetBrushColor(pGC, clSilver);
	ClearGC(pGC, clSilver);
	DeleteGC(pGC);
	calc_active = false;
}
