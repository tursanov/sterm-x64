/* �뢮� �� �࠭ ���ଠ樨 ���. (c) gsr, Alex 2001-2004, 2009 */

#if !defined GUI_LOG_EXPRESS_H
#define GUI_LOG_EXPRESS_H

#if defined __cplusplus
extern "C" {
#endif

#include "gui/log/generic.h"
#include "log/express.h"

extern struct log_gui_context *xlog_gui_ctx;

/* ����� ⥪�饩 ����� */
extern bool xlog_print_current(struct log_gui_context *ctx);
/* ����� ⥪�饩 ����� �� ��� */
extern bool xlog_print_aux(struct log_gui_context *ctx);
/* ����� ��������� ����ᥩ */
extern bool xlog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to);
/* ������ ����� ����஫쭮� ����� */
extern bool xlog_print_all(struct log_gui_context *ctx);

#if defined __cplusplus
}
#endif

#endif		/* GUI_LOG_EXPRESS_H */
