/* Работа с потоком TCP/IP POS-эмулятора. (c) A.Popov, gsr 2004 */

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pos/error.h"
#include "pos/tcp.h"
#include "cfg.h"

struct data_unit {
	struct data_unit *next;
	uint8_t *data;
	int unit_len;
	int data_head;
	int data_len;
};

static struct data_unit *new_data_unit(int len)
{
	struct data_unit *p = calloc(1, sizeof(*p));
	if (p != NULL){
		p->data = malloc(len);
		if (p->data != NULL){
			p->unit_len = len;
			return p;
		}else
			free(p);
	}
	return NULL;
}

static void free_data_unit(struct data_unit *p)
{
	if (p != NULL){
		if (p->data != NULL)
			free(p->data);
		free(p);
	}
}

static void reset_data_unit(struct data_unit *p)
{
	if (p != NULL)
		p->data_head = p->data_len = 0;
}

/* Чтение данных */
static int read_unit_data(struct data_unit *p, uint8_t *data, int len)
{
	int l, l1, l2 = 0;
	if ((p == NULL) || (p->data == NULL) ||
			(data == NULL) || (len <= 0) || (p->data_len == 0))
		return 0;
	if ((p->data_head + p->data_len) > p->unit_len)
		l1 = p->unit_len - p->data_head;
	else
		l1 = p->data_len;
	if (l1 > len)
		l1 = len;
	else{
		l2 = p->data_len - l1;
		if ((l1 + l2) > len)
			l2 = len - l1;
	}
	if (l1 > 0)
		memcpy(data, p->data + p->data_head, l1);
	if (l2 > 0)
		memcpy(data + l1, p->data, l2);
	l = l1 + l2;
	p->data_len -= l;
	p->data_head += l;
	p->data_head %= p->unit_len;
	return l;
}

/* Запись данных */
static int write_unit_data(struct data_unit *p, uint8_t *data, int len)
{
	int l, l1, l2, tail;
/*	printf(__func__": p = %p; p->data = %p; data = %p; len = %d; "
			"p->data_len = %d; p->unit_len = %d\n",
			p, p->data, data, len, p->data_len, p->unit_len);*/
	if ((p == NULL) || (p->data == NULL) ||
			(data == NULL) || (len <= 0) ||
			(p->data_len == p->unit_len))
		return 0;
	tail = (p->data_head + p->data_len) % p->unit_len;
	if (p->data_head <= tail){
		l1 = p->unit_len - tail;
		l2 = p->data_head;
	}else{
		l1 = p->unit_len - p->data_len;
		l2 = 0;
	}
	if (l1 > len){
		l1 = len;
		l2 = 0;
	}else if ((l1 + l2) > len)
		l2 = len - l1;
	if (l1 > 0)
		memcpy(p->data + tail, data, l1);
	if (l2 > 0)
		memcpy(p->data, data + l1, l2);
	l = l1 + l2;
	p->data_len += l;
	return l;
}

/* Прием данных из заданного сокета */
static bool recv_unit_data(int sock, struct data_unit *p, bool *closed)
{
	int i, l[2] = {0, 0}, tail, ret;
	if ((sock == -1) || (p == NULL) || (p->data == NULL) ||
			(p->data_len == p->unit_len))
		return false;
	if (closed != NULL)
		*closed = false;
	tail = (p->data_head + p->data_len) % p->unit_len;
	if (p->data_head <= tail){
		l[0] = p->unit_len - tail;
		l[1] = p->data_head;
	}else{
		l[0] = p->unit_len - p->data_len;
		l[1] = 0;
	}
	for (i = 0; i < ASIZE(l); i++){
		if (l[i] == 0)
			continue;
		ret = recv(sock, p->data + tail, l[i], MSG_NOSIGNAL);
		if (ret == 0){
			if (closed != NULL)
				*closed = true;
			return true;
		}else if (ret < 0)
			return errno == EAGAIN;
		else{
			p->data_len += ret;
			tail = (p->data_head + p->data_len) % p->unit_len;
			if (ret < l[i])
				break;
		}
	}
	return true;
}

/* Отправка данных в заданный сокет */
static bool send_unit_data(int sock, struct data_unit *p)
{
	int i, l[2] = {0, 0}, ret;
	if ((sock == -1) || (p == NULL) || (p->data == NULL) ||
			(p->data_len == 0))
		return false;
	if ((p->data_head + p->data_len) > p->unit_len)
		l[0] = p->unit_len - p->data_head;
	else
		l[0] = p->data_len;
	l[1] = p->data_len - l[0];
	for (i = 0; i < ASIZE(l); i++){
		if (l[i] == 0)
			continue;
		ret = send(sock, p->data + p->data_head, l[i], MSG_NOSIGNAL);
		if (ret < 0)
			return errno == EAGAIN;
		else{
			p->data_head += ret;
			p->data_head %= p->unit_len;
			p->data_len -= ret;
			if (ret < l[i])
				break;
		}
	}
	return true;
}

/* Цепочка данных */
struct data_chain {
	struct data_unit *head;
	struct data_unit *tail;
	int nr_units;
};

/* Максимальное число блоков в цепочке */
#define MAX_CHAIN_UNITS		1024

/* Освобождение цепочки данных */
static void free_data_chain(struct data_chain *chain)
{
	struct data_unit *p, *tmp;
	if (chain != NULL){
		for (p = chain->head; p != NULL; p = tmp){
			tmp = p->next;
			free_data_unit(p);
		}
		chain->head = chain->tail = NULL;
		chain->nr_units = 0;
	}
}

/* Инициализация цепочки данных */
static bool init_data_chain(struct data_chain *chain, int unit_len)
{
	if (chain != NULL){
		if (chain->nr_units != 0)
			free_data_chain(chain);
		chain->head = chain->tail = new_data_unit(unit_len);
		if (chain->head != NULL){
			chain->nr_units = 1;
			return true;
		}
	}
	return false;
}

/* Добавление нового блока в конец цепочки */
static bool add_data_unit(struct data_chain *chain, int unit_len)
{
	if ((chain != NULL) && (chain->nr_units < MAX_CHAIN_UNITS) &&
			(chain->tail != NULL)){
		struct data_unit *p = new_data_unit(unit_len);
		if (p != NULL){
			chain->tail->next = p;
			chain->tail = p;
			chain->nr_units++;
/*			printf(__func__": chain = %p; nr_units = %d\n",
					chain, chain->nr_units);*/
			return true;
		}
	}
	return false;
}

/* Удаление блока из начала цепочки */
static bool del_data_unit(struct data_chain *chain)
{
	if ((chain == NULL) || (chain->head == NULL))
		return false;
	else if (chain->nr_units == 1)
		return true;
	else{
		struct data_unit *p = chain->head;
		chain->head = p->next;
		free_data_unit(p);
		chain->nr_units--;
/*		printf(__func__": chain = %p; nr_units = %d\n",
				chain, chain->nr_units);*/
		return true;
	}
}

/* Сброс цепочки данных в начальное состояние */
static void reset_data_chain(struct data_chain *chain)
{
	if (chain != NULL){
		while (chain->nr_units > 1)
			del_data_unit(chain);
		reset_data_unit(chain->head);
	}
}

/* Определение длины данных в цепочке */
static int get_chain_data_len(struct data_chain *chain)
{
	int len = 0;
	if (chain != NULL){
		struct data_unit *p;
		for (p = chain->head; p != NULL; p = p->next)
			len += p->data_len;
	}
	return len;
}

/* Чтение данных из цепочки */
static int read_chain_data(struct data_chain *chain, uint8_t *data, int len)
{
	int l = 0, m;
	struct data_unit *p, *tmp;
	if ((chain == NULL) || (data == NULL) || (len <= 0))
		return 0;
	m = get_chain_data_len(chain);
	if (m > len)
		m = len;
	for (p = chain->head; (p != NULL) && (m > l); p = tmp){
		tmp = p->next;
		l += read_unit_data(p, data + l, m - l);
		if (p->data_len == 0)
			del_data_unit(chain);
	}
	return l;
}

/* Запись данных в цепочку */
static int write_chain_data(struct data_chain *chain, uint8_t *data, int len,
		int unit_len)
{
	int l = 0, ret;
	struct data_unit *p;
	if ((chain == NULL) || (chain->tail == NULL) ||
			(data == NULL) || (len <= 0) || (unit_len <= 0))
		return 0;
	p = chain->tail;
	while (len > 0){
		if (p->data_len == p->unit_len){
			if (add_data_unit(chain, unit_len))
				p = chain->tail;
			else
				break;
		}
		ret = write_unit_data(p, data + l, len);
		l += ret;
		len -= ret;
	}
	return l;
}

/* Прием данных в цепочку из заданного сокета */
static bool recv_chain_data(int sock, struct data_chain *chain, int unit_len,
		bool *closed)
{
	struct data_unit *p;
	int old_len;
	bool _closed;
	if ((sock == -1) || (chain == NULL) || (chain->tail == NULL) ||
			(unit_len <= 0))
		return false;
	if (closed != NULL)
		*closed = false;
	p = chain->tail;
	do {
		if (p->data_len == p->unit_len){
			if (add_data_unit(chain, unit_len))
				p = chain->tail;
			else
				break;
		}
		old_len = p->data_len;
		if (!recv_unit_data(sock, p, &_closed))
			break;
		else if (_closed){
			if (closed != NULL)
				*closed = true;
			break;
		}
	} while (p->data_len != old_len);
	return true;
}

/* Передача данных из цепочки в заданный сокет */
static bool send_chain_data(int sock, struct data_chain *chain)
{
	struct data_unit *p;
	int old_len;
	if ((sock == -1) || (chain == NULL) || (chain->head == NULL))
		return false;
	p = chain->head;
	while ((chain->nr_units > 1) || (chain->tail->data_len > 0)){
		if (p->data_len == 0){
			del_data_unit(chain);
			p = chain->head;
		}
		old_len = p->data_len;
		send_unit_data(sock, p);
		if (p->data_len == old_len)
			break;
	}
	return true;
}

/* Состояние транзакции */
enum {
	ts_none,	/* нет транзакции */
	ts_connecting,	/* идет соединение */
	ts_ready,	/* соединение установлено */
	ts_close_wait,	/* соединение закрыто */
};
		
/* Транзакции */
static struct tcp_pos_transaction {
	struct sockaddr_in addr;	/* адрес процессингового центра */
	int sock;			/* -1, если нет транзакции */
	int st;				/* состояние транзакции */
	struct data_chain in;		/* принятые данные */
	struct data_chain out;		/* данные для передачи */
} pos_transaction[MAX_POS_TCP_TRANSACTIONS];

/* Инициализация массива транзакций */
void pos_init_transactions(void)
{
/*	static uint8_t data[8193];*/
	struct tcp_pos_transaction *tr;
	uint16_t port = TCP_POS_PORT_BASE;
	uint32_t ip = ntohl(cfg.bank_proc_ip);
	int i;
	for (i = 0; i < MAX_POS_TCP_TRANSACTIONS; i++, port++){
		if (i == 4){
			port = TCP_POS_PORT_BASE;
			ip++;
		}
		tr = pos_transaction + i;
		tr->addr.sin_family = AF_INET;
		tr->addr.sin_port = htons(port);
		tr->addr.sin_addr.s_addr = htonl(ip);
		tr->sock = -1;
		tr->st = ts_none;
		init_data_chain(&tr->in, 8192);
		init_data_chain(&tr->out, 8192);
	}
/*	for (i = 0; i < sizeof(data); i++)
		data[i] = i & 0xff;
	write_chain_data(&pos_transaction->out, data, sizeof(data), 8192);
	memset(data, 0, sizeof(data));
	read_chain_data(&pos_transaction->out, data, sizeof(data));
	for (i = 0; i < sizeof(data); i++){
		if (data[i] != (i & 0xff))
			break;
	}
	if (i != sizeof(data))
		printf(__func__": error reading data\n");*/
}

/* Сброс транзакции в начальное состояние */
static void pos_reset_transaction(struct tcp_pos_transaction *tr,
		bool full_reset)
{
	if (tr->sock != -1){
		close(tr->sock);
		tr->sock = -1;
	}
	reset_data_chain(&tr->in);
	reset_data_chain(&tr->out);
	tr->st = ts_none;
/* Для транзакции с номером 1 сбрасываем также адрес процессингового центра */
	if (full_reset && (tr == pos_transaction))
		tr->addr.sin_addr.s_addr = cfg.bank_proc_ip;
}

/* Освобождение массива транзакций */
void pos_release_transactions(void)
{
	struct tcp_pos_transaction *tr;
	int i;
	for (i = 0; i < MAX_POS_TCP_TRANSACTIONS; i++){
		tr = pos_transaction + i;
		if (tr->sock != -1){
			close(tr->sock);
			tr->sock = -1;
		}
		free_data_chain(&tr->in);
		free_data_chain(&tr->out);
		tr->st = ts_none;
	}
}

/* Начало транзакции */
static bool pos_begin_transaction(int n, bool full_reset)
{
	struct tcp_pos_transaction *tr = pos_transaction + n;
	if ((tr->st == ts_connecting) || (tr->st == ts_ready))
		return true;	/* транзакция уже начата */
	pos_reset_transaction(tr, full_reset);
	tr->sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tr->sock == -1){
		pos_set_error(POS_ERROR_CLASS_TCP, POS_ERR_TCP_INIT, 0);
		return false;
	}
	fcntl(tr->sock, F_SETFL, O_NONBLOCK);
	connect(tr->sock, (struct sockaddr *)&tr->addr, sizeof(tr->addr));
	tr->st = ts_connecting;
	return true;
}

/* Завершение всех транзакций */
void pos_stop_transactions(void)
{
	int i;
	for (i = 0; i < MAX_POS_TCP_TRANSACTIONS; i++)
		pos_reset_transaction(pos_transaction + i, true);
}

/* Проверка транзакции на ошибку */
static bool pos_check_tcp_error(struct tcp_pos_transaction *tr)
{
	int err, l = sizeof(err);
	if ((getsockopt(tr->sock, SOL_SOCKET, SO_ERROR,	&err, (socklen_t *)&l) == -1) ||
			((err != 0) && (err != EAGAIN) &&
			 (err != EINPROGRESS)))
		return false;
	else
		return true;
}

/* Для транзакции 1 в случае ошибки переключаемся на адрес 5 */
static bool do_alternate_transaction(int n)
{
	struct tcp_pos_transaction *tr = pos_transaction + n;
	if ((n == 0) && (tr->addr.sin_addr.s_addr != tr[4].addr.sin_addr.s_addr)){
		tr->addr.sin_addr.s_addr = tr[4].addr.sin_addr.s_addr;
		tr->st = ts_none;
		return pos_begin_transaction(n, false);
	}else
		return false;
}

/* Соединение с сервером */
static bool pos_do_connect(int n)
{
	struct tcp_pos_transaction *tr = pos_transaction + n;
	struct timeval tv = {0, 0};
	fd_set wfds;
	if (!pos_check_tcp_error(tr)){
		if (do_alternate_transaction(n))
			return true;
		pos_set_error(POS_ERROR_CLASS_TCP, POS_ERR_CONNECT, n + 1);
		return false;
	}
	FD_ZERO(&wfds);
	FD_SET(tr->sock, &wfds);
	if (select(tr->sock + 1, NULL, &wfds, NULL, &tv) == -1){
		pos_set_error(POS_ERROR_CLASS_TCP, POS_ERR_CONNECT, n + 1);
		return false;
	}else if (FD_ISSET(tr->sock, &wfds))
		tr->st = ts_ready;
	return true;
}

/* Прием данных */
static bool pos_do_recv(int n)
{
	struct tcp_pos_transaction *tr = pos_transaction + n;
	struct timeval tv = {0, 0};
	bool closed = false;
	fd_set rfds;
	if (!pos_check_tcp_error(tr)){
		if ((get_chain_data_len(&tr->in) == 0) && do_alternate_transaction(n))
			return true;
		pos_set_error(POS_ERROR_CLASS_TCP, POS_ERR_DATA_TX, n + 1);
		return false;
	}
	FD_ZERO(&rfds);
	FD_SET(tr->sock, &rfds);
	if (select(tr->sock + 1, &rfds, NULL, NULL, &tv) == -1){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}else if (FD_ISSET(tr->sock, &rfds)){
		recv_chain_data(tr->sock, &tr->in, 8192, &closed);
		if (closed){
			if ((get_chain_data_len(&tr->in) == 0) && do_alternate_transaction(n))
				return true;
			else{
				tr->st = ts_close_wait;
				close(tr->sock);
				tr->sock = -1;
			}
		}
	}
	return true;
}

/* Передача данных */
static bool pos_do_send(int n)
{
	struct tcp_pos_transaction *tr = pos_transaction + n;
	struct timeval tv = {0, 0};
	fd_set wfds;
	if (!pos_check_tcp_error(tr)){
		pos_set_error(POS_ERROR_CLASS_TCP, POS_ERR_DATA_TX, n + 1);
		return false;
	}
	FD_ZERO(&wfds);
	FD_SET(tr->sock, &wfds);
	if (select(tr->sock + 1, NULL, &wfds, NULL, &tv) == -1){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}else if (FD_ISSET(tr->sock, &wfds))
		send_chain_data(tr->sock, &tr->out);
	return true;
}

/* Обработка транзакции. Возвращает false в случае ошибки транзакции */
static bool pos_do_transaction(int n)
{
	struct tcp_pos_transaction *tr = pos_transaction + n;
	switch (tr->st){
		case ts_connecting:
			return	pos_do_connect(n);
		case ts_ready:
			if (pos_do_recv(n)){
				if (tr->st == ts_ready)
					return pos_do_send(n);
				else
					return true;
			}else
				return false;
		default:
			return true;
	}
}

/* Обработка всех транзакций */
bool pos_tcp_process(void)
{
	int i;
	for (i = 0; i < MAX_POS_TCP_TRANSACTIONS; i++){
		if (!pos_do_transaction(i))
			return false;
	}
	return true;
}

static bool pos_parse_connect(struct pos_data_buf *buf, bool check_only)
{
	uint8_t n;
	if (!pos_read_byte(buf, &n)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}else if (n > MAX_POS_TCP_TRANSACTIONS){
		pos_set_error(POS_ERROR_CLASS_TCP, POS_ERR_HOST_NOT_CNCT, n + 1);
		return false;
	}
	return check_only ? true : pos_begin_transaction(n - 1, true);
}

static bool pos_parse_disconnect(struct pos_data_buf *buf, bool check_only)
{
	uint8_t n;
	if (!pos_read_byte(buf, &n)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}else if (n > MAX_POS_TCP_TRANSACTIONS){
		pos_set_error(POS_ERROR_CLASS_TCP, POS_ERR_HOST_NOT_CNCT, n + 1);
		return false;
	}
	if (!check_only)
		pos_reset_transaction(pos_transaction + n - 1, true);
	return true;
}

static bool pos_parse_data(struct pos_data_buf *buf, bool check_only)
{
	static uint8_t data[MAX_POS_TCP_DATA_LEN];
	uint8_t n;
	struct tcp_pos_transaction *tr;
	int l;
	if (!pos_read_byte(buf, &n)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}else if (n > MAX_POS_TCP_TRANSACTIONS){
		pos_set_error(POS_ERROR_CLASS_TCP, POS_ERR_HOST_NOT_CNCT, n + 1);
		return false;
	}
	tr = pos_transaction + n - 1;
	if (!check_only){
		if ((tr->st == ts_none) || (tr->st == ts_close_wait)){
			pos_set_error(POS_ERROR_CLASS_TCP, POS_ERR_HOST_NOT_CNCT, n + 1);
			return false;
		}
	}
	l = pos_read_array(buf, data, sizeof(data));
	if (l <= 0){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	if (!check_only){
		if (write_chain_data(&tr->out, data, l, 8192) != l){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			return false;
		}
	}
	return true;
}

bool pos_parse_tcp_stream(struct pos_data_buf *buf, bool check_only)
{
	static struct {
		uint8_t code;
		bool (*parser)(struct pos_data_buf *, bool);
	} parsers[] = {
		{POS_TCP_CONNECT,	pos_parse_connect},
		{POS_TCP_DISCONNECT,	pos_parse_disconnect},
		{POS_TCP_DATA,		pos_parse_data},
	};
	int i, l;
	uint16_t len;
	if (!pos_read_word(buf, &len) || (len > POS_MAX_BLOCK_DATA_LEN)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	l = buf->data_index + len;
	if (l > buf->data_len){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	while (buf->data_index < l){
		uint8_t code;
		if (!pos_read_byte(buf, &code)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		for (i = 0; i < ASIZE(parsers); i++){
			if (code == parsers[i].code){
				if (!parsers[i].parser(buf, check_only))
					return false;
				break;
			}
		}
		if (i == ASIZE(parsers)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
	}
	if (buf->data_index == l)
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
}

/* Запись команды DISCONNECT */
static bool pos_req_save_tcp_disconnect(struct pos_data_buf *buf, int n)
{
	return	pos_write_byte(buf, POS_TCP_DISCONNECT) &&
		pos_write_byte(buf, n + 1);
}

/* Запись команды DATA */
static bool pos_req_save_tcp_data(struct pos_data_buf *buf, int n, int max_l)
{
	static uint8_t data[MAX_POS_TCP_DATA_LEN];
	struct tcp_pos_transaction *tr = pos_transaction + n;
	int l = max_l - 4, ret;
	if (l > sizeof(data))
		l = sizeof(data);
	ret = read_chain_data(&tr->in, data, l);
	return (ret > 0) && pos_write_byte(buf, POS_TCP_DATA) &&
			pos_write_byte(buf, n + 1) &&
			pos_write_array(buf, data, ret);
}

/* Определение числа событий для записи в поток TCP/IP */
int pos_count_tcp_events(void)
{
	int i, n = 0;
	struct tcp_pos_transaction *tr;
	for (i = 0, tr = pos_transaction; i < MAX_POS_TCP_TRANSACTIONS; i++, tr++){
		if (tr->st == ts_close_wait)
			n++;
		if (get_chain_data_len(&tr->in) > 0)
			n++;
	}
	return n;
}

/* Запись потока TCP/IP */
bool pos_req_save_tcp(struct pos_data_buf *buf)
{
	int i, start, max_len = POS_MAX_BLOCK_DATA_LEN;
	struct tcp_pos_transaction *tr;
	if ((buf == NULL) || (pos_count_tcp_events() == 0))
		return false;
	if (!pos_req_stream_begin(buf, POS_STREAM_TCP)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
	start = buf->data_index;
	for (i = 0, tr = pos_transaction; i < MAX_POS_TCP_TRANSACTIONS; i++, tr++){
		if (get_chain_data_len(&tr->in) > 0){
			if (max_len > 4){
				pos_req_save_tcp_data(buf, i, max_len);
				max_len -= buf->data_index - start;
			}else
				break;
		}
		if (tr->st == ts_close_wait){
			if (max_len >= 3){
				pos_req_save_tcp_disconnect(buf, i);
				pos_reset_transaction(tr, true);
				max_len -= 3;
			}else
				break;
		}
	}
	if (!pos_req_stream_end(buf)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}else
		return true;
}
