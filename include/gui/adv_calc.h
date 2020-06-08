/*
	sterm calculator
	(c) gsr 2000
*/

#ifndef ADV_CALC_H
#define ADV_CALC_H

#include "gui/calc.h"

//#define MAX_DIGITS	14	/* Максимальное число разрядов операнда */

/* Операции калькулятора */
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

extern void 	adv_clear_calc(void);	/* Сброс калькулятора в начальное состояние */
extern void 	adv_clear_op(void);		/* Сброс текущего операнда */
extern bool	adv_get_op_val(double *v);
extern bool	adv_set_op_val(double v);
extern void	adv_calc_error(void);
extern bool	adv_calculate(void);	/* Вычисление текущего результата */
extern bool	adv_calc_right_char(int ch);
extern bool	adv_calc_right_op(int ch);
extern bool	adv_calc_input_char(int ch);/* Ввод очередного символа */
extern int	adv_get_action(int index);
extern bool	adv_calc_input_op(int op);
extern bool	adv_ch_op_sign(void);	/* Изменение знака */
extern bool	adv_calc_bk_char(void);	/* Удаление предыдущего символа */
extern bool	adv_adjust_op(void);
extern bool	adv_draw_op(void);	/* Вывод операнда на экран */

#endif		/* ADV_CALC_H */
