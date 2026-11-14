/* Интерфейс для работы с разметками бланков для C. (c) gsr 2024 */

#if !defined X3DATA_GRIDS_H
#define X3DATA_GRIDS_H

#include "x3data/common.h"

__BEGIN_DECLS

extern bool check_x3_grids(const uint8_t *data, size_t len);
extern bool need_grids_update_sprn(void);
extern bool need_grids_update_kkt(void);
static inline bool need_grids_update(void)
{
	return need_grids_update_sprn() || need_grids_update_kkt();
}
extern bool sync_grids_sprn(void);
extern bool sync_grids_kkt(void);
extern void on_response_grid(void);

/* Обновление разметок бланков в БПУ */
extern bool update_sprn_grids(void);
/* Обновление разметок бланков в ККТ */
extern bool update_kkt_grids(void);

__END_DECLS

#endif		/* X3DATA_GRIDS_H */
