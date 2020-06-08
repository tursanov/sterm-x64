/* ����७�� �������� �ନ����. (c) gsr, �.����� 2000-2001 */

#if !defined CALC_H
#define CALC_H

#include "kbd.h"

#define MAX_DIGITS	14	/* ���ᨬ��쭮� �᫮ ࠧ�冷� ���࠭�� */

/* ����樨 �������� */
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

extern void 	make_calc_geom(void);	/* ������� ������ਨ */
extern void 	clear_calc(void);	/* ���� �������� � ��砫쭮� ���ﭨ� */
extern void 	clear_op(void);		/* ���� ⥪�饣� ���࠭�� */
extern bool	get_op_val(double *v);
extern bool	set_op_val(double v);
extern void	calc_error(void);
extern bool	calculate(void);	/* ���᫥��� ⥪�饣� १���� */
extern bool	calc_right_char(int ch);
extern bool	calc_right_op(int ch);
extern bool	calc_input_char(int ch);/* ���� ��।���� ᨬ���� */
extern int 	get_action(uint16_t key);
extern bool 	calc_input_op(int op);
extern bool	ch_op_sign(void);	/* ��������� ����� */
extern bool	calc_bk_char(void);	/* �������� �।��饣� ᨬ���� */
extern bool	adjust_op(void);
extern bool	draw_op(void);	/* �뢮� ���࠭�� �� �࠭ */

#endif		/* CALC_H */
