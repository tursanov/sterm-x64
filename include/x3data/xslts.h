/* Интерфейс для работы с таблицами XSLT для C. (c) gsr, 2024 */

#if !defined X3DATA_XSLTS_H
#define X3DATA_XSLTS_H

#include "x3data/common.h"

#if defined __cplusplus
extern "C" {
#endif

extern bool check_x3_xslt(const uint8_t *data, size_t len);
extern bool need_xslt_update(void);
extern bool sync_xslt(x3_sync_callback_t cbk);
extern void on_response_xslt(void);

#if defined __cplusplus
}
#endif

#endif		/* X3DATA_XSLTS_H */
