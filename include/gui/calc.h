/* Расширенный калькулятор терминала. (c) gsr, А.Попов 2000-2001 */

#if !defined CALC_H
#define CALC_H

#include "kbd.h"

#define MAX_DIGITS	14	/* Максимальное число разрядов операнда */

/* Операции калькулятора */
enum {
	op_none,
	op_plus,
	op_minus,
	op_mul,
	op_div,
	op_percent,
};

extern void 	init_calc(void);
extern void 	release_calc(void);
extern bool 	draw_calc(void);
extern bool 	process_calc(struct kbd_event *e);

extern void 	make_calc_geom(void);	/* Задание геометрии */
extern void 	clear_calc(void);	/* Сброс калькулятора в начальное состояние */
extern void 	clear_op(void);		/* Сброс текущего операнда */
extern bool	get_op_val(double *v);
extern bool	set_op_val(double v);
extern void	calc_error(void);
extern bool	calculate(void);	/* Вычисление текущего результата */
extern bool	calc_right_char(int ch);
extern bool	calc_right_op(int ch);
extern bool	calc_input_char(int ch);/* Ввод очередного символа */
extern int 	get_action(uint16_t key);
extern bool 	calc_input_op(int op);
extern bool	ch_op_sign(void);	/* Изменение знака */
extern bool	calc_bk_char(void);	/* Удаление предыдущего символа */
extern bool	adjust_op(void);
extern bool	draw_op(void);	/* Вывод операнда на экран */

#endif		/* CALC_H */
