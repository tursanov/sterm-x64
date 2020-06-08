/* Общие функции для работы с принтерами. (c) gsr 2009 */

#if !defined PRN_GENERIC_H
#define PRN_GENERIC_H

#include "blimits.h"
#include "sysdefs.h"

/* Буфер печати */
extern uint8_t prn_buf[PRN_BUF_LEN];
/* Длина данных в буфере печати */
extern int prn_buf_len;

extern bool prn_number_correct(uint8_t *number);
extern bool prn_numbers_ok(void);

#endif		/* PRN_GENERIC_H */
