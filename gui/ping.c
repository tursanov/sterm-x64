/*
 * �஢�ઠ ����� TCP/IP ���뫪�� icmp-����⮢ (ping).
 * 11.05.2019 ��������� �஢�ઠ ��� ���� ��⠭���� ᮥ������� TCP.
 * (c) gsr, Alex P. Popov 2002, 2004, 2005, 2019.
 */

#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "devinfo.h"
#include "genfunc.h"
#include "kbd.h"
#include "paths.h"
#include "sterm.h"
#include "gui/exgdi.h"
#include "gui/ping.h"

enum {
	HOST_GW = 0,
	HOST_X3,
/*	HOST_X3_ALT,*/
	HOST_BANK1,
	HOST_BANK2,
	HOST_FDO,
	NR_HOSTS,
};

static struct ping_rec {
	struct in_addr ip;
	union {
		uint16_t id;
		uint16_t port;
	};
	uint16_t seq;
	union {
		int nr_replies;
		int state;
	};
	uint32_t t0;
} hosts[NR_HOSTS];

static int icmp_sock = -1;
static int fdo_sock  = -1;

enum {
	fdo_st_start,
	fdo_st_conn,
	fdo_st_ok,
	fdo_st_na,
};

static const char *fdo_st_str(int fdo_st)
{
	static char buf[8];
	const char *ret = buf;
	switch (fdo_st){
		case fdo_st_start:
			ret = "";
			break;
		case fdo_st_conn:
			ret = "ᮥ�������";
			break;
		case fdo_st_ok:
			ret = "����㯥�  ";
			break;
		case fdo_st_na:
			ret = "������㯥�";
			break;
		default:
			snprintf(buf, sizeof(buf), "%d", fdo_st);
	}
	return ret;
}

static void make_host_name(int n, uint32_t ip)
{
	if ((n >= 0) && (n < NR_HOSTS)){
		hosts[n].ip.s_addr = ip;
		if (n == HOST_FDO){
			hosts[n].port = (ip == INADDR_NONE) ? 0 : cfg.fdo_port;
			hosts[n].state = (ip == INADDR_NONE) ? fdo_st_na : fdo_st_start;
		}else{
			hosts[n].id = (ip == INADDR_NONE) ? 0 : rand() & 0xffff;
			hosts[n].nr_replies = 0;
		}
		hosts[n].seq = 0;
		hosts[n].t0 = 0;
	}
}

static void make_host_names(void)
{
	make_host_name(HOST_GW, cfg.gateway);
	make_host_name(HOST_X3, get_x3_ip());
	if (cfg.bank_system){
		uint32_t ip = ntohl(cfg.bank_proc_ip);
		make_host_name(HOST_BANK1, htonl(ip++));
		make_host_name(HOST_BANK2, htonl(ip));
	}else{
		make_host_name(HOST_BANK1, INADDR_NONE);
		make_host_name(HOST_BANK2, INADDR_NONE);
	}
	if (cfg.has_kkt && (cfg.fdo_iface == KKT_FDO_IFACE_USB))
		make_host_name(HOST_FDO, cfg.fdo_ip);
	else
		make_host_name(HOST_FDO, INADDR_NONE);
}

static bool create_icmp_sock(void)
{
	struct protoent *proto = NULL;	//getprotobyname(ICMP_PROTO_NAME);
	icmp_sock = socket(PF_INET,SOCK_RAW,
			(proto != NULL) ? proto->p_proto : IPPROTO_ICMP);
	if (icmp_sock != -1){
		if (fcntl(icmp_sock, F_SETFL, O_NONBLOCK) == -1){
			close(icmp_sock);
			icmp_sock = -1;
		}
	}
	return icmp_sock != -1;
}

static int in_cksum(uint16_t *buf, int sz)
{
	int nleft = sz;
	int sum = 0;
	uint16_t *w = buf, ans = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(uint16_t *) (&ans) = *(uint8_t *) w;
		sum += ans;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	return ans;
}

static bool send_ping(struct ping_rec *rec, uint32_t t)
{
	bool ret = false;
	if (rec == NULL)
		return ret;
	char packet[PING_DATA_LEN + 8];
	struct icmp *pkt = (struct icmp *)packet;
	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_code = 0;
	pkt->icmp_cksum = 0;
	pkt->icmp_seq = rec->seq++;
	pkt->icmp_id = rec->id;
	strcpy((char *)pkt->icmp_data, ICMP_DATA);
	pkt->icmp_cksum = in_cksum((uint16_t *)pkt, sizeof(packet));
	struct sockaddr_in sa = {
		.sin_family	= AF_INET,
		.sin_port	= 0,
		.sin_addr	= {
			.s_addr	= rec->ip.s_addr
		}
	};
	if (sendto(icmp_sock, packet, sizeof(packet), 0,
			(struct sockaddr *)&sa, sizeof(sa)) == sizeof(packet)){
		rec->t0 = t;
		ret = true;
	}
	return ret;
}

static int parse_reply(char *buf, int sz, struct sockaddr_in *from)
{
	int ret = -1;
	if (sz < (PING_DATA_LEN + ICMP_MINLEN))
		return ret;
	struct iphdr *ip_hdr = (struct iphdr *)buf;
	int hlen = ip_hdr->ihl << 2;
	sz -= hlen;
	struct icmp *icmp_pkt = (struct icmp *)(buf + hlen);
	for (int i = 0; i < NR_HOSTS; i++){
		if (hosts[i].ip.s_addr != from->sin_addr.s_addr)
			continue;
		else if ((icmp_pkt->icmp_type == ICMP_ECHOREPLY) &&
				(icmp_pkt->icmp_id == hosts[i].id) &&
				(icmp_pkt->icmp_seq < hosts[i].seq)){
			hosts[i].nr_replies++;
			ret = i;
			break;
		}
	}
	return ret;
}

static bool fdo_sock_open(void)
{
	fdo_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fdo_sock != -1){
		if (fcntl(fdo_sock, F_SETFL, O_NONBLOCK) == -1){
			close(fdo_sock);
			fdo_sock = -1;
		}
	}
	return fdo_sock != -1;
}

static void fdo_sock_close(void)
{
	if (fdo_sock != -1){
		shutdown(fdo_sock, SHUT_RDWR);
		close(fdo_sock);
		fdo_sock = -1;
	}
}

static bool fdo_begin_connect(struct ping_rec *rec, uint32_t t)
{
	bool ret = false;
	struct sockaddr_in addr = {
		.sin_family	= AF_INET,
		.sin_port	= htons(rec->port),
		.sin_addr	= rec->ip
	};
	if ((connect(fdo_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) &&
			(errno != EINPROGRESS)){
		fdo_sock_close();
		rec->state = fdo_st_na;
	}else{
		rec->t0 = t;
		rec->state = fdo_st_conn;
		ret = true;
	}
	return ret;
}

#define FDO_CONNECT_TIMEOUT		300	/* �� */
static bool fdo_connect(struct ping_rec *rec, uint32_t t)
{
	bool ret = true;
	struct pollfd fds = {
		.fd		= fdo_sock,
		.events		= POLLOUT,
		.revents	= 0
	};
	int rc = poll(&fds, 1, 1);
	if ((rc == -1) || (fds.revents & (POLLERR | POLLHUP | POLLNVAL)))
		ret = false;
	else if (fds.revents & POLLOUT)
		rec->state = fdo_st_ok;
	else if ((t - rec->t0) > FDO_CONNECT_TIMEOUT)
		ret = false;
	if (!ret){
		fdo_sock_close();
		rec->state = fdo_st_na;
	}
	return ret;
}

#define MAX_PING_LINES	22
#define MAX_STR_LEN	78

static PixmapFontPtr ping_font = NULL;
static char ping_lines[MAX_PING_LINES][MAX_STR_LEN + 1];
static int cur_ping_line = 0;

static inline void reset_ping_line(int n)
{
	if (n < ASIZE(ping_lines))
		ping_lines[n][0] = 0;
}

static bool draw_ping_hints(void)
{
	char *p = "Esc -- ��室";
	const int hint_h = 34;
	int cx = DISCX - 5;
	int cy = hint_h*2+4;
	GCPtr pGC = CreateGC(4, DISCY - hint_h*2-8, cx, cy);
	GCPtr pMemGC = CreateMemGC(cx, cy);
	FontPtr pFont = CreateFont(_("fonts/terminal10x18.fnt"), false);
	
	SetFont(pMemGC, pFont);
	ClearGC(pMemGC, clBtnFace);
	DrawBorder(pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC), 1,
		clBtnShadow, clBtnHighlight);
	
	DrawText(pMemGC, 0,0,pMemGC->box.width,pMemGC->box.height, p, 0);
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, cx, cy);
	DeleteFont(pFont);
	DeleteGC(pMemGC);
	DeleteGC(pGC);

	return true;
}

static bool draw_ping_lines(void)
{
	int cx = DISCX-2, cy = DISCY-42;
	GCPtr pGC = CreateGC(20, 35, cx, cy);
	GCPtr pMemGC = CreateMemGC(MAX_STR_LEN*ping_font->max_width, ping_font->max_height);
	int m = 0;

	for (int n = 0; n < MAX_PING_LINES; n++){
		if (ping_lines[n][0] != 0)
			m = n + 1;
	}

	for (int n = 0, y = 0; n < m; n++){
		if (ping_lines[n][0] != 0){
			ClearGC(pMemGC, cfg.bg_color);
			PixmapTextOut(pMemGC, ping_font, 0, 0, ping_lines[n]);
			CopyGC(pGC, 0, y, pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC));
			y += ping_font->max_height;
		}
	}
	
	DeleteGC(pMemGC);
	DeleteGC(pGC);

	return true;
}

static void make_gap_line(void)
{
	for (int i = 0; i < MAX_PING_LINES - 1; i++)
		strcpy(ping_lines[i], ping_lines[i + 1]);
	memset(ping_lines[MAX_PING_LINES - 1], 0, MAX_STR_LEN + 1);
}

static void check_pos(void)
{
	if (cur_ping_line >= MAX_PING_LINES) {
		cur_ping_line = MAX_PING_LINES - 1;
		make_gap_line();
	}
}

__attribute__((format (printf, 1, 2)))
static bool add_ping_line(char *format, ...)
{
#define TAB_SPACES	8
#define PING_LINE_LEN	256
	if (format == NULL)
		return false;
	int n = strlen(ping_lines[cur_ping_line]);
	char s[PING_LINE_LEN + 1], *text;
	void new_line(void)
	{
		n = 0;
		cur_ping_line++;
		check_pos();
	}
	va_list p;
	va_start(p, format);
	vsnprintf(s, PING_LINE_LEN, format, p);
	s[PING_LINE_LEN] = 0;
	va_end(p);
	for (text = s; *text; text++){
		switch (*text){
			int m = 0;
			case '\t':
				m = (n / TAB_SPACES + 1) * TAB_SPACES - n;
				for (int i = 0; i < m; i++) {
					ping_lines[cur_ping_line][n++] = ' ';
					if (n == MAX_STR_LEN)
						new_line();
				}
				break;
			case '\n':
				new_line();
				break;
			default:
				ping_lines[cur_ping_line][n++] = *text;
				if (n == MAX_STR_LEN)
					new_line();
		}
	}
	draw_ping_lines();
	return true;
}

bool draw_ping(void)
{
	draw_ping_hints();
	return true;
}


bool init_ping()
{
	make_host_names();
	if (!create_icmp_sock())
		return false;
	ping_active = true;

	cur_ping_line = 0;
	memset(ping_lines, 0, sizeof(ping_lines));
	set_term_busy(true);
	hide_cursor();
	set_scr_mode(m80x20, false, false);
	clear_text_field();

	set_term_state(st_ping);
	set_term_astate(ast_none);

	scr_show_pgnum(false);
	scr_show_mode(false);
	scr_show_language(false);
	scr_show_log(false);
/*	scr_visible = false;*/

	ping_font = CreatePixmapFont(_("fonts/sterm/80x20/courier10x21.fnt"), 
		cfg.rus_color, cfg.bg_color);
	draw_ping();
	draw_clock(true);
	return true;
}

void release_ping(void)
{
	DeletePixmapFont(ping_font);
	close(icmp_sock);
	ping_active = false;
}

static char *get_ping_line(int n)
{
	static char line[128];
	char desc[30];
	struct in_addr ip = {.s_addr = 0};
	switch (n){
		case HOST_GW:
			snprintf(desc, sizeof(desc), "���");
			ip.s_addr = cfg.gateway;
			break;
		case HOST_X3:
			snprintf(desc, sizeof(desc), "����-��� \"������-3\"");
			ip.s_addr = cfg.x3_p_ip;
			break;
/*		case HOST_X3_ALT:
			snprintf(desc, sizeof(desc), "����-���-2 \"������-3\"");
			ip.s_addr = cfg.x3_s_ip;
			break;*/
		case HOST_BANK1:
			snprintf(desc, sizeof(desc), "����ᨭ���� 業�� #1");
			ip.s_addr = cfg.bank_proc_ip;
			break;
		case HOST_BANK2:
			snprintf(desc, sizeof(desc), "����ᨭ���� 業�� #2");
			ip.s_addr = htonl(ntohl(cfg.bank_proc_ip) + 1);
			break;
		case HOST_FDO:
			snprintf(desc, sizeof(desc), "���");
			ip.s_addr = cfg.fdo_ip;
			break;
		default:
			line[0] = 0;
	}
#define DESC_WIDTH	30
#define IP_WIDTH	25
	if (n == HOST_FDO){
		char addr[30];
		snprintf(addr, sizeof(addr), "%s:%hu", inet_ntoa(ip), cfg.fdo_port);
		snprintf(line, sizeof(line), "%-*s%-*s%s", DESC_WIDTH, desc, IP_WIDTH, addr,
			fdo_st_str(hosts[n].state));
	}else
		snprintf(line, sizeof(line), "%-*s%-*s%d �� %hu", DESC_WIDTH, desc,
			IP_WIDTH, inet_ntoa(ip), hosts[n].nr_replies, hosts[n].seq);
	return line;
}

static bool fdo_process(uint32_t t)
{
	struct ping_rec *rec = hosts + HOST_FDO;
	if (rec->ip.s_addr == INADDR_NONE)
		return false;
	bool rc = false;
	int prev_fdo_st = rec->state;
	switch (rec->state){
		case fdo_st_start:
			rc = fdo_sock_open() && fdo_begin_connect(rec, t);
			break;
		case fdo_st_conn:
			rc = fdo_connect(rec, t);
			break;
	}
	if (!rc && (fdo_sock != -1))
		fdo_sock_close();
	return prev_fdo_st != rec->state;
}

bool process_ping(struct kbd_event *e)
{
	if (e->pressed && (e->key == KEY_ESCAPE))
		return false;
	uint32_t t = u_times();
	char packet[PING_DATA_LEN + MAX_IP_LEN + MAX_ICMP_LEN];
	struct sockaddr_in from;
	int from_len = sizeof(from);
	int l = recvfrom(icmp_sock, packet, sizeof(packet), 0, (struct sockaddr *)&from,
		(socklen_t *)&from_len);
	if (l > 0){
		int n = parse_reply(packet, l, &from);
		if (n != -1){
			cur_ping_line = n;
			reset_ping_line(n);
			add_ping_line(get_ping_line(n));
		}
	}
	for (int i = 0; i < NR_HOSTS; i++){
		if ((hosts[i].ip.s_addr == INADDR_NONE) ||
				(hosts[i].nr_replies == NR_PINGS))
			continue;
		bool rc = false;
		if (i == HOST_FDO)
			rc = fdo_process(t);
		else if ((hosts[i].seq < NR_PINGS) &&
				(t - hosts[i].t0) > ICMP_PING_INTERVAL)
			rc = send_ping(hosts + i, t);
		if (rc){
			cur_ping_line = i;
			reset_ping_line(i);
			add_ping_line(get_ping_line(i));
		}
	}
	return true;
}
