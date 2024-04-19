/*
	sterm calculator
	(c) gsr 2000
*/

#ifndef ADV_CALC_H
#define ADV_CALC_H

#if defined __cplusplus
extern "C" {
#endif

#include "gui/calc.h"

//#define MAX_DIGITS	14	/* ���ᨬ��쭮� �᫮ ࠧ�冷� ���࠭�� */

/* ����樨 �������� */
/*enum calc_operation{	op_none=0,
			op_plus,
			op_minus,
			op_mul,
			op_div,
			op_percent};*/

extern void 	adv_init_calc(void);
extern void 	adv_release_calc(void);
extern bool 	adv_draw_calc(void);
extern bool 	adv_process_calc(struct kbd_event *e);

extern void 	adv_clear_calc(void);	/* ���� �������� � ��砫쭮� ���ﭨ� */
extern void 	adv_clear_op(void);		/* ���� ⥪�饣� ���࠭�� */
extern bool	adv_get_op_val(double *v);
extern bool	adv_set_op_val(double v);
extern void	adv_calc_error(void);
extern bool	adv_calculate(void);	/* ���᫥��� ⥪�饣� १���� */
extern bool	adv_calc_right_char(int ch);
extern bool	adv_calc_right_op(int ch);
extern bool	adv_calc_input_char(int ch);/* ���� ��।���� ᨬ���� */
extern int	adv_get_action(int index);
extern bool	adv_calc_input_op(int op);
extern bool	adv_ch_op_sign(void);	/* ��������� ����� */
extern bool	adv_calc_bk_char(void);	/* �������� �।��饣� ᨬ���� */
extern bool	adv_adjust_op(void);
extern bool	adv_draw_op(void);	/* �뢮� ���࠭�� �� �࠭ */

#if defined __cplusplus
}
#endif

#endif		/* ADV_CALC_H */
