/* �뢮� �� �࠭ ���ଠ樨 ���. (c) gsr, Alex 2001-2004, 2009 */

#if !defined GUI_LOG_LOCAL_H
#define GUI_LOG_LOCAL_H

#include "gui/log/generic.h"
#include "log/local.h"

extern struct log_gui_context *llog_gui_ctx;

/* ����� ⥪�饩 ����� */
extern bool llog_print_current(struct log_gui_context *ctx);
/* ����� ⥪�饩 ����� �� ��� */
extern bool llog_print_xprn(struct log_gui_context *ctx);
/* ����� ⥪�饩 ����� �� ��� */
extern bool llog_print_aux(struct log_gui_context *ctx);
/* ����� ��������� ����ᥩ */
extern bool llog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to);
/* ������ ����� ����஫쭮� ����� */
extern bool llog_print_all(struct log_gui_context *ctx);

#endif		/* GUI_LOG_LOCAL_H */
