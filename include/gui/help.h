/* Вывод на экран справочной информации. (c) gsr 2000, 2026 */

#if !defined GUI_HELP_H
#define GUI_HELP_H

#if defined __cplusplus
extern "C" {
#endif

#include "kbd.h"
#include "gui/scr.h"

extern void init_help(const char *fname);
extern void release_help(void);
extern bool draw_help(void);
extern bool process_help(struct kbd_event *e);

#if defined __cplusplus
}
#endif

#endif		/* GUI_HELP_H */
