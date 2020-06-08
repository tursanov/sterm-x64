/* Запись отладочной информации при работе с контрольными лентами. (c) gsr, 2019 */

#if !defined LOG_LOGDBG_H
#define LOG_LOGDBG_H

#include "sysdefs.h"

extern bool logdbg(const char *fmt, ...) __attribute__((format (printf, 1, 2)));

#endif		/* LOG_LOGDBG_H */
