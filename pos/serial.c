/* Работа с ИПТ через COM-порт. (c) gsr 2004, 2026 */

#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "pos/serial.h"
#include "cfg.h"
#include "serial.h"
#include "termlog.h"

static int serial_dev = -1;

/* Буферы данных */
static uint8_t in_data[POS_MAX_MESSAGE_LEN];
static int in_data_head;
static int in_data_len;
/* Ожидаемая длина данных первого пакета */
static int first_block_exp_len = -1;
/* Смещение текущего принимаемого блока данных */
static int cur_block_head;
/* Ожидаемая длина текущего принимаемого блока */
static int cur_block_exp_len = -1;
/* Длина принятых данных текущего блока */
static int cur_block_len;

static uint8_t out_data[POS_MAX_MESSAGE_LEN];
static int out_data_head;
static int out_data_len;

bool pos_serial_open(void)
{
	log_dbg("serial_dev = %d.", serial_dev);
	if (serial_dev != -1)
		return true;
	bool ret = false;
	char dev_name[32];
	struct serial_settings ss = {
		.csize		= CS8,
		.parity		= SERIAL_PARITY_NONE,
		.stop_bits	= SERIAL_STOPB_1,
		.control	= SERIAL_FLOW_NONE,
		.baud		= B115200
	};
//	pos_serial_close();
	snprintf(dev_name, sizeof(dev_name), "/dev/ttyS%d", cfg.bank_pos_port);
	serial_dev = serial_open(dev_name, &ss, O_RDWR);
	if (serial_dev != -1){
		in_data_len = in_data_head = 0;
		first_block_exp_len = cur_block_exp_len = -1;
		cur_block_head = cur_block_len = 0;
		out_data_len = out_data_head = 0;
		ret = true;
	}
	return ret;
}

void pos_serial_close(void)
{
	log_dbg("serial_dev = %d.", serial_dev);
	if (serial_dev != -1){
		serial_close(serial_dev);
		serial_dev = -1;
	}
}

/* Буфер передачи полностью свободен */
bool pos_serial_is_free(void)
{
	return out_data_len == 0;
}

/* Чтение ожидаемой длины входящего сообщения */
static int read_exp_len(int head)
{
	int i, l = 0;
	for (i = 0; i < sizeof(uint32_t); i++){
		l <<= 8;
		l |= in_data[(head + i + 4) % sizeof(in_data)];
	}
	l += 8;
	if (l < 0)
		l = sizeof(in_data);
	return l;
}

ssize_t pos_serial_receive(void)
{
	struct iovec v[2];
	int n = 1;
	if ((serial_dev == -1) || (in_data_len == sizeof(in_data)))
		return 0;
	int offs = (in_data_head + in_data_len) % sizeof(in_data);
	if (offs >= in_data_head){
		v[0].iov_base = in_data + offs;
		v[0].iov_len = sizeof(in_data) - offs;
		if (in_data_head > 0){
			v[1].iov_base = in_data;
			v[1].iov_len = in_data_head;
			n++;
		}
	}else{
		v[0].iov_base = in_data + offs;
		v[0].iov_len = in_data_head - offs;
	}
	ssize_t len = readv(serial_dev, v, n);
	if (len == -1){
		if (errno == EWOULDBLOCK)
			len = 0;
		else
			log_sys_err("Ошибка readv:");
	}else if (len > 0){
		size_t l = len;
		if (l > v[0].iov_len)
			l = v[0].iov_len;
		log_data_pos("ИПТ --> ТМ", v[0].iov_base, l);
		if ((n == 2) && (len > v[0].iov_len)){
			l = len - v[0].iov_len;
			log_data_pos(NULL, v[1].iov_base, l);
		}
		in_data_len += len;
		cur_block_len += len;
		while (cur_block_len >= cur_block_exp_len){
			if (cur_block_exp_len != -1){	/* получен очередной блок данных */
				cur_block_head += cur_block_exp_len;
				cur_block_head %= sizeof(in_data);
				cur_block_len -= cur_block_exp_len;
				cur_block_exp_len = -1;
				poll_ok = true;
			}else if (cur_block_len >= 8){
				cur_block_exp_len = read_exp_len(cur_block_head);
				if (first_block_exp_len == -1)
					first_block_exp_len = cur_block_exp_len;
			}else
				break;
		}
	}
	return len;
}

ssize_t pos_serial_transmit(void)
{
	struct iovec v[2];
	int n = 1;
	if ((serial_dev == -1) || (out_data_len == 0))
		return 0;
	int offs = (out_data_head + out_data_len) % sizeof(out_data);
	v[0].iov_base = out_data + out_data_head;
	if (offs > out_data_head)
		v[0].iov_len = offs - out_data_head;
	else{
		v[0].iov_len = sizeof(out_data) - out_data_head;
		if (offs > 0){
			v[1].iov_base = out_data;
			v[1].iov_len = offs;
			n++;
		}
	}
	ssize_t len = writev(serial_dev, v, n);
	if (len == -1){
		if (errno == EWOULDBLOCK)
			len = 0;
		else
			log_sys_err("Ошибка writev:");
	}else if (len > 0){
		size_t l = len;
		log_data_pos("ТМ --> ИПТ", v[0].iov_base, l);
		if ((n == 2) && (len > v[0].iov_len)){
			l = len - v[0].iov_len;
			log_data_pos(NULL, v[1].iov_base, l);
		}
		out_data_head += len;
		out_data_head %= sizeof(out_data);
		out_data_len -= len;
		if (out_data_len == 0){		/* передача завершена */
			pos_t0 = u_times();
			poll_ok = false;
		}
	}
	return len;
}

bool pos_serial_has_msg(void)
{
	return (first_block_exp_len != -1) && (in_data_len >= first_block_exp_len);
}

/* Получение сообщения от POS-эмулятора */
bool pos_serial_get_msg(struct pos_data_buf *buf)
{
	int l1, l2 = 0;
	if ((buf == NULL) || (first_block_exp_len == -1) ||
			(in_data_len < first_block_exp_len))
		return false;
	l1 = sizeof(in_data) - in_data_head;
	if (l1 > first_block_exp_len)
		l1 = first_block_exp_len;
	else
		l2 = first_block_exp_len - l1;
	memcpy(buf->un.data, in_data + in_data_head, l1);
	if (l2 > 0)
		memcpy(buf->un.data + l1,
			in_data + (in_data_head + l1) % sizeof(in_data), l2);
	buf->data_len = l1 + l2;
	buf->data_index = buf->block_start = 0;
//	log_data_pos("ИПТ --> ТМ", buf->un.data, buf->data_len);
	in_data_head += l1 + l2;
	in_data_head %= sizeof(in_data);
	in_data_len -= l1 + l2;
	first_block_exp_len = -1;
	if (in_data_len >= 8)
		first_block_exp_len = read_exp_len(in_data_head);
	return true;
}

/* Передача сообщения POS-эмулятору */
bool pos_serial_send_msg(struct pos_data_buf *buf)
{
	int offs, l1, l2 = 0;
	if ((buf == NULL) || (buf->data_len == 0) ||
			((out_data_len + buf->data_len) > sizeof(out_data)))
		return false;
	offs = (out_data_head + out_data_len) % sizeof(out_data);
	if (offs >= out_data_head)
		l1 = sizeof(out_data) - offs;
	else
		l1 = out_data_head - offs;
	if (l1 > buf->data_len)
		l1 = buf->data_len;
	else
		l2 = buf->data_len - l1;
	if (l1 > 0)
		memcpy(out_data + offs, buf->un.data, l1);
	if (l2 > 0)
		memcpy(out_data + (offs + l1) % sizeof(out_data),
				buf->un.data + l1, l2);
	out_data_len += l1 + l2;
//	log_data_pos("ТМ --> ИПТ", buf->un.data, buf->data_len);
	pos_t0 = u_times();
	poll_ok = false;
	return true;
}
