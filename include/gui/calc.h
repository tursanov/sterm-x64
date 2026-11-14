/* Калькулятор терминала. (c) gsr, А.Попов 2000-2001, 2026 */

#if !defined GUI_CALC_H
#define GUI_CALC_H

#include "kbd.h"

__BEGIN_DECLS

#define MAX_DIGITS	14

/* Операции калькулятора */
enum {
	op_none,
	op_plus,
	op_minus,
	op_mul,
	op_div,
	op_percent,
};

extern void init_calc(void);
extern void release_calc(void);
extern bool draw_calc(void);
extern bool process_calc(struct kbd_event *e);

__END_DECLS

#endif		/* GUI_CALC_H */
