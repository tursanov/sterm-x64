/* Работа с ККТ по виртуальному COM-порту. (c) gsr 2018 */

#if !defined KKT_IO_H
#define KKT_IO_H

#include "devinfo.h"

#define RX_BUF_LEN	4096
#define TX_BUF_LEN	262144

extern void kkt_io_init(const struct dev_info *di);
extern void kkt_io_release(void);

extern bool kkt_open_dev(void);
extern bool kkt_open_dev_if_need(void);
extern void kkt_close_dev(void);

extern void kkt_begin_batch_mode(void);
extern void kkt_end_batch_mode(void);

extern bool kkt_on_com_error(uint32_t timeout);

extern uint8_t kkt_tx[TX_BUF_LEN];
extern size_t kkt_tx_len;

extern void kkt_reset_tx(void);

extern uint8_t kkt_rx[RX_BUF_LEN];
extern size_t kkt_rx_len;
extern size_t kkt_rx_exp_len;

extern void kkt_reset_rx(void);

extern ssize_t kkt_io_write(uint32_t *timeout);
extern ssize_t kkt_io_read(size_t len, uint32_t *timeout);

#endif		/* KKT_IO_H */
