/* �뢮� �� �࠭ ���ଠ樨 ��3. (c) gsr 2018 */

#if !defined GUI_LOG_KKT_H
#define GUI_LOG_KKT_H

#include "gui/log/generic.h"
#include "gui/scr.h"

extern struct log_gui_context *klog_gui_ctx;

/* ����� ⥪�饩 ����� */
extern bool klog_print_current(struct log_gui_context *ctx);
/* ����� ��������� ����ᥩ */
extern bool klog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to);
/* ������ ����� ����஫쭮� ����� */
extern bool klog_print_all(struct log_gui_context *ctx);

#endif		/* GUI_LOG_KKT_H */
