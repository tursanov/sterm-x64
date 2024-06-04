/* Интерфейс для работы с пиктограммами для C. (c) gsr, 2024 */

#if !defined X3DATA_ICONS_H
#define X3DATA_ICONS_H

#include "x3data/common.h"

#if defined __cplusplus
extern "C" {
#endif

extern bool check_x3_icons(const uint8_t *data, size_t len);
extern bool need_icons_update_xprn(void);
extern bool need_icons_update_kkt(void);
static inline bool need_icons_update(void)
{
	return need_icons_update_xprn() || need_icons_update_kkt();
}
extern bool sync_icons_xprn(x3_sync_callback_t cbk);
extern bool sync_icons_kkt(x3_sync_callback_t cbk);
extern void on_response_icon(void);

/* Обновление пиктограмм в ККТ */
extern bool update_kkt_icons(void);

#if defined __cplusplus
}
#endif

#endif		/* X3DATA_ICONS_H */
