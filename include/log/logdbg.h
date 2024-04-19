/* Запись отладочной информации при работе с контрольными лентами. (c) gsr, 2019 */

#if !defined LOG_LOGDBG_H
#define LOG_LOGDBG_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

extern bool logdbg(const char *fmt, ...) __attribute__((format (printf, 1, 2)));

#if defined __cplusplus
}
#endif

#endif		/* LOG_LOGDBG_H */
