/* Транспортный уровень терминала. (c) gsr 2000, 2004 */

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/route.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "gui/scr.h"
#include "gui/status.h"
#include "prn/express.h"
#include "genfunc.h"
#include "cfg.h"
#include "gd.h"
#include "iplir.h"
#include "numbers.h"
#include "ppp.h"
#include "sterm.h"
#include "transport.h"
#include "xchange.h"
#include "gui/xchange.h"

/* Состояние канального уровня */
int c_state = cs_nc;

/* Буферы терминала */
/* ОЗУ необработанного ответа */
uint8_t resp_buf[RESP_BUF_LEN];
/* Длина ответа */
int resp_len = 0;
/* Ответ получен полностью */
bool full_resp;
/* Смещение начала прикладного ответа */
int text_offset = 0;
/* Длина прикладного ответа */
int text_len = 0;
/* ОЗУ запроса для передачи */
uint8_t req_buf[REQ_BUF_LEN];
/* Длина запроса */
int req_len = 0;
/* Длина переданных данных */
static int sent_len;
/* Время отправки последнего запроса */
uint32_t req_time = 0;
/* Время реакции терминала */
uint32_t reaction_time = 0;

/* Аналоги ifup/ifdown */
/* Вызывается только для сетевой карты */
bool ifup(char *iface)
{
	bool flag = false;
	char cmd[128], local_ip[16], net_mask[16], gateway[16];
	if (iface == NULL)
		return false;
	strcpy(local_ip, inet_ntoa(dw2ip(cfg.local_ip)));
	strcpy(net_mask, inet_ntoa(dw2ip(cfg.net_mask)));
	strcpy(gateway, inet_ntoa(dw2ip(cfg.gateway)));
	snprintf(cmd, sizeof(cmd), "ifconfig %s %s netmask %s",
			iface, local_ip, net_mask);
	if (system(cmd) == 0){
		snprintf(cmd, sizeof(cmd), "route add default gw %s", gateway);
		flag = system(cmd) == 0;
	}
	return flag;
}

bool ifdown(char *iface)
{
	char cmd[64];
	if (iface == NULL)
		return false;
	snprintf(cmd, sizeof(cmd), "ifconfig %s down", iface);
	return system(cmd) == 0;
}

/* Установка сетевых настроек после установки соединения PPP */
bool set_ppp_cfg(void)
{
	int sock;
	struct ifreq if_cfg;
	if (cfg.use_iplir){
		iplir_stop();
/*		check_iplir();
		cfg_iplir();*/
		iplir_start();
	}
	if ((sock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) == -1)
		return false;
	strcpy(if_cfg.ifr_name, "ppp0");
/* Получение нашего адреса */
	if (ioctl(sock, SIOCGIFADDR, &if_cfg) == 0)
		cfg.local_ip = ((struct sockaddr_in *)&if_cfg.ifr_addr)->sin_addr.s_addr;
	else
		perror("SIOCGIFADDR");
/* Получение адреса шлюза */
	if (ioctl(sock, SIOCGIFDSTADDR, &if_cfg) == 0)
		cfg.gateway = ((struct sockaddr_in *)&if_cfg.ifr_addr)->sin_addr.s_addr;
	else
		perror("SIOCGIFDSTADDR");
/* Маска подсети известна заранее */
	cfg.net_mask = 0xffffffff;
	close(sock);
	write_cfg();
	return true;
}

static int term_socket = -1;
static struct sockaddr_in host_addr;
/* Все времена отсчитываются в сотых долях секунды */
static uint32_t t0;		/* время начала операции */
static uint32_t wresp_t0;		/* время перехода в состояние "Ждите ответа" */

static uint16_t expected_len;	/* ожидаемая длина ответа */

static void _log_data(uint8_t *p, uint16_t len, int dir)
{
	xlog_add_item(p, len, dir);
	if (xchg_active)
		on_new_xchg_item();
}

static bool init_tcpip(void)
{
	signal(SIGPIPE, SIG_IGN);
	return true;
}

static void release_tcpip(void)
{
	if (term_socket != -1){
		close(term_socket);
		term_socket = -1;
	}
	signal(SIGPIPE, SIG_DFL);
}

static void make_host_addr(void)
{
	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(EXPRESS_SERVER_PORT);
	host_addr.sin_addr.s_addr = get_x3_ip();
}

static bool make_term_socket(void)
{
	c_state = cs_nc;
	make_host_addr();
	if ((term_socket != -1) && cfg.tcp_cbt)
		release_term_socket();
	if (term_socket == -1){
		term_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (term_socket != -1){
			fcntl(term_socket, F_SETFL, O_NONBLOCK);
			c_state = cs_ready;
			return true;
		}else
			return false;
	}else
		return true;
}

void release_term_socket(void)
{
	c_state = cs_nc;
	if (term_socket != -1){
		close(term_socket);
		term_socket = -1;
	}
}

static void on_sock_error(int e)
{
	if (e == 0)
		return;
	switch (c_state){
		case cs_wconnect:
			switch (e){
				case ENETDOWN:
				case ENETUNREACH:
				case EHOSTDOWN:
				case EHOSTUNREACH:
					slayer_error(TCPERR_HOSTUNREACH);
					break;
				case ECONNRESET:
				case ECONNREFUSED:
					slayer_error(TCPERR_CONNREFUSED);
					break;
				default:
					slayer_error(TCPERR_CONN);
					break;
			}
			break;
		case cs_wsend:
			switch (e){
				case ECONNRESET:
				case ECONNREFUSED:
				case ENOTCONN:
					slayer_error(TCPERR_IPREFUSED);
					break;
				default:
					slayer_error(TCPERR_CONN);
					break;
			}
			break;
		case cs_sending:
			slayer_error(TCPERR_SEND);
			break;
		case cs_sent:
			switch (e){
				case ECONNRESET:
				case ECONNREFUSED:
					if ((s_state == ss_initializing) &&
						((u_times() - t0) < INIT_REFUSE_WAIT))
						slayer_error(TCPERR_TERMREFUSED);
					else
						slayer_error(TCPERR_WRESPREFUSED);
					break;
				case ENOTCONN:
					slayer_error(TCPERR_WRESPCLOSED);
					break;
				default:
					slayer_error(TCPERR_WRESP);
					break;
			}
			break;
		case cs_selected:
			switch (e){
				case ECONNRESET:
				case ECONNREFUSED:
					slayer_error(TCPERR_RCVREFUSED);
					break;
				case ENOTCONN:
					slayer_error(TCPERR_RCVCLOSED);
					break;
				default:
					slayer_error(TCPERR_RCV);
					break;
			}
			break;
	}
}

static bool check_sock_error(void)
{
	if (term_socket != -1){
		int e, l = sizeof(e);
		if ((getsockopt(term_socket, SOL_SOCKET, SO_ERROR, &e, (socklen_t *)&l) == 0) &&
				(e != 0) && (e != EAGAIN) && (e != EINPROGRESS)){
			on_sock_error(e);
			return true;
		}
	}
	return false;
}

/* Передача данных. Возвращает true, если переданы все данные */
static bool send_sock_data(void)
{
	int l, ll;
	if (term_socket == -1)
		return false;
	l = req_len - sent_len;
	if (l > 0){
		ll = send(term_socket, req_buf + sent_len, l, MSG_NOSIGNAL);
		if (ll > 0)
			sent_len += ll;
		else if ((ll != -1) || (errno != EAGAIN)){
			slayer_error(TCPERR_SEND);
			return false;
		}
	}
	if (sent_len >= req_len){
		_log_data(req_buf, req_len, xlog_out);
		return true;
	}else
		return false;
}

/* Начало передачи данных */
static bool start_sending(uint32_t t)
{
	if ((term_socket == -1) || (req_len == 0))
		return false;
	sent_len = 0;
	c_state = cs_sending;
	set_term_state(st_transmit);
	t0 = t;
	return true;
}

static void process_tcpip(void)
{
	fd_set rfd, wfd;
	struct timeval timeout = {0, 0};
	int l = 0;
	bool flag;
	uint32_t t;
	if (cfg.use_ppp){
		ppp_process();
		if (ppp_state != ppp_ready)
			return;
	}
	if (term_socket != -1){
		if (check_sock_error())
			return;
		FD_ZERO(&rfd);
		FD_ZERO(&wfd);
		FD_SET(term_socket, &rfd);
		FD_SET(term_socket, &wfd);
		select(term_socket + 1, &rfd, &wfd, NULL, &timeout);
	}
	t = u_times();
	switch (c_state){
		case cs_hasreq:
			if (_term_state != st_wait)
				set_term_state(st_wait);
			if ((t - t0) < TCP_SND_INTERVAL)
				break;
			if (term_socket == -1){
				if (make_term_socket()){
					connect(term_socket,
						(struct sockaddr *)&host_addr,
						sizeof(host_addr));
					c_state = cs_wconnect;
					t0 = t;
				}else
					slayer_error(TCPERR_SOCKET);
			}else{		/* сокет уже создан */
				c_state = cs_wconnect;
				t0 = t;
			}
			break;
		case cs_wconnect:
			if (FD_ISSET(term_socket, &wfd)){
				if (s_state == ss_ready){
					if (!start_sending(t))
						slayer_error(TCPERR_SEND);
				}else{
					c_state = cs_wsend;
					t0 = t;
				}
			}else if ((t - t0) >= TCP_WCONN_TIMEOUT)
				slayer_error(TCPERR_CONN);
			break;
		case cs_wsend:
			if ((t - t0) >= INIT_SEND_WAIT){
				if (!start_sending(t))
					slayer_error(TCPERR_SEND);
				t0 = t;
			}
			break;
		case cs_sending:
			if ((t - t0) >= TCP_SND_TIMEOUT)
				slayer_error(TCPERR_SEND);
			else if (FD_ISSET(term_socket, &wfd)){
				if (send_sock_data()){	/* передача завершена */
					c_state = cs_sent;
					set_term_state(st_wrecv);
					if (hbyte != HB_INIT)
						set_term_led(hbyte = HB_WRESP);
					wresp_t0 = t0 = t;
				}
			}
			break;
		case cs_selected:
			if ((t - t0) >= TCP_RCV_TIMEOUT){
				slayer_error(TCPERR_RCVTIMEOUT);
				break;
			}
			__fallthrough__;
		case cs_sent:
			if (FD_ISSET(term_socket, &rfd)){
				if (c_state == cs_sent){
					resp_len = expected_len = 0;
					c_state = cs_selected;
					set_term_state(st_recv);
					t0 = t;
				}
				l = recv(term_socket, resp_buf + resp_len,
					sizeof(resp_buf) - resp_len, 0);
				if (l > 0){
					flag = resp_len < 4;
					resp_len += l;
					if (flag && (resp_len >= 4)){
						expected_len = read_hex_word(resp_buf);
						if (expected_len == 0){
							slayer_error(SERR_LENGTH);
							break;
						}
					}
					if ((expected_len > 0) && (resp_len >= expected_len)){
						resp_len = expected_len;
						full_resp = true;
						set_term_state(st_resp);
						_log_data(resp_buf, resp_len, xlog_in);
						c_state = cs_ready;
					}
				}else
					slayer_error(TCPERR_RCVCLOSED);
			}
			break;
	}
}

/* Проверка возможности послать запрос */
bool can_send_request(void)
{
	if (c_state == cs_sent)
		return (u_times() - wresp_t0) >= TCP_WRESP_SILENT_INTERVAL;
	else
		return true;
}

/* Инициализация транспортного уровня */
void open_transport(void)
{
	req_len = 0;
	resp_len = 0;
	text_offset = text_len = 0;
	memset(resp_buf, 0, RESP_BUF_LEN);
	c_state = cs_nc;
	init_tcpip();
}

/* Закрытие транспортного уровня */
void close_transport(void)
{
	release_tcpip();
}

/* Начало передачи запроса */
void begin_transmit(void)
{
	if (cfg.tcp_cbt)
		release_term_socket();
	c_state = cs_hasreq;
}

#include "bscsym.h"

static int strip_resp(void)
{
	bool is_valid_char(uint8_t ch)
	{
		switch (ch){
			case BSC_RUS:
			case BSC_LAT:
			case BSC_SYN:
				return false;
			default:
				return true;
		}
	}
	int i, j;
	for (i = j = 0; j < resp_len; j++){
		if (is_valid_char(resp_buf[j]))
			resp_buf[i++] = resp_buf[j];
	}
	resp_len = i;
#ifdef __CLEAR_PREV_RESP__
	memset(resp_buf + resp_len, 0, sizeof(resp_buf) - resp_len);
#endif
	text_offset = text_len = 0;
	return resp_len;
}

/* Проверка присутствия полного ответа */
static bool is_full_resp(void)
{
	if (full_resp){
		full_resp = false;
		strip_resp();
		return true;
	}else
		return false;
}

/* Возвращает true, если получен ответ */
bool process_transport(void)
{
	if (s_state == ss_finit)
		return false;
	else if (s_state == ss_new){
		if (!prn_numbers_ok()){
			s_state = ss_finit;
			set_term_astate(ast_prn_number);
			err_beep();
			return false;
		}
	}
	if (cfg.use_ppp){
		if ((ppp_state == ppp_nc) && (c_state == cs_hasreq)){
			if (ppp_connect()){
				if (s_state == ss_initializing)
					init_t0 = u_times();
			}
		}else if ((ppp_state != ppp_ready) && (ppp_state != ppp_nc))
			return false;
	}
	process_tcpip();
	if ((s_state == ss_new) || (s_state == ss_winit))
		begin_initialization();
	else if (is_full_resp()){	/* есть ответ от ЭВМ */
		reaction_time = u_times() - req_time;
/* Проверка минимальной длины ответа */
		if (!check_min_resp_len())
			return false;
		set_term_led(hbyte = HB_OK);
		session_error = SLAYER_OK;
/* Проверка контрольной суммы */
		if (!check_crc())
			return false;
/* Проверка правил гарантрованной доставки */
		if (!check_gd_rules())
			return false;
/* Преобразование ответа для синтаксического разбора */
		set_text_len();
		if (cfg.tcp_cbt)
			release_term_socket();
		return true;
	}else if (s_state == ss_initializing){
		if ((u_times() - init_t0) >= INIT_TIMEOUT)
			slayer_error(SERR_TIMEOUT);
	}
	return false;
}

/* Подготовка к разбору ответа, введенного вручную */
bool prepare_raw_resp(void)
{
	strip_resp();
	if (!is_escape(resp_buf[0])){
		if (resp_len < 18)
			return false;
		text_offset = 13;
		text_len = resp_len - 18;
	}else{
		text_offset = 0;
		text_len = resp_len;
	}
	return true;
}
