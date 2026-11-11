/* Вывод на экран справочной информации. (c) gsr 2000, 2026 */

#if !defined GUI_HELP_H
#define GUI_HELP_H

#include "kbd.h"
#include "gui/scr.h"

__BEGIN_DECLS

extern void init_help(const char *fname);
extern void release_help(void);
extern bool draw_help(void);
extern bool process_help(struct kbd_event *e);

__END_DECLS

#endif		/* GUI_HELP_H */
