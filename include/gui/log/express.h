/* Вывод на экран информации ЦКЛ. (c) gsr, Alex 2001-2004, 2009 */

#if !defined GUI_LOG_EXPRESS_H
#define GUI_LOG_EXPRESS_H

#include "gui/log/generic.h"
#include "log/express.h"

extern struct log_gui_context *xlog_gui_ctx;

/* Печать текущей записи */
extern bool xlog_print_current(struct log_gui_context *ctx);
/* Печать текущей записи на ДПУ */
extern bool xlog_print_aux(struct log_gui_context *ctx);
/* Печать диапазона записей */
extern bool xlog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to);
/* Полная печать контрольной ленты */
extern bool xlog_print_all(struct log_gui_context *ctx);

#endif		/* GUI_LOG_EXPRESS_H */
