/* Работа с POS-эмулятором через COM-порт. (c) gsr 2004 */

#if !defined POS_SERIAL_H
#define POS_SERIAL_H

#include "pos/pos.h"

extern bool pos_serial_open(void);
extern void pos_serial_close(void);
extern bool pos_serial_is_free(void);
extern int  pos_serial_receive(void);
extern int  pos_serial_transmit(void);
extern bool pos_serial_peek_msg(void);
extern bool pos_serial_get_msg(struct pos_data_buf *buf);
extern bool pos_serial_send_msg(struct pos_data_buf *buf);

/*extern bool read_pos_data(void);*/

#endif		/* POS_SERIAL_H */
