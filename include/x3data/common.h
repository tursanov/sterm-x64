/* Общие определения для всех файлов. (c) gsr, 2024 */

#if !defined X3DATA_COMMON_H
#define X3DATA_COMMON_H

#include "sysdefs.h"

#if defined __cplusplus
extern "C" {
#endif

typedef void (*x3_sync_callback_t)(bool done, const char *message);

#if defined __cplusplus
}
#endif

#endif		/* X3DATA_COMMON_H */
