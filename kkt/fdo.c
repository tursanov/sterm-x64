/* Обмен данными с ОФД. (c) gsr 2018-2020 */

#include <arpa/inet.h>
#include <log/logdbg.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "cfg.h"
#include "kkt/cmd.h"
#include "kkt/fs.h"
#include "kkt/kkt.h"

/* Отладочная печать */
__attribute__((format (printf, 2, 3))) static void __dbg(const char *fn, const char *fmt, ...)
{
	struct timeb tb;
	ftime(&tb);
	struct tm *tm = localtime(&tb.time);
	fprintf(stderr, "%.2d:%.2d:%.2d.%.3d %s: ", tm->tm_hour, tm->tm_min, tm->tm_sec,
		tb.millitm, fn);
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

#define dbg(fmt, arg...) __dbg(__func__, fmt "\n", ## arg)

/* Заголовок сеансового уровня */
struct fdo_session_header {
	uint32_t sign;
#define SESSION_SIGN		0x0a41082a
	uint16_t sversion;
#define SVERSION_0_1		0xa281
#define SVERSION_CURRENT	SVERSION_0_1
	uint16_t aversion;
#define AVERSION_0_1		0x0100
#define AVERSION_CURRENT	AVERSION_0_1
	char     fr_number[KKT_FS_NR_LEN];
	uint16_t data_len;
	uint16_t flags;
#define SFLAG_PRIORITY_MASK	0x00c0
#define SFLAG_PRIORITY_NORMAL	0x0000
#define SFLAG_FEATURES_MASK	0x0030
#define SFLAG_FEATURE_NORESP	0x0000
#define SFLAG_FEATURE_RESP	0x0010
#define SFLAG_CONTAINER		0x0004
#define SFLAG_CRC_MASK		0x0003
#define SFLAG_NO_CRC		0x0000
#define SFLAG_HEAD_CRC		0x0001
#define SFLAG_FULL_CRC		0x0002
	uint16_t crc16;
	uint8_t container[0];
} __attribute__((__packed__));

/* Максимальная длина данных сеансового уровня */
#define MAX_SMSG_LEN		65536

#define FDO_TX_BUF_LEN		(MAX_SMSG_LEN + sizeof(struct fdo_session_header))
#define FDO_RX_BUF_LEN		FDO_TX_BUF_LEN

/* Буфер приёма от ОФД */
static uint8_t fdo_rx[FDO_RX_BUF_LEN];
static size_t fdo_rx_len = 0;

static inline void fdo_reset_rx(void)
{
	fdo_rx_len = 0;
}

/* Поток для работы с ОФД */
static pthread_t fdo_thread = 0;

/* Состояния потока работы с ОФД */
enum {
	fdo_thread_active,
	fdo_thread_suspended,
	fdo_thread_stopped,
};

/* Состояние потока для работы с ОФД */
static volatile int fdo_thread_state = fdo_thread_active;

#if 0
/* Для сигнализации об изменении состояния потока используется механизм сигналов */
#define SIG_THREAD_STATE_CHANGED	SIGRTMIN

static void sig_handler(int arg /*__attribute__((used))*/)
{
	dbg("arg = %d.", arg);
}

static bool fdo_set_sig_handler(void)
{
	struct sigaction sa = {
		.sa_handler = sig_handler,
		.sa_flags = 0,
		.sa_restorer = NULL
	};
	sigemptyset(&sa.sa_mask);
	bool ret = (sigaction(SIG_THREAD_STATE_CHANGED, &sa, NULL) == 0);
	if (!ret)
		dbg("ошибка sigaction(): %s.", strerror(errno));
	return ret;
}

static bool fdo_reset_sig_handler(void)
{
	struct sigaction sa = {
		.sa_handler = SIG_DFL,
		.sa_flags = 0,
		.sa_restorer = NULL
	};
	sigemptyset(&sa.sa_mask);
	bool ret = (sigaction(SIG_THREAD_STATE_CHANGED, &sa, NULL) == 0);
	if (!ret)
		dbg("ошибка sigaction(): %s.", strerror(errno));
	return ret;
}
#endif

static pthread_mutex_t fdo_mtx = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t susp_mtx = PTHREAD_MUTEX_INITIALIZER;

bool fdo_lock(void)
{
	return pthread_mutex_lock(&fdo_mtx) == 0;
}

bool fdo_unlock(void)
{
	return (pthread_mutex_unlock(&fdo_mtx) == 0) || (errno == EPERM);
}

static bool fdo_set_thread_state(int state)
{
	bool ret = false;
	if ((state != fdo_thread_state) && fdo_lock()){
		dbg("%d --> %d", fdo_thread_state, state);
		logdbg("%s: %d --> %d.\n", __func__, fdo_thread_state, state);
		fdo_thread_state = state;
		if (state == fdo_thread_active)
			fdo_reset_rx();
		logdbg("%s: calling fdo_unlock().\n", __func__);
		fdo_unlock();
/*		int rc = pthread_kill(fdo_thread, SIG_THREAD_STATE_CHANGED);
		if (rc == 0){*/
			logdbg("%s: calling pthread_mutex_lock(susp_mtx).\n", __func__);
			pthread_mutex_lock(&susp_mtx);
			logdbg("%s: calling pthread_mutex_unlock(susp_mtx).\n", __func__);
			pthread_mutex_unlock(&susp_mtx);
			ret = true;
			logdbg("%s: state changed.\n", __func__);
/*		}else
			dbg("ошибка pthread_kill(): %s.", strerror(errno));*/
	}
	return ret;
}

bool fdo_suspend(void)
{
	return fdo_set_thread_state(fdo_thread_suspended);
}

bool fdo_resume(void)
{
	return fdo_set_thread_state(fdo_thread_active);
}

static void fdo_stop_thread(void)
{
	logdbg("%s.\n", __func__);
	fdo_set_thread_state(fdo_thread_stopped);
	logdbg("%s: calling pthread_join; fdo_thread_state = %d.\n", __func__, fdo_thread_state);
	pthread_join(fdo_thread, NULL);
	logdbg("%s thread terminated.\n", __func__);
}

static bool fdo_sleep(uint32_t ms)
{
	bool ret = false;
	ms /= 10;
	uint32_t t0 = u_times();
	while ((fdo_thread_state == fdo_thread_active) && ((u_times() - t0) < ms))
		pthread_yield();
/*	if (ms == 0)
		ret = pthread_yield() == 0;
	else{
		struct timespec ts = {
			.tv_sec = ms / 1000,
			.tv_nsec = (ms % 1000) * 1000000
		};
		ret = nanosleep(&ts, NULL) == 0;
	}*/
	return ret;
}

/* Таймаут соединения с сервером ОФД */
#define FDO_CONNECT_TIMEOUT		3000	/* 3 сек */
/* Таймаут передачи данных ОФД */
#define FDO_SEND_TIMEOUT		3000	/* 3 сек */
/* Таймаут приёма данных от ОФД */
#define FDO_RECV_TIMEOUT		3000	/* 3 сек */

#define get_timeb(t) \
	struct timeb t; \
	ftime(&t)

static uint32_t time_diff(const struct timeb *t0)
{
	get_timeb(t);
	return (t.time - t0->time) * 1000 +
		((time_t)t.millitm - (time_t)t0->millitm);
}

static int fdo_sock = -1;

static bool fdo_connected = false;

static bool fdo_sock_close(void)
{
	bool ret = true;
	dbg("fdo_sock = %d.", fdo_sock);
	if (fdo_sock != -1){
		if (shutdown(fdo_sock, SHUT_RDWR) == -1){
			dbg("ошибка shutdown() для сокета: %s.", strerror(errno));
			ret = false;
		}
		if (close(fdo_sock) == -1){
			dbg("ошибка close() для сокета: %s", strerror(errno));
			ret = false;
		}
		fdo_sock = -1;
	}else
		ret = false;
	fdo_connected = false;
	return ret;
}

static bool fdo_sock_open(void)
{
	bool ret = false;
	if (fdo_sock != -1)
		fdo_sock_close();
	fdo_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fdo_sock == -1)
		dbg("ошибка socket(): %s.", strerror(errno));
	else if (fcntl(fdo_sock, F_SETFL, O_NONBLOCK) == 0)
		ret = true;
	else{
		dbg("ошибка fcntl(O_NONBLOCK): %s.", strerror(errno));
		fdo_sock_close();
	}
	return ret;
}

static bool fdo_sock_open_if_need(void)
{
	return (fdo_sock == -1) ? fdo_sock_open() : true;
}

static int fdo_get_sock_error(void)
{
	int err = 0;
	socklen_t len = sizeof(err);
	if (getsockopt(fdo_sock, SOL_SOCKET, SO_ERROR, &err, &len) == -1)
		err = errno;
	return err;
}

static bool fdo_parse_addr(const uint8_t *data, size_t len, uint32_t *ip, uint16_t *port)
{
	if ((data == NULL) || (len != 21) || (ip == NULL) || (port == NULL))
		return false;
	uint32_t v[4] = {[0 ... ASIZE(v) - 1] = 0};
	bool err = false;
	for (int i = 0, k = 0; i < 15; i++){
		uint8_t b = data[i];
		if ((i == 3) || (i == 7) || (i == 11)){
			if (b == '.')
				k++;
			else
				err = true;
		}else if (isdigit(b)){
			v[k] *= 10;
			v[k] += b - '0';
		}else
			err = true;
		if (err)
			break;
	}
	if (!err)
		err = (v[0] > 255) || (v[1] > 255) || (v[2] > 255) || (v[3] >= 255);
	if (!err){
		*ip = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
		uint16_t p = 0;
		for (int i = 0; i < 5; i++){
			uint8_t b = data[i + 16];
			if (isdigit(b)){
				p *= 10;
				p += b - '0';
			}else{
				err = true;
				break;
			}
		}
		if (!err)
			*port = htons(p);
	}
	return !err;
}

/*static void *debug_thread_proc(void *arg)
{
	usleep(100);
	pthread_kill((pthread_t)arg, SIG_THREAD_STATE_CHANGED);
	return NULL;
}*/

/* Установка соединения с ОФД */
static uint16_t fdo_connect(const uint8_t *data, size_t len)
{
	uint16_t ret = FDO_OPEN_ERROR;
	struct sockaddr_in addr = {
		.sin_family	= AF_INET,
		.sin_port	= 0,
		.sin_addr	= {
			.s_addr	= 0
		}
	};
	if (fdo_connected){
		dbg("соединение было установлено ранее.");
		ret = FDO_OPEN_ALREADY_CONNECTED;
	}else if (!fdo_parse_addr(data, len, &addr.sin_addr.s_addr, &addr.sin_port)){
		dbg("ошибка разбора адреса (%.*s).", (int)len, data);
		ret = FDO_OPEN_BAD_ADDRESS;
	}else if (!fdo_sock_open_if_need())
		dbg("ошибка создания сокета.");
	else if ((connect(fdo_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) &&
				(errno != EINPROGRESS))
		dbg("ошибка connect(): %s.", strerror(errno));
	else{
		struct pollfd fds = {
			.fd		= fdo_sock,
			.events		= POLLOUT,
			.revents	= 0
		};
/*		pthread_t tid = 0;
		pthread_create(&tid, NULL, debug_thread_proc, (void *)pthread_self());*/
		int rc = poll(&fds, 1, FDO_CONNECT_TIMEOUT);
		if (rc == -1){
			if (errno == EINTR)
				dbg("операция прервана.");
			else
				dbg("ошибка poll(): %s.", strerror(errno));
		}else if (rc == 0)
			dbg("таймаут соединения.");
		else if (fds.revents & (POLLERR | POLLHUP | POLLNVAL))
			dbg("ошибка сокета: %s.", strerror(fdo_get_sock_error()));
		else if (fds.revents & POLLOUT){
			dbg("соединение с ОФД установлено.");
			fdo_connected = true;
			ret = FDO_OPEN_CONNECTED;
		}else
			dbg("revents = 0x%.4hx.", fds.revents);
	}
	if (ret == FDO_OPEN_ERROR)
		fdo_sock_close();
	return ret;
}

/* Закрытие соединения с ОФД */
static uint16_t fdo_close(void)
{
	uint16_t ret = FDO_CLOSE_COMPLETE;
	if (!fdo_connected)
		ret = FDO_CLOSE_NOT_CONNECTED;
	else if (!fdo_sock_close())
		ret = FDO_CLOSE_ERROR;
	else
		dbg("соединение с ОФД закрыто.");
	return ret;
}

/* Определение состояния соединения с ОФД */
static uint16_t fdo_get_connection_status(void)
{
	return fdo_connected ? FDO_STATUS_CONNECTED : FDO_STATUS_NOT_CONNECTED;
}

/* Передача данных для ОФД */
static uint16_t fdo_send(const uint8_t *data, size_t len)
{
	if ((data == NULL) || (len == 0) || (len > FDO_TX_BUF_LEN))
		return FDO_SEND_ERROR;
	uint16_t ret = FDO_SEND_ERROR;
	size_t sent_len = 0;
	uint32_t dt = 0;
	get_timeb(t0);
	while (sent_len < len){
		bool flag = false;
		struct pollfd fds = {
			.fd		= fdo_sock,
			.events		= POLLOUT,
			.revents	= 0
		};
		int rc = poll(&fds, 1, FDO_SEND_TIMEOUT - dt);
		dt = time_diff(&t0);
		if (rc == -1){
			if (errno == EINTR)
				dbg("операция прервана.");
			else
				dbg("ошибка poll(): %s.", strerror(errno));
		}else if (rc == 0)
			dbg("таймаут передачи.");
		else if (fds.revents & (POLLERR | POLLHUP | POLLNVAL))
			dbg("ошибка сокета: %s.", strerror(fdo_get_sock_error()));
		else if (fds.revents & POLLOUT)
			flag = true;
		else
			dbg("revents = 0x%.4hx.", fds.revents);
		if (!flag)
			break;
		ssize_t l = send(fdo_sock, data + sent_len, len - sent_len, MSG_NOSIGNAL);
		if (l == -1){
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
				flag = true;
			else
				dbg("ошибка send(): %s.", strerror(errno));
		}else if (l > 0){
			dbg("ОФД отправлено %zd байт.", l);
			sent_len += l;
		}
	}
	if (sent_len == len)
		ret = FDO_SEND_OK;
	return ret;
}

/* Приём данных из ОФД */
static uint16_t fdo_recv(void)
{
	uint16_t ret = FDO_RCV_NOT_CONNECTED;
	if (!fdo_connected)
		return ret;
	struct pollfd fds = {
		.fd		= fdo_sock,
		.events		= POLLIN,
		.revents	= 0
	};
	int rc = poll(&fds, 1, FDO_RECV_TIMEOUT);
	if (rc == -1){
		if (errno == EINTR)
			dbg("операция прервана.");
		else
			dbg("ошибка poll(): %s.", strerror(errno));
	}else if (rc == 0)
		dbg("ОФД завершил соединение.");
	else if (fds.revents & (POLLERR | POLLHUP | POLLNVAL))
		dbg("ошибка сокета: %s.", strerror(fdo_get_sock_error()));
	else if (fds.revents & POLLIN){
		ssize_t len = recv(fdo_sock, fdo_rx + fdo_rx_len,
			sizeof(fdo_rx) - fdo_rx_len, 0);
		if (len == -1){
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
				ret = FDO_RCV_NO_DATA;
			else
				dbg("ошибка recv: %s.", strerror(errno));
		}else if (len == 0)
			dbg("ОФД завершил соединение.");
		else if (len > 0){
			dbg("из ОФД получено %zd байт.", len);
			fdo_rx_len += len;
			ret = FDO_RCV_OK;
		}else
			dbg("recv() вернул %zd.", len);
	}
	return ret;
}

/* Передача в ККТ данных, принятых от ОФД */
static uint16_t fdo_send_kkt(void)
{
	uint16_t ret = FDO_RCV_OK;
	if (fdo_rx_len == 0){
		if (fdo_connected)
			ret = FDO_RCV_NO_DATA;
		else
			ret = FDO_RCV_NOT_CONNECTED;
	}else{
		uint8_t status = kkt_send_fdo_data(fdo_rx, fdo_rx_len);
		if (status == FDO_DATA_STATUS_OK){
			dbg("в ККТ передано %zu байт.", fdo_rx_len);
			fdo_reset_rx();
		}else
			dbg("ошибка передачи данных в ККТ: 0x%.2hhx.", status);
	}
	return ret;
}

static uint8_t fdo_prev_op = UINT8_MAX;
static uint16_t fdo_prev_op_status = 0;

static void fdo_poll_kkt(void)
{
	static uint8_t data[65536];
	size_t data_len = sizeof(data);
	uint8_t cmd = 0;
	if (kkt_get_fdo_cmd(fdo_prev_op, fdo_prev_op_status,
			&cmd, data, &data_len) == KKT_STATUS_OK){
		dbg("%.2hhx:%.4u; cmd = %.2hhx; data_len = %zu.",
			fdo_prev_op, fdo_prev_op_status, cmd, data_len);
		fdo_prev_op = cmd;
		switch (cmd){
			case FDO_REQ_NOP:
				fdo_prev_op_status = 0;
				fdo_sleep(cfg.fdo_poll_period * 1000);
				break;
			case FDO_REQ_OPEN:
				fdo_prev_op_status = fdo_connect(data, data_len);
				break;
			case FDO_REQ_CLOSE:
				fdo_prev_op_status = fdo_close();
				break;
			case FDO_REQ_CONN_ST:
				fdo_prev_op_status = fdo_get_connection_status();
				break;
			case FDO_REQ_SEND:
				fdo_prev_op_status = fdo_send(data, data_len);
				break;
			case FDO_REQ_RECEIVE:
				fdo_prev_op_status = fdo_recv();
				if (fdo_prev_op_status == FDO_RCV_OK)
					fdo_prev_op_status = fdo_send_kkt();
				break;
			case FDO_REQ_MESSAGE:
				fdo_prev_op_status = FDO_MSG_OK;
				break;
		}
	}else
		fdo_sleep(cfg.fdo_poll_period * 1000);
}

static void *fdo_thread_proc(void *arg __attribute__((unused)))
{
	while (fdo_thread_state != fdo_thread_stopped){
//		logdbg("%s: %d.\n", __func__, fdo_thread_state);
		if (fdo_thread_state == fdo_thread_suspended)
/*			pause();*/
			pthread_yield();
		else if ((fdo_thread_state == fdo_thread_active) &&
				cfg.has_kkt && (kkt != NULL)){
			pthread_mutex_lock(&susp_mtx);
			fdo_poll_kkt();
			pthread_mutex_unlock(&susp_mtx);
		}else
			pthread_yield();
	}
	logdbg("%s: terminated.\n", __func__);
	return NULL;
}

bool fdo_init(void)
{
	bool ret = false;
/*	if (!fdo_set_sig_handler())
		;
	else*/ if (pthread_create(&fdo_thread, NULL, fdo_thread_proc, NULL) == 0){
		dbg("модуль ОФД готов к работе.");
		ret = true;
	}else
		dbg("ошибка pthread_create(): %s.", strerror(errno));
	return ret;
}

void fdo_release(void)
{
	fdo_stop_thread();
	pthread_mutex_unlock(&fdo_mtx);
	fdo_sock_close();
/*	fdo_reset_sig_handler();*/
	dbg("модуль ОФД завершил работу.");
}
