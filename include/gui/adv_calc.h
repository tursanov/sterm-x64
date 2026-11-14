/* Расширенный калькулятор терминала. (c) gsr, А.Попов 2000-2001, 2026 */

#if !defined GUI_ADV_CALC_H
#define GUI_ADV_CALC_H

#include "gui/calc.h"

__BEGIN_DECLS

extern void 	adv_init_calc(void);
extern void 	adv_release_calc(void);
extern bool 	adv_draw_calc(void);
extern bool 	adv_process_calc(struct kbd_event *e);

__END_DECLS

#endif		/* GUI_ADV_CALC_H */
