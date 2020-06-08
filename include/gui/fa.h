/* KKT (c) gsr & alex 2000-2004, 2018 */

#if !defined GUI_FA_H
#define GUI_FA_H

#include "cfg.h"
#include "kbd.h"

extern bool	init_fa(int arg);
extern void	release_fa(void);
extern bool	draw_fa(void);
extern bool	process_fa(const struct kbd_event *e);

extern bool cashier_load();
extern bool cashier_save();
extern bool cashier_destroy();
extern bool cashier_set(const char *name, const char *post, const char *inn);
extern bool cashier_set_name(const char *name);
extern const char *cashier_get_name();
extern const char *cashier_get_post();
extern const char *cashier_get_inn();
extern const char *cashier_get_cashier();

typedef void (*update_screen_func_t)(void *arg);
bool fa_create_doc(uint16_t doc_type, const uint8_t *pattern_footer,
		size_t pattern_footer_size, update_screen_func_t update_func, 
		void *update_func_arg);

extern const char *str_tax_systems[];
extern size_t str_tax_system_count;
extern const char *str_short_kkt_modes[];
extern const char *str_kkt_modes[];
extern size_t str_kkt_mode_count;

extern int agents_load();
extern int articles_load();
extern int agents_save();
extern int articles_save();
extern int agents_destroy();
extern int articles_destroy();

#endif		/* GUI_KKT_H */
