/* Интерфейс для работы с шаблонами печати для C. (c) gsr 2024 */

#if !defined X3DATA_PATTERNS_H
#define X3DATA_PATTERNS_H

#include "x3data/common.h"

__BEGIN_DECLS

extern const char *get_local_patterns_ver(void);
extern time_t jul_date_to_unix_date(const char *jul_date);
extern bool check_x3_kkt_patterns(const uint8_t *data, size_t len);
extern bool need_patterns_update(void);
extern bool sync_patterns(void);
extern void on_response_patterns(void);

__END_DECLS

#endif		/* X3DATA_PATTERNS_H */
