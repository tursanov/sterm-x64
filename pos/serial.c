/* Работа с POS-эмулятором через COM-порт. (c) gsr 2004 */

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

#if 0
static void serial_dump(void)
{
	struct timeb tp;
	struct tm *tm;
	int i;
	ftime(&tp);
	tm = localtime(&tp.time);
	printf("%.2d:%.2d:%.2d.%.3hu\n", tm->tm_hour, tm->tm_min, tm->tm_sec,
			tp.millitm);
	for (i = 0; i < in_data_len; i++){
		if (((i & 0x07) == 0) && i)
			printf("%c", (i & 0x0f) ? ' ' : '\n');
		printf("%.2hx ", (uint16_t)in_data[(in_data_head + i) % sizeof(in_data)]);
	}
	printf("\n");
}
#endif

#if 0
/* Чтение данных в буфер приема из файла pos.dat */
#define POS_DATA_FILE		"pos.dat"
static int read_pos_data(void)
{
	int fd, ret;
	struct stat st;
	if (stat(POS_DATA_FILE, &st) == -1){
		fprintf(stderr, "Ошибка получения информации о " POS_DATA_FILE ": %s.\n",
			strerror(errno));
		return -1;
	}else if (st.st_size > POS_MAX_MESSAGE_LEN){
		fprintf(stderr, "Размер файла " POS_DATA_FILE " не должен превышать %d байт.\n",
				POS_MAX_MESSAGE_LEN);
		return -1;
	}
	fd = open(POS_DATA_FILE, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "Ошибка открытия " POS_DATA_FILE " для чтения: %s.\n",
			strerror(errno));
		return -1;
	}
/*	in_data_head = 0;*/
	ret = read(fd, in_data, st.st_size);
	close(fd);
	if (ret != st.st_size){
		fprintf(stderr, "Ошибка чтения из " POS_DATA_FILE ": %s.\n",
			strerror(errno));
		return -1;
	}
/*	first_block_exp_len = in_data_len;*/
	return ret;
}
#endif

bool pos_serial_open(void)
{
	char dev_name[32];
	struct serial_settings ss = {
		.csize		= CS8,
		.parity		= SERIAL_PARITY_NONE,
		.stop_bits	= SERIAL_STOPB_1,
		.control	= SERIAL_FLOW_NONE,
		.baud		= B115200
	};
	snprintf(dev_name, sizeof(dev_name), "/dev/ttyS%d", cfg.bank_pos_port);
	serial_dev = serial_open(dev_name, &ss, O_RDWR);
	if (serial_dev != -1){
		in_data_len = in_data_head = 0;
		first_block_exp_len = cur_block_exp_len = -1;
		cur_block_head = cur_block_len = 0;
		out_data_len = out_data_head = 0;
		return true;
	}else
		return false;
}

void pos_serial_close(void)
{
	if (serial_dev != -1){
		serial_close(serial_dev);
		serial_dev = -1;
	}
}

#if 0
/* Определение размера свободной части буфера передачи */
int pos_serial_get_free_size(void)
{
	return sizeof(out_data) - out_data_len;
}
#endif

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
#if defined __POS_DEBUG__
	printf("%s: (%d): %d\n", __func__, head, l);
#endif
	if (l < 0){
#if defined __POS_DEBUG__
		printf("%s: корректируем длину до %d\n", __func__, sizeof(in_data));
#endif
		l = sizeof(in_data);
	}
	return l;
}

int pos_serial_receive(void)
{
	struct iovec v[2];
	int n = 1, offs;
	if ((serial_dev == -1) || (in_data_len == sizeof(in_data)))
		return 0;
	offs = (in_data_head + in_data_len) % sizeof(in_data);
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
	n = readv(serial_dev, v, n);
	if (n == -1)
		return 0;
	else if (n > 0){
#if defined __POS_DEBUG__
		printf("%s: %d bytes received\n", __func__, n);
#endif
		in_data_len += n;
		cur_block_len += n;
		while (cur_block_len >= cur_block_exp_len){
			if (cur_block_exp_len != -1){	/* получен очередной блок данных */
				cur_block_head += cur_block_exp_len;
				cur_block_head %= sizeof(in_data);
				cur_block_len -= cur_block_exp_len;
				cur_block_exp_len = -1;
/*				pos_t0 = u_times();*/
				poll_ok = true;
			}else if (cur_block_len >= 8){
				cur_block_exp_len = read_exp_len(cur_block_head);
				if (first_block_exp_len == -1)
					first_block_exp_len = cur_block_exp_len;
			}else
				break;
		}
/*		serial_dump();*/
	}
	return n;
}

int pos_serial_transmit(void)
{
	struct iovec v[2];
	int n = 1, offs;
	if ((serial_dev == -1) || (out_data_len == 0))
		return 0;
	offs = (out_data_head + out_data_len) % sizeof(out_data);
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
	n = writev(serial_dev, v, n);
	if (n == -1)
		return 0;
	else if (n > 0){
#if defined __POS_DEBUG__
		printf("%s: %d bytes sent\n", __func__, n);
#endif
		out_data_head += n;
		out_data_head %= sizeof(out_data);
		out_data_len -= n;
		if (out_data_len == 0){		/* передача завершена */
			pos_t0 = u_times();
			poll_ok = false;
		}
	}
	return n;
}

bool pos_serial_peek_msg(void)
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
#if defined __POS_DEBUG__
	printf("%s: in_data_head = %d; in_data_len = %d; l1 = %d; l2 = %d\n",
			__func__, in_data_head, in_data_len, l1, l2);
#endif
	memcpy(buf->un.data, in_data + in_data_head, l1);
	if (l2 > 0)
		memcpy(buf->un.data + l1,
			in_data + (in_data_head + l1) % sizeof(in_data), l2);
	buf->data_len = l1 + l2;
	buf->data_index = buf->block_start = 0;
	pos_dump(buf);
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
#if defined __POS_DEBUG__
	printf("%s: offs = %d; out_data_head = %d; out_data_len = %d\n",
			__func__, offs, out_data_head, out_data_len);
#endif
	pos_dump(buf);
	pos_t0 = u_times();
	poll_ok = false;
	return true;
}
