/* Вывод на экран информации КЛ3. (c) gsr 2018 */

#if !defined GUI_LOG_KKT_H
#define GUI_LOG_KKT_H

#include "gui/log/generic.h"
#include "gui/scr.h"

extern struct log_gui_context *klog_gui_ctx;

/* Печать текущей записи */
extern bool klog_print_current(struct log_gui_context *ctx);
/* Печать диапазона записей */
extern bool klog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to);
/* Полная печать контрольной ленты */
extern bool klog_print_all(struct log_gui_context *ctx);

#endif		/* GUI_LOG_KKT_H */
