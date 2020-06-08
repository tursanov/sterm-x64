/* Настройка параметров терминала. (c) gsr & alex 2000-2004, 2018-2019 */

#if !defined GUI_OPTIONS_H
#define GUI_OPTIONS_H

#include "cfg.h"
#include "kbd.h"

/* Параметры ППУ были успешно прочитаны */
extern bool	lprn_params_read;

extern int	optn_cm;

extern void	init_options(void);
extern void	release_options(bool need_clear);
extern void	adjust_kkt_brightness(uint32_t n);
extern bool	optn_get_items(struct term_cfg *p);
extern bool	draw_options(void);
extern bool	process_options(const struct kbd_event *e);

#endif		/* GUI_OPTIONS_H */
