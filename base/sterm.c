/* �᭮��� �㭪樨 �ନ����. (c) gsr 2000-2020 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <vipnet/vpn_api.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "gui/adv_calc.h"
#include "gui/calc.h"
#include "gui/dialog.h"
#include "gui/help.h"
#include "gui/log/express.h"
#include "gui/log/kkt.h"
#include "gui/log/pos.h"
#include "gui/fa.h"
#include "gui/newcheque.h"
#include "gui/menu.h"
#include "gui/options.h"
#include "gui/ping.h"
#include "gui/scr.h"
#include "gui/ssaver.h"
#include "gui/xchange.h"
#include "kkt/fdo.h"
#include "kkt/kkt.h"
#include "kkt/fd/ad.h"
#include "log/express.h"
#include "log/kkt.h"
#include "log/pos.h"
#include "pos/error.h"
#include "pos/pos.h"
#include "pos/screen.h"
#include "pos/tcp.h"
#include "prn/aux.h"
#include "prn/express.h"
#include "prn/local.h"
#include "x3data/grids.h"
#include "x3data/icons.h"
#include "x3data/patterns.h"
#include "x3data/xslts.h"
#include "bscsym.h"
#include "devinfo.h"
#include "gd.h"
#include "genfunc.h"
#include "hash.h"
#include "iplir.h"
#include "kbd.h"
#include "keys.h"
#include "numbers.h"
#include "paths.h"
#include "ppp.h"
#include "sterm.h"
#include "termlog.h"
#include "tki.h"
#include "transport.h"

uint16_t term_check_sum = 0;	/* ����஫쭠� �㬬� �ନ���� */

/* ���᮪ ���ன�� ������祭��� � �ନ���� */
struct dev_lst *devices = NULL;

char main_title[MAIN_TITLE_LEN];		/* ��������� �������� ���� �ନ���� */

char *term_string = "������-2�-�";

static int ret_val = RET_NORMAL;

/* ���⮣ࠬ�� ��� POS-�ନ���� */
static BitmapPtr bmp_up;
static BitmapPtr bmp_down;

/*
 * ������ � ᥪ㭤�� ����� �६���� �� ��� � �� �ନ����.
 * �६� �����뢠���� �� �ਭ����.
 */
time_t time_delta = 0;	/* T��� - T��. */

#if defined __WATCH_EXPRESS__
static bool watch_transaction;
static bool watch_crying;
#endif

/* ��⠭���������� �� �뢮�� ᮮ⢥������� ���� �ନ���� */
bool menu_active	= false;
bool optn_active	= false;
bool calc_active	= false;
bool xlog_active	= false;
bool plog_active	= false;
bool klog_active	= false;
bool xchg_active	= false;
bool help_active	= false;
bool ssaver_active	= false;
bool ping_active	= false;
bool pos_active		= false;
bool fa_active		= false;

static bool term_busy = false;		/* 䫠� ������� �ନ���� */

int kt = key_none;			/* ⨯ ���� DS1990A */
/* ����� ���� DS1990A */
ds_number dsn = {[0 ... DS_NUMBER_LEN - 1] = 0};
/* ��� ����஥��� ���祩 */
static struct md5_hash srv_keys[NR_SRV_KEYS];
/* ��� �⫠����� ���祩 */
static struct md5_hash dbg_keys[NR_DBG_KEYS];

uint8_t hbyte = HB_INIT;		/* H-���� */
int _term_state = st_none;		/* ����� ���� ��ப� ����� */
intptr_t _term_aux_state = ast_none;	/* �ࠢ�� ���� ��ப� ����� */
/* �᫨ quick_astate(term_astate), �뢮��� �� ���ﭨ� � show_req �� resp_executing */
int astate_for_req = ast_none;

struct menu	*mnu = NULL;		/* ���� */

uint8_t text_buf[TEXT_BUF_LEN];		/* ���� ����� �⢥� */
uint8_t *err_ptr = NULL;			/* 㪠��⥫� �訡�� � �⢥� */
int ecode = E_OK;			/* ��� �訡�� */
struct para_info map[MAX_PARAS + 1];	/* "����" �⢥� */
int n_paras = 0;			/* �᫮ ����楢 � �⢥� */
int cur_para = 0;			/* ����� ⥪�饣� ����� */
struct hash *rom = NULL;		/* ��� ����⠭� */
struct hash *prom = NULL;		/* ��� */
bool can_reject = false;		/* 䫠� ���������� �⪠�� �� ������ */
bool resp_handling = false;		/* ����� ��ࠡ��뢠�� �⢥� �� F8 */
bool resp_executing = false;		/* ��⠭���������� � execute_resp */
bool resp_printed = false;		/* �� ��ࠡ�⪥ �⢥� �믮��﫠�� ����� �� ��� */
bool has_bank_data = false;		/* � �⢥� ���� ����� ��� ��� */
bool has_kkt_data = false;		/* � �⢥� ���� ����� ��� ��� */

bool full_resp;				/* 䫠� ��室� �⢥� �� ���-��� */
bool rejecting_req;			/* �� ��᫠� ����� � �⪠��� �� ������ */
bool online = true;			/* 䫠� ���������� ��襭�� ��࠭� */
static int prev_s_state;		/* �ᯮ������ �� ࠡ�� � ����� ����஥� */

bool into_on_response;			/* �ᯮ������ � show_error */

/* ��⠭���������� ��᫥ ����祭�� SIGTERM */
static bool sigterm_caught = false;

/* ����⠭������� ��ࠡ��稪� SIGTERM �� 㬮�砭�� */
static void restore_sigterm_handler(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(SIGTERM, &sa, NULL);
}

/* ��ࠡ��稪 ᨣ���� SIGTERM */
static void sigterm_handler(int n __attribute__((unused)))
{
	restore_sigterm_handler();
	sigterm_caught = true;
}

/* ��⠭���� ��ࠡ��稪� ᨣ���� SIGTERM */
static void set_sigterm_handler(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigterm_handler;
	sa.sa_flags = SA_RESETHAND;
	sigaction(SIGTERM, &sa, NULL);
}

/* ��⠭���������� �� �६� ��⮬���᪮� ���� 祪� �� ��� */
bool apc = false;

/*
 * �� ����㧪� �ନ���� 䠩����� ��⥬� �� DiskOnChip/DiskOnModule
 * ��������� � ०��� ro. ����� �ନ���� ����室��� ������� ����� �� ���,
 * ����室��� ��६���஢��� /home � ०��� rw, ������� ����� � ᭮��
 * ᬮ��஢��� /home � ०��� ro.
 */
static bool remount_home(bool rw __attribute__((unused)))
{
#if defined __FAKE_REMOUNT__
	return true;
#else
	char cmd[64];
	sprintf(cmd, "mount -n -o remount,%s /home", rw ? "rw" : "ro");
	return system(cmd) == 0;
#endif		/* __FAKE_REMOUNT__ */
}

/* ���� ���஢ �� ��� */
void flush_home(void)
{
	remount_home(false);
	usleep(100000);
	remount_home(true);
}

/* ������ ����஫쭮� �㬬� ��������� 䠩�� */
#define CHKSUM_BLOCK_SIZE	4096
static uint16_t make_check_sum(char *fname)
{
	if (fname == NULL)
		return 0;
	static uint8_t buf[CHKSUM_BLOCK_SIZE];
	uint32_t s = 0UL;
	uint16_t div = ((uint16_t)1 << 0) | ((uint16_t)1 << 13) | ((uint16_t)1 << 15), w = 0;
	int fd = open(fname, O_RDONLY);
	if (fd != -1){
		while (true){
			int l = read(fd, buf, CHKSUM_BLOCK_SIZE);
			for (int i = 0; i < l;){
				w = buf[i++];
				s <<= 8;
				if (i < l){
					w <<= 8;
					w |= buf[i++];
					s <<= 8;
				}
				s |= w;
				s %= div;
			}
			if (l != CHKSUM_BLOCK_SIZE)
				break;
		}
		close(fd);
	}
	return (uint16_t)s;
}

bool is_escape(uint8_t c)
{
	return (c == BSC_DLE) || (c == X_DLE);
}

/* ������ ��������� ��ப� */
static bool parse_cmd_line(int argc, char **argv)
{
	extern char *optarg;
	char *shortopts = "v";
	const struct option longopts[] = {
		{"version",	no_argument,		NULL,	'v'},
		{NULL,		0,			NULL,	0},
	};
	bool loop_flag = true, ret_flag = true;
	while (loop_flag){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'v':
				dump_config();
				loop_flag = false;
				ret_val = RET_VERSION;
				break;
			case ':':
			case '?':
				ret_flag = false;
				__fallthrough__;
			case EOF:
				loop_flag = false;
				break;
		}
	}
	return ret_flag;
}

/* ��⠭���� ����஥� �� 㬮�砭�� */
static bool set_term_defaults(void)
{
	cfg.gaddr = 0x31;
	cfg.iaddr = 0x21;
#if defined __WATCH_EXPRESS__
	cfg.watch_interval = 0;
#endif
	cfg.use_iplir = false;

	cfg.has_xprn = true;
	cfg.xprn_number[0] = 0;
	cfg.has_aprn = false;
	cfg.aprn_number[0] = 0;
	cfg.has_sprn = false;
	cfg.s0 = 0;
	cfg.s1 = 0;
	cfg.s2 = 0;
	cfg.s3 = 0;
	cfg.s4 = 0;
	cfg.s5 = 0;
	cfg.s6 = 0;
	cfg.s7 = 0;
	cfg.s8 = 0;
	cfg.s9 = 0;

	cfg.use_ppp = false;
	cfg.local_ip = 0;
	cfg.x3_p_ip = 0;
	cfg.x3_s_ip = 0;
	cfg.use_p_ip = true;
	cfg.net_mask = 0;
	cfg.gateway = 0;
	cfg.tcp_cbt = true;

	cfg.ppp_tty = 0;
	cfg.ppp_speed = 115200;
	memset(cfg.ppp_init, 0, sizeof(cfg.ppp_init));
	memset(cfg.ppp_phone, 0, sizeof(cfg.ppp_phone));
	cfg.ppp_tone_dial = false;
	cfg.ppp_auth = ppp_chat;
	memset(cfg.ppp_login, 0, sizeof(cfg.ppp_login));
	memset(cfg.ppp_passwd, 0, sizeof(cfg.ppp_passwd));
	cfg.ppp_crtscts = true;

	cfg.bank_system = false;
	cfg.bank_pos_port = 0;
	cfg.bank_proc_ip = 0;
	cfg.ext_pos = false;

	cfg.has_kkt = false;
	cfg.fiscal_mode = false;
	cfg.fdo_iface = KKT_FDO_IFACE_USB;
	cfg.fdo_ip = 0;
	cfg.fdo_port = 0;
	cfg.fdo_poll_period = 0;
	cfg.kkt_ip = 0;
	cfg.kkt_netmask = 0;
	cfg.kkt_gw = 0;
	memset(cfg.kkt_gprs_apn, 0, sizeof(cfg.kkt_gprs_apn));
	memset(cfg.kkt_gprs_user, 0, sizeof(cfg.kkt_gprs_user));
	memset(cfg.kkt_gprs_passwd, 0, sizeof(cfg.kkt_gprs_passwd));
	cfg.tax_system = 0;
	cfg.kkt_log_level = 0;
	cfg.kkt_log_stream = KLOG_STREAM_ALL;
	cfg.tz_offs = 0;
	cfg.kkt_base_timeout = KKT_BASE_TIMEOUT * 10;
	cfg.kkt_brightness = 0;
	cfg.kkt_apc = false;

	cfg.blank_time = 0;
	cfg.color_scheme = 0;
	translate_color_scheme(cfg.color_scheme,
		&cfg.rus_color, &cfg.lat_color, &cfg.bg_color);
	cfg.scr_mode = m80x20;	//m32x8;
	cfg.has_hint = true;
	cfg.simple_calc = true;

	cfg.kbd_rate = 30;
	cfg.kbd_delay = 250;
	cfg.kbd_beeps = false;
	
	return true;
}

/* �⥭�� ����஥� �ନ���� �� 䠩�� */
static bool load_term_props(void)
{
	set_term_defaults();
	bool ret = read_cfg();
	kkt_base_timeout = cfg.kkt_base_timeout / 10;
	return ret;
}

/* �⥭�� ����஢ ���祩 DALLAS �� 䠩�� tki */
void get_dallas_keys(void)
{
	get_tki_field(&tki, TKI_SRV_KEYS, (uint8_t *)srv_keys);
	get_tki_field(&tki, TKI_DBG_KEYS, (uint8_t *)dbg_keys);
}

/* ��।������ ⨯� ���� DS1990A */
int get_key_type(void)
{
#if defined __REAL_KEYS__
	if (xprn_printing)
		return kt;
	struct md5_hash md5;
	int i;
	ds_read(dsn);
/* �᫨ ���� �� �����㦥�, ��� ����� ���������� ��ﬨ */
	for (i = 0; (i < DS_NUMBER_LEN) && (dsn[i] == 0); i++){}
	if ((i == DS_NUMBER_LEN) || !ds_hash(dsn, &md5))
		return key_none;
/* ���� ����� ���� ����஥��... */
	for (i = 0; i < NR_SRV_KEYS; i++){
		if (cmp_md5(srv_keys + i, &md5))
			return key_srv;
	}
/* ...�⫠����... */
	for (i = 0; i < NR_DBG_KEYS; i++){
		if (cmp_md5(dbg_keys + i, &md5))
			return key_dbg;
	}
/* ...���� �� ���� ����� */
	return key_reg;
#else
	return key_dbg;
#endif		/* __REAL_KEYS__ */
}

/* �������� � �ࠢ�� ��� ��ப� ����� ⨯ ���� */
static void show_key_type(void)
{
	int ast = ast_none;
	switch (kt){
		case key_reg:
			ast = ast_rkey;
			break;
		case key_srv:
			ast = ast_skey;
			break;
		case key_dbg:
			ast = ast_dkey;
			break;
	}
	set_term_astate(ast);
}

const char *find_term_state(int st)
{
	static struct {
		int st;
		char *descr;
	} states[] = {
		{st_none,		""},
		{st_input,		"������"},
		{st_wait,		"�������� �裡"},
		{st_transmit,		"��।��"},
		{st_recv,		"�ਥ�"},
		{st_wrecv,		"���� �⢥�"},
		{st_ireq,		"�����"},
		{st_resp,		"�⢥�"},
		{st_fresp,		"�㦮� �⢥�"},
		{st_scr,		"�⢥� ��"},
		{st_prn,		"�⢥� ��"},
		{st_req,		"�⢥� ��"},
		{st_keys,		"��� ���祩"},
		{st_hash,		"��� ����⠭�"},
		{st_array,		"���"},
		{st_raw_resp,		"��� �⢥�"},
		{st_log,		"����஫쭠� ����"},
		{st_xchange,		"����� � ������"},
		{st_help,		"��ࠢ��"},
		{st_ping,		"�����ਭ� ��"},
		{st_ppp_startup,	"����� PPP"},
		{st_ppp_cleanup,	"����� ��ᨨ PPP"},
		{st_ppp_init,		"���樠������ ������"},
		{st_ppp_dialing,	"������ PPP"},
		{st_ppp_login,		"���ਧ���"},
		{st_ppp_ipcp,		"���䨣���� TCP/IP"},
		{st_stop_iplir,		"���㧪� ViPNet"},
		{st_xprn_grids,		"�����⪨ ���"},
		{st_xprn_icons,		"���⮣ࠬ�� ���"},
		{st_kkt_grids,		"�����⪨ ���"},
		{st_kkt_icons,		"���⮣ࠬ�� ���"},
	};
	int i;
	static char buf[MAX_TERM_STATE_LEN+1];
	for (i = 0; i < ASIZE(states); i++){
		if (st == states[i].st){
			if (st == st_input)
				snprintf(buf, sizeof(buf), "%s %d",
					states[i].descr, cur_window);
			else
				snprintf(buf, sizeof(buf), "%s",
					states[i].descr);
			return buf;
		}
	}
	return NULL;
}

bool set_term_state(int st)
{
	const char *str;
	if ((st == st_resp) && TST_FLAG(ZBp, GDF_REQ_FOREIGN))
		st = st_fresp;
	str = find_term_state(st);
	if (str != NULL){
		_term_state = st;
		scr_set_lstatus(str);
		return true;
	}else
		return false;
}

const char *find_term_astate(intptr_t ast)
{
	static struct astate_entry {
		intptr_t ast;
		char *descr;
	} astates[] = {
		{ast_none,		""},
		{ast_noxprn,		"��� ��⮢���� ���"},
		{ast_noaprn,		"��� ��⮢���� ���"},
		{ast_nosprn,		"��� ��⮢���� ���"},
		{ast_sprn_err,		"�訡�� ���"},
		{ast_sprn_ch_media,	"������� ����� � ���"},
		{ast_rejected,		"�⢥� �� �ਭ��"},
		{ast_repeat,		"������ �����"},
		{ast_resp,		"�������� ������"},
		{ast_prn,		"������"},
		{ast_req,		"�����"},
		{ast_nokey,		"��� ����"},
		{ast_init,		"���樠������"},
		{ast_finit,		"���樠������"},
		{ast_rkey,		"����稩 ����"},
		{ast_skey,		"��⠭���� ��ࠬ��஢"},
		{ast_dkey,		"�⫠���� ����"},
		{ast_no_log_item,	"������ �� �������"},
		{ast_illegal,		"����饭�� ०��"},
		{ast_error,		"������"},
		{ast_prn_number,	"������ ����� �ਭ��"},
		{ast_pos_error,		"�訡�� ���"},
		{ast_pos_need_init,	"�ந��樠������� ���"},
		{ast_no_kkt,		"��� �� �����㦥��"},
	};
	struct astate_entry *p;
	int i;
	static char buf[MAX_TERM_ASTATE_LEN + 2];	/* ��� ���� 0x01 */
	const char *ret = NULL;
	bool flag = false;
	for (i = 0; i < ASIZE(astates); i++){
		p = &astates[i];
		if (ast == p->ast){
			switch (ast){
				case ast_sprn_err:
					if (lprn_status != 0){
						snprintf(buf, sizeof(buf),
							"%s: %.2hhX",
							p->descr, lprn_status);
						flag = true;
					}
					break;
				case ast_rejected:
					if (ecode != E_OK){
						snprintf(buf, sizeof(buf),
							"%s �:%.2X",
							p->descr, ecode);
						flag = true;
						break;
					}		/* fall through */
				case ast_repeat:
				case ast_finit:
					if (session_error != SLAYER_OK){
						if (session_error >= PPPERR_BASE)
							snprintf(buf, sizeof(buf),
								"%s �:%.2d",
								p->descr, session_error);
						else if (session_error >= TCPERR_BASE)
							snprintf(buf, sizeof(buf),
								"%s �:%.2d",
								p->descr, session_error);
						else if (session_error == SERR_SPECIAL)
							snprintf(buf, sizeof(buf),
								"%s �:%.2s",
								p->descr, resp_buf + 13);
						else
							snprintf(buf, sizeof(buf),
								"%s �:%.2d",
								p->descr, session_error);
						flag = true;
					}
					break;
				case ast_error:
					if (ecode != E_OK){
						snprintf(buf, sizeof(buf),
							"%s �:%.2X",
							p->descr, ecode);
						flag = true;
					}
					break;
				case ast_pos_error:
					if (pos_err_xdesc != NULL){
						snprintf(buf, sizeof(buf),
							"%s %s",
							p->descr, pos_err_xdesc);
						flag = true;
					}
					break;
			}
			if (!flag)
				snprintf(buf, sizeof(buf), "%s", p->descr);
			ret = buf;
		}
	}
	if ((ret == NULL) && (ast > ast_max)){
		snprintf(buf, sizeof(buf), "%s", (const char *)ast);
		ret = buf;
	}
	return ret;
}

bool set_term_astate(intptr_t ast)
{
	const char *str = NULL;
	if ((str = find_term_astate(ast)) != NULL){
		_term_aux_state=ast;
		scr_set_rstatus(str);
		return true;
	}else
		return false;
}

bool set_term_led(char c)
{
	return scr_set_led(recode(c));
}

/* �뢮� � ��ப� ����� ���ன�⢠ ��� ⥪�饣� ����� */
void show_dest(int dst)
{
	switch (dst){
		case dst_text:
			set_term_state(st_scr);
			break;
		case dst_xprn:
		case dst_sprn:
		case dst_kprn:
		case dst_aprn:
			set_term_state(st_prn);
			break;
		case dst_out:
			set_term_state(st_req);
			break;
		default:
			set_term_state(st_resp);
		}
}

/* �뢮� � ��ப� ����� ���ன�⢠ ��� ᫥���饣� ����ࠡ�⠭���� ����� */
void show_ndest(int n)
{
	struct para_info *p=NULL;
	if (n >= n_paras)
		return;
	p=&map[n];
	if (!p->handled){	/* ����� �� ��ࠡ�⠭ */
		switch (p->dst){
			case dst_text:
			case dst_keys:
			case dst_hash:
			case dst_kkt:
			case dst_log:
				set_term_astate(ast_resp);
				astate_for_req = ast_resp;
				break;
			case dst_xprn:
			case dst_sprn:
			case dst_kprn:
			case dst_aprn:
				set_term_astate(ast_prn);
				astate_for_req = ast_prn;
				break;
			case dst_out:
			case dst_qout:
				set_term_astate(ast_req);
				astate_for_req = ast_req;
				break;
		}
	}
}

bool set_term_busy(bool busy)
{
	term_busy = busy;
	if (busy){
		if (cfg.tcp_cbt)
			release_term_socket();
	}
	return true;
}

/* ���ଠ�� � ⥪�饬 ���ﭨ� �ନ���� */
struct term_info{
	int st;		/* st_* */
	intptr_t ast;	/* ast_* */
	int m;
	bool scr_visible;
	bool cursor_visible;
	bool term_busy;
};

/* �⥪ ���ﭨ� �ନ���� */
static struct term_info	tstack[10];
static int tstack_ptr;

bool push_term_info(void)
{
	if (tstack_ptr < ASIZE(tstack)){
		struct term_info *ti=&tstack[tstack_ptr++];
		ti->st=_term_state;
		ti->ast=_term_aux_state;
		ti->m=cur_mode;
		ti->scr_visible=scr_visible;
		ti->cursor_visible=cursor_visible;
		ti->term_busy=term_busy;
		return true;
	}else
		return false;
}

bool pop_term_info(void)
{
	if (tstack_ptr > 0){
		struct term_info *ti=&tstack[--tstack_ptr];
		_term_state=ti->st;
		_term_aux_state=ti->ast;
		set_scr_mode(ti->m,false,false);
		scr_visible=ti->scr_visible;
		set_cursor_visibility(ti->cursor_visible);
		set_term_busy(ti->term_busy);
		return true;
	}else
		return false;
}

/* �㭪樨-"����⪨" ��� �ᮢ���� �� */
static bool draw_xlog(void)
{
	return log_draw(xlog_gui_ctx);
}

static bool draw_plog(void)
{
	return log_draw(plog_gui_ctx);
}

static bool draw_klog(void)
{
	return log_draw(klog_gui_ctx);
}


/* ������ ����ᮢ�� �ନ���� */
void redraw_term(bool show_text, const char *title)
{
/*
 * ������ ����ᮢ�� ��࠭��� ������⮢. �᫨ �� ��࠭� �����६����
 * ��������� ��᪮�쪮 ������⮢, � �������, ����� �ॡ���� ����ᮢ���
 * �����, ������ �ᯮ�������� ����� � ����� ���ᨢ�.
 */
	static struct scr_elem{
		bool *active;
		bool (*draw_fn)(void);
	} scr_elems[]={
		{&optn_active,	draw_options},
		{&help_active,	draw_help},
		{&xlog_active,	draw_xlog},
		{&plog_active,	draw_plog},
		{&klog_active,	draw_klog},
		{&xchg_active,	draw_xchange},
		{&calc_active,	draw_calc},
		{&ping_active,	draw_ping},
		{&pos_active,	pos_screen_draw},
		{&fa_active,	draw_fa},
	};
	int i;
	bool v=scr_visible;
	scr_visible=true;
	draw_scr(show_text, title);
	set_term_state(_term_state);
	set_term_astate(_term_aux_state);
	set_term_led(hbyte);
	scr_visible=v;
	for (i=0; i < ASIZE(scr_elems); i++)
		if (*scr_elems[i].active)
			scr_elems[i].draw_fn();
	if (menu_active)
		draw_menu(mnu);
}

/* ����⠭������� ��࠭� ��᫥ ��襭�� */
void scr_wakeup(void)
{
	kbd_reset_idle_interval();
	release_ssaver();
	pop_term_info();
	redraw_term(true, main_title);
}

/* �����⨥ ���㦭�� ���� �� ��� */
void release_garbage(void)
{
	if (menu_active){
		release_menu(mnu, true);
		mnu = NULL;
		ClearScreen(clBtnFace);
	}
	if (xchg_active)
		release_xchg_view();
	if (help_active)
		release_help();
	if (optn_active)
		release_options(true);
	if (xlog_active)
		log_release_view(xlog_gui_ctx);
	if (plog_active)
		log_release_view(plog_gui_ctx);
	if (klog_active)
		log_release_view(klog_gui_ctx);
	if (calc_active) {
		if (cfg.simple_calc)
			release_calc();
		else
			adv_release_calc();
	}
	if (ping_active)
		release_ping();
	if (pos_active){
		pos_error_clear();
		pos_set_state(pos_break);
		pos_active = false;
	}else
		pos_set_state(pos_new);
	if (fa_active)
		release_fa();
	end_message_box();
	if (ssaver_active)
		scr_wakeup();
	while (tstack_ptr > 0)
		pop_term_info();
}

/* ����祭�� ��������� �������� ���� �ନ���� */
char *get_main_title(void)
{
	snprintf(main_title, sizeof(main_title), MAIN_TITLE " ("
		_s(STERM_VERSION_MAJOR) "."
		_s(STERM_VERSION_MINOR) "."
		_s(STERM_VERSION_RELEASE) "  %s %s) [%s]",
		__DATE__, __TIME__,
		use_integrator ? "����������" : "����");
	return main_title;
}

#if 0
/* �뢮� �� ��࠭ ᮮ�饭�� �� �訡�� �⥭�� �����᪮�� ����� ��� */
static void show_lprn_nonumber_error(void)
{
	online = false;
	guess_term_state();
	push_term_info();
	hide_cursor();
	scr_visible = false;
	set_term_busy(true);
	ClearScreen(clBlack);
	err_beep();
	message_box("������ ���", "��� ��ࠡ��ᯮᮡ��.\n"
		"�ॡ���� ������ �ਭ��.", dlg_yes, DLG_BTN_YES, al_center);
	online = true;
	pop_term_info();
	ClearScreen(clBtnFace);
	redraw_term(true, main_title);
}

/* ���樠������ ��� */
static bool init_lprn(void)
{
	bool flag = false;
	int ret;
	if ((ret = lprn_get_status()) != LPRN_RET_OK)
		;
	else if (cfg.has_sd_card && (lprn_sd_status > 0x01)){
		set_term_astate(ast_lprn_sd_err);
		need_lock_term = true;
	}else if (lprn_status != 0)
		set_term_astate(ast_sprn_err);
	else if ((ret = lprn_init()) != LPRN_RET_OK)
		need_lock_term = ret != LPRN_RET_RST;
	else if (lprn_status != 0){
		set_term_astate(ast_sprn_err);
		if (lprn_status == 0x01)	/* ����� ��� �� �ய�ᠭ � ����� */
			show_lprn_nonumber_error();
		need_lock_term = true;
	}else{
		flag = true;
		if (!cfg.has_sprn){
			cfg.has_sprn = true;
			write_cfg();
		}
	}
	lprn_close();
	if (!flag && (ret != LPRN_RET_RST)){
		err_beep();
		if (cfg.has_sprn){
			cfg.has_sprn = false;
			write_cfg();
		}
	}
	return flag;
}
#endif

static bool is_kkt(const struct dev_info *dev)
{
	if ((dev == NULL) || (dev->type != DEV_KKT) || strcmp(kkt->name, "������-�"))
		return false;
	uint32_t sn = get_dev_param_uint(dev, "SN-FISCAL");
	if (sn == UINT32_MAX)
		return false;
	uint32_t year = sn / 10000000, mon = (sn / 100000) % 100, nr = sn % 100000;
	if ((year < 18) || (mon < 1) || (mon > 12) || (nr == 0))
		return false;
	const char *ver = get_dev_param_str(dev, "VERSION");
	if (ver == NULL)
		return false;
	uint32_t v[3] = {[0 ... ASIZE(v) - 1] = 0};
	char c = 0;
	return sscanf(ver, "%u.%u.%u%c", v, v + 1, v + 2, &c) == 3;
}

/* ���४�஢�� ����஥� ��� � ᮮ⢥��⢨� � ���ଠ樥�, ����祭��� �� ���ன�⢠ */
static bool adjust_kkt_cfg(const struct dev_info *kkt)
{
	if (kkt == NULL)
		return false;
	cfg.fdo_iface = get_dev_param_uint(kkt, KKT_FDO_IFACE);
	if (cfg.fdo_iface == UINT32_MAX)
		cfg.fdo_iface = KKT_FDO_IFACE_USB;
	cfg.fdo_ip = get_dev_param_ip(kkt, KKT_FDO_IP);
	cfg.fdo_port = get_dev_param_uint(kkt, KKT_FDO_PORT);
	cfg.kkt_ip = get_dev_param_ip(kkt, KKT_ETH_IP);
	cfg.kkt_netmask = get_dev_param_ip(kkt, KKT_ETH_NETMASK);
	cfg.kkt_gw = get_dev_param_ip(kkt, KKT_ETH_GW);
	const char *s = get_dev_param_str(kkt, KKT_GPRS_APN);
	if (s == NULL)
		memset(cfg.kkt_gprs_apn, 0, sizeof(cfg.kkt_gprs_apn));
	else{
		snprintf(cfg.kkt_gprs_apn, sizeof(cfg.kkt_gprs_apn), "%s", s);
		recode_str(cfg.kkt_gprs_apn, -1);
	}
	s = get_dev_param_str(kkt, KKT_GPRS_USER);
	if (s == NULL)
		memset(cfg.kkt_gprs_user, 0, sizeof(cfg.kkt_gprs_user));
	else{
		snprintf(cfg.kkt_gprs_user, sizeof(cfg.kkt_gprs_user), "%s", s);
		recode_str(cfg.kkt_gprs_user, -1);
	}
	s = get_dev_param_str(kkt, KKT_GPRS_PASSWORD);
	if (s == NULL)
		memset(cfg.kkt_gprs_passwd, 0, sizeof(cfg.kkt_gprs_passwd));
	else{
		snprintf(cfg.kkt_gprs_passwd, sizeof(cfg.kkt_gprs_passwd), "%s", s);
		recode_str(cfg.kkt_gprs_passwd, -1);
	}
	uint8_t rc = kkt_get_brightness(&kkt_brightness);
	if (rc != KKT_STATUS_OK)
		kkt_brightness.current = kkt_brightness.def = kkt_brightness.max;
	cfg.kkt_brightness = kkt_brightness.current;
	char fs_nr[KKT_FS_NR_LEN + 1];
	kkt_get_fs_nr(fs_nr);
	struct serial_settings *ss = (struct serial_settings *)&kkt->ss;
	int fc0 = ss->control;
	uint32_t fc = get_dev_param_uint(kkt, "FLOW_CONTROL");
	switch (fc){
		case 0:
			ss->control = SERIAL_FLOW_NONE;
			break;
		case 1:
			ss->control = SERIAL_FLOW_XONXOFF;
			break;
		case 2:
			ss->control = SERIAL_FLOW_RTSCTS;
			break;
	}
	if (!is_kkt(kkt) || (ss->control != fc0))
		serial_configure2(0, &kkt->ss);
	return write_cfg();
}

/* �������� ���ଠ樨 �� ���訢����� ���ன�⢠� */
static void release_devices(void)
{
	if (devices != NULL){
		kkt_release();
		free_dev_lst(devices);
		devices = NULL;
	}
	kkt = NULL;
}

/* ����祭�� ���ଠ樨 �� ���訢����� ���ன�⢠� */
static void init_devices(void)
{
	fdo_suspend();
	release_devices();
	devices = poll_devices();
	nosound();
	if (devices != NULL){
		kkt = get_dev_info(devices, DEV_KKT);
		if (kkt != NULL){
			kkt_init(kkt);
			adjust_kkt_cfg(kkt);
//			test_kkt();
		}
	}
#if defined __FAKE_KKT__
	if (kkt == NULL)
		kkt = (const struct dev_info *)1;
#endif		/* __FAKE_KKT__ */
	fdo_resume();
}

/* ���樠������ �ନ���� */
static void init_term(bool need_init)
{
	bool flag = xlog_active || plog_active || klog_active;
	set_log_lvl(Info);
	can_reject = false;
	err_ptr = NULL;
	set_term_state(st_stop_iplir);
	iplir_stop();
	set_term_state(st_none);
	if (!bank_ok)
		cfg.bank_system = false;
	if (cfg.has_aprn)
		aprn_init();
	else
		aprn_release();
	lprn_close();
#if defined __WATCH_EXPRESS__
	watch_transaction = false;
#endif
	online = true;
	set_term_busy(false);
	release_garbage();
	rejecting_req = false;
	c_state = cs_ready;
	tstack_ptr = 0;
	use_integrator = false;
	if (flag)
		ClearScreen(clBtnFace);
	reset_scr();
	clear_hash(prom);
	set_hash_defaults(rom);
#if !defined __USE_NFSROOT__
	ifdown("eth0");
#endif
/*	ppp_hangup();
	ifdown("ppp0");*/
#if !defined __USE_NFSROOT__
	if (!cfg.use_ppp){
		ifup("eth0");
		sleep(3);
	}
#endif
	kbd_set_rate(cfg.kbd_rate, cfg.kbd_delay);
	set_scr_mode(cfg.scr_mode, false, true);
	if (scr_visible)
		draw_scr(true, main_title);
	set_scr_text(NULL, -1, txt_plain, scr_visible);
	set_term_state(st_input);
	show_key_type();
	set_term_led(hbyte = HB_INIT);
	reset_gd(need_init);
	mark_all();
	xprn_flush();
	aprn_flush();
	resp_handling = resp_executing = false;
	if (cfg.bank_system)
		pos_init_transactions();
	clear_bank_info();
	rollback_keys(true);
	apc = false;
	init_devices();
	update_kkt_grids();
	update_kkt_icons();
	if (cfg.use_iplir)
		iplir_start();
}

/* ���� �ନ���� � ��砫쭮� ���ﭨ� */
bool reset_term(bool force)
{
	bool ret = false;
	if (!force && (kt != key_srv) && (kt != key_dbg))
		err_beep();
	else{
		load_term_props();
		init_term(true);
		get_main_title();
		redraw_term(true, main_title);
		ret = true;
	}
	return ret;
}

/* �����誠 ��� ⠡���� ������ � process_term */
static void __reset_term(void)
{
	reset_term(false);
}

/* �뢥�� ᮮ�饭�� � ��砥 �訡�� ���祢��� USB-��᪠. */
static bool show_usb_msg(void)
{
	static bool usb_msg_shown = false;
	if (iplir_disabled && !usb_msg_shown){
		set_term_busy(true);
		online = false;
		message_box(NULL, "� ���㫥 ������᭮�� �� ������� ���祢� ���� VipNet. "
			"��ନ��� �㤥� ࠡ���� ��� VipNet � ����⮬ ०���.",
			dlg_yes, 0, al_center);
		online = true;
		set_term_busy(false);
		usb_msg_shown = true;
		return true;
	}else
		return false;
}

static bool open_log(struct log_handle *hlog)
{
	bool ret = true;
	printf("����⨥ %s...", hlog->log_type);
	fflush(stdout);
	if (log_open(hlog, true))
		printf("ok.\n");
	else{
		fprintf(stderr, "�訡�� ������ %s.\n", hlog->log_type);
		ret = false;
	}
	return ret;
}

static inline bool open_logs(void)
{
	return	open_log(hxlog) && (!bank_ok || open_log(hplog)) && open_log(hklog);
}

static bool create_term(void)
{
	if (!read_tki(STERM_TKI_NAME, false))
		return false;
	set_sigterm_handler();
	load_term_props();
#if defined __REAL_KEYS__
	if (!ds_init()){
		fprintf(stderr, "�訡�� ���樠����樨 ��⮭�� DS1990A.\n");
		return false;
	}
#endif
	if (!init_ppp_ipc()){
		fprintf(stderr, "�訡�� ���樠����樨 PPP.\n");
		return false;
	}
	if (!pos_create()){
		fprintf(stderr, "�訡�� ���樠����樨 ���.\n");
		return false;
	}
	clear_bank_info();
	init_keys();
	rom = create_hash(ROM_BUF_LEN);
	if (rom == NULL){
		fprintf(stderr, "�訡�� ᮧ����� ��� ����⠭�.\n");
		return false;
	}
	prom = create_hash(PROM_BUF_LEN);
	if (prom == NULL){
		fprintf(stderr, "�訡�� ᮧ����� ���.\n");
		return false;
	}
	check_tki();
	check_usb_bind();
	check_iplir_bind();
	if (!tki_ok){
		fprintf(stderr, "���� ���祢�� ���ଠ樨 �ନ���� ���०���.\n");
		return false;
	}
	iplir_disabled = !(usb_ok && iplir_ok);
	if (!iplir_disabled)
		iplir_disabled = !(
			iplir_init() &&
			iplir_stop() &&
			iplir_delete_keys() &&
			iplir_install_keys() &&
			iplir_start() &&
			iplir_is_active()
		);
	get_dallas_keys();
	kt = get_key_type();
/* ��� ���४⭮� ���㧪� drviplir.o ������ ���� ����饭 iplircfg */
/*	start_iplir();*/
	check_bank_license();
	if (!open_logs())
		return false;
	fdo_init();
	/* ����㧪� ��২�� �᪠�쭮�� �ਫ������ */
	AD_load((1 << cfg.tax_system), false);
	/* ����㧪� ���ଠ樨 � ����� */
	cashier_load();
	/* ����㧪� �ࠢ�筨�� ����⮢ */
	agents_load();
	/* ����㧪� �ࠢ�筨�� ⮢�஢/ࠡ��/��� */
	articles_load();
	/* ����㧪� 祪� */
	newcheque_load();
/* FIXME: ��७��� �� � ����� ���室�饥 ���� */
	bmp_up = CreateBitmap(_("pict/pos/up.bmp"));
	bmp_down = CreateBitmap(_("pict/pos/down.bmp"));
/* � �⮣� ���� �ନ��� ���室�� � ����᪨� ०�� */
	create_scr();
	xprn_init();
	init_kbd();
	init_gd();
	get_main_title();
#if defined __LOG_LPRN__
	lprn_create_log();
#endif
	init_term(kt == key_reg);
	redraw_term(true, main_title);
	if (show_usb_msg())
		redraw_term(true, main_title);
	return true;
}

static void release_term(void)
{
	restore_sigterm_handler();
	fdo_release();
	if (devices != NULL){
		free_dev_lst(devices);
		devices = NULL;
	}
	pos_release();
	release_ppp_ipc();
	log_close(hxlog);
	log_close(hplog);
	log_close(hklog);
	release_keys();
	release_kbd();
	xprn_release();
	release_scr();
	release_gd();
	release_hash(prom);
	release_hash(rom);

	AD_destroy();
	agents_destroy();
	articles_destroy();
	newcheque_destroy();
	iplir_release();
}

/* �஢�ઠ ������ �������樨 ������ ��� ࠧ�뢠 ��������� ᮥ������� */
static bool is_hangup_key(struct kbd_event *e)
{
	return (e->shift_state & SHIFT_CTRL) && (e->key == KEY_Q);
}

static bool skip_kbd(void)
{
	struct kbd_event e;
	kbd_get_event(&e);
	if (is_hangup_key(&e))
		return false;
	else if (e.pressed && !e.repeated)
		err_beep();
	return true;
}

/* ��� ���� */
static void show_key_error(void)
{
	resp_handling = resp_executing = false;
	set_term_busy(true);
	err_beep();
	if (scr_visible){
		set_term_state(st_none);
		set_term_astate(ast_nokey);
		set_term_led(' ');
		scr_show_pgnum(false);
		scr_show_mode(false);
		scr_show_language(false);
		scr_show_log(false);
	}
}

/* ��ࠡ�⪠ ���祩 DALLAS. �����頥� false �᫨ ��� ���� ��� �㦮� ���� */
static bool process_dallas(void)
{
	bool ret = true;
	static int _kt = key_none - 1;	/* ⨯ �।��饣� ���� */
	kt = get_key_type();
	if (kt == key_none){
		if (kt != _kt){
			_kt = kt;
			show_key_error();
		}
		skip_kbd();
		ret = false;
	}else if (_kt == (key_none - 1))
		_kt = kt;
	else if (_kt != kt){
		_kt = kt;
		kbd_reset_idle_interval();
		load_term_props();
		init_term(kt == key_reg);
	}
	return ret;
}

static void handle_channel(void)
{
	bool flag;
	if (((_term_aux_state == ast_rejected) || (_term_aux_state == ast_repeat)) &&
		(_term_state == st_none))
		guess_term_state();
	flag = process_transport();
	if (!full_resp)
		full_resp = flag;
}

bool kbd_alive(struct kbd_event *e)
{
	bool ret;
	if (e == NULL)
		ret = false;
	else if ((e->key == KEY_F9) || is_hangup_key(e))
		ret = true;
	else if (s_state == ss_initializing)
		ret = false;
	else if (cfg.use_ppp && (ppp_state != ppp_nc) && (ppp_state != ppp_ready))
		ret = false;
	else
		ret = 	(c_state != cs_hasreq) && (c_state != cs_sending) &&
			(c_state != cs_wconnect) && (c_state != cs_selected) &&
			(c_state != cs_wsend);
	return ret;
}

static bool bad_repeat(struct kbd_event *e)
{
	uint16_t single_keys[] = {
		KEY_ENTER,	/* ����� */
		KEY_CAPS,	/* ���/��� */
		KEY_LSHIFT,	/* ���/��� */
		KEY_RSHIFT,	/* ���/��� */
		KEY_LALT,	/* ᫥���騩 ���� */
		KEY_RALT,	/* ᫥���騩 ���� */
		KEY_TAB,	/* �������� */
		KEY_F2,		/* ����� */
		KEY_F3,		/* ���� ���� */
		KEY_F4,		/* ���� ���� */
		KEY_F5,		/* �⪠� �� ������ */
		KEY_F6,		/* ���⪠ ⥪�饣� ��� */
		KEY_F7,		/* ��� ������ */
		KEY_F8,		/* ��� �⢥�, ᫥���騩 ����� */
		KEY_F9,		/* ��� */
		KEY_F10,	/* ��� � �� */
		KEY_F12,	/* ���� ��⭠����筮�� ᨬ���� */
		KEY_PRNSCR,	/* ��� */
		KEY_SCRLOCK,	/* ��� ����⠭� */
		KEY_BREAK,	/* ��� �⢥� (����ࠡ�⠭����) */
	};
	uint16_t ctrl_keys[] = {
		KEY_LCTRL,
		KEY_RCTRL,
		KEY_G,		/* Ctrl+� -- ����� ����� ��࠭� */
		KEY_I,		/* Ctrl+I -- ���ଠ�� � �ନ���� */
		KEY_J,		/* Ctrl+� -- ��ᬮ�� ������ � ������ */
		KEY_K,		/* Ctrl+� -- �� "������" (��1) */
		KEY_L,		/* Ctrl+L -- ���ଠ�� � ���ᨨ VipNet-������ */
		KEY_P,		/* Ctrl+P -- ping */
		KEY_Q,		/* Ctrl+Q -- ࠧ�� ᮥ������� PPP */
		KEY_R,		/* Ctrl+� -- ��� ���祩 */
		KEY_S,		/* Ctrl+S -- �᭮����/�ਣ�த�� ०�� */
		KEY_T,		/* Ctrl+� -- �訡�� � ⥪�� �⢥� */
		KEY_U,		/* Ctrl+U -- ����� ��࠭��� ��ࠧ�� ������� �� ��� */
		KEY_X,		/* Ctrl+X -- ��� (��2) */
		KEY_Z,		/* Ctrl+Z -- ����祭�� ����� ��� */
	};
	uint16_t *pp = single_keys;
	int l = ASIZE(single_keys), i;
	if (!e->pressed || !e->repeated)
		return false;
	if (e->shift_state & SHIFT_CTRL){
		pp = ctrl_keys;
		l = ASIZE(ctrl_keys);
	}
	for (i = 0; i < l; i++)
		if (e->key == pp[i])
			return true;
	return false;
}

static void show_raw_resp(void);

/* �᭮���� ��ࠡ��稪 ���������� */
static int handle_kbd(struct kbd_event *e, bool check_scr, bool busy)
{
	static struct {
		uint16_t key;
		int cm;
	} ctrl_keys[] = {
		{KEY_G, cmd_snap_shot},		/* ����� ����� ��࠭� */
		{KEY_H, cmd_view_klog},		/* ��ᬮ�� ��� */
		{KEY_I, cmd_term_info},		/* ���ଠ�� � �ନ���� */
		{KEY_J, cmd_view_xchange},	/* ��ᬮ�� ������ � ������ */
		{KEY_K, cmd_view_xlog},		/* ��ᬮ�� ��� */
		{KEY_L, cmd_iplir_version},	/* ���ଠ�� � ���ᨨ VipNet-������ */
		{KEY_O, cmd_fa},		/* �᪠�쭮� �ਫ������ */
		{KEY_P, cmd_ping},		/* ping */
		{KEY_Q,	cmd_ppp_hangup},	/* ࠧ�� ᮥ������� PPP */
		{KEY_R, cmd_view_keys},		/* ���� */
		{KEY_T, cmd_view_error},	/* �訡�� � ⥪�� �⢥� */
		{KEY_X, cmd_view_plog},		/* ��ᬮ�� ��� */
		{KEY_Z,	cmd_ticket_number},	/* �⥭�� ����� ��� � �ਣ�த��� ०��� */
//		{KEY_COMMA, cmd_pos},		/* �맮� POS-�ନ���� */
		{KEY_F10, cmd_exit},		/* ��室 */
	},
	keys[] = {
		{KEY_TAB, cmd_calculator},	/* ०�� �������� */
		{KEY_BREAK, cmd_view_raw_resp},	/* ��ᬮ�� ����ࠡ�⠭���� �⢥� */
		{KEY_SCRLOCK, cmd_view_rom},
		{KEY_PRNSCR, cmd_view_prom},
		{KEY_F1, cmd_help},		/* �ࠢ�� */
		{KEY_F2, cmd_print},		/* ����� */
		{KEY_F5, cmd_reject},		/* �⪠� �� ������ */
		{KEY_F7, cmd_view_req},		/* ��ᬮ�� ������ */
		{KEY_F8, cmd_view_resp},	/* ��ᬮ�� �⢥� */
		{KEY_F9, cmd_reset},		/* ��� �ନ���� */
		{KEY_F10, cmd_kkt_info},	/* ��� � �� */
		{KEY_F11, cmd_options},		/* ��⠭���� ��ࠬ��஢ */
	},
	ctrl_shift_keys[] = {
		{KEY_O, cmd_cheque_fa},		/* ����� 祪� */
	};
	int i;
	if ((e->key == KEY_NONE) || bad_repeat(e))
		return cmd_none;
	else if (busy || !kbd_alive(e)){
		if (e->pressed && !e->repeated)
			err_beep();
		return cmd_none;
	}else if (e->pressed){
/* �⫠���� ������ */
		if ((e->shift_state & SHIFT_RCTRL) &&
				!(e->shift_state & SHIFT_LCTRL) &&
				(e->shift_state & SHIFT_LSHIFT) &&
				!(e->shift_state & SHIFT_RSHIFT)){
			if (e->key == KEY_S)
				return cmd_shell;
		}
		if (e->shift_state & SHIFT_CTRL){
			if (e->shift_state & SHIFT_SHIFT){
				for (i = 0; i < ASIZE(ctrl_shift_keys); i++)
					if (e->key == ctrl_shift_keys[i].key)
						return ctrl_shift_keys[i].cm;
			}
			for (i = 0; i < ASIZE(ctrl_keys); i++)
				if (e->key == ctrl_keys[i].key)
					return ctrl_keys[i].cm;
		}else{
			if (resp_executing){
				if (e->key == KEY_PGUP)
					return cmd_pgup;
				else if (e->key == KEY_PGDN)
					return cmd_pgdn;
			}
			for (i = 0; i < ASIZE(keys); i++)
				if (e->key == keys[i].key)
					return keys[i].cm;
			if (e->key == KEY_ENTER){
				if (scr_is_req() || resp_executing)
					return cmd_enter;
				else
					return cmd_view_req;
			}
		}
	}
	return check_scr ? scr_handle_kbd(e) : cmd_none;
}

/* ��ࠡ��稪 ��襭�� ��࠭� */
static int handle_ssaver(struct kbd_event *e)
{
	return process_ssaver(e) ? cmd_none : cmd_wakeup;
}

/* ��ࠡ��稪 ���� */
static int handle_menu(struct kbd_event *e)
{
	if (mnu == NULL)
		return cmd_none;
	if (process_menu(mnu,e) != cmd_none){
		int cm = get_menu_command(mnu);
		release_menu(mnu,true);
		mnu = NULL;
		online = !xlog_active && !plog_active && !klog_active;
		pop_term_info();
		ClearScreen(clBtnFace);
		if (cm == cmd_none){
			if (xlog_active)
				log_redraw(xlog_gui_ctx);
			else if (plog_active)
				log_redraw(plog_gui_ctx);
			else if (klog_active)
				log_redraw(klog_gui_ctx);
			else
				redraw_term(true, main_title);
		}
		return cm;
	}else
		return cmd_none;
}

static uint32_t prev_kkt_log_stream = 0;

static uint32_t kkt_log_stream_to_idx(uint32_t stream)
{
	uint32_t ret;
	switch (stream){
		case KLOG_STREAM_CTL:
			ret = 0;
			break;
		case KLOG_STREAM_PRN:
			ret = 1;
			break;
		case KLOG_STREAM_FDO:
			ret = 2;
			break;
		default:
			ret = 3;
	}
	return ret;
}

static uint32_t idx_to_kkt_log_stream(uint32_t idx)
{
	uint32_t ret;
	switch (idx){
		case 0:
			ret = KLOG_STREAM_CTL;
			break;
		case 1:
			ret = KLOG_STREAM_PRN;
			break;
		case 2:
			ret = KLOG_STREAM_FDO;
			break;
		default:
			ret = KLOG_STREAM_ALL;
	}
	return ret;
}

/* ��ࠡ��稪 ���� ����஥� �ନ���� */
static int handle_options(struct kbd_event *e)
{
/* �� �맮�� �� process_options lprn_get_params �������� ����୮� �宦����� */
	static bool in_progress = false;
	if (in_progress){
		if (e->pressed && !e->repeated)
			err_beep();
		return cmd_none;
	}
	in_progress = true;
	if (!process_options(e)){
		if (optn_cm == cmd_store_optn){
			optn_get_items(&cfg);
			cfg.kkt_log_stream = idx_to_kkt_log_stream(cfg.kkt_log_stream);
#if defined INSERT_SPRN_CODE_HERE
			if ((wm == wm_local) && lprn_params_read &&
					(lprn_set_params(&cfg) == LPRN_RET_ERR)){
				ClearScreen(clBlack);
				err_beep();
				message_box("������ ���", "�� 㤠���� ��।��� "
					"��ࠬ���� ࠡ��� � ���.",
					dlg_yes, DLG_BTN_YES, al_center);
			}
#endif		/* INSERT_SPRN_CODE_HERE */
		}else
			cfg.kkt_log_stream = idx_to_kkt_log_stream(cfg.kkt_log_stream);
		release_options(true);
		online = true;
		pop_term_info();
		s_state = prev_s_state;
		if (optn_cm == cmd_store_optn){
			scr_visible = false;
			write_cfg();
			if (prev_kkt_log_stream != cfg.kkt_log_stream){
				log_close(hklog);
				open_log(hklog);
			}
			if (cfg.has_kkt && (kkt != NULL)){
				uint8_t rc = kkt_set_cfg();
/*
 * NB: ����� �।����������, �� ��᫥���� ������� ࠡ��� � ��� � ��砥 �ᯥ�
 * �����頥� KKT_STATUS_OK, � �� KKT_STATUS_OK2.
 */
				if (rc != KKT_STATUS_OK){
					err_beep();
					message_box("������ ���", "�� 㤠���� ��।��� "
						"��ࠬ���� ࠡ��� � KKT.",
						dlg_yes, DLG_BTN_YES, al_center);
				}
			}
			init_term(false);
			scr_visible = true;
			show_cursor();
		}
		redraw_term(true, main_title);
	}
	in_progress = false;
	return cmd_none;
}

/* ��ࠡ��稪 �������� */
static int handle_calculator(struct kbd_event *e)
{
	bool b = (cfg.simple_calc) ? process_calc(e) : adv_process_calc(e);
	if (!b){
		if (cfg.simple_calc)
			release_calc();
		else
			adv_release_calc();
		online = true;
		pop_term_info();
		redraw_term(true, main_title);
	}
	return cmd_none;
}

/* ��ࠡ��稪 ��ᬮ�� ��� */
static int handle_xlog(struct kbd_event *e)
{
	int cm = log_process(xlog_gui_ctx, e);
	if (cm == cmd_exit){
		log_release_view(xlog_gui_ctx);
		online = true;
		pop_term_info();
		ClearScreen(clBtnFace);
		redraw_term(true, main_title);
		if (xlog_can_write(hxlog))
			set_term_astate(ast_none);
		return cmd_none;
	}
	return cm;
}

/* ��ࠡ��稪 ��ᬮ�� ��� */
static int handle_plog(struct kbd_event *e)
{
	int cm = log_process(plog_gui_ctx, e);
	if (cm == cmd_exit){
		log_release_view(plog_gui_ctx);
		online = true;
		pop_term_info();
		ClearScreen(clBtnFace);
		redraw_term(true, main_title);
		set_term_astate(ast_none);
		return cmd_none;
	}
	return cm;
}

/* ��ࠡ��稪 ��ᬮ�� ��� */
static int handle_klog(struct kbd_event *e)
{
	int cm = log_process(klog_gui_ctx, e);
	if (cm == cmd_exit){
		log_release_view(klog_gui_ctx);
		online = true;
		pop_term_info();
		ClearScreen(clBtnFace);
		redraw_term(true, main_title);
		set_term_astate(ast_none);
		fdo_resume();
		return cmd_none;
	}
	return cm;
}

/* ��ࠡ��稪 ��ᬮ�� ������ */
static int handle_xchange(struct kbd_event *e)
{
	int cm = process_xchange(e);
	if (cm == cmd_exit){
		release_xchg_view();
		online = true;
		pop_term_info();
		redraw_term(true, main_title);
		if ((hbyte < HB_ERROR) && (hbyte > HB_OK)){	/* ��ࠡ�⪠ �⢥� */
			set_scr_text(text_buf,-1,txt_rich,true);
			return cmd_view_resp;
		}else
			return cmd_none;
	}
	return cm;
}

/* ��ࠡ��稪 ���� �ࠢ�� */
static int handle_help(struct kbd_event *e)
{
	if (!process_help(e)){
		release_help();
		online = true;
		pop_term_info();
		redraw_term(true, main_title);
	}
	return cmd_none;
}

/* ��ࠡ��稪 ���� �஢�ઠ �� */
static int handle_ping(struct kbd_event *e)
{
	if (!process_ping(e)){
		release_ping();
		online = true;
		pop_term_info();
		redraw_term(true, main_title);
	}
	return cmd_none;
}

static void show_pos(void);
static void show_cheque_fa(void);

/* �����襭�� ࠡ��� ������᪮�� �ਫ������ */
static void on_end_pos(void)
{
	bool reset_bi = true;
	pos_active = false;
	online = true;
	pop_term_info();
	if (apc){
		static struct custom_btn _btns[] = {
			{
				.text	= "�ᯥ�� ����",
				.cmd	= DLG_BTN_YES,
			},
			{
				.text	= "����� ���",
				.cmd	= DLG_BTN_RETRY,
			},
			{
				.text	= "�⪠� �� ����",
				.cmd	= DLG_BTN_CANCEL,
			},
			{
				.text	= NULL,
				.cmd	= 0,
			},
		};
		struct custom_btn *btns = (pos_err_xdesc == NULL) ? _btns : _btns + 1;
		hide_cursor();
		int rc = message_box("�������������� ������ ����",
			"�롥�� ���쭥�襥 ����⢨�", (intptr_t)btns, 0, al_center);
		show_cursor();
		switch (rc){
			case DLG_BTN_YES:
				show_cheque_fa();
				apc = fa_active;
				break;
			case DLG_BTN_RETRY:
				reset_bi = false;
/* FIXME: � �⮬ ��砥 �� ����砭�� ࠡ��� � ��� �� �� ���뫠�� INIT CHECK (pos_new) */
				if (pos_get_state() == pos_finish)
					pos_set_state(pos_idle);
				show_pos();
				apc = pos_active;
				break;
			default:
				apc = false;
		}

	}
	if (!apc && (pos_err_xdesc != NULL))
		set_term_astate(ast_pos_error);
	if (reset_bi)
		clear_bank_info();
	if (!apc)
		redraw_term(true, main_title);
}

/* ��ࠡ��稪 ���� POS-�ନ���� */
static int handle_pos(struct kbd_event *e)
{
	switch (pos_get_state()){
		case pos_ready:
			pos_screen_process(e);
			break;
		case pos_enter:
		case pos_print:
		case pos_printing:
		case pos_err:
		case pos_err_out:
		case pos_ewait:
			break;
		default:
			on_end_pos();
	}
	return cmd_none;
}

/* ��ࠡ��稪 �᪠�쭮�� �ਫ������ */
static int handle_fa(struct kbd_event *e)
{
	if (!process_fa(e)){
		release_fa();
		online = true;
		pop_term_info();
		scr_visible = true;
		show_cursor();
		apc = false;
		redraw_term(true, main_title);
	}

	return cmd_none;
}

/* ��襭�� ��࠭� */
static void blank_screen(void)
{
	if (!ssaver_active){
		push_term_info();
		init_ssaver();
	}
}

/* �஢�ઠ ���������� ��襭�� ��࠭� */
static bool check_screen_blank(uint32_t idle)
{
	if ((cfg.blank_time > 0) && !ssaver_active &&
			(idle > cfg.blank_time * 60) && online){
		blank_screen();
		return true;
	}else
		return false;
}

/* ����祭�� ������� �ନ���� */
int get_cmd(bool check_scr, bool busy)
{
	static struct {
		bool *active;
		int (*handler)(struct kbd_event *);
	} elems[] = {
		{&ssaver_active,	handle_ssaver},
		{&menu_active,		handle_menu},
		{&optn_active,		handle_options},
		{&calc_active,		handle_calculator},
		{&xlog_active,		handle_xlog},
		{&plog_active,		handle_plog},
		{&klog_active,		handle_klog},
		{&xchg_active,		handle_xchange},
		{&help_active,		handle_help},
		{&ping_active,		handle_ping},
		{&pos_active,		handle_pos},
		{&fa_active,		handle_fa},
	};
	int i;
	struct kbd_event e;
	int cm = cmd_none;
	uint32_t idle = kbd_idle_interval();
	if (sigterm_caught){
		ret_val = RET_SIGTERM;
		return cmd_exit;
	}
	process_scr();
	if (!process_dallas())
		return cmd_none;
	if (cfg.bank_system)
		pos_process();
	handle_channel();
#if defined __WATCH_EXPRESS__
	if ((cfg.watch_interval != 0) && (idle >= cfg.watch_interval) && online){
		kbd_reset_idle_interval();
		return cmd_watch_req;
	}
#endif
	if (check_screen_blank(idle))
		return cmd_none;
	kbd_get_event(&e);
	if (e.pressed){
#if defined __WATCH_EXPRESS__
		if (watch_crying){
			nosound();
			watch_crying = false;
		}
#endif
		if (!e.repeated && (e.key == KEY_F9))
			return cmd_reset;
	}
	for (i = 0; i < ASIZE(elems); i++){
		if (*elems[i].active){
			cm = elems[i].handler(&e);
			if (cm == cmd_wakeup){
				scr_wakeup();
				cm = cmd_none;
			}
			return cm;
		}
	}
	return handle_kbd(&e, check_scr, busy);
}

/* �������� ��� ������ */
void show_req(void)
{
	set_scr_text(NULL, 1600, txt_plain, true);
	if (resp_executing){
		set_term_state(st_ireq);
		if (quick_astate(_term_aux_state))
			set_term_astate(astate_for_req);
	}else{
		set_term_state(st_input);
		if (!is_rstatus_error_msg())
			set_term_astate(ast_none);
	}
}

/* �⪠� �� ������ */
void reject_req(void)
{
	if (can_reject){
		rejecting_req = true;
		mark_all();
		show_req();
		set_term_astate(ast_none);
/*		if (cfg.bank_system)
			rollback_bank_info();*/		/* FIXME */
		reject();
		xlog_write_rec(hxlog, NULL, 0, XLRT_REJECT, log_para++);
	}else{
		set_term_astate(ast_illegal);
		err_beep();
	}
}

/* ��ࠢ�� ����� */
void send_request(void)
{
	if (full_resp || !can_send_request()){
		err_beep();
		return;
	}else if (s_state == ss_finit){
		s_state = ss_new;
		return;
	}
	req_type = req_regular;
	flush_resp_mode();
	resp_handling = false;
	if (delayed_init)
		s_state = ss_new;
	else if (rejecting_req)
		reject_req();
	else if (!prn_numbers_ok()){
		set_term_astate(ast_prn_number);
		err_beep();
	}else{
		req_len = get_req_offset();
		req_len += get_scr_text(req_buf + req_len, get_max_req_len(req_len));
		wrap_text();
		set_term_astate(ast_none);
	}
}

#if defined __WATCH_EXPRESS__
static void send_watch_request(void)
{
	static char *req = "P55K02I";
	int l = strlen(req);
	if (watch_transaction){
		init_term(true);
		beep(500, 0);	/* � ����� �������� ��������� ॢ� :-) */
		watch_crying = true;
	}else if (delayed_init)
		s_state = ss_new;
	else{
		req_len = get_req_offset();
		memcpy(req_buf + req_len, req, l);
		req_len += l;
		wrap_text();
	}
	watch_transaction = true;
}
#endif

/* �������� ��� ����ࠡ�⠭���� �⢥� */
static void show_raw_resp(void)
{
	set_scr_text(resp_buf,RESP_BUF_LEN,txt_plain,true);
	set_term_state(st_raw_resp);
	set_term_astate(ast_none);
}

/* �������� ���� ᨭ⠪��᪮� �訡�� � ⥪�� �⢥� */
void show_error(void)
{
	if (err_ptr != NULL){
		show_raw_resp();
		scr_goto_char(err_ptr);
		set_term_state(st_none);
		set_term_astate(into_on_response ? ast_rejected : ast_error);
	}
}

/* �������� ��� �⢥� */
static void show_resp(void)
{
	if (check_raw_resp()){
		resp_handling = true;
		if (n_unhandled() > 0)
			return (void)execute_resp();
	}
	if (resp_handling && (n_paras > 0)){
		int n = 0;
		if (!scr_is_resp())
			cur_para=-1;
		do{
			cur_para++;
			cur_para %= n_paras;
			n++;
			if (can_show(map[cur_para].dst)){
				int l = handle_para(cur_para);
				if (map[cur_para].scr_mode == m_undef)
					set_scr_text(text_buf, l, txt_rich, true);
				else{
					set_scr_text(text_buf, l, txt_rich, false);
					set_scr_mode(map[cur_para].scr_mode, true, false);
				}
				show_dest(map[cur_para].dst);
				set_term_astate(ast_none);
				return;
			}
		}while (n < n_paras);
	}
	err_beep();
}

/* �������� ��� ����⠭� */
static void show_rom(void)
{
	if (kt == key_dbg){
		set_scr_text(rom->data, ROM_BUF_LEN, txt_plain, true);
		set_term_state(st_hash);
		set_term_astate(ast_none);
	}else
		err_beep();
}

/* �������� ��� */
static void show_prom(void)
{
	if (kt == key_dbg){
		set_scr_text(prom->data, PROM_BUF_LEN, txt_plain, true);
		set_term_state(st_array);
		set_term_astate(ast_none);
	}else
		err_beep();
}

/* �������� ��� ���祩 */
static void show_keys(void)
{
	if (kt == key_dbg){
		set_scr_text(keys, KEY_BUF_LEN, txt_plain, true);
		set_term_state(st_keys);
		set_term_astate(ast_none);
	}else
		err_beep();
}

/* �������� ����� � ������ */
static void show_xchange(void)
{
	push_term_info();
	online = false;
	init_xchg_view();
}

/* �뢮� �ࠢ�� */
static void show_help(void)
{
	if (!help_active){
		online = false;
		guess_term_state();
		push_term_info();
		init_help(_("readme"));
	}
}

/* �������� �������� */
static void show_calculator(void)
{
	if (!calc_active){
		online = false;
		guess_term_state();
		push_term_info();
		if (cfg.simple_calc)
			init_calc();
		else
			adv_init_calc();
	}
}

/* �������� ��� */
static void show_xlog(void)
{
	if (!xlog_active){
		online = false;
		guess_term_state();
		push_term_info();
		log_init_view(xlog_gui_ctx);
	}
}

/* �������� ���� ��� */
static void show_xlog_menu(void)
{
	if (!menu_active){
		push_term_info();
		hide_cursor();
		scr_visible = false;
		set_term_busy(true);
		mnu = new_menu(true, false);
		if (xlog_rec_hdr.type == XLRT_AUX)
			add_menu_item(mnu, new_menu_item("����� ⥪�饩 ����� (���)",
				cmd_print_xlog_aux, true));
		else
			add_menu_item(mnu, new_menu_item("����� ⥪�饩 ����� (���)",
				cmd_print_xlog_rec, xlog_can_print_range(hxlog)));
		add_menu_item(mnu, new_menu_item("����� ��������� ����ᥩ",
				cmd_print_xlog_range, xlog_can_print_range(hxlog)));
		add_menu_item(mnu, new_menu_item("���� ����� �� ���",
				cmd_find_xlog_date, xlog_can_find(hxlog)));
		add_menu_item(mnu, new_menu_item("���� ����� �� ������",
				cmd_find_xlog_number, xlog_can_find(hxlog)));
		ClearScreen(clBlack);
		draw_menu(mnu);
	}
}

/* �������� ��� */
static void show_plog(void)
{
	if (!cfg.bank_system){
		set_term_astate(ast_illegal);
		err_beep();
	}else if (!plog_active){
		online = false;
		guess_term_state();
		push_term_info();
		log_init_view(plog_gui_ctx);
	}
}

/* �������� ���� ��� */
static void show_plog_menu(void)
{
	if (!menu_active){
		push_term_info();
		hide_cursor();
		scr_visible = false;
		set_term_busy(true);
		mnu = new_menu(true, false);
		add_menu_item(mnu, new_menu_item("����� ⥪�饩 �����",
				cmd_print_plog_rec, plog_can_print_range(hplog)));
		add_menu_item(mnu, new_menu_item("����� ��������� ����ᥩ",
				cmd_print_plog_range, plog_can_print_range(hplog)));
		add_menu_item(mnu, new_menu_item("���� ����� �� ���",
				cmd_find_plog_date, plog_can_find(hplog)));
		add_menu_item(mnu, new_menu_item("���� ����� �� ������",
				cmd_find_plog_number, plog_can_find(hplog)));
		ClearScreen(clBlack);
		draw_menu(mnu);
	}
}

/* �������� ��� */
static void show_klog(void)
{
	if (!cfg.has_kkt){
		set_term_astate(ast_illegal);
		err_beep();
	}{
		online = false;
		guess_term_state();
		push_term_info();
		fdo_suspend();
		log_init_view(klog_gui_ctx);
	}
}

/* �������� ���� ��� */
static void show_klog_menu(void)
{
	bool can_print = klog_can_print_range(hklog);
	bool can_find = klog_can_find(hklog);
	if (!menu_active && (can_print || can_find)){
		push_term_info();
		hide_cursor();
		scr_visible = false;
		set_term_busy(true);
		mnu = new_menu(true, false);
		add_menu_item(mnu, new_menu_item("����� ⥪�饩 �����",
				cmd_print_klog_rec, can_print));
		add_menu_item(mnu, new_menu_item("����� ��������� ����ᥩ",
				cmd_print_klog_range, can_print));
		add_menu_item(mnu, new_menu_item("���� ����� �� ���",
				cmd_find_klog_date, can_find));
		add_menu_item(mnu, new_menu_item("���� ����� �� ������",
				cmd_find_klog_number, can_find));
		ClearScreen(clBlack);
		draw_menu(mnu);
	}
}

/* �������� ���� ����ன�� ��ࠬ��஢ */
static void show_options(void)
{
	if ((kt != key_srv) && (kt != key_dbg))
		err_beep();
	else if (!optn_active){
		online = false;
		cfg.kkt_log_stream = kkt_log_stream_to_idx(cfg.kkt_log_stream);
		if (s_state == ss_ready)
			guess_term_state();
		prev_kkt_log_stream = cfg.kkt_log_stream;
		adjust_kkt_brightness(kkt_brightness.max + 1);
		prev_s_state = s_state;
		s_state = ss_ready;
		push_term_info();
		init_options();
	}
}

/* �������� ���� �஢�ન ᮥ������� TCP/IP */
static void show_ping(void)
{
	if (!ping_active){
		online = false;
		guess_term_state();
		push_term_info();
		if (!init_ping()){
			online = true;
			pop_term_info();
		}
	}
}

/* �������� ���� POS-�ନ���� */
static void show_pos(void)
{
#define POS_WIDTH		32
#define POS_HEIGHT		8
	GCPtr pGC;
	if (!cfg.bank_system || !cfg.has_xprn){
		set_term_astate(ast_illegal);
		err_beep();
	}else if (pos_get_state() != pos_idle){
		set_term_astate(ast_pos_error);
		err_beep();
	}else if (!pos_active){
		pos_active = true;
		online = false;
		guess_term_state();
		set_term_astate(ast_none);
		push_term_info();
		set_term_busy(true);
		set_scr_mode(m32x8, true, false);
		hide_cursor();
/*		scr_visible = false;*/
		pGC = CreateGC(8, 30, DISCX-16, DISCY-144);
		ClearGC(pGC, clBlack);
		DeleteGC(pGC);
		if (pos_screen_create((DISCX - font32x8->max_width * POS_WIDTH) / 2,
				44, POS_WIDTH, POS_HEIGHT, 4, 4, font32x8,
				bmp_up, bmp_down)){
			pos_screen_draw();
			pos_set_state(pos_init);
		}else{
			online = true;
			pop_term_info();
		}
	}
}

static inline void show_no_kkt(void){
	set_term_astate(ast_no_kkt);
	err_beep();
}

/* �������� ���� �᪠�쭮�� �ਫ������ */
static void show_fa_with_arg(int arg) {
//	if (!cfg.has_kkt || (kkt == NULL))
//		show_no_kkt();
//	else 
if (!fa_active){
		online = false;
		guess_term_state();
		push_term_info();
		if (!init_fa(arg)) {
			online = true;
			pop_term_info();
		}
	}
}

static void show_fa(void)
{
	show_fa_with_arg(cmd_fa);
}

static void show_cheque_fa(void)
{
	show_fa_with_arg(cmd_cheque_fa);
}

/* �뢮� �� ��࠭ ���ଠ樨 � �ନ���� */
static void show_term_info(void)
{
	static char buf[512];
	term_number tn;
	online = false;
	guess_term_state();
	set_term_astate(ast_none);
	push_term_info();
	hide_cursor();
	scr_visible = false;
	set_term_busy(true);
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn);
	init_devices();
	snprintf(buf, sizeof(buf),
		"%29s: \"������-2�-�\"\n"
		"%29s:  " _s(STERM_VERSION_MAJOR) "."
		_s(STERM_VERSION_MINOR) "." _s(STERM_VERSION_RELEASE) "\n"
		"%29s:  %.4hX\n"
		"%29s:  %.*s\n"
		"%29s:  %s (%s)\n"
		"%29s:  %s\n"
		"%29s:  %s\n"
		"%29s:  �� ��� \"������\"\n\n"
		"%29s:  ��室",
		"��ନ���", "��� ���ᨨ",
		"CRC", term_check_sum,
		"��਩�� �����", isizeof(tn), tn,
		"IP ���-���", inet_ntoa(dw2ip(get_x3_ip())),
		cfg.use_p_ip ? "��." : "���.",
		"��業��� ���", bank_ok ? "����" : "���",
		"���", (kkt == NULL) ? "���" : kkt->name,
		"����⮢�⥫�", "Esc");
	ClearScreen(clBlack);
	message_box("���ଠ�� � �ନ����", buf, dlg_none, 0, al_left);
	online = true;
	pop_term_info();
	ClearScreen(clBtnFace);
	redraw_term(true, main_title);
}

static const char *vpn_priv_to_str(enum VpnPrivilege priv)
{
	static const struct {
		enum VpnPrivilege priv;
		const char *name;
	} map[] = {
		{VpnPrivilegeUnknown,	"�������⭮"},
		{VpnPrivilegeMin,	"���������"},
		{VpnPrivilegeMiddle,	"�।���"},
		{VpnPrivilegeMax,	"���ᨬ����"},
		{VpnPrivilegeSpecial,	"ᯥ樠���"},
	};
	const char *ret = NULL;
	for (int i = 0; i < ASIZE(map); i++){
		if (priv == map[i].priv){
			ret = map[i].name;
			break;
		}
	}
	if (ret == NULL){
		static char txt[10];
		snprintf(txt, sizeof(txt), "%d", priv);
		ret = txt;
	}
	return ret;
}

static const char *vpn_encryption_name(const char *raw_name)
{
	const char *ret = NULL;
	static const struct {
		const char *raw_name;
		const char *name;
	} map[] = {
		{"gost",	"���� 28147-89"},
		{"hybrid",	"���ਤ"},
	};
	for (int i = 0; i < ASIZE(map); i++){
		if (strcasecmp(raw_name, map[i].raw_name) == 0){
			ret = map[i].name;
			break;
		}
	}
	if (ret == NULL)
		ret = raw_name;
	return ret;
}

/* �뢮� �� ��࠭ ���ଠ樨 � ���ᨨ VipNet-������ */
static void show_iplir_version(void)
{
	bool ok = false;
	static char txt[1024];
	size_t offs = 0;
	int l = 0;
	static char ver[128];
	size_t ver_sz = sizeof(ver);
	const int name_len = 30;
	txt[0] = 0;
	ItcsVpnApi *vpn_api = GetVpnApi();
	VpnApiReturnCode rc = vpn_api->getClientVersion(ver, &ver_sz);
	if (rc.code == 0){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%*s : %s\n", name_len,
			"����� �� ViPNet", ver);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto end;
	}else
		fprintf(stderr, "getClientVersion: %x (%s).\n", rc.code, rc.message);
	VpnStatus vpn_st;
	rc = vpn_api->getVpnStatus(&vpn_st);
	if (rc.code == 0){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%*s : %s\n%*s : %s\n",
			name_len, "���� ��஢����",
			vpn_st.isKeysInstalled ? "��⠭������" : "�� ��⠭������",
			name_len, "���஢���� ViPNet",
			vpn_st.isVpnEnabled ? "����祭�" : "�몫�祭�");
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto end;
	}else
		fprintf(stderr, "getVpnStatus: %x (%s).\n", rc.code, rc.message);
	char *data = NULL;
	uint32_t data_len = 0;
	rc = vpn_api->getClientParam(paramSecurityEncryptionMode,
		strlen(paramSecurityEncryptionMode), &data, &data_len);
	if ((rc.code == 0) && (data != NULL)){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%*s : %s\n",
			name_len, "����� ��஢����", vpn_encryption_name(data));
		vpn_api->releaseClientParamData(data);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto end;
	}else
		fprintf(stderr, "getClientParam: %x (%s).\n", rc.code, rc.message);
	enum VpnPrivilege priv = VpnPrivilegeUnknown;
	rc = vpn_api->getOwnPrivilegeLevel(&priv);
	if (rc.code == 0){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%*s : %s\n",
			name_len, "�஢��� �ਢ������", vpn_priv_to_str(priv));
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto end;
	}else
		fprintf(stderr, "getOwnPrivilegeLevel: %x (%s).\n", rc.code, rc.message);
	VpnNodeInfo *node = NULL;
	rc = vpn_api->getOwnNodeInfo(&node);
	if ((rc.code == 0) && (node != NULL)){
		l = snprintf(txt + offs, sizeof(txt) - offs,
				"%*s : %s\n%*s : %.8X\n%*s : %s\n%*s : %.8X\n",
			name_len, "��� 㧫�", node->name,
			name_len, "ID 㧫�", node->id,
			name_len, "IP 㧫�", inet_ntoa(dw2ip(htonl(node->ip))),
			name_len, "����� 㧫�", node->tasksMask);
		vpn_api->releaseVpnNodeInfo(node, 1);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto end;
	}else
		fprintf(stderr, "getOwnNodeInfo: %x (%s).\n", rc.code, rc.message);
	rc = vpn_api->getClientParam(paramKeyNetworkNumber,
		strlen(paramKeyNetworkNumber), &data, &data_len);
	if ((rc.code == 0) && (data != NULL)){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%*s : %s\n",
			name_len, "�����䨪��� �� ViPNet", data);
		vpn_api->releaseClientParamData(data);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto end;
	}else
		fprintf(stderr, "getClientParam: %x (%s).\n", rc.code, rc.message);
	uint32_t id = 0;
	rc = vpn_api->getActiveCoordinator(&id);
	if (rc.code == 0){
		node = NULL;
		rc = vpn_api->getNodeInfo(id, &node);
		if ((rc.code == 0) && (node != NULL)){
			l = snprintf(txt + offs, sizeof(txt) - offs, "%*s : %.8X, %s, %s, %.8X\n",
				name_len, "��⨢�� ���न����",
				node->id, node->name, inet_ntoa(dw2ip(htonl(node->ip))),
				node->tasksMask);
			vpn_api->releaseVpnNodeInfo(node, 1);
			if (l > 0)
				offs += l;
			if ((l <= 0) || ((offs + 1) > sizeof(txt)))
				goto end;
		}else
			fprintf(stderr, "getNodeInfo: %x (%s).\n", rc.code, rc.message);
	}else
		fprintf(stderr, "getActiveCoordinator: %x (%s).\n", rc.code, rc.message);
	time_t t = 0;
	rc = vpn_api->getLicenseExpiration(&t);
	if (rc.code == 0){
		struct tm *tm = gmtime(&t);
		if (tm != NULL){
			l = snprintf(txt + offs, sizeof(txt) - offs,
					"%*s : %.2d.%.2d.%.4d %.2d:%.2d:%.2d\n",
				name_len, "��業��� ����⢨⥫쭠 ��",
				tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
				tm->tm_hour, tm->tm_min, tm->tm_sec);
			if (l > 0)
				offs += l;
			if ((l <= 0) || ((offs + 1) > sizeof(txt)))
				goto end;
		}else
			fprintf(stderr, "gmtime: %m.\n");
	}else
		fprintf(stderr, "getLicenseExpiration: %x (%s).\n", rc.code, rc.message);
	int32_t log_lvl = 0;
	rc = vpn_api->getLogLevel(&log_lvl);
	if (rc.code == 0){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%*s : %d\n",
			name_len, "�஢��� ��⮪���஢����", log_lvl);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto end;
		ok = true;
	}else
		fprintf(stderr, "getLogLevel: %x (%s).\n", rc.code, rc.message);
	if (ok){
		online = false;
		guess_term_state();
		push_term_info();
		hide_cursor();
		scr_visible = false;
		set_term_busy(true);
		ClearScreen(clBlack);
		message_box("���ଠ�� � �� VipNet", txt, dlg_none, 0, al_left);
		online = true;
		pop_term_info();
		ClearScreen(clBtnFace);
		redraw_term(true, main_title);
	}
end:;	/* ��� ; �뤠���� ᮮ�饭�� �� �訡�� ��������� "label at end of compound statement" */
}

static const char *fdo_iface_str(uint8_t fdo_iface)
{
	static char txt[3];
	const char *ret = NULL;
	switch (cfg.fdo_iface){
		case KKT_FDO_IFACE_USB:
		case KKT_FDO_IFACE_INT:
			ret = "USB";
			break;
		case KKT_FDO_IFACE_ETH:
			ret = "Ethernet";
			break;
		case KKT_FDO_IFACE_GPRS:
			ret = "GPRS";
			break;
		default:
			snprintf(txt, sizeof(txt), "%.2hhX", fdo_iface);
			ret = txt;
	}
	return ret;
}

static const char *fs_lifestate_str(uint8_t lifestate)
{
	static char txt[4];
	const char *ret = NULL;
	switch (lifestate){
		case FS_LIFESTATE_NONE:
			ret = "����ன��";
			break;
		case FS_LIFESTATE_FREADY:
			ret = "��⮢ � �᪠����樨";
			break;
		case FS_LIFESTATE_FMODE:
			ret = "��᪠��� ०��";
			break;
		case FS_LIFESTATE_POST_FMODE:
			ret = "�����᪠��� ०��";
			break;
		case FS_LIFESTATE_ARCH_READ:
			ret = "�⥭�� �� ��娢� ��";
			break;
		default:
			snprintf(txt, sizeof(txt), "%hhu", lifestate);
			ret = txt;
	}
	return ret;
}

static const char *fs_doc_type_str(uint8_t doc_type)
{
	static char txt[4];
	const char *ret = NULL;
	switch (doc_type){
		case FS_CURDOC_NOT_OPEN:
			ret = "���";
			break;
		case FS_CURDOC_REG_REPORT:
			ret = "����� � ॣ����樨";
			break;
		case FS_CURDOC_SHIFT_OPEN_REPORT:
			ret = "����� �� ����⨨ ᬥ��";
			break;
		case FS_CURDOC_CHEQUE:
			ret = "���ᮢ� 祪";
			break;
		case FS_CURDOC_SHIFT_CLOSE_REPORT:
			ret = "����� � �����⨨ ᬥ��";
			break;
		case FS_CURDOC_FMODE_CLOSE_REPORT:
			ret = "����� � �����⨨� ��";
			break;
		case FS_CURDOC_BSO:
			ret = "���";
			break;
		case FS_CURDOC_REG_PARAMS_REPORT_ON_FS_CHANGE:
			ret = "����� � ���ॣ����樨 (������ ��)";
			break;
		case FS_CURDOC_REG_PARAMS_REPORT:
			ret = "����� � ���ॣ����樨";
			break;
		case FS_CURDOC_CORRECTION_CHEQUE:
			ret = "��� ���४樨";
			break;
		case FS_CURDOC_CORRECTION_BSO:
			ret = "��� ���४樨";
			break;
		case FS_CURDOC_CURRENT_PAY_REPORT:
			ret = "����� � ������";
			break;
		default:
			snprintf(txt, sizeof(txt), "%hhu", doc_type);
			ret = txt;
	}
	return ret;
}

static const char *fs_alert_str(uint8_t alert)
{
	static char txt[3];
	const char *ret = NULL;
	switch (alert){
		case 0:
			ret = "���";
			break;
		case FS_ALERT_CC_REPLACE_URGENT:
			ret = "��筠� ������ ��";
			break;
		case FS_ALERT_CC_EXHAUST:
			ret = "���௠��� ����� ��";
			break;
		case FS_ALERT_MEMORY_FULL:
			ret = "��९������� ����� ��";
			break;
		case FS_ALERT_RESP_TIMEOUT:
			ret = "������� �⢥� ���";
			break;
		case FS_ALERT_CRITRICAL:
			ret = "����᪠� �訡�� ��";
			break;
		default:
			snprintf(txt, sizeof(txt), "%.2hhX", alert);
			ret = txt;
	}
	return ret;
}

static const char *fs_date_str(const struct kkt_fs_date *date)
{
	static char txt[10][11];
	static int txt_idx = 0;
	snprintf(txt[txt_idx], sizeof(txt[txt_idx]), "%.2hhu.%.2hhu.%u",
		date->day, date->month, date->year);
	const char *ret = txt[txt_idx++];
	txt_idx %= ASIZE(txt);
	return ret;
}

static const char *fs_time_str(const struct kkt_fs_time *time)
{
	static char txt[10][9];
	static int txt_idx = 0;
	snprintf(txt[txt_idx], sizeof(txt[txt_idx]), "%.2hhu:%.2hhu", time->hour, time->minute);
	const char *ret = txt[txt_idx++];
	txt_idx %= ASIZE(txt);
	return ret;
}

static const char *fs_rtc_str(const struct kkt_rtc_data *rtc)
{
	static char txt[20];
	snprintf(txt, sizeof(txt), "%.2hhu.%.2hhu.%.4hu %.2hhu:%.2hhu:%.2hhu",
		rtc->day, rtc->month, rtc->year, rtc->hour, rtc->minute, rtc->second);
	return txt;
}

static const char *fs_type_str(uint8_t type)
{
	static char txt[3];
	const char *ret = NULL;
	switch (type){
		case 0:
			ret = "�⫠����";
			break;
		case 1:
			ret = "��਩�멛�";
			break;
		default:
			snprintf(txt, sizeof(txt), "%.2hhX", type);
			ret = txt;
	}
	return ret;
}

static const char *fs_nr_unconfirmed(void)
{
	uint32_t nr_unconf = 0;
	static char txt[20];
	if (kkt_get_unconfirmed_docs_nr(&nr_unconf) == KKT_STATUS_OK)
		snprintf(txt, sizeof(txt), "%u", nr_unconf);
	else
		snprintf(txt, sizeof(txt), "%s", "����������");
	return txt;
}

static void show_kkt_info(void)
{
	if (!cfg.has_kkt || (kkt == NULL)){
		show_no_kkt();
		return;
	}else
		set_term_astate(ast_none);
	fdo_suspend();
	struct kkt_fs_status fs_status;
	bool fs_status_ok = kkt_get_fs_status(&fs_status) == KKT_STATUS_OK;
	struct kkt_fs_lifetime fs_lifetime;
	bool fs_lifetime_ok = kkt_get_fs_lifetime(&fs_lifetime) == KKT_STATUS_OK;
	struct kkt_fs_version fs_version;
	bool fs_version_ok = kkt_get_fs_version(&fs_version) == KKT_STATUS_OK;
	if (!fs_version_ok)
		snprintf(fs_version.version, sizeof(fs_version.version), "����������");
	struct kkt_fs_shift_state fs_shift_state;
	bool fs_shift_ok = kkt_get_fs_shift_state(&fs_shift_state) == KKT_STATUS_OK;
	struct kkt_rtc_data rtc;
	bool rtc_ok = kkt_get_rtc(&rtc) == RTC_GET_STATUS_OK;
	struct kkt_fs_transmission_state fs_tstate;
	bool tstate_ok = kkt_get_fs_transmission_state(&fs_tstate) == KKT_STATUS_OK;
	fdo_resume();
	
	if (!fs_status_ok || !rtc_ok){
		show_no_kkt();
		return;
	}
	fa_get_reregistration_data();
	static char txt[4096];
	snprintf(txt, sizeof(txt),
			"%29s: \"%s\"\n"	/* ��� */
			"%29s:  %s\n"		/* �����᪮� ����� ��� */
			"%29s:  %s\n"		/* ����� �� */
			"%29s:  %s\n\n"		/* ��������� RTC */

			"%29s:  %s\n"		/* �����᪮� ����� �� */
			"%29s:  %s\n"		/* ����� �� */
			"%29s:  %s\n"		/* ��� �� */
			"%29s:  %s\n"		/* ���� ����� �� */
			"%29s:  %s\n"		/* ����騩 ���㬥�� */
			"%29s:  %s (%u)\n"	/* ����� */
			"%29s:  %lu\n"		/* ��� ����� */
			"%29s:  %s\n"		/* �।�०����� */
			"%29s:  %u (%s %s)\n"	/* ��᫥���� ���. ���㬥�� */
			"%29s:  %s\n"		/* ��� ���⠭樨 ��� */
			"%29s:  %u (%s %s)\n"	/* ���� ���㬥�� ��� ��� */
			"%29s:  %s\n"		/* �ப ����⢨� �� */
			"%29s:  %u\n"		/* �믮����� ॣ����権 */
			"%29s:  %u\n\n"		/* ��⠫��� ॣ����権 */

			"%29s:  %u.%u.%u.%u\n"	/* IP-���� ��� */
			"%29s:  %u.%u.%u.%u\n"	/* ��᪠ ����� */
			"%29s:  %u.%u.%u.%u\n"	/* IP-���� � */

			"%29s:  %s\n"		/* ��窠 ����㯠 GPRS */
			"%29s:  %s\n"		/* ���짮��⥫� GPRS */
			"%29s:  %s\n"		/* ��஫� GPRS */

			"%29s:  %s\n"		/* ����䥩� � ��� */
			"%29s:  %u.%u.%u.%u\n"	/* IP-���� ��� */
			"%29s:  %hu\n"		/* TCP-���� ��� */

			"%29s:  %s\n"		/* SUPPORT_1222_1224_1225 */
			"%29s:  %s\n"		/* COMP1057WO1171 */
			"%29s:  %s\n"		/* SUPPORT_NULL_IN_TEMPLATE */
			"%29s:  %s\n"		/* SUPPORT_FRAGMENTATION */
			"%29s:  %s\n",		/* SUPPORT_ESC_R */
		"���", kkt->name,
		"�����᪮� ����� ���", (kkt_nr == NULL) ? "�� ����������" : kkt_nr,
		"����� ��", (kkt_ver == NULL) ? "����������" : kkt_ver,
		"��������� RTC", rtc_ok ? fs_rtc_str(&rtc) : "����������",
		"�����᪮� ����� ��", (kkt_fs_nr == NULL) ? "����������" : kkt_fs_nr,
		"����� ��", fs_version.version,
		"��� ��", fs_version_ok ? fs_type_str(fs_version.type) : "���",
		"���� ����� ��", fs_status_ok ? fs_lifestate_str(fs_status.life_state) : "���",
		"����訩 ���㬥��", fs_status_ok ? fs_doc_type_str(fs_status.current_doc) : "���",
		"�����", fs_shift_ok ? (fs_shift_state.opened ? "�����" : "������") : "���",
		fs_shift_ok ? fs_shift_state.shift_nr : 0,
		"��� ���짮��⥫�", user_inn,
		"�।�०�����", fs_status_ok ? fs_alert_str(fs_status.alert_flags) : "���",
		"��᫥���� ���. ���㬥��", fs_status_ok ? fs_status.last_doc_nr : 0,
		fs_status_ok ? fs_date_str(&fs_status.dt.date) : "00.00.0000",
		fs_status_ok ? fs_time_str(&fs_status.dt.time) : "00:00",
		"��� ���⠭樨 ���", fs_nr_unconfirmed(),
		"���� ���㬥�� ��� ���", tstate_ok ? fs_tstate.first_doc_nr : 0,
		tstate_ok ? fs_date_str(&fs_tstate.first_doc_dt.date) : "00.00.0000",
		tstate_ok ? fs_time_str(&fs_tstate.first_doc_dt.time) : "00:00",
		"�ப ����⢨� ��", fs_lifetime_ok ? fs_date_str(&fs_lifetime.dt.date) : "00.00.0000",
		"�믮����� ॣ����権", fs_lifetime_ok ? fs_lifetime.reg_complete : 0,
		"��⠫��� ॣ����権", fs_lifetime_ok ? fs_lifetime.reg_remain : 0,
		"IP-���� ���", cfg.kkt_ip & 0xff, (cfg.kkt_ip >> 8) & 0xff,
		(cfg.kkt_ip >> 16) & 0xff, cfg.kkt_ip >> 24,
		"��᪠ �����", cfg.kkt_netmask & 0xff, (cfg.kkt_netmask >> 8) & 0xff,
		(cfg.kkt_netmask >> 16) & 0xff, cfg.kkt_netmask >> 24,
		"IP-���� �", cfg.kkt_gw & 0xff, (cfg.kkt_gw >> 8) & 0xff,
		(cfg.kkt_gw >> 16) & 0xff, cfg.kkt_gw >> 24,
		"��窠 ����㯠 GPRS", cfg.kkt_gprs_apn,
		"���짮��⥫� GPRS", cfg.kkt_gprs_user,
		"��஫� GPRS", cfg.kkt_gprs_passwd,
		"����䥩� � ���", fdo_iface_str(cfg.fdo_iface),
		"IP-���� ���", cfg.fdo_ip & 0xff, (cfg.fdo_ip >> 8) & 0xff,
		(cfg.fdo_ip >> 16) & 0xff, cfg.fdo_ip >> 24,
		"TCP-���� ���", cfg.fdo_port,
		"���� ��� 1222 � 1224", kkt_has_param("SUPPORT_1222_1224_1225") ? "��" : "���",
		"���� ��� 1057 ��� 1171", kkt_has_param("COMP1057WO1171") ? "��" : "���",
		"�⪠� �� ���� ��", kkt_has_param("SUPPORT_NULL_IN_TEMPLATE") ? "��" : "���",
		"����� �ࠣ���⠬�", kkt_has_param("SUPPORT_FRAGMENTATION") ? "��" : "���",
		"����� 蠡������", kkt_has_param("SUPPORT_ESC_R") ? "��" : "���"
	);
	online = false;
	guess_term_state();
	push_term_info();
	hide_cursor();
	scr_visible = false;
	set_term_busy(true);
	ClearScreen(clBlack);
	message_box("��� � ��", txt, dlg_none, 0, al_left);
	online = true;
	pop_term_info();
	ClearScreen(clBtnFace);
	redraw_term(true, main_title);
}

/* �८�ࠧ������ ����� ����ࠡ�⠭��� ����楢 � H-���� */
uint8_t n2hbyte(int n)
{
	n &= 0x7f;
	if (n < 0x20)
		return HB_OK + n;
	else
		return n;
}

/*
 * ��㧠 �� ��ࠡ�⪥ �⢥�. ����� �ᯮ������ ��� �����஢�� �ନ����
 * � ��砥 �訡�� ࠡ��� � ���. �����頥� false, �᫨ �� ��� �ନ����.
 */
bool term_delay(int d)
{
	uint32_t dt = d * 100;
	uint32_t t0 = u_times();
	while ((d == -1) ||
			(((u_times() - t0) < dt) && resp_executing)){
		if ((get_cmd(false, true) == cmd_reset) &&
				reset_term(false))
			return false;
		else if (kt == key_none)
			return false;
	}
	return true;
}

/* ��⠭���� ����� ��� ��ப� ����� � ����ᨬ��� �� ⥪�饣� ���� */
void guess_term_state(void)
{
	if (scr_is_resp())
		show_dest(map[cur_para].dst);
	else if (cur_buf == keys)
		set_term_state(st_keys);
	else if (cur_buf == resp_buf)
		set_term_state(st_raw_resp);
	else if (cur_buf == rom->data)
		set_term_state(st_hash);
	else if (cur_buf == prom->data)
		set_term_state(st_array);
	else
		set_term_state(st_input);
}

#define N_REMOTE_CHARS		40
#define N_XPRN_CHARS		24
#define N_APRN_CHARS		N_XPRN_CHARS
#define N_LPRN_CHARS		40

/* ����� ⥪�� � ०��� �������஢���� */
static void print_text(void)
{
	bool use_xprn = true;
	if (scr_is_req()){
		if (!cfg.has_xprn && !cfg.has_aprn){
			set_term_astate(ast_illegal);
			err_beep();
			return;
		}
	}else{
		show_req();
		return;
	}
	if (scr_is_req() && !menu_active){
		int cmd = cmd_none;
		push_term_info();
		mnu = new_menu(true, false);
		add_menu_item(mnu, new_menu_item("  ����� �� ���",
					cmd_use_xprn, cfg.has_xprn));
		add_menu_item(mnu, new_menu_item("  ����� �� ���",
					cmd_use_aprn, cfg.has_aprn));
		hide_cursor();
		set_term_busy(true);
		draw_menu(mnu);
		while (menu_active){
			cmd = get_cmd(false, false);
			if (cmd == cmd_reset){
				if (reset_term(false))
					return;
				else
					cmd = cmd_none;
			}
		}
		if (cmd == cmd_use_aprn)
			use_xprn = false;
		else if (cmd != cmd_use_xprn)
			return;
	}
	redraw_term(true, main_title);	/* � handle_menu ��뢠���� ⮫쪮 �� ����⨨ Esc */
	if ((use_xprn && cfg.has_xprn) || (!use_xprn && cfg.has_aprn)){
		if (scr_is_req()){
			char buf[N_XPRN_CHARS + 4];
			int l = scr_get_24(buf, N_XPRN_CHARS, true);
			recode_str(buf, l);
			if (use_xprn)
				xprn_print(buf, l);
			else
				aprn_print(buf, l);
		}else
			show_req();
	}else
		err_beep();
}

/* ���� ����� �� �� ��� �� ᮧ����� */
static void do_find_log_date(struct log_gui_context *ctx)
{
	int hour, day, mon, year;
	time_t tt = time(NULL);
	struct tm *t = localtime(&tt);
	if (t == NULL)
		return;
	hour = t->tm_hour;
	day = t->tm_mday;
	mon = t->tm_mon + 1;
	year = t->tm_year + 1900;
	set_term_busy(true);
	ClearScreen(clBlack);
	if (find_log_date_dlg(&hour, &day, &mon, &year) == DLG_BTN_YES){
		struct date_time dt;
		dt.sec = dt.min = 0;
		dt.hour = hour;
		dt.day = day - 1;
		dt.mon = mon - 1;
		if (year < 2000)
			year = 2000;
		dt.year = year - 2000;
		log_show_rec_by_date(ctx, &dt);
	}
	ClearScreen(clBtnFace);
	log_redraw(ctx);
}

/* ���� ����� �� �� ������ */
static void do_find_log_number(struct log_gui_context *ctx)
{
	uint32_t n0, n1;
	if (!log_get_boundary_numbers(ctx->hlog, &n0, &n1))
		return;
	if (n1 < n0){		/* �ந��諮 ��९������� number */
		n0 = 0;
		n1 = -2U;	/* �. ���� n1++ */
	}
	n0++;
	n1++;
	set_term_busy(true);
	ClearScreen(clBlack);
	if (find_log_number_dlg(&n0, n0, n1) == DLG_BTN_YES)
		log_show_rec_by_number(ctx, n0);
	ClearScreen(clBtnFace);
	log_redraw(ctx);
}

/* ����� �� */
static void do_print_log(struct log_gui_context *ctx,
		bool (*prn_fn)(struct log_gui_context *),
		void (*wipe_fn)(void))
{
	if (ctx->hlog->hdr->n_recs == 0){
		set_term_astate(ast_illegal);
		err_beep();
		redraw_term(true, main_title);
	}else{
		log_redraw(ctx);
		prn_fn(ctx);
		if (*ctx->active)
			log_release_view(ctx);
		if (wipe_fn != NULL)
			wipe_fn();
		online = true;
		pop_term_info();
		ClearScreen(clBtnFace);
		redraw_term(true, main_title);
	}
}

/* ����� ⥪�饩 ����� �� */
static void do_print_log_rec(struct log_gui_context *ctx,
		bool (*prn_fn)(struct log_gui_context *))
{
	if (ctx->active){
		log_redraw(ctx);
		prn_fn(ctx);
	}
}

/* ����� ��������� ����ᥩ �� */
static void do_print_log_range(struct log_gui_context *ctx,
		bool (*prn_fn)(struct log_gui_context *, uint32_t, uint32_t))
{
	uint32_t n0, n1;
	int cmd;
	if (!*ctx->active || (ctx->hlog->hdr->n_recs == 0) ||
			!log_get_boundary_numbers(ctx->hlog, &n0, &n1)){
		release_garbage();
		set_term_astate(ast_illegal);
		err_beep();
		redraw_term(true, main_title);
	}else{
		if (n1 < n0){		/* �ந��諮 ��९������� number */
			n0 = 0;
			n1 = -2U;	/* �. ���� n1++ */
		}
		n0++;
		n1++;
		set_term_busy(true);
		ClearScreen(clBlack);
		cmd = log_range_dlg(&n0, &n1, n0, n1);
		ClearScreen(clBtnFace);
		log_redraw(ctx);
		if (cmd == DLG_BTN_YES)
			prn_fn(ctx, n0, n1);
	}
}

/* �㭪樨-"����⪨" ��� ࠧ����� ⨯�� �� */

/* ���� ����� ��� �� ��� �� ᮧ����� */
static void find_xlog_date(void)
{
	do_find_log_date(xlog_gui_ctx);
}

/* ���� ����� ��� �� ������ */
static void find_xlog_number(void)
{
	do_find_log_number(xlog_gui_ctx);
}

/* ����� ��� */
static void print_xlog(void)
{
	do_print_log(xlog_gui_ctx, xlog_print_all, NULL);
}

/* ����� ⥪�饩 ����� ��� */
static void print_xlog_rec(void)
{
	do_print_log_rec(xlog_gui_ctx, xlog_print_current);
}

/* ����� ⥪�饩 ����� ��� "��� ����" */
static void print_xlog_aux(void)
{
	do_print_log_rec(xlog_gui_ctx, xlog_print_aux);
}

/* ����� ��������� ����ᥩ ��� */
static void print_xlog_range(void)
{
	do_print_log_range(xlog_gui_ctx, xlog_print_range);
}


/* ���� ����� ��� �� ��� �� ᮧ����� */
static void find_plog_date(void)
{
	do_find_log_date(plog_gui_ctx);
}

/* ���� ����� ��� �� ������ */
static void find_plog_number(void)
{
	do_find_log_number(plog_gui_ctx);
}

/* ����� ��� */
static void print_plog(void)
{
	do_print_log(plog_gui_ctx, plog_print_all, NULL);
}

/* ����� ⥪�饩 ����� ��� */
static void print_plog_rec(void)
{
	do_print_log_rec(plog_gui_ctx, plog_print_current);
}

/* ����� ��������� ����ᥩ ��� */
static void print_plog_range(void)
{
	do_print_log_range(plog_gui_ctx, plog_print_range);
}


/* ���� ����� ��� �� ��� �� ᮧ����� */
static void find_klog_date(void)
{
	do_find_log_date(klog_gui_ctx);
}

/* ���� ����� ��� �� ������ */
static void find_klog_number(void)
{
	do_find_log_number(klog_gui_ctx);
}

/* ����� ��� */
static void print_klog(void)
{
	do_print_log(klog_gui_ctx, klog_print_all, NULL);
}

/* ����� ⥪�饩 ����� ��� */
static void print_klog_rec(void)
{
	do_print_log_rec(klog_gui_ctx, klog_print_current);
}

/* ����� ��������� ����ᥩ ��� */
static void print_klog_range(void)
{
	do_print_log_range(klog_gui_ctx, klog_print_range);
}


/* ����� ����� ��࠭� */
static void print_sshot(void)
{
	static uint8_t snap_affix[] = {0x1c, 0x0b};
	if (cfg.has_xprn){
		int n = sg->lines * (2 * sg->w + 2) + TERM_STATUS_LEN +
			2 * sizeof(snap_affix) + 1;
		int m = 0;
		uint8_t *buf = malloc(n);
		if (buf == NULL)
			return;
		memcpy(buf, snap_affix, sizeof(snap_affix));
		m = scr_snap_shot(buf + sizeof(snap_affix),
				n - sizeof(snap_affix));
		memcpy(buf + sizeof(snap_affix) + m, snap_affix, sizeof(snap_affix));
		m += 2*sizeof(snap_affix);
		buf[m] = 0;
		if (m > 0){
			bool busy = term_busy;
			guess_term_state();
			set_term_busy(true);
			xprn_print((char *)buf, m);
			set_term_busy(busy);
		}
		scr_show_mode(true);
		free(buf);
	}else{
		set_term_astate(ast_noxprn);
		err_beep();
	}
}

/* ����� ᮥ������� PPP */
void hangup_ppp(void)
{
	if (cfg.use_ppp){
		if (ppp_hangup()){
			c_state = cs_nc;
			ifdown("ppp0");
			if (s_state == ss_initializing)
				s_state = ss_finit;
		}
	}else{
		err_beep();
		set_term_astate(ast_illegal);
	}
}

/* �����頥� true, �᫨ ��襫 �⢥� � ��� ����� ��ࠡ���� */
static bool need_handle_resp(void)
{
	bool flag = full_resp && !xchg_active;
	if (flag)
		full_resp = false;
	return flag && (kt != key_none);
}

static bool need_apc(void)
{
	bool ret = false;
	if (cfg.kkt_apc && !cfg.ext_pos && resp_printed && has_kkt_data){
		struct AD_state ads;
		ret = AD_get_state(&ads);
	}
	return ret;
}

static int need_pos(void)
{
	int ret = 0;
	if (cfg.bank_system &&
#if defined __EXT_POS__
			!cfg.ext_pos &&
#endif
			cfg.kkt_apc){
		struct AD_state ads;
		AD_get_state(&ads);
		if (ads.cashless_cheque_count > 0 && (has_bank_data || ads.order_id > 0)) {
			if (ads.cashless_cheque_count > 1) {
				message_box(
					"��������!",
					"����� ��⮬���᪮� ���� �� ����� ���� �த�����.\n"
					"��� �믮������ ���� �� ������᪮� ���� ����室���\n"
					"���� � �᪠�쭮� �ਫ������, ��������� ��� ������� �㬬�\n"
					"�� ������� 祪�, ᮤ�ঠ饬� ������ �� �������筮�� �����,\n"
					"� ⠪�� �஠������஢��� ��� ����樨 (�����, ������ ��� �⬥��).\n"
					"����� ����室��� ���� � ������᪮� �ਫ������ � ᮢ�����\n"
					"ᮮ⢥�����騥 ����樨 �� ������᪮� ���� ��� ������� 祪� �⤥�쭮.\n"
					"��᫥ �ᯥ譮�� ���� �� ������᪮� ���� ��� ����\n"
					"���ᮢ�� 祪�� ����室��� �ᯮ�짮���� ��᪠�쭮� �ਫ������.\n"
					"�����: ��� ������� ���ᮢ��� 祪� � �㬬�� ������ �� �������筮��\n"
					"����� ������ ���� ��ଫ��� ������ �� ������᪮� ���� �� �� �� �㬬�.",
					dlg_yes, DLG_BTN_YES, al_center);
				ClearScreen(clBtnFace);
				redraw_term(true, main_title);
				ret = -1;
			}else{
				int64_t s = ads.cashless_total_sum;
				if (s < 0)
					s *= -1;
/*				bi.amount1 = s / 100;
				bi.amount2 = ((s % 100) + 5) / 10;
				if (ads.order_id > 0)
					bi.id = ads.order_id;
				clear_bank_info(&bi_pos, true);*/	/* FIXME */
				ret = 1;
			}
		}
	}
	return ret;
}

/* ����� ᨭ�஭���樨 ������ � "������" */
static bool x3data_sync_dlg(uint32_t x3data_to_sync)
{
	if (x3data_to_sync == X3_SYNC_NONE)
		return false;
	static char msg[1024];
	int offs = 0, n = 0;
	int rc = snprintf(msg, sizeof(msg), "����室��� ᨭ�஭���஢��� � \"������\" ᫥���騥 �����:\n");
	if (rc > 0)
		offs += rc;
	if ((offs < sizeof(msg)) && (x3data_to_sync & X3_SYNC_XPRN_GRIDS)){
		rc = snprintf(msg + offs, sizeof(msg) - offs, "ࠧ��⪨ ������� ���;\n");
		if (rc > 0){
			offs += rc;
			n++;
		}
	}
	if ((offs < sizeof(msg)) && (x3data_to_sync & X3_SYNC_XPRN_ICONS)){
		rc = snprintf(msg + offs, sizeof(msg) - offs, "���⮣ࠬ��� ���;\n");
		if (rc > 0){
			offs += rc;
			n++;
		}
	}
	if ((offs < sizeof(msg)) && (x3data_to_sync & X3_SYNC_KKT_GRIDS)){
		rc = snprintf(msg + offs, sizeof(msg) - offs, "ࠧ��⪨ ������� ���;\n");
		if (rc > 0){
			offs += rc;
			n++;
		}
	}
	if ((offs < sizeof(msg)) && (x3data_to_sync & X3_SYNC_KKT_ICONS)){
		rc = snprintf(msg + offs, sizeof(msg) - offs, "���⮣ࠬ��� ���;\n");
		if (rc > 0){
			offs += rc;
			n++;
		}
	}
	if ((offs < sizeof(msg)) && (x3data_to_sync & X3_SYNC_KKT_PATTERNS)){
		rc = snprintf(msg + offs, sizeof(msg) - offs, "蠡���� ���� ���;\n");
		if (rc > 0){
			offs += rc;
			n++;
		}
	}
	if ((offs < sizeof(msg)) && (x3data_to_sync & X3_SYNC_XSLT)){
		rc = snprintf(msg + offs, sizeof(msg) - offs, "ࠧ��⪨ ���㬥�⮢ \"������\";\n");
		if (rc > 0){
			offs += rc;
			n++;
		}
	}
	if (n > 0)
		msg[offs - 2] = '.';
	if (offs < sizeof(msg))
		snprintf(msg + offs, sizeof(msg) - offs, "����஭���஢��� 㪠����� ����� � \"������\"?");
/*	set_term_busy(true);
	online = false;*/
	rc = message_box("������������� ����\x9b�", msg, dlg_yes_no, DLG_BTN_YES, al_center);
/*	online = true;
	set_term_busy(false);*/
	redraw_term(true, main_title);
	return rc == DLG_BTN_YES;
}

static bool begin_x3data_sync(uint32_t x3data_to_sync)
{
	bool ret = false;
	if (x3data_to_sync && X3_SYNC_XPRN_GRIDS)
		ret = sync_grids_xprn(NULL);
	else if (x3data_to_sync && X3_SYNC_KKT_GRIDS)
		ret = sync_grids_kkt(NULL);
	else if (x3data_to_sync && X3_SYNC_XPRN_ICONS)
		ret = sync_icons_xprn(NULL);
	else if (x3data_to_sync && X3_SYNC_KKT_ICONS)
		ret = sync_icons_kkt(NULL);
	else if (x3data_to_sync && X3_SYNC_KKT_PATTERNS)
		ret = sync_patterns(NULL);
	else if (x3data_to_sync && X3_SYNC_XSLT)
		ret = sync_xslt(NULL);
	return ret;
}

/* ��뢠���� �� ��室� �⢥� */
static void on_response(bool *need_sync_dev_data)
{
	into_on_response = true;
	*need_sync_dev_data = false;
#if defined __WATCH_EXPRESS__
	watch_transaction = false;
#endif
	if (ssaver_active)
		scr_wakeup();
	release_garbage();
	resp_handling = true;
	clear_hash(prom);
	err_ptr = check_syntax(resp_buf + text_offset, text_len, &ecode);
	if (err_ptr == NULL){	/* ��稭��� ��ࠡ��� �⢥� */
		CLR_FLAG(ZBp, GDF_REQ_SYNTAX);
/* �� �஢�થ ᨭ⠪�� � ��� ��������� �஡��� �� ���� ᮮ⢥������� ����ᥩ */
		clear_hash(prom);
		if (s_state != ss_ready)
			s_state = ss_ready;
		delayed_init = false;
		rejecting_req = false;
		n_paras = make_resp_map();
/* �� �஢�ઠ ���� �����筮� */
		if (TST_FLAG(ZBp, GDF_REQ_FOREIGN)){
			err_beep();
			set_term_state(st_fresp);
			mark_all();
		}else{
			set_term_busy(true);
			set_term_state(st_resp);
			if ((req_type == req_grid_xprn) || (req_type == req_grid_kkt)){
				on_response_grid();
				*need_sync_dev_data = c_state != cs_hasreq;
			}else if ((req_type == req_icon_xprn) || (req_type == req_icon_kkt)){
				on_response_icon();
				*need_sync_dev_data = c_state != cs_hasreq;
			}else if (req_type == req_patterns){
				on_response_patterns();
				*need_sync_dev_data = c_state != cs_hasreq;
			}else if (req_type == req_xslt){
				on_response_xslt();
				*need_sync_dev_data = c_state != cs_hasreq;
			}else if (!execute_resp() && !rejecting_req)
				show_req();
			if ((req_type == req_regular) && (c_state != cs_hasreq)){
				if (need_apc()){
					show_req();
					apc = true;
					int np = need_pos();
					if (np == 1){
						show_pos();
						apc = pos_active;
					}else if (np == 0){
						show_cheque_fa();
						apc = fa_active;
					}else
						apc = false;
				}else if (TST_FLAG(OBp, GDF_RESP_INIT)){
					uint32_t x3data_to_sync = need_x3_sync();
					if (x3data_to_sync != X3_SYNC_NONE){
						if (x3data_sync_dlg(x3data_to_sync))
							begin_x3data_sync(x3data_to_sync);
					}
				}
			}
			if (!apc)
				set_term_busy(false);
		}
	}else{
		SET_FLAG(ZBp, GDF_REQ_SYNTAX);
		resp_handling = false;
		if (s_state == ss_initializing){
			slayer_error(SERR_SYNTAX);
			set_term_led(hbyte = HB_INIT);
		}else{
			show_error();
			set_term_led(hbyte = HB_ERROR);
		}
	}
	into_on_response = false;
}

static void on_shell(void)
{
	ret_val = RET_SHELL;
}

/* ��ନ஢���� ����� ������ � ����஫쭮� �㬬�� */
static uint8_t *make_ticket_number(const uint8_t *src)
{
	static uint8_t number[LPRN_BLANK_NUMBER_LEN + 10];
	int offs, i;
	uint16_t crc;
	uint8_t b;
	number[0] = '(';
	memcpy(number + 1, src, LPRN_BLANK_NUMBER_LEN);
	number[LPRN_BLANK_NUMBER_LEN + 1] = ')';
	crc = x3_crc(number, LPRN_BLANK_NUMBER_LEN + 2);
	for (i = 0, offs = sizeof(number) - 1; i < 4; i++){
		b = crc & 0x0f;
		if (b < 10)
			b += 0x30;
		else
			b += 0x37;
		number[offs--] = 0x30 + (b & 0x0f);
		number[offs--] = 0x30 + (b >> 4);
		crc >>= 4;
	}
	return number;
}

/* �⥭�� �� ��� ����� ����� */
static void do_ticket_number(void)
{
	bool need_beep = true;
	set_term_astate(ast_none);
	if (hex_input || key_input || wait_key)
		;
	else if (!scr_is_req()){
		show_req();
		need_beep = false;
	}else if (lprn_get_status() != LPRN_RET_OK)
		;
	else if (lprn_status != 0)
		set_term_astate(ast_sprn_err);
	else{
		if (lprn_get_media_type() != LPRN_RET_OK)
			;
		else if (lprn_status != 0)
			set_term_astate(ast_sprn_err);
		else if ((lprn_media != LPRN_MEDIA_BLANK) &&
				(lprn_media != LPRN_MEDIA_BOTH))
			set_term_astate(ast_sprn_ch_media);
		else if (lprn_get_blank_number() != LPRN_RET_OK)
			;
		else if (lprn_status != 0)
			set_term_astate(ast_sprn_err);
		else
			need_beep = !input_chars(
				make_ticket_number(lprn_blank_number),
				sizeof(lprn_blank_number) + 10);
	}
	lprn_close();
	if (need_beep)
		err_beep();
}

/* �᭮���� ��ࠡ��稪 ������ �ନ���� */
static bool process_term(void)
{
	static struct {
		int cm;
		void (*fn)(void);
		bool ret_val;
	} handlers[]={
		{cmd_help,		show_help,		true},
		{cmd_exit,		NULL,			false},
		{cmd_reset,		__reset_term,		true},
		{cmd_enter,		send_request,		true},
		{cmd_print,		print_text,		true},
		{cmd_view_xlog,		show_xlog,		true},
		{cmd_print_xlog,	print_xlog,		true},
		{cmd_print_xlog_rec,	print_xlog_rec,		true},
		{cmd_print_xlog_aux,	print_xlog_aux,		true},
		{cmd_print_xlog_range,	print_xlog_range,	true},
		{cmd_find_xlog_date,	find_xlog_date,		true},
		{cmd_find_xlog_number,	find_xlog_number,	true},
		{cmd_snap_shot,		print_sshot,		true},
		{cmd_view_req,		show_req,		true},
		{cmd_view_resp,		show_resp,		true},
		{cmd_view_raw_resp,	show_raw_resp,		true},
		{cmd_view_rom,		show_rom,		true},
		{cmd_view_prom,		show_prom,		true},
		{cmd_view_keys,		show_keys,		true},
		{cmd_view_xchange,	show_xchange,		true},
		{cmd_reject,		reject_req,		true},
		{cmd_xlog_menu,		show_xlog_menu,		true},
		{cmd_options,		show_options,		true},
		{cmd_calculator,	show_calculator,	true},
		{cmd_ping,		show_ping,		true},
		{cmd_fa,		show_fa,		true},
		{cmd_cheque_fa,		show_cheque_fa,		true},
		{cmd_view_error,	show_error,		true},

		{cmd_shell,		on_shell,		false},
#if defined __WATCH_EXPRESS__
		{cmd_watch_req,		send_watch_request,	true},
#endif
		{cmd_ppp_hangup,	hangup_ppp,		true},
		{cmd_view_plog,		show_plog,		true},
		{cmd_print_plog,	print_plog,		true},
		{cmd_plog_menu,		show_plog_menu,		true},
		{cmd_print_plog_rec,	print_plog_rec,		true},
		{cmd_print_plog_range,	print_plog_range,	true},
		{cmd_find_plog_date,	find_plog_date,		true},
		{cmd_find_plog_number,	find_plog_number,	true},
		{cmd_pos,		show_pos,		true},
		{cmd_term_info,		show_term_info,		true},
		{cmd_iplir_version,	show_iplir_version,	true},
		{cmd_kkt_info,		show_kkt_info,		true},
		{cmd_ticket_number,	do_ticket_number,	true},
		{cmd_view_klog,		show_klog,		true},
		{cmd_klog_menu,		show_klog_menu,		true},
		{cmd_print_klog,	print_klog,		true},
		{cmd_print_klog_rec,	print_klog_rec,		true},
		{cmd_print_klog_range,	print_klog_range,	true},
		{cmd_find_klog_date,	find_klog_date,		true},
		{cmd_find_klog_number,	find_klog_number,	true},
	};
	int cm = get_cmd(true, false);
	for (int i = 0; i < ASIZE(handlers); i++){
		typeof(*handlers) *p = handlers + i;
		if (cm == p->cm){
			if (p->fn != NULL)
				p->fn();
			return p->ret_val;
		}
	}
	if (need_handle_resp()){
		bool need_sync_dev_data = false;
		on_response(&need_sync_dev_data);
		if (need_sync_dev_data){
			if (need_grids_update_kkt())
				update_kkt_grids();
			if (need_icons_update_kkt())
				update_kkt_icons();
		}
	}
	return true;
}

int main(int argc, char **argv)
{
	term_check_sum = make_check_sum(argv[0]);
	if (!parse_cmd_line(argc, argv))
		ret_val = RET_ERROR;
	else if (ret_val == RET_VERSION)
		ret_val = RET_NORMAL;
	else if (create_term()){
		while(process_term());
		release_term();
	}else
		ret_val = RET_ERROR;
	return ret_val;
}
