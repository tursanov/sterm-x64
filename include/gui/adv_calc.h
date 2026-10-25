/* Расширенный калькулятор терминала. (c) gsr, А.Попов 2000-2001, 2026 */

#if !defined GUI_ADV_CALC_H
#define GUI_ADV_CALC_H

#if defined __cplusplus
extern "C" {
#endif

#include "gui/calc.h"

extern void 	adv_init_calc(void);
extern void 	adv_release_calc(void);
extern bool 	adv_draw_calc(void);
extern bool 	adv_process_calc(struct kbd_event *e);

#if defined __cplusplus
}
#endif

#endif		/* GUI_ADV_CALC_H */
