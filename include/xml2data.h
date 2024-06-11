/* Обработка XML внутри команды Ар2 R. (c) gsr 2024 */

#if !defined XML2DATA_H
#define XML2DATA_H

#include "sysdefs.h"

#if defined __cplusplus
extern "C" {
#endif

/* Используется как для проверки синтаксиса, так и для обработки команды Ар2 R */
extern uint8_t *check_xml(uint8_t *p, size_t l, int dst, int *ecode,
	uint8_t *data_buf, size_t *data_buf_len, uint8_t *scr_buf, size_t *scr_buf_len);

#if defined __cplusplus
}
#endif

#endif		/* XML2DATA_H */
