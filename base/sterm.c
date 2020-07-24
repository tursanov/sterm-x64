/* Основные функции терминала. (c) gsr 2000-2019 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
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
#include "gui/log/local.h"
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
#include "log/local.h"
#include "log/pos.h"
#include "pos/error.h"
#include "pos/pos.h"
#include "pos/screen.h"
#include "pos/tcp.h"
#include "prn/aux.h"
#include "prn/express.h"
#include "prn/local.h"
#include "bscsym.h"
#include "devinfo.h"
#include "gd.h"
#include "genfunc.h"
#include "hash.h"
#include "hex.h"
#include "iplir.h"
#include "kbd.h"
#include "keys.h"
#include "paths.h"
#include "ppp.h"
#include "sterm.h"
#include "tki.h"
#include "transport.h"

bool kkt_test = false;

/* Режим работы терминала */
int wm = wm_main;
/* Необходима блокировка терминала (ошибка работы с ППУ) */
bool need_lock_term = false;

uint16_t term_check_sum = 0;	/* контрольная сумма терминала */

/* Список устройств подключенных к терминалу */
struct dev_lst *devices = NULL;

char main_title[80];		/* заголовок главного окна терминала */

char *term_string = "Экспресс-2А-К";

static int ret_val = RET_NORMAL;

/* Пиктограммы для POS-терминала */
static BitmapPtr bmp_up;
static BitmapPtr bmp_down;

/*
 * Разница в секундах между временем на хосте и на терминале.
 * Время отсчитывается по Гринвичу.
 */
time_t time_delta = 0;	/* Tхост - Tтерм. */

#if defined __WATCH_EXPRESS__
static bool watch_transaction;
static bool watch_crying;
#endif

/* Устанавливаются при выводе соответствующих окон терминала */
bool menu_active	= false;
bool lprn_menu_active	= false;
bool optn_active	= false;
bool calc_active	= false;
bool xlog_active	= false;
bool plog_active	= false;
bool llog_active	= false;
bool klog_active	= false;
bool xchg_active	= false;
bool help_active	= false;
bool ssaver_active	= false;
bool ping_active	= false;
bool pos_active		= false;
bool fa_active		= false;

static bool term_busy = false;		/* флаг занятости терминала */

int kt = key_none;			/* тип ключа DS1990A */
/* Номер ключа DS1990A */
ds_number dsn = {[0 ... DS_NUMBER_LEN - 1] = 0};
/* Хеши настроечных ключей */
static struct md5_hash srv_keys[NR_SRV_KEYS];
/* Хеши отладочных ключей */
static struct md5_hash dbg_keys[NR_DBG_KEYS];

uint8_t hbyte = HB_INIT;			/* H-байт */
int _term_state = st_none;		/* левая часть строки статуса */
int _term_aux_state = ast_none;		/* правая часть строки статуса */
/* Если quick_astate(term_astate), выводим это состояние в show_req при resp_executing */
int astate_for_req = ast_none;

struct menu	*mnu = NULL;		/* меню */

uint8_t text_buf[TEXT_BUF_LEN];		/* буфер абзаца ответа */
uint8_t *err_ptr = NULL;			/* указатель ошибки в ответе */
int ecode = E_OK;			/* код ошибки */
struct para_info map[MAX_PARAS + 1];	/* "карта" ответа */
int n_paras = 0;			/* число абзацев в ответе */
int cur_para = 0;			/* номер текущего абзаца */
struct hash *rom = NULL;		/* ОЗУ констант */
struct hash *prom = NULL;		/* ДЗУ */
bool can_reject = false;		/* флаг возможности отказа от заказа */
bool resp_handling = false;		/* можно обрабатывать ответ по F8 */
bool resp_executing = false;		/* устанавливается в execute_resp */
bool resp_printed = false;		/* при обработке ответа выполнялась печать на БПУ */
bool has_bank_data = false;		/* в ответе есть данные для ИПТ */
bool has_kkt_data = false;		/* в ответе есть данные для ККТ */

static bool full_resp;			/* флаг прихода ответа от хост-ЭВМ */
static bool rejecting_req;		/* был послан запрос с отказом от заказа */
bool online = true;			/* флаг возможности гашения экрана */
static int prev_s_state;		/* используется при работе с окном настроек */

/* Используется в show_error */
static bool into_on_response;

/* Устанавливается при выводе на экран сообщений об ошибках ППУ */
bool lprn_error_shown = false;

/* Устанавливается после получения SIGTERM */
static bool sigterm_caught = false;

/* Восстановление обработчика SIGTERM по умолчанию */
static void restore_sigterm_handler(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(SIGTERM, &sa, NULL);
}

/* Обработчик сигнала SIGTERM */
static void sigterm_handler(int n __attribute__((unused)))
{
	restore_sigterm_handler();
	sigterm_caught = true;
}

/* Установка обработчика сигнала SIGTERM */
static void set_sigterm_handler(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigterm_handler;
	sa.sa_flags = SA_RESETHAND;
	sigaction(SIGTERM, &sa, NULL);
}

/* Устанавливается во время автоматической печати чека на ККТ */
bool apc = false;

/*
 * При загрузке терминала файловая система на DiskOnChip/DiskOnModule
 * монтируется в режиме ro. Когда терминалу необходимо записать данные на диск,
 * необходимо перемонтировать /home в режиме rw, записать данные и снова
 * смонтировать /home в режиме ro.
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

/* Сброс буферов на диск */
void flush_home(void)
{
	remount_home(false);
	usleep(100000);
	remount_home(true);
}

/* Подсчет контрольной суммы заданного файла */
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

/* Разбор командной строки */
static bool parse_cmd_line(int argc, char **argv)
{
	extern char *optarg;
	char *shortopts =
		"v"
		"k";
	const struct option longopts[] = {
		{"version",	no_argument,		NULL,	'v'},
		{"testkkt",	no_argument,		NULL,	'k'},
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
			case 'k':
				kkt_test = true;
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

/* Установка настроек по умолчание */
static bool set_term_defaults(void)
{
	cfg.gaddr = 0x31;
	cfg.iaddr = 0x21;
#if defined __WATCH_EXPRESS__
	cfg.watch_interval = 0;
#endif
	cfg.use_iplir = false;
	cfg.autorun_local = false;

	cfg.has_xprn = true;
	cfg.xprn_number[0] = 0;
	cfg.has_aprn = false;
	cfg.aprn_number[0] = 0;
	cfg.has_lprn = false;
	cfg.has_sd_card = false;
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
	cfg.scr_mode = m32x8;
	cfg.has_hint = true;
	cfg.simple_calc = true;

	cfg.kbd_rate = 30;
	cfg.kbd_delay = 250;
	cfg.kbd_beeps = false;
	
	return true;
}

/* Чтение настроек терминала из файла */
static bool load_term_props(void)
{
	set_term_defaults();
	bool ret = read_cfg();
	kkt_base_timeout = cfg.kkt_base_timeout / 10;
	return ret;
}

/* Чтение номеров ключей DALLAS из файла tki */
void get_dallas_keys(void)
{
	get_tki_field(&tki, TKI_SRV_KEYS, (uint8_t *)srv_keys);
	get_tki_field(&tki, TKI_DBG_KEYS, (uint8_t *)dbg_keys);
}

/* Определение типа ключа DS1990A */
int get_key_type(void)
{
#if defined __REAL_KEYS__
	if (xprn_printing)
		return kt;
	struct md5_hash md5;
	int i;
	ds_read(dsn);
/* Если ключ не обнаружен, его номер заполняется нулями */
	for (i = 0; (i < DS_NUMBER_LEN) && (dsn[i] == 0); i++){}
	if ((i == DS_NUMBER_LEN) || !ds_hash(dsn, &md5))
		return key_none;
/* Ключ может быть настроечным... */
	for (i = 0; i < NR_SRV_KEYS; i++){
		if (cmp_md5(srv_keys + i, &md5))
			return key_srv;
	}
/* ...отладочным... */
	for (i = 0; i < NR_DBG_KEYS; i++){
		if (cmp_md5(dbg_keys + i, &md5))
			return key_dbg;
	}
/* ...иначе это ключ кассира */
	return key_reg;
#else
	return key_dbg;
#endif		/* __REAL_KEYS__ */
}

/* Показать в правой части строки статуса тип ключа */
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
		{st_input,		"Вводите"},
		{st_wait,		"Ожидание связи"},
		{st_transmit,		"Передача"},
		{st_recv,		"Прием"},
		{st_wrecv,		"Ждите ответа"},
		{st_ireq,		"Заказ"},
		{st_resp,		"Ответ"},
		{st_fresp,		"Чужой ответ"},
		{st_scr,		"Ответ Эк"},
		{st_prn,		"Ответ Пч"},
		{st_req,		"Ответ Зк"},
		{st_keys,		"ОЗУ ключей"},
		{st_hash,		"ОЗУ констант"},
		{st_array,		"ДЗУ"},
		{st_raw_resp,		"ОЗУ ответа"},
		{st_log,		"Контрольная лента"},
		{st_xchange,		"Обмен в канале"},
		{st_help,		"Справка"},
		{st_ping,		"Мониторинг сети"},
		{st_ppp_startup,	"Запуск PPP"},
		{st_ppp_cleanup,	"Разрыв сессии PPP"},
		{st_ppp_init,		"Инициализация модема"},
		{st_ppp_dialing,	"Дозвон PPP"},
		{st_ppp_login,		"Авторизация"},
		{st_ppp_ipcp,		"Конфигурация TCP/IP"},
		{st_ppp_ready,		"PPP OK"},
		{st_stop_iplir,		"Выгрузка VipNet"},
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

const char *find_term_astate(int ast)
{
	static struct astate_entry {
		int ast;
		char *descr;
	} astates[] = {
		{ast_none,		""},
		{ast_noxprn,		"Нет готовности ОПУ"},
		{ast_noaprn,		"Нет готовности ДПУ"},
		{ast_nolprn,		"\x01Нет готовности ППУ"},
		{ast_lprn_err,		"\x01Ошибка ППУ"},
		{ast_lprn_ch_media,	"\x01Замените ленту в ППУ"},
		{ast_lprn_sd_err,	"\x01Ош. карты памяти"},
		{ast_rejected,		"Ответ не принят"},
		{ast_repeat,		"Повторите заказ"},
		{ast_resp,		"ПРОСМОТР ОТВЕТА"},
		{ast_prn,		"ПЕЧАТЬ"},
		{ast_req,		"ЗАКАЗ"},
		{ast_nokey,		"Нет ключа"},
		{ast_init,		"Инициализация"},
		{ast_finit,		"Инициализация"},
		{ast_rkey,		"Рабочий ключ"},
		{ast_skey,		"Установка параметров"},
		{ast_dkey,		"Отладочный ключ"},
		{ast_no_log_item,	"Запись не найдена"},
		{ast_illegal,		"Запрещенный режим"},
		{ast_error,		"ОШИБКА"},
		{ast_prn_number,	"Введите номер принтера"},
		{ast_pos_error,		"Ошибка ИПТ"},
		{ast_pos_need_init,	"Проинициализируйте ИПТ"},
		{ast_no_kkt,		"ККТ не обнаружена"},
	};
	struct astate_entry *p;
	int i;
	static char buf[MAX_TERM_ASTATE_LEN + 2];	/* для учёта 0x01 */
	bool flag = false;
	for (i = 0; i < ASIZE(astates); i++){
		p = &astates[i];
		if (ast == p->ast){
			switch (ast){
				case ast_lprn_err:
					if (lprn_status != 0){
						snprintf(buf, sizeof(buf),
							"%s: %.2hhX",
							p->descr, lprn_status);
						flag = true;
					}
					break;
				case ast_lprn_sd_err:
					if (lprn_sd_status > 0x01){
						snprintf(buf, sizeof(buf),
							"%s: %.2hhX",
							p->descr, lprn_sd_status);
						flag = true;
					}
					break;
				case ast_rejected:
					if (ecode != E_OK){
						snprintf(buf, sizeof(buf),
							"%s П:%.2X",
							p->descr, ecode);
						flag = true;
						break;
					}		/* fall through */
				case ast_repeat:
				case ast_finit:
					if (session_error != SLAYER_OK){
						if (session_error >= PPPERR_BASE)
							snprintf(buf, sizeof(buf),
								"%s К:%.2d",
								p->descr, session_error);
						else if (session_error >= TCPERR_BASE)
							snprintf(buf, sizeof(buf),
								"%s Т:%.2d",
								p->descr, session_error);
						else if (session_error == SERR_SPECIAL)
							snprintf(buf, sizeof(buf),
								"%s С:%.2s",
								p->descr, resp_buf + 13);
						else
							snprintf(buf, sizeof(buf),
								"%s С:%.2d",
								p->descr, session_error);
						flag = true;
					}
					break;
				case ast_error:
					if (ecode != E_OK){
						snprintf(buf, sizeof(buf),
							"%s П:%.2X",
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
			return buf;
		}
	}
	return NULL;
}

bool set_term_astate(int ast)
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

/* Вывод в строке статуса устройства для текущего абзаца */
void show_dest(int dst)
{
	switch (dst){
		case dst_text:
			set_term_state(st_scr);
			break;
		case dst_xprn:
		case dst_aprn:
		case dst_lprn:
			set_term_state(st_prn);
			break;
		case dst_out:
			set_term_state(st_req);
			break;
		default:
			set_term_state(st_resp);
		}
}

/* Вывод в строке статуса устройства для следующего необработанного абзаца */
void show_ndest(int n)
{
	struct para_info *p=NULL;
	if (n >= n_paras)
		return;
	p=&map[n];
	if (!p->handled){	/* Абзац не обработан */
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
			case dst_aprn:
			case dst_lprn:
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

/* Информация о текущем состоянии терминала */
struct term_info{
	int st;		/* st_* */
	int ast;	/* ast_* */
	int m;
	bool scr_visible;
	bool cursor_visible;
	bool term_busy;
};

/* Стек состояний терминала */
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

/* Функции-"обёртки" для рисования КЛ */
static bool draw_xlog(void)
{
	return log_draw(xlog_gui_ctx);
}

static bool draw_plog(void)
{
	return log_draw(plog_gui_ctx);
}

static bool draw_llog(void)
{
	return log_draw(llog_gui_ctx);
}

static bool draw_klog(void)
{
	return log_draw(klog_gui_ctx);
}


/* Полная перерисовка терминала */
void redraw_term(bool show_text, const char *title)
{
/*
 * Таблица перерисовки экранных элементов. Если на экране одновременно
 * присутствует несколько элементов, то элемент, который требуется перерисовать
 * позже, должен располагаться ближе к концу массива.
 */
	static struct scr_elem{
		bool *active;
		bool (*draw_fn)(void);
	} scr_elems[]={
		{&optn_active,	draw_options},
		{&help_active,	draw_help},
		{&xlog_active,	draw_xlog},
		{&plog_active,	draw_plog},
		{&llog_active,	draw_llog},
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
	if (menu_active || lprn_menu_active)
		draw_menu(mnu);
}

/* Восстановление экрана после гашения */
static void scr_wakeup(void)
{
	kbd_reset_idle_interval();
	release_ssaver();
	pop_term_info();
	redraw_term(true, main_title);
}

/* Закрытие ненужных окон при сбросе */
static void release_garbage(void)
{
	if (menu_active || lprn_menu_active){
		release_menu(mnu, true);
		mnu = NULL;
		lprn_menu_active = false;
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
	if (llog_active)
		log_release_view(llog_gui_ctx);
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

/* Получение заголовка главного окна терминала */
#include "build.h"
char *get_main_title(void)
{
	snprintf(main_title, sizeof(main_title), MAIN_TITLE " ("
		_s(STERM_VERSION_BRANCH) "."
		_s(STERM_VERSION_RELEASE) "."
		_s(STERM_VERSION_PATCH) "."
		_s(STERM_VERSION_BUILD) ") -- \x01%s РЕЖИМ",
		(wm == wm_main) ? "ОСНОВНОЙ" : "ПРИГОРОДН\x9bЙ");
	return main_title;
}

/* Вывод на экран сообщения об ошибке чтения заводского номера ППУ */
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
	message_box("ОШИБКА ППУ", "ППУ неработоспособно.\n"
		"Требуется замена принтера.", dlg_yes, DLG_BTN_YES, al_center);
	online = true;
	pop_term_info();
	ClearScreen(clBtnFace);
	redraw_term(true, main_title);
}

/* Инициализация ППУ */
static bool init_lprn(void)
{
	bool flag = false;
	int ret;
	if ((ret = lprn_get_status()) != LPRN_RET_OK)
		;
	else if (cfg.has_sd_card && (lprn_sd_status > 0x01)){
		set_term_astate(ast_lprn_sd_err);
		llog_write_sd_error(hllog, lprn_sd_status, 0);
		need_lock_term = true;
	}else if (lprn_status != 0)
		set_term_astate(ast_lprn_err);
	else if ((ret = lprn_init()) != LPRN_RET_OK)
		need_lock_term = ret != LPRN_RET_RST;
	else if (lprn_status != 0){
		set_term_astate(ast_lprn_err);
		if (lprn_status == 0x01)	/* номер ППУ не прописан в памяти */
			show_lprn_nonumber_error();
		need_lock_term = true;
	}else{
		flag = true;
		if (!cfg.has_lprn){
			cfg.has_lprn = true;
			write_cfg();
		}
	}
	lprn_close();
	if (!flag && (ret != LPRN_RET_RST)){
		err_beep();
		if (cfg.has_lprn){
			cfg.has_lprn = false;
			write_cfg();
		}
	}
	return flag;
}

static bool is_kkt(const struct dev_info *dev)
{
	if ((dev == NULL) || (dev->type != DEV_KKT) || strcmp(kkt->name, "СПЕКТР-Ф"))
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

/* Корректировка настроек ККТ в соответствии с информацией, полученной из устройства */
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

/* Удаление информации об опрашиваемых устройствах */
static void release_devices(void)
{
	if (devices != NULL){
		kkt_release();
		free_dev_lst(devices);
		devices = NULL;
	}
	kkt = NULL;
}

/* Получение информации об опрашиваемых устройствах */
static void init_devices(void)
{
	fdo_suspend();
	release_devices();
	devices = poll_devices();
	if (devices != NULL){
		kkt = get_dev_info(devices, DEV_KKT);
		if (kkt != NULL){
			kkt_init(kkt);
			adjust_kkt_cfg(kkt);
//			test_kkt();
		}
	}
	fdo_resume();
	
	if (kkt_test && kkt == NULL) {
		kkt = (struct dev_info *)malloc(sizeof(struct dev_info));
	}
}

/* Инициализация терминала */
static void init_term(bool need_init)
{
	bool flag = xlog_active || plog_active || llog_active || klog_active;
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
	if (!cfg.use_ppp)
		ifup("eth0");
#endif
	kbd_set_rate(cfg.kbd_rate, cfg.kbd_delay);
	set_scr_mode(cfg.scr_mode, false, true);
	if (scr_visible)
		draw_scr(true, main_title);
	set_scr_text(NULL, -1, txt_plain, scr_visible);
	set_term_state(st_input);
	show_key_type();
	set_term_led(hbyte = HB_INIT);
	if (wm == wm_local)
		init_lprn();	/* необходимо вызывать ДО reset_gd */
	reset_gd(need_init);
	mark_all();
	xprn_flush();
	aprn_flush();
	resp_handling = resp_executing = false;
	wm_transition = false;
	if (cfg.bank_system)
		pos_init_transactions();
	reset_bank_info();
	lprn_error_shown = false;
	rollback_keys(true);
	apc = false;
	init_devices();
	if (cfg.use_iplir)
		iplir_start();
}

/* Сброс терминала в начальное состояние */
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

/* Заглушка для таблицы команд в process_term */
static void __reset_term(void)
{
	reset_term(false);
}

/* Вывести сообщение в случае ошибки ключевого USB-диска. */
static bool show_usb_msg(void)
{
	static bool usb_msg_shown = false;
	if (iplir_disabled && !usb_msg_shown){
		set_term_busy(true);
		online = false;
		message_box(NULL, "В модуле безопасности не найдены ключевые базы VipNet. "
			"Терминал будет работать без VipNet в открытом режиме.",
			dlg_yes, 0, al_center);
		online = true;
		set_term_busy(false);
		usb_msg_shown = true;
		return true;
	}else
		return false;
}

/* Установка интерпретации времени в BIOS как московского */
#if defined __GNUC__ && (__GNUC__ > 3)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#endif
static void set_timezone(void)
{
#if 0
	struct timezone tz = {
		.tz_minuteswest	= -180,	/* GMT+3 */
		.tz_dsttime	= 0,
	};
	settimeofday(NULL, &tz);
#endif
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	printf("localtime:\t%.2d.%.2d.%.2d %.2d:%.2d:%.2d\n",
			tm->tm_mday, tm->tm_mon + 1, tm->tm_year % 100,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	tm = gmtime(&t);
	printf("gmtime:\t\t%.2d.%.2d.%.2d %.2d:%.2d:%.2d\n",
			tm->tm_mday, tm->tm_mon + 1, tm->tm_year % 100,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
}
#if defined __GNUC__ && (__GNUC__ > 3)
#pragma GCC diagnostic pop
#endif

/* Проверка возможности автоматического перехода в пригородный режим */
static bool can_set_local_wm(void)
{
	return false;
#if 0
	struct stat st;
	return	cfg.autorun_local &&
		(stat(LOCAL_FLAG_NAME, &st) == 0) &&
		S_ISREG(st.st_mode);
#endif
}

/* Создание/удаление файла признака работы в пригородном режиме */
static bool change_local_flag(void)
{
	bool ret = false;
/*	if (remount_home(true)){*/
		if (wm == wm_local){
			int fd = open(LOCAL_FLAG_NAME,
				O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
				S_IRUSR | S_IWUSR);
			if (fd == -1)
				fprintf(stderr, "Ошибка создания "
					LOCAL_FLAG_NAME ": %s.\n",
					strerror(errno));
			else{
				fsync(fd);
				close(fd);
				ret = true;
			}
		}else if (unlink(LOCAL_FLAG_NAME) == -1)
			fprintf(stderr, "Ошибка удаления " LOCAL_FLAG_NAME ": %s.\n",
				strerror(errno));
		else
			ret = true;
/*	}
	remount_home(false);*/
	if (ret)
		flush_home();
	return ret;
}

static bool open_log(struct log_handle *hlog)
{
	bool ret = true;
	printf("Открытие %s...", hlog->log_type);
	fflush(stdout);
	if (log_open(hlog, true))
		printf("ok.\n");
	else{
		fprintf(stderr, "Ошибка открытия %s.\n", hlog->log_type);
		ret = false;
	}
	return ret;
}

static inline bool open_logs(void)
{
	return	open_log(hxlog) && (!bank_ok || open_log(hplog)) &&
		open_log(hllog) && open_log(hklog);
}

static bool create_term(void)
{
	if (!read_tki(STERM_TKI_NAME, false))
		return false;
	set_timezone();
	set_sigterm_handler();
	load_term_props();
#if defined __REAL_KEYS__
	if (!ds_init()){
		fprintf(stderr, "Ошибка инициализации жетонов DS1990A.\n");
		return false;
	}
#endif
	if (!init_ppp_ipc()){
		fprintf(stderr, "Ошибка инициализации PPP.\n");
		return false;
	}
	if (!pos_create()){
		fprintf(stderr, "Ошибка инициализации ИПТ.\n");
		return false;
	}
	clear_bank_info(&bi, true);
	clear_bank_info(&bi_pos, true);
	init_keys();
	rom = create_hash(ROM_BUF_LEN);
	if (rom == NULL){
		fprintf(stderr, "Ошибка создания ОЗУ констант.\n");
		return false;
	}
	prom = create_hash(PROM_BUF_LEN);
	if (prom == NULL){
		fprintf(stderr, "Ошибка создания ДЗУ.\n");
		return false;
	}
	check_tki();
	check_usb_bind();
	check_iplir_bind();
	if (!tki_ok){
		fprintf(stderr, "Файл ключевой информации терминала поврежден.\n");
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
/* FIXME: закомментировать в release */
/*	printf("%s: tki_ok = %d; usb_ok = %d; iplir_ok = %d\n",
		__func__, tki_ok, usb_ok, iplir_ok);*/
/* Для корректной выгрузки drviplir.o должен быть запущен iplircfg */
/*	start_iplir();*/
	check_bank_license();
	if (!open_logs())
		return false;
	fdo_init();
	/* загрузка корзины фискального приложения */
	AD_load((1 << cfg.tax_system), false);
	/* загрузка информации о кассире */
	cashier_load();
	/* загрузка справочника агентов */
	agents_load();
	/* загрузка справочника товаров/работ/услуг */
	articles_load();
	/* загрузка чека */
	newcheque_load();
/* FIXME: перенести это в более подходящее место */
	bmp_up = CreateBitmap(_("pict/pos/up.bmp"));
	bmp_down = CreateBitmap(_("pict/pos/down.bmp"));
/* С этого места терминал переходит в графический режим */
	create_scr();
	xprn_init();
	init_kbd();
	init_gd();
	if (can_set_local_wm())
		wm = wm_local;
	change_local_flag();
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

#include "log/logdbg.h"
static void release_term(void)
{
	logdbg("%s\n", __func__);
	restore_sigterm_handler();
	logdbg("restore_sigterm_handler();\n");
	fdo_release();
	logdbg("fdo_release();\n");
	if (devices != NULL){
		logdbg("devices != NULL\n");
		free_dev_lst(devices);
		logdbg("free_dev_lst(devices);\n");
		devices = NULL;
	}
	pos_release();
	logdbg("pos_release();\n");
	release_ppp_ipc();
	logdbg("release_ppp_ipc();\n");
	log_close(hxlog);
	logdbg("log_close(hxlog);\n");
	log_close(hplog);
	logdbg("log_close(hplog);\n");
	log_close(hllog);
	logdbg("log_close(hllog);\n");
	log_close(hklog);
	logdbg("log_close(hklog);\n");
	release_keys();
	logdbg("release_keys();\n");
	release_kbd();
	logdbg("release_kbd();\n");
	xprn_release();
	logdbg("xprn_release();\n");
	release_scr();
	logdbg("release_scr();\n");
	release_gd();
	logdbg("release_gd();\n");
	release_hash(prom);
	logdbg("release_hash(prom);\n");
	release_hash(rom);
	logdbg("release_hash(rom);\n");

	AD_destroy();
	logdbg("AD_destroy();\n");
	agents_destroy();
	logdbg("agents_destroy();\n");
	articles_destroy();
	logdbg("articles_destroy();\n");
	newcheque_destroy();
	logdbg("newcheque_destroy();\n");
	iplir_release();
	logdbg("iplir_release();\n");
}

/* Проверка нажатия комбинации клавиш для разрыва модемного соединения */
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

/* Нет ключа */
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

/* Обработка ключей DALLAS. Возвращает false если нет ключа или чужой ключ */
static bool process_dallas(void)
{
	bool ret = true;
	static int _kt = key_none - 1;	/* тип предыдущего ключа */
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
		KEY_ENTER,	/* заказ */
		KEY_CAPS,	/* рус/лат */
		KEY_LSHIFT,	/* рус/лат */
		KEY_RSHIFT,	/* рус/лат */
		KEY_LALT,	/* следующий ключ */
		KEY_RALT,	/* следующий ключ */
		KEY_TAB,	/* калькулятор */
		KEY_F2,		/* печать */
		KEY_F3,		/* поиск ключа */
		KEY_F4,		/* ввод ключа */
		KEY_F5,		/* отказ от заказа */
		KEY_F6,		/* очистка текущего ОЗУ */
		KEY_F7,		/* ОЗУ заказа */
		KEY_F8,		/* ОЗУ ответа, следующий абзац */
		KEY_F9,		/* сброс */
		KEY_F10,	/* ККТ и ФН */
		KEY_F12,	/* ввод шестнадцатеричного символа */
		KEY_PRNSCR,	/* ДЗУ */
		KEY_SCRLOCK,	/* ОЗУ констант */
		KEY_BREAK,	/* ОЗУ ответа (необработанного) */
	};
	uint16_t ctrl_keys[] = {
		KEY_LCTRL,
		KEY_RCTRL,
		KEY_A,		/* Ctrl+Ф -- формат экрана */
		KEY_F,		/* Ctrl+А -- обмен основного и альтернативного ip хост-ЭВМ */
		KEY_G,		/* Ctrl+П -- печать копии экрана */
		KEY_I,		/* Ctrl+I -- информация о терминале */
		KEY_J,		/* Ctrl+О -- просмотр обмена в канале */
		KEY_K,		/* Ctrl+Л -- ЦКЛ */
		KEY_L,		/* Ctrl+L -- информация о версии VipNet-клиента */
		KEY_M,		/* Ctrl+Ь -- ПКЛ */
		KEY_P,		/* Ctrl+P -- ping */
		KEY_Q,		/* Ctrl+Q -- разрыв соединения PPP */
		KEY_R,		/* Ctrl+К -- ОЗУ ключей */
		KEY_S,		/* Ctrl+S -- основной/пригородный режим */
		KEY_T,		/* Ctrl+Е -- ошибка в тексте ответа */
		KEY_U,		/* Ctrl+U -- печать сохранённых образов бланков на ППУ */
		KEY_X,		/* Ctrl+X -- БКЛ */
		KEY_Z,		/* Ctrl+Z -- получение номера БСО */
		KEY_COMMA,	/* Ctrl+Б -- вызов банковского приложения */
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

/* Основной обработчик клавиатуры */
static int handle_kbd(struct kbd_event *e, bool check_scr, bool busy)
{
	static struct {
		uint16_t key;
		int cm;
	} ctrl_keys[] = {
		{KEY_A, cmd_switch_res},	/* изменение разрешения экрана */
		{KEY_G, cmd_snap_shot},		/* печать копии экрана */
		{KEY_H, cmd_view_klog},		/* просмотр ККЛ */
		{KEY_I, cmd_term_info},		/* информация о терминале */
		{KEY_J, cmd_view_xchange},	/* просмотр обмена в канале */
		{KEY_K, cmd_view_xlog},		/* просмотр ЦКЛ */
		{KEY_L, cmd_iplir_version},	/* информация о версии VipNet-клиента */
		{KEY_M, cmd_view_llog},		/* просмотр ПКЛ */
		{KEY_O, cmd_fa},		/* фискальное приложение */
		{KEY_P, cmd_ping},		/* ping */
		{KEY_Q,	cmd_ppp_hangup},	/* разрыв соединения PPP */
		{KEY_R, cmd_view_keys},		/* ключи */
		{KEY_S, cmd_switch_wm},		/* переключение режимов работы терминала */
		{KEY_T, cmd_view_error},	/* ошибка в тексте ответа */
		{KEY_U, cmd_lprn_menu},		/* меню работы с образами бланков в ППУ */
		{KEY_X, cmd_view_plog},		/* просмотр БКЛ */
		{KEY_Z,	cmd_ticket_number},	/* чтение номера БСО в пригородном режиме */
		{KEY_COMMA, cmd_pos},		/* вызов POS-терминала */
		{KEY_F10, cmd_exit},		/* выход */
	},
	keys[] = {
		{KEY_TAB, cmd_calculator},	/* режим калькулятора */
		{KEY_BREAK, cmd_view_raw_resp},	/* просмотр необработанного ответа */
		{KEY_SCRLOCK, cmd_view_rom},
		{KEY_PRNSCR, cmd_view_prom},
		{KEY_F1, cmd_help},		/* справка */
		{KEY_F2, cmd_print},		/* печать */
		{KEY_F5, cmd_reject},		/* отказ от заказа */
		{KEY_F7, cmd_view_req},		/* просмотр заказа */
		{KEY_F8, cmd_view_resp},	/* просмотр ответа */
		{KEY_F9, cmd_reset},		/* сброс терминала */
		{KEY_F10, cmd_kkt_info},	/* ККТ и ФН */
		{KEY_F11, cmd_options},		/* установка параметров */
	},
	ctrl_shift_keys[] = {
		{KEY_O, cmd_cheque_fa},		/* печать чека */
	};
	int i;
	if ((e->key == KEY_NONE) || bad_repeat(e))
		return cmd_none;
	else if (resp_executing && lprn_error_shown &&
			e->pressed && !e->repeated && (e->shift_state == 0))
		return cmd_continue;
	else if (busy || !kbd_alive(e)){
		if (e->pressed && !e->repeated)
			err_beep();
		return cmd_none;
	}else if (e->pressed){
/* Отладочные клавиши */
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

/* Обработчик гашения экрана */
static int handle_ssaver(struct kbd_event *e)
{
	return process_ssaver(e) ? cmd_none : cmd_wakeup;
}

/* Обработчик меню */
static int handle_menu(struct kbd_event *e)
{
	if (mnu == NULL)
		return cmd_none;
	if (process_menu(mnu,e) != cmd_none){
		int cm = get_menu_command(mnu);
		release_menu(mnu,true);
		mnu = NULL;
		online = !xlog_active && !plog_active && !llog_active && !klog_active;
		pop_term_info();
		ClearScreen(clBtnFace);
		if (cm == cmd_none){
			if (xlog_active)
				log_redraw(xlog_gui_ctx);
			else if (plog_active)
				log_redraw(plog_gui_ctx);
			else if (llog_active)
				log_redraw(llog_gui_ctx);
			else if (klog_active)
				log_redraw(klog_gui_ctx);
			else
				redraw_term(true, main_title);
		}
		return cm;
	}else
		return cmd_none;
}

/* Обработчик меню работы с ППУ */
static int handle_lprn_menu(struct kbd_event *e)
{
	int cm = cmd_none;
	if ((mnu != NULL) && (process_menu(mnu, e) != cmd_none)){
		cm = get_menu_command(mnu);
		release_menu(mnu, false);
		mnu = NULL;
		lprn_menu_active = false;
		if ((cm == cmd_exit) || (cm == cmd_none)){
			online = true;
			cm = cmd_none;
			pop_term_info();
			ClearScreen(clBtnFace);
			redraw_term(true, main_title);
		}else
			ClearScreen(clBlack);
	}
	return cm;
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

/* Обработчик окна настроек терминала */
static int handle_options(struct kbd_event *e)
{
/* При вызове из process_options lprn_get_params возможно повторное вхождение */
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
			if ((wm == wm_local) && lprn_params_read &&
					(lprn_set_params(&cfg) == LPRN_RET_ERR)){
				ClearScreen(clBlack);
				err_beep();
				message_box("ОШИБКА ППУ", "Не удалось передать "
					"параметры работы в ППУ.",
					dlg_yes, DLG_BTN_YES, al_center);
			}
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
			if (cfg.has_kkt){
				uint8_t rc = kkt_set_cfg();
/*
 * NB: здесь предполагается, что последняя команда работы с ККТ в случае успеха
 * возвращает KKT_STATUS_OK, а не KKT_STATUS_OK2.
 */
				if (rc != KKT_STATUS_OK){
					err_beep();
					message_box("ОШИБКА ККТ", "Не удалось передать "
						"параметры работы в KKT.",
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

/* Обработчик калькулятора */
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

/* Обработчик просмотра ЦКЛ */
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

/* Обработчик просмотра БКЛ */
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

/* Обработчик просмотра ПКЛ */
static int handle_llog(struct kbd_event *e)
{
	int cm = log_process(llog_gui_ctx, e);
	if (cm == cmd_exit){
		log_release_view(llog_gui_ctx);
		online = true;
		pop_term_info();
		ClearScreen(clBtnFace);
		redraw_term(true, main_title);
		if (!resp_executing)
			set_term_astate(ast_none);
		return cmd_none;
	}
	return cm;
}

/* Обработчик просмотра ККЛ */
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

/* Обработчик просмотра обмена */
static int handle_xchange(struct kbd_event *e)
{
	int cm = process_xchange(e);
	if (cm == cmd_exit){
		release_xchg_view();
		online = true;
		pop_term_info();
		redraw_term(true, main_title);
		if ((hbyte < HB_ERROR) && (hbyte > HB_OK)){	/* Обработка ответа */
			set_scr_text(text_buf,-1,txt_rich,true);
			return cmd_view_resp;
		}else
			return cmd_none;
	}
	return cm;
}

/* Обработчик окна справки */
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

/* Обработчик окна проверка сети */
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

/* Завершение работы банковского приложения */
static void on_end_pos(void)
{
	bool reset_bi = true;
	pos_active = false;
	online = true;
	pop_term_info();
	if (pos_err_xdesc == NULL){
		if (apc){
			hide_cursor();
			static struct custom_btn btns[] = {
				{
					.text	= "Успешная оплата",
					.cmd	= DLG_BTN_YES,
				},
				{
					.text	= "Повтор оплаты",
					.cmd	= DLG_BTN_RETRY,
				},
				{
					.text	= "Отказ от оплаты",
					.cmd	= DLG_BTN_CANCEL,
				},
				{
					.text	= NULL,
					.cmd	= 0,
				},
			};
			int rc = message_box("АВТОМАТИЧЕСКАЯ ПЕЧАТЬ ЧЕКА",
				"Выберите дальнейшее действие", (intptr_t)btns, 0, al_center);
			show_cursor();
			switch (rc){
				case DLG_BTN_YES:
					show_cheque_fa();
					apc = fa_active;
					break;
				case DLG_BTN_RETRY:
					rollback_bank_info();
					reset_bi = false;
/* FIXME: в этом случае по окончании работы с ИПТ мы не посылаем INIT CHECK (pos_new) */
					if (pos_get_state() == pos_finish)
						pos_set_state(pos_idle);
					show_pos();
					apc = pos_active;
					break;
				default:
					apc = false;
			}
		}
	}else{
		apc = false;
		set_term_astate(ast_pos_error);
	}
	if (reset_bi)
		reset_bank_info();
	if (!apc)
		redraw_term(true, main_title);

}

/* Обработчик окна POS-терминала */
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

/* Обработчик фискального приложения */
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

/* Гашение экрана */
static void blank_screen(void)
{
	if (!ssaver_active){
		push_term_info();
		init_ssaver();
	}
}

/* Проверка возможности гашения экрана */
static bool check_screen_blank(uint32_t idle)
{
	if ((cfg.blank_time > 0) && !ssaver_active &&
			(idle > cfg.blank_time * 60) && online){
		blank_screen();
		return true;
	}else
		return false;
}

/* Получение команды терминала */
int get_cmd(bool check_scr, bool busy)
{
	static struct {
		bool *active;
		int (*handler)(struct kbd_event *);
	} elems[] = {
		{&ssaver_active,	handle_ssaver},
		{&menu_active,		handle_menu},
		{&lprn_menu_active,	handle_lprn_menu},
		{&optn_active,		handle_options},
		{&calc_active,		handle_calculator},
		{&xlog_active,		handle_xlog},
		{&plog_active,		handle_plog},
		{&llog_active,		handle_llog},
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

/* Показать ОЗУ заказа */
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

/* Отказ от заказа */
void reject_req(void)
{
	if (can_reject){
		rejecting_req = true;
		mark_all();
		show_req();
		set_term_astate(ast_none);
		if (cfg.bank_system)
			rollback_bank_info();
		reject();
		if (wm == wm_main)
			xlog_write_rec(hxlog, NULL, 0, XLRT_REJECT, log_para++);
		else
			llog_write_rec(hllog, NULL, 0, LLRT_REJECT, log_para++);
	}else{
		set_term_astate(ast_illegal);
		err_beep();
	}
}

/* Отправка запроса */
void send_request(void)
{
	if (full_resp || !can_send_request()){
		err_beep();
		return;
	}else if ((wm == wm_local) &&
			(!cfg.has_lprn || lprn_is_zero_number()) && !init_lprn())
		return;
	else if (s_state == ss_finit){
		s_state = ss_new;
		return;
	}
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
		if (!wm_transition)
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
		beep(500, 0);	/* а вместо динамика подключить ревун :-) */
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

/* Показать ОЗУ необработанного ответа */
static void show_raw_resp(void)
{
	set_scr_text(resp_buf,RESP_BUF_LEN,txt_plain,true);
	set_term_state(st_raw_resp);
	set_term_astate(ast_none);
}

/* Показать место синтаксической ошибки в тексте ответа */
void show_error(void)
{
	if (err_ptr != NULL){
		show_raw_resp();
		scr_goto_char(err_ptr);
		set_term_state(st_none);
		set_term_astate(into_on_response ? ast_rejected : ast_error);
	}
}

/* Показать ОЗУ ответа */
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

/* Показать ОЗУ констант */
static void show_rom(void)
{
	if (kt == key_dbg){
		set_scr_text(rom->data,ROM_BUF_LEN,txt_plain,true);
		set_term_state(st_hash);
		set_term_astate(ast_none);
	}else
		err_beep();
}

/* Показать ДЗУ */
static void show_prom(void)
{
	if (kt == key_dbg){
		set_scr_text(prom->data,PROM_BUF_LEN,txt_plain,true);
		set_term_state(st_array);
		set_term_astate(ast_none);
	}else
		err_beep();
}

/* Показать ОЗУ ключей */
static void show_keys(void)
{
	if (kt != key_dbg)
		err_beep();
	else{
		set_scr_text(keys,KEY_BUF_LEN,txt_plain,true);
		set_term_state(st_keys);
	}
}

/* Показать обмен в канале */
static void show_xchange(void)
{
	push_term_info();
	online = false;
	init_xchg_view();
}

/* Вывод справки */
static void show_help(void)
{
	if (!help_active){
		online = false;
		guess_term_state();
		push_term_info();
		init_help(_("readme"));
	}
}

/* Показать калькулятор */
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

/* Показать ЦКЛ */
static void show_xlog(void)
{
	if (wm != wm_main){
		set_term_astate(ast_illegal);
		err_beep();
	}else if (!xlog_active){
		online = false;
		guess_term_state();
		push_term_info();
		log_init_view(xlog_gui_ctx);
	}
}

/* Показать меню ЦКЛ */
static void show_xlog_menu(void)
{
	if (!menu_active){
		push_term_info();
		hide_cursor();
		scr_visible = false;
		set_term_busy(true);
		mnu = new_menu(true, false);
		if (xlog_rec_hdr.type == XLRT_AUX)
			add_menu_item(mnu, new_menu_item("Печать текущей записи (ДПУ)",
				cmd_print_xlog_aux, true));
		else
			add_menu_item(mnu, new_menu_item("Печать текущей записи (ОПУ)",
				cmd_print_xlog_rec, xlog_can_print_range(hxlog)));
		add_menu_item(mnu, new_menu_item("Печать диапазона записей",
				cmd_print_xlog_range, xlog_can_print_range(hxlog)));
		add_menu_item(mnu, new_menu_item("Поиск записи по дате",
				cmd_find_xlog_date, xlog_can_find(hxlog)));
		add_menu_item(mnu, new_menu_item("Поиск записи по номеру",
				cmd_find_xlog_number, xlog_can_find(hxlog)));
		ClearScreen(clBlack);
		draw_menu(mnu);
	}
}

/* Показать БКЛ */
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

/* Показать меню БКЛ */
static void show_plog_menu(void)
{
	if (!menu_active){
		push_term_info();
		hide_cursor();
		scr_visible = false;
		set_term_busy(true);
		mnu = new_menu(true, false);
		add_menu_item(mnu, new_menu_item("Печать текущей записи",
				cmd_print_plog_rec, plog_can_print_range(hplog)));
		add_menu_item(mnu, new_menu_item("Печать диапазона записей",
				cmd_print_plog_range, plog_can_print_range(hplog)));
		add_menu_item(mnu, new_menu_item("Поиск записи по дате",
				cmd_find_plog_date, plog_can_find(hplog)));
		add_menu_item(mnu, new_menu_item("Поиск записи по номеру",
				cmd_find_plog_number, plog_can_find(hplog)));
		ClearScreen(clBlack);
		draw_menu(mnu);
	}
}

/* Показать ПКЛ */
void show_llog(void)
{
	if (wm != wm_local){
		set_term_astate(ast_illegal);
		err_beep();
	}else if (!llog_active){
		online = false;
		guess_term_state();
		push_term_info();
		log_init_view(llog_gui_ctx);
	}
}

/* Показать меню ПКЛ */
static void show_llog_menu(void)
{
	if (!menu_active){
		push_term_info();
		hide_cursor();
		scr_visible = false;
		set_term_busy(true);
		mnu = new_menu(true, false);
		if (llog_rec_hdr.type == LLRT_EXPRESS)
			add_menu_item(mnu, new_menu_item("Печать текущей записи (ОПУ)",
				cmd_print_llog_express, true));
		else if (llog_rec_hdr.type == LLRT_AUX)
			add_menu_item(mnu, new_menu_item("Печать текущей записи (ДПУ)",
				cmd_print_llog_aux, true));
		else
			add_menu_item(mnu, new_menu_item("Печать текущей записи (ППУ)",
				cmd_print_llog_rec, true));
		add_menu_item(mnu, new_menu_item("Печать диапазона записей",
				cmd_print_llog_range, llog_can_print_range(hllog)));
		add_menu_item(mnu, new_menu_item("Поиск записи по дате",
				cmd_find_llog_date, llog_can_find(hllog)));
		add_menu_item(mnu, new_menu_item("Поиск записи по номеру",
				cmd_find_llog_number, llog_can_find(hllog)));
		ClearScreen(clBlack);
		draw_menu(mnu);
	}
}

/* Показать ККЛ */
static void show_klog(void)
{
	if (!cfg.has_kkt){
		set_term_astate(ast_illegal);
		err_beep();
	}else if (!llog_active){
		online = false;
		guess_term_state();
		push_term_info();
		fdo_suspend();
		log_init_view(klog_gui_ctx);
	}
}

/* Показать меню ККЛ */
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
		add_menu_item(mnu, new_menu_item("Печать текущей записи",
				cmd_print_klog_rec, can_print));
		add_menu_item(mnu, new_menu_item("Печать диапазона записей",
				cmd_print_klog_range, can_print));
		add_menu_item(mnu, new_menu_item("Поиск записи по дате",
				cmd_find_klog_date, can_find));
		add_menu_item(mnu, new_menu_item("Поиск записи по номеру",
				cmd_find_klog_number, can_find));
		ClearScreen(clBlack);
		draw_menu(mnu);
	}
}

/* Показать окно настройки параметров */
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

/* Показать окно проверки соединения TCP/IP */
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

/* Показать окно POS-терминала */
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
			add_bank_info();
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

/* Показать окно фискального приложения */

static void show_fa_with_arg(int arg) {
	if (!cfg.has_kkt || (kkt == NULL))
		show_no_kkt();
	else if (!fa_active){
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

/* Вывод на экран информации о терминале */
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
		"%29s: \"Экспресс-2А-К\"\n"
		"%29s:  " _s(STERM_VERSION_BRANCH) "."
		_s(STERM_VERSION_RELEASE) "." _s(STERM_VERSION_PATCH) "."
		_s(STERM_VERSION_BUILD) "\n"
		"%29s:  %.4hX\n"
		"%29s:  %.*s\n"
		"%29s:  %s (%s)\n"
		"%29s:  %s\n"
		"%29s:  %s\n"
		"%29s:  АО НПЦ \"Спектр\"\n\n"
		"%29s:  выход",
		"Терминал", "Код версии",
		"CRC", term_check_sum,
		"Серийный номер", isizeof(tn), tn,
		"IP хост-ЭВМ", inet_ntoa(dw2ip(get_x3_ip())),
		cfg.use_p_ip ? "осн." : "доп.",
		"Лицензия ИПТ", bank_ok ? "Есть" : "Нет",
		"ККТ", (kkt == NULL) ? "Нет" : kkt->name,
		"Изготовитель", "Esc");
	ClearScreen(clBlack);
	message_box("Информация о терминале", buf, dlg_none, 0, al_left);
	online = true;
	pop_term_info();
	ClearScreen(clBtnFace);
	redraw_term(true, main_title);
}

/* Вывод на экран информации о версии VipNet-клиента */
#define VERSION_BUF_LEN		512
#define MAX_VERSION_LINES	10
static void show_iplir_version(void)
{
/*	static char version[VERSION_BUF_LEN];
	int len, i, n, fd = open(IPLIR_VERSION, O_RDONLY);
	if (fd != -1){
		len = read(fd, version, sizeof(version) - 1);
		close(fd);
		if (len > 0){
			for (i = 0, n = 1; i < len; i++){
				if ((version[i] == '\n') &&
						(++n >= MAX_VERSION_LINES))
					break;
			}
			version[i] = 0;
			online = false;
			guess_term_state();
			push_term_info();
			hide_cursor();
			scr_visible = false;
			set_term_busy(true);
			ClearScreen(clBlack);
			message_box("Информация о ПО VipNet", version,
				dlg_none, 0, al_center);
			online = true;
			pop_term_info();
			ClearScreen(clBtnFace);
			redraw_term(true, main_title);
		}
	}*/
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
			ret = "Настройка";
			break;
		case FS_LIFESTATE_FREADY:
			ret = "Готов к фискализации";
			break;
		case FS_LIFESTATE_FMODE:
			ret = "Фискальный режим";
			break;
		case FS_LIFESTATE_POST_FMODE:
			ret = "Постфискальный режим";
			break;
		case FS_LIFESTATE_ARCH_READ:
			ret = "Чтение из архива ФН";
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
			ret = "Нет";
			break;
		case FS_CURDOC_REG_REPORT:
			ret = "Отчёт о регистрации";
			break;
		case FS_CURDOC_SHIFT_OPEN_REPORT:
			ret = "Отчёт об открытии смены";
			break;
		case FS_CURDOC_CHEQUE:
			ret = "Кассовый чек";
			break;
		case FS_CURDOC_SHIFT_CLOSE_REPORT:
			ret = "Отчёт о закрытии смены";
			break;
		case FS_CURDOC_FMODE_CLOSE_REPORT:
			ret = "Отчёт о закрытиии ФН";
			break;
		case FS_CURDOC_BSO:
			ret = "БСО";
			break;
		case FS_CURDOC_REG_PARAMS_REPORT_ON_FS_CHANGE:
			ret = "Отчёт о перерегистрации (замена ФН)";
			break;
		case FS_CURDOC_REG_PARAMS_REPORT:
			ret = "Отчёт о перерегистрации";
			break;
		case FS_CURDOC_CORRECTION_CHEQUE:
			ret = "Чек коррекции";
			break;
		case FS_CURDOC_CORRECTION_BSO:
			ret = "БСО коррекции";
			break;
		case FS_CURDOC_CURRENT_PAY_REPORT:
			ret = "Отчёт о расчётах";
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
			ret = "Нет";
			break;
		case FS_ALERT_CC_REPLACE_URGENT:
			ret = "Срочная замена КС";
			break;
		case FS_ALERT_CC_EXHAUST:
			ret = "Исчерпание ресурса КС";
			break;
		case FS_ALERT_MEMORY_FULL:
			ret = "Переполнение памяти ФН";
			break;
		case FS_ALERT_RESP_TIMEOUT:
			ret = "Таймаут ответа ОФД";
			break;
		case FS_ALERT_CRITRICAL:
			ret = "Критическая ошибка ФН";
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
			ret = "Отладочный";
			break;
		case 1:
			ret = "СерийныйЫЙ";
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
		snprintf(txt, sizeof(txt), "%s", "НЕДОСТУПНО");
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
		snprintf(fs_version.version, sizeof(fs_version.version), "НЕИЗВЕСТНО");
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
	static char txt[4096];
	snprintf(txt, sizeof(txt),
			"%29s: \"%s\"\n"	/* ККТ */
			"%29s:  %s\n"		/* Заводской номер ККТ */
			"%29s:  %s\n"		/* Версия ПО */
			"%29s:  %s\n\n"		/* Показания RTC */

			"%29s:  %s\n"		/* Заводской номер ФН */
			"%29s:  %s\n"		/* Версия ФН */
			"%29s:  %s\n"		/* Тип ФН */
			"%29s:  %s\n"		/* Фаза жизни ФН */
			"%29s:  %s\n"		/* Текущий документ */
			"%29s:  %s (%u)\n"	/* Смена */
			"%29s:  %s\n"		/* Предупреждения */
			"%29s:  %u (%s %s)\n"	/* Последний сформ. документ */
			"%29s:  %s\n"		/* Без квитанции ОФД */
			"%29s:  %u (%s %s)\n"	/* Первый документ для ОФД */
			"%29s:  %s\n"		/* Срок действия ФН */
			"%29s:  %u\n"		/* Выполнено регистраций */
			"%29s:  %u\n\n"		/* Осталось регистраций */

			"%29s:  %u.%u.%u.%u\n"	/* IP-адрес ККТ */
			"%29s:  %u.%u.%u.%u\n"	/* Маска подсети */
			"%29s:  %u.%u.%u.%u\n\n"/* IP-адрес шлюза */

			"%29s:  %s\n"		/* Точка доступа GPRS */
			"%29s:  %s\n"		/* Пользователь GPRS */
			"%29s:  %s\n\n"		/* Пароль GPRS */

			"%29s:  %s\n"		/* Интерфейс с ОФД */
			"%29s:  %u.%u.%u.%u\n"	/* IP-адрес ОФД */
			"%29s:  %hu",		/* TCP-порт ОФД */
		"ККТ", kkt->name,
		"Заводской номер ККТ", (kkt_nr == NULL) ? "НЕ УСТАНОВЛЕН" : kkt_nr,
		"Версия ПО", (kkt_ver == NULL) ? "НЕИЗВЕСТНО" : kkt_ver,
		"Показания RTC", rtc_ok ? fs_rtc_str(&rtc) : "НЕИЗВЕСТНО",
		"Заводской номер ФН", (kkt_fs_nr == NULL) ? "НЕИЗВЕСТНО" : kkt_fs_nr,
		"Версия ФН", fs_version.version,
		"Тип ФН", fs_version_ok ? fs_type_str(fs_version.type) : "НЕТ",
		"Фаза жизни ФН", fs_status_ok ? fs_lifestate_str(fs_status.life_state) : "НЕТ",
		"Текуший документ", fs_status_ok ? fs_doc_type_str(fs_status.current_doc) : "НЕТ",
		"Смена", fs_shift_ok ? (fs_shift_state.opened ? "Открыта" : "Закрыта") : "НЕТ",
		fs_shift_ok ? fs_shift_state.shift_nr : 0,
		"Предупреждения", fs_status_ok ? fs_alert_str(fs_status.alert_flags) : "НЕТ",
		"Последний сформ. документ", fs_status_ok ? fs_status.last_doc_nr : 0,
		fs_status_ok ? fs_date_str(&fs_status.dt.date) : "00.00.0000",
		fs_status_ok ? fs_time_str(&fs_status.dt.time) : "00:00",
		"Без квитанции ОФД", fs_nr_unconfirmed(),
		"Первый документ для ОФД", tstate_ok ? fs_tstate.first_doc_nr : 0,
		tstate_ok ? fs_date_str(&fs_tstate.first_doc_dt.date) : "00.00.0000",
		tstate_ok ? fs_time_str(&fs_tstate.first_doc_dt.time) : "00:00",
		"Срок действия ФН", fs_lifetime_ok ? fs_date_str(&fs_lifetime.dt.date) : "00.00.0000",
		"Выполнено регистраций", fs_lifetime_ok ? fs_lifetime.reg_complete : 0,
		"Осталось регистраций", fs_lifetime_ok ? fs_lifetime.reg_remain : 0,
		"IP-адрес ККТ", cfg.kkt_ip & 0xff, (cfg.kkt_ip >> 8) & 0xff,
		(cfg.kkt_ip >> 16) & 0xff, cfg.kkt_ip >> 24,
		"Маска подсети", cfg.kkt_netmask & 0xff, (cfg.kkt_netmask >> 8) & 0xff,
		(cfg.kkt_netmask >> 16) & 0xff, cfg.kkt_netmask >> 24,
		"IP-адрес шлюза", cfg.kkt_gw & 0xff, (cfg.kkt_gw >> 8) & 0xff,
		(cfg.kkt_gw >> 16) & 0xff, cfg.kkt_gw >> 24,
		"Точка доступа GPRS", cfg.kkt_gprs_apn,
		"Пользователь GPRS", cfg.kkt_gprs_user,
		"Пароль GPRS", cfg.kkt_gprs_passwd,
		"Интерфейс с ОФД", fdo_iface_str(cfg.fdo_iface),
		"IP-адрес ОФД", cfg.fdo_ip & 0xff, (cfg.fdo_ip >> 8) & 0xff,
		(cfg.fdo_ip >> 16) & 0xff, cfg.fdo_ip >> 24,
		"TCP-порт ОФД", cfg.fdo_port
	);
	online = false;
	guess_term_state();
	push_term_info();
	hide_cursor();
	scr_visible = false;
	set_term_busy(true);
	ClearScreen(clBlack);
	message_box("ККТ и ФН", txt, dlg_none, 0, al_left);
	online = true;
	pop_term_info();
	ClearScreen(clBtnFace);
	redraw_term(true, main_title);
}

/* Циклическое переключение разрешеня экрана терминала */
void switch_term_mode(void)
{
	flush_resp_mode();
	if (scr_is_resp() && resp_handling){
		int n = cur_para;
		bool flag = false;
		switch_scr_mode(false);
		do{
			flag = can_show(map[n].dst);
			if (flag)
				break;
			else{
				n += (n_paras-1);
				n %= n_paras;
			}
		}while (n != cur_para);
		if (flag){
			int l = handle_para(n);
			set_scr_text(text_buf, l, txt_rich, true);
		}
	}else
		switch_scr_mode(true);
}

/* Преобразование номера необработанных абзацев в H-байт */
uint8_t n2hbyte(int n)
{
	n &= 0x7f;
	if (n < 0x20)
		return HB_OK + n;
	else
		return n;
}

/*
 * Пауза при обработке ответа. Также используется для блокировки терминала
 * в случае ошибок работы с ППУ. Возвращает false, если был сброс терминала.
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

/* Установка левой части строки статуса в зависимости от текущего буфера */
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

/* Печать текста в режиме компостирования (основной режим) */
static void print_text_main(void)
{
	bool use_xprn = true;
	if (scr_is_req()){
		if ((wm != wm_main) || (!cfg.has_xprn && !cfg.has_aprn)){
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
		add_menu_item(mnu, new_menu_item("  Печать на ОПУ",
					cmd_use_xprn, cfg.has_xprn));
		add_menu_item(mnu, new_menu_item("  Печать на ДПУ",
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
	redraw_term(true, main_title);	/* в handle_menu вызывается только при нажатии Esc */
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

/* Печать текста в режиме компостирования (пригородный режим) */
static void print_text_local(void)
{
/* NB: предполагается, что N_LPRN_CHARS >= N_XPRN_CHARS */
	char buf[N_LPRN_CHARS + 7] = {LPRN_DLE, LPRN_LOG, LPRN_STX};
	int cmd = cmd_use_xprn, l;
	if (scr_is_req()){
		if ((wm != wm_local) || (!cfg.has_xprn && !cfg.has_lprn)){
			set_term_astate(ast_illegal);
			err_beep();
			return;
		}else{
			if (!menu_active){
				push_term_info();
				mnu = new_menu(true, false);
				add_menu_item(mnu, new_menu_item("  Печать на ОПУ",
					cmd_use_xprn, cfg.has_xprn));
				add_menu_item(mnu, new_menu_item("  Печать на ППУ",
					cmd_use_lprn, cfg.has_lprn));
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
				if (cmd == cmd_none)
					return;
			}
			redraw_term(true, main_title);	/* см. print_text_main */
			if (cmd == cmd_use_xprn){
				l = scr_get_24(buf, N_XPRN_CHARS, true);
				recode_str(buf, l);
				xprn_print(buf, l);
			}else{
				l = scr_get_24(buf + 3, N_LPRN_CHARS, false);
				recode_str(buf + 3, l);
				buf[l + 3] = LPRN_FORM_FEED;
				buf[l + 4] = LPRN_ETX;
				if (lprn_get_status() != LPRN_RET_OK)
					;
				else if (lprn_status != 0)
					set_term_astate(ast_lprn_err);
				else if (lprn_get_media_type() != LPRN_RET_OK)
					;
				else if (lprn_status != 0)
					set_term_astate(ast_lprn_err);
				else if ((lprn_media != LPRN_MEDIA_PAPER) &&
						(lprn_media != LPRN_MEDIA_BOTH))
					set_term_astate(ast_lprn_ch_media);
				else if ((lprn_print_log((const uint8_t *)buf, l + 5) ==
						LPRN_RET_OK) && (lprn_status != 0))
					set_term_astate(ast_lprn_err);
				lprn_close();
			}
		}
	}else
		err_beep();
}

/* Печать текста в режиме компостирования */
static void print_text(void)
{
	if (wm == wm_main)
		print_text_main();
	else
		print_text_local();
}

/* Поиск записи КЛ по дате её создания */
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

/* Поиск записи КЛ по номеру */
static void do_find_log_number(struct log_gui_context *ctx)
{
	uint32_t n0, n1;
	if (!log_get_boundary_numbers(ctx->hlog, &n0, &n1))
		return;
	if (n1 < n0){		/* произошло переполнение number */
		n0 = 0;
		n1 = -2U;	/* см. ниже n1++ */
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

/* Печать КЛ */
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

/* Печать текущей записи КЛ */
static void do_print_log_rec(struct log_gui_context *ctx,
		bool (*prn_fn)(struct log_gui_context *))
{
	if (ctx->active){
		log_redraw(ctx);
		prn_fn(ctx);
	}
}

/* Печать диапазона записей КЛ */
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
		if (n1 < n0){		/* произошло переполнение number */
			n0 = 0;
			n1 = -2U;	/* см. ниже n1++ */
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

/* Функции-"обёртки" для различных типов КЛ */

/* Поиск записи ЦКЛ по дате её создания */
static void find_xlog_date(void)
{
	do_find_log_date(xlog_gui_ctx);
}

/* Поиск записи ЦКЛ по номеру */
static void find_xlog_number(void)
{
	do_find_log_number(xlog_gui_ctx);
}

/* Печать ЦКЛ */
static void print_xlog(void)
{
	do_print_log(xlog_gui_ctx, xlog_print_all, NULL);
}

/* Печать текущей записи ЦКЛ */
static void print_xlog_rec(void)
{
	do_print_log_rec(xlog_gui_ctx, xlog_print_current);
}

/* Печать текущей записи ЦКЛ "как есть" */
static void print_xlog_aux(void)
{
	do_print_log_rec(xlog_gui_ctx, xlog_print_aux);
}

/* Печать диапазона записей ЦКЛ */
static void print_xlog_range(void)
{
	do_print_log_range(xlog_gui_ctx, xlog_print_range);
}


/* Поиск записи БКЛ по дате её создания */
static void find_plog_date(void)
{
	do_find_log_date(plog_gui_ctx);
}

/* Поиск записи БКЛ по номеру */
static void find_plog_number(void)
{
	do_find_log_number(plog_gui_ctx);
}

/* Печать БКЛ */
static void print_plog(void)
{
	do_print_log(plog_gui_ctx, plog_print_all, NULL);
}

/* Печать текущей записи БКЛ */
static void print_plog_rec(void)
{
	do_print_log_rec(plog_gui_ctx, plog_print_current);
}

/* Печать диапазона записей БКЛ */
static void print_plog_range(void)
{
	do_print_log_range(plog_gui_ctx, plog_print_range);
}


/* Поиск записи ПКЛ по дате её создания */
static void find_llog_date(void)
{
	do_find_log_date(llog_gui_ctx);
}

/* Поиск записи ПКЛ по номеру */
static void find_llog_number(void)
{
	do_find_log_number(llog_gui_ctx);
}

/* Печать ПКЛ */
static void print_llog(void)
{
	do_print_log(llog_gui_ctx, llog_print_all, NULL);
}

/* Печать текущей записи ПКЛ */
static void print_llog_rec(void)
{
	do_print_log_rec(llog_gui_ctx, llog_print_current);
}

/* Печать текущей записи ПКЛ на ОПУ */
static void print_llog_express(void)
{
	do_print_log_rec(llog_gui_ctx, llog_print_xprn);
}

/* Печать текущей записи ПКЛ на ДПУ */
static void print_llog_aux(void)
{
	do_print_log_rec(llog_gui_ctx, llog_print_aux);
}

/* Печать диапазона записей ПКЛ */
static void print_llog_range(void)
{
	do_print_log_range(llog_gui_ctx, llog_print_range);
}


/* Поиск записи ККЛ по дате её создания */
static void find_klog_date(void)
{
	do_find_log_date(klog_gui_ctx);
}

/* Поиск записи ККЛ по номеру */
static void find_klog_number(void)
{
	do_find_log_number(klog_gui_ctx);
}

/* Печать ККЛ */
static void print_klog(void)
{
	do_print_log(klog_gui_ctx, klog_print_all, NULL);
}

/* Печать текущей записи ККЛ */
static void print_klog_rec(void)
{
	do_print_log_rec(klog_gui_ctx, klog_print_current);
}

/* Печать диапазона записей ККЛ */
static void print_klog_range(void)
{
	do_print_log_range(klog_gui_ctx, klog_print_range);
}


/* Печать копии экрана */
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

/* Разрыв соединения PPP */
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

/* Возвращает true, если пришел ответ и его можно обработать */
static bool need_handle_resp(void)
{
	bool flag = full_resp && !xchg_active;
	if (flag)
		full_resp = false;
	return flag && (kt != key_none);
}

#if 0
/* Запрос пользователя о смене режима работы терминала */
static bool ask_wm_switch(void)
{
	bool ret;
	char msg[256];
	snprintf(msg, sizeof(msg), "Убедитесь в исключении возможности "
		"потери информации при получении повторно предыдущего ответа.\n\n"
		"Вы действительно хотите перейти в %s режим работы ?",
		(wm == wm_main) ? "пригородный" : "основной");
	online = false;
	set_term_busy(true);
	ClearScreen(clBlack);
	ret = message_box(NULL, msg, dlg_yes_no, DLG_BTN_YES, al_center) ==
		DLG_BTN_YES;
	online = true;
	ClearScreen(clBtnFace);
	set_term_busy(false);
	redraw_term(true, main_title);
	return ret;
}
#endif

/* Вывод окна с сообщением о синтаксической ошибке */
static void show_syntax_error_msg(uint8_t code)
{
	static struct custom_btn btns[] = {
		{
			.text	= "Отказ от заказа",
			.cmd	= cmd_reject
		},
		{
			.text	= NULL,
			.cmd	= cmd_none
		}
	};
	static char msg[256];
	snprintf(msg, sizeof(msg), "В полученном ответе обнаружена "
		"синтаксическая ошибка (П:%.2hhd).\n"
		"Нажмите Enter для отказа от заказа.", code);
	message_box("Ошибка в ответе", msg, (intptr_t)btns, 0, al_center);
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

static bool need_pos(void)
{
	bool ret = false;
	if (cfg.bank_system &&
#if defined __EXT_POS__
			!cfg.ext_pos &&
#endif
			cfg.kkt_apc && has_bank_data){
		struct AD_state ads;
		AD_get_state(&ads);
		if (ads.has_cashless_payments){
			int64_t s = ads.cashless_total_sum;
			if (s < 0)
				s *= -1;
			bi.amount1 = s / 100;
			bi.amount2 = ((s % 100) + 5) / 10;
			clear_bank_info(&bi_pos, true);
			ret = true;
		}
	}
	return ret;
}

/* Вызывается при приходе ответа */
static void on_response(void)
{
	into_on_response = true;
#if defined __WATCH_EXPRESS__
	watch_transaction = false;
#endif
	if (ssaver_active)
		scr_wakeup();
	release_garbage();
	resp_handling = true;
	clear_hash(prom);
	err_ptr = check_syntax(resp_buf + text_offset, text_len, &ecode);
	if (err_ptr == NULL){	/* начинаем обработку ответа */
		CLR_FLAG(ZBp, GDF_REQ_SYNTAX);
/* При проверке синтаксиса в ДЗУ заносятся пробелы на место соответствующих записей */
		clear_hash(prom);
		if (s_state != ss_ready)
			s_state = ss_ready;
		delayed_init = false;
		rejecting_req = false;
		n_paras = make_resp_map();
/* Эта проверка является избыточной */
		if (TST_FLAG(ZBp, GDF_REQ_FOREIGN)){
			err_beep();
			set_term_state(st_fresp);
			mark_all();
		}else{
			set_term_busy(true);
			set_term_state(st_resp);
			if (!execute_resp() && !rejecting_req)
				show_req();
			if ((c_state != cs_hasreq) && need_apc()){
				show_req();
				apc = true;
				if (need_pos()){
					show_pos();
					apc = pos_active;
				}else{
					show_cheque_fa();
					apc = fa_active;
				}
			}
			if (!apc)
				set_term_busy(false);
		}
		if (wm_transition){
			if (TST_FLAG(ZBp, GDF_REQ_FIRST))
				send_request();
			else{
				wm_transition = false;
				if (!wm_transition_interactive){
					bool wm_changed = true;
					if (wm == wm_main){
						if (init_lprn())
							wm = wm_local;
						else
							wm_changed = false;
					}else
						wm = wm_main;
					if (wm_changed){
						change_local_flag();
						init_input_windows();
						reset_term(true);
					}
				}
			}
		}/*else if (TST_FLAG(ZBp, GDF_REQ_FIRST) && (wm == wm_main) &&
				can_set_local_wm()){
			wm_transition = init_lprn();
			if (wm_transition)
				send_request();
			else
				change_local_flag();
		}*/
	}else{
		SET_FLAG(ZBp, GDF_REQ_SYNTAX);
		resp_handling = false;
		if (wm == wm_local)
			llog_write_syntax_error(hllog, ecode);
		if (s_state == ss_initializing){
			slayer_error(SERR_SYNTAX);
			set_term_led(hbyte = HB_INIT);
		}else{
			if (wm == wm_local){
				show_req();
/*				cm_clear();*/
				show_syntax_error_msg(ecode);
				can_reject = true;
				reject_req();
			}else{
				show_error();
			}
			set_term_led(hbyte = HB_ERROR);
		}
	}
	into_on_response = false;
}

static void on_shell(void)
{
	ret_val = RET_SHELL;
}

/* Смена режима работы терминала */
static void switch_wm(void)
{
	set_term_astate(ast_illegal);
	err_beep();
#if 0
	wm_transition = true;
	send_request();
#endif
}

/* Формирование номера бланка с контрольной суммой */
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

/* Чтение из ППУ номера билета */
static void do_ticket_number(void)
{
	bool need_beep = true;
	set_term_astate(ast_none);
	if (wm != wm_local)
		set_term_astate(ast_illegal);
	else if (hex_input || key_input || wait_key)
		;
	else if (!scr_is_req()){
		show_req();
		need_beep = false;
	}else if (lprn_get_status() != LPRN_RET_OK)
		;
	else if (lprn_status != 0)
		set_term_astate(ast_lprn_err);
	else if (cfg.has_sd_card && (lprn_sd_status > 0x01))
		set_term_astate(ast_lprn_sd_err);
	else/*{
		if (!cfg.has_lprn){
			int ret = lprn_init();
			if (ret == LPRN_RET_OK){
				if (lprn_status != 0)
					set_term_astate(ast_lprn_err);
				else if (cfg.has_sd_card && (lprn_sd_status > 0x01))
					set_term_astate(ast_lprn_sd_err);
				else{
					cfg.has_lprn = true;
					write_cfg();
				}
			}else if (ret != LPRN_RET_RST)
				set_term_astate(ast_nolprn);
		}
		if (cfg.has_lprn){*/
			if (lprn_get_media_type() != LPRN_RET_OK)
				;
			else if (lprn_status != 0)
				set_term_astate(ast_lprn_err);
			else if ((lprn_media != LPRN_MEDIA_BLANK) &&
					(lprn_media != LPRN_MEDIA_BOTH))
				set_term_astate(ast_lprn_ch_media);
			else if (lprn_get_blank_number() != LPRN_RET_OK)
				;
			else if (lprn_status != 0){
				llog_write_rd_error(hllog, lprn_status);
				set_term_astate(ast_lprn_err);
			}else
				need_beep = !input_chars(
					make_ticket_number(lprn_blank_number),
					sizeof(lprn_blank_number) + 10);
/*		}
	}*/
	lprn_close();
	if (need_beep)
		err_beep();
}

/* Показать меню работы с образами бланков ППУ */
static void show_lprn_menu(bool can_erase_sd)
{
	if (!lprn_menu_active){
		if (!can_erase_sd){	/* функция вызвана в первый раз */
			push_term_info();
			hide_cursor();
			scr_visible = false;
			set_term_busy(true);
		}
		mnu = new_menu(false, true);
		lprn_menu_active = true;
		add_menu_item(mnu, new_menu_item("Печать копий оформленных документов",
			cmd_lprn_snapshots, true));
		add_menu_item(mnu, new_menu_item("Очистка карты памяти, "
				"содержащей изображения оформленных документов",
			cmd_lprn_erase_sd, can_erase_sd));
		add_menu_item(mnu, new_menu_item("Выход", cmd_exit, true));
		ClearScreen(clBlack);
		draw_menu(mnu);
	}
}

/* Обёртка для show_lprn_menu, вызываемая из process_term */
static void do_lprn_menu(void)
{
	set_term_astate(ast_none);
	if (wm != wm_local || !cfg.has_lprn || !cfg.has_sd_card)
		set_term_astate(ast_illegal);
	else if (hex_input || key_input || wait_key)
		;
	else if (lprn_get_status() == LPRN_RET_OK){
		if (lprn_status != 0)
			set_term_astate(ast_lprn_err);
		else if (lprn_sd_status > 0x01)
			set_term_astate(ast_lprn_sd_err);
		else if (lprn_get_media_type() == LPRN_RET_OK){
			if (lprn_status != 0)
				set_term_astate(ast_lprn_err);
			else if ((lprn_media != LPRN_MEDIA_PAPER) &&
					(lprn_media != LPRN_MEDIA_BOTH))
				set_term_astate(ast_lprn_ch_media);
			else
				show_lprn_menu(false);
		}
	}
	if (!lprn_menu_active)
		err_beep();
}

/* Вывод сообщения о результате работы с ППУ */
static void show_lprn_result(const char *msg, bool need_beep)
{
	if (need_beep)
		err_beep();
	ClearScreen(clBlack);
	message_box(NULL, msg, dlg_yes, 0, al_center);
	pop_term_info();
	show_cursor();
	ClearScreen(clBtnFace);
	redraw_term(true, main_title);
	if (lprn_status != 0)
		set_term_astate(ast_lprn_err);
	else if (cfg.has_sd_card && (lprn_sd_status > 0x01))
		set_term_astate(ast_lprn_sd_err);
	else
		set_term_astate(ast_none);
	online = true;
}

/* Печать сохранённых изображений на сплошной ленте ППУ */
static void do_lprn_snapshots(void)
{
	bool err = true;
	int ret = LPRN_RET_ERR;
	ClearScreen(clBlack);	/* необходимо после работы с меню */
	online = false;
	begin_message_box("ППУ", "Печать копий оформленных документов...",
		al_center);
	if ((ret = lprn_get_status()) != LPRN_RET_OK){
/*		fprintf(stderr, "%s: lprn_get_status failed.\n", __func__);*/
	}else if (lprn_status != 0){
/*		fprintf(stderr, "%s: lprn_status after lprn_get_status = 0x%.2hhx.\n",
			__func__, lprn_status);*/
		set_term_astate(ast_lprn_err);
	}else if (lprn_sd_status > 0x01)
		set_term_astate(ast_lprn_sd_err);
	else if ((ret = lprn_get_media_type()) != LPRN_RET_OK){
/*		fprintf(stderr, "%s: lprn_get_media failed.\n", __func__);*/
	}else if (lprn_status != 0){
/*		fprintf(stderr, "%s: lprn_status after lprn_get_media_type = 0x%.2hhx.\n",
			__func__, lprn_status);*/
		set_term_astate(ast_lprn_err);
	}else if ((lprn_media != LPRN_MEDIA_PAPER) &&
			(lprn_media != LPRN_MEDIA_BOTH)){
/*		fprintf(stderr, "%s: lprn_media = 0x%.2hhx.\n", __func__, lprn_media);*/
		set_term_astate(ast_lprn_ch_media);
	}else{
	       	if ((ret = lprn_snapshots()) != LPRN_RET_OK){
/*			fprintf(stderr, "%s: lprn_snapshots failed.\n", __func__);*/
		}else if (lprn_status != 0){
/*			fprintf(stderr, "%s: lprn_status after lprn_snapshots = 0x%.2hhx.\n",
				__func__, lprn_status);*/
			set_term_astate(ast_lprn_err);
		}else if (lprn_sd_status > 0x01){
/*			fprintf(stderr, "%s: lprn_sd_status after lprn_snapshots = 0x%.2hhx.\n",
				__func__, lprn_sd_status);*/
			set_term_astate(ast_lprn_sd_err);
		}else
			err = false;
	}
	lprn_close();
	end_message_box();
	if (ret != LPRN_RET_RST){
		if (err)
			show_lprn_result("Ошибка ППУ при печати изображений",
				true);
		else if (lprn_sd_status == 0x01)
			show_lprn_result("Карта памяти пуста", false);
		else
			show_lprn_menu(true);
	}
}

/* Очистка SD-карты ППУ */
static void do_lprn_erase_sd(void)
{
	bool err = true;
	int ret = LPRN_RET_ERR;
	ClearScreen(clBlack);	/* необходимо после работы с меню */
	online = false;
	if (message_box(NULL, "Вы действительно отпечатали копии оформленных "
			"документов и хотите очистить память ?",
			dlg_yes_no, DLG_BTN_NO, al_center) == DLG_BTN_YES){
		begin_message_box("ППУ", "Очистка памяти, содержащей копии "
			"оформленных документов...", al_center);
		if ((ret = lprn_erase_sd()) == LPRN_RET_OK){
			if (lprn_sd_status > 0x01){
				set_term_astate(ast_lprn_sd_err);
				err_beep();
			}else if (lprn_status != 0){
				set_term_astate(ast_lprn_err);
				err_beep();
			}else
				err = false;
		}
		lprn_close();
		end_message_box();
		if (ret != LPRN_RET_RST){
			ClearScreen(clBlack);
			if (err)
				show_lprn_result("Ошибка при стирании копий "
					"оформленных документов", true);
			else{
				llog_write_erase_sd(hllog);
				show_lprn_result("Карта памяти очищена", false);
			}
		}
	}else{
		pop_term_info();
		show_cursor();
		ClearScreen(clBtnFace);
		redraw_term(true, main_title);
	}
	online = true;
}

/* Основной обработчик команд терминала */
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
		{cmd_switch_res,	switch_term_mode,	true},
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
		{cmd_switch_wm,		switch_wm,		true},
		{cmd_term_info,		show_term_info,		true},
		{cmd_iplir_version,	show_iplir_version,	true},
		{cmd_kkt_info,		show_kkt_info,		true},
		{cmd_view_llog,		show_llog,		true},
		{cmd_llog_menu,		show_llog_menu,		true},
		{cmd_print_llog,	print_llog,		true},
		{cmd_print_llog_rec,	print_llog_rec,		true},
		{cmd_print_llog_express,print_llog_express,	true},
		{cmd_print_llog_aux,	print_llog_aux,		true},
		{cmd_print_llog_range,	print_llog_range,	true},
		{cmd_find_llog_date,	find_llog_date,		true},
		{cmd_find_llog_number,	find_llog_number,	true},
		{cmd_ticket_number,	do_ticket_number,	true},
		{cmd_lprn_menu,		do_lprn_menu,		true},
		{cmd_lprn_snapshots,	do_lprn_snapshots,	true},
		{cmd_lprn_erase_sd,	do_lprn_erase_sd,	true},
		{cmd_view_klog,		show_klog,		true},
		{cmd_klog_menu,		show_klog_menu,		true},
		{cmd_print_klog,	print_klog,		true},
		{cmd_print_klog_rec,	print_klog_rec,		true},
		{cmd_print_klog_range,	print_klog_range,	true},
		{cmd_find_klog_date,	find_klog_date,		true},
		{cmd_find_klog_number,	find_klog_number,	true},
	};
	if (need_lock_term){
		need_lock_term = false;
		if ((wm == wm_local) && (kt == key_reg))
			term_delay(-1);
	}else{
		int cm = get_cmd(true, false);
		for (int i = 0; i < ASIZE(handlers); i++){
			typeof(*handlers) *p = handlers + i;
			if (cm == p->cm){
				if (!p->ret_val)
					printf("%s: cmd_exit received.\n", __func__);
				if (p->fn != NULL)
					p->fn();
				return p->ret_val;
			}
		}
		if (need_handle_resp())
			on_response();
/*		else
			usleep(1000);*/
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
