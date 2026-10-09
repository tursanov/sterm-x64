/* Работа с ИПТ через COM-порт. (c) gsr 2004, 2026 */

#if !defined POS_SERIAL_H
#define POS_SERIAL_H

#if defined __cplusplus
extern "C" {
#endif

#include "pos/pos.h"

extern bool pos_serial_open(void);
extern void pos_serial_close(void);
extern bool pos_serial_is_free(void);
extern ssize_t  pos_serial_receive(void);
extern ssize_t  pos_serial_transmit(void);
extern bool pos_serial_has_msg(void);
extern bool pos_serial_get_msg(struct pos_data_buf *buf);
extern bool pos_serial_send_msg(struct pos_data_buf *buf);

/*extern bool read_pos_data(void);*/

#if defined __cplusplus
}
#endif

#endif		/* POS_SERIAL_H */
