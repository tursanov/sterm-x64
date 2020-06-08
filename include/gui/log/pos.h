/* Вывод на экран информации КЛ2. (c) gsr, Alex 2001-2004 */

#if !defined GUI_LOG_POS_H
#define GUI_LOG_POS_H

#include "gui/log/generic.h"
#include "gui/scr.h"

extern struct log_gui_context *plog_gui_ctx;

/* Печать текущей записи */
extern bool plog_print_current(struct log_gui_context *ctx);
/* Печать диапазона записей */
extern bool plog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to);
/* Полная печать контрольной ленты */
extern bool plog_print_all(struct log_gui_context *ctx);

#endif		/* GUI_LOG_POS_H */
