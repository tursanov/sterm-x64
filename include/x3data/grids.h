/* Интерфейс для работы с разметками бланков для C. (c) gsr 2024 */

#if !defined X3DATA_GRIDS_H
#define X3DATA_GRIDS_H

#include "x3data/common.h"

#if defined __cplusplus
extern "C" {
#endif

extern bool check_x3_grids(const uint8_t *data, size_t len);
extern bool need_grids_update_xprn(void);
extern bool need_grids_update_kkt(void);
static inline bool need_grids_update(void)
{
	return need_grids_update_xprn() || need_grids_update_kkt();
}
extern bool sync_grids_xprn(x3_sync_callback_t cbk);
extern bool sync_grids_kkt(x3_sync_callback_t cbk);
extern void on_response_grid(void);

/* Обновление разметок в ККТ */
extern bool update_kkt_grids(void);

#if defined __cplusplus
}
#endif

#endif		/* X3DATA_GRIDS_H */
