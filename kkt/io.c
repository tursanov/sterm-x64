/* Работа с ККТ по виртуальному COM-порту. (c) gsr 2018 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "kkt/cmd.h"
#include "kkt/io.h"
#include "kkt/parser.h"
#include "serial.h"

static int kkt_dev = -1;

static const struct dev_info *kkt_di = NULL;

static bool batch_mode = false;

void kkt_close_dev(void)
{
	if (!batch_mode && (kkt_dev != -1)){
		serial_close(kkt_dev);
		kkt_dev = -1;
	}
}

void kkt_begin_batch_mode(void)
{
	batch_mode = true;
}

void kkt_end_batch_mode(void)
{
	batch_mode = false;
	kkt_close_dev();
}

bool kkt_open_dev(void)
{
	bool ret = false;
	if (kkt_di != NULL){
		if (kkt_dev != -1)
			kkt_close_dev();
		kkt_dev = serial_open(kkt_di->ttyS_name, &kkt_di->ss, O_RDWR);
		ret = kkt_dev != -1;
	}
	if (!ret)
		kkt_status = KKT_STATUS_COM_ERROR;
	return ret;
}

bool kkt_open_dev_if_need(void)
{
	bool ret = true;
	if (kkt_dev == -1)
		ret = kkt_open_dev();
	return ret;
}

bool kkt_on_com_error(uint32_t timeout)
{
	if (timeout == 0)
		kkt_status = KKT_STATUS_OP_TIMEOUT;
	else
		kkt_status = KKT_STATUS_COM_ERROR;
	return false;
}

void kkt_io_init(const struct dev_info *di)
{
	kkt_di = di;
	batch_mode = false;
}

void kkt_io_release(void)
{
	kkt_close_dev();
}

uint8_t kkt_tx[TX_BUF_LEN];
size_t kkt_tx_len = 0;

void kkt_reset_tx(void)
{
	kkt_tx_len = 0;
	tx_prefix = tx_cmd = 0;
}

uint8_t kkt_rx[RX_BUF_LEN];
size_t kkt_rx_len = 0;
size_t kkt_rx_exp_len = 0;

void kkt_reset_rx(void)
{
	kkt_rx_len = kkt_rx_exp_len = 0;
	kkt_status = KKT_STATUS_OK;
}

ssize_t kkt_io_write(uint32_t *timeout)
{
	ssize_t ret = serial_write(kkt_dev, kkt_tx, kkt_tx_len, timeout);
/*	if (ret > 0)
		write(STDOUT_FILENO, kkt_tx, ret);*/
	return ret;
}

ssize_t kkt_io_read(size_t len, uint32_t *timeout)
{
	ssize_t ret = serial_read(kkt_dev, kkt_rx + kkt_rx_len, len, timeout);
/*	if (ret > 0)
		write(STDOUT_FILENO, kkt_rx + kkt_rx_len, ret);*/
	return ret;
}
