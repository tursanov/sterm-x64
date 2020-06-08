/*
 * Чтение/запись файла конфигурации терминала.
 * Формат: name = 'value'
 * (c) gsr 2003-2019.
 */

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cfg.h"
#include "iplir.h"
#include "paths.h"

/* Буфер для работы с конфигурацией */
static char cfg_buf[MAX_CFG_LEN];
/* Длина информации в буфере */
static int cfg_len = 0;
/* Индексы */
static int rd_index = 0;
static int wr_index = 0;

/* Конфигурация терминала */
struct term_cfg cfg;

/* Сброс буфера */
static void reset_cfg(void)
{
	cfg_len = 0;
	rd_index = wr_index = 0;
}

/* Чтение файла */
static bool read_cfg_file(const char *name)
{
	struct stat st;
	int fd;
	bool ret = false;
	if (stat(name, &st) == -1)
		fprintf(stderr, "Ошибка получения информации о файле %s: %s.\n",
			name, strerror(errno));
	else if (st.st_size > MAX_CFG_LEN)
		fprintf(stderr, "Размер файла %s (%lu байт) превышает "
			"максимально допустимый (%d байт).\n",
			name, st.st_size, MAX_CFG_LEN);
	else{
		reset_cfg();
		cfg_len = st.st_size;
		fd = open(name, O_RDONLY);
		if (fd == -1)
			fprintf(stderr, "Ошибка открытия %s для чтения: %s.\n",
				name, strerror(errno));
		else{
			if (read(fd, cfg_buf, cfg_len) == cfg_len)
				ret = true;
			else
				fprintf(stderr, "Ошибка чтения из %s: %s.\n",
					name, strerror(errno));
			close(fd);
		}
	}
	return ret;
}

extern void flush_home(void);

/* Запись файла */
static bool write_cfg_file(const char *name)
{
	int fd;
	bool ret = false;
	fd = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
		S_IRUSR | S_IWUSR);
	if (fd == -1)
		fprintf(stderr, "Ошибка открытия %s для записи: %s.\n",
			name, strerror(errno));
	else{
		if (write(fd, cfg_buf, cfg_len) == cfg_len)
			ret = true;
		else
			fprintf(stderr, "Ошибка записи в %s: %s.\n",
				name, strerror(errno));
		fsync(fd);
		close(fd);
		flush_home();
	}
	return ret;
}

/* Сканирование */
static int read_char(void)
{
	if (rd_index < cfg_len)
		return cfg_buf[rd_index++];
	else
		return EOF;
}

static bool is_eq(int c)
{
	return c == '=';
}

static bool is_quot(int c)
{
	return c == '\'';
}

static bool is_esc(int c)
{
	return c == '\\';
}

static bool is_first_id_char(int c)
{
	return isalpha(c) || (c == '_');
}

static bool is_id_char(int c)
{
	return isalnum(c) || (c == '_') || (c == '-');
}

enum {
	scan_ok,
	scan_err,
	scan_eof,
};

static int scan_cfg_line(char *name, char *val)
{
	enum {
		st_space1,
		st_name,
		st_space2,
		st_space3,
		st_val,
		st_escape,
		st_stop,
		st_err,
	};
	int st = st_space1;
	int c, n = 0;
	if ((name == NULL) || (val == NULL))
		return scan_err;
	while ((st != st_stop) && (st != st_err)){
		c = read_char();
		if (c == EOF)
			st = (st == st_space1) ? st_stop : st_err;
		switch (st){
			case st_space1:
				if (is_first_id_char(c)){
					n = 0;
					rd_index--;
					st = st_name;
				}else if (!isspace(c))
					st = st_err;
				break;
			case st_name:
				if (is_id_char(c)){
					if (n >= MAX_NAME_LEN)
						st = st_err;
					else
						name[n++] = c;
				}else if (isspace(c)){
					name[n] = 0;
					st = st_space2;
				}else if (is_eq(c)){
					name[n] = 0;
					st = st_space3;
				}else
					st = st_err;
				break;
			case st_space2:
				if (is_eq(c))
					st = st_space3;
				else if (!isspace(c))
					st = st_err;
				break;
			case st_space3:
				if (is_quot(c)){
					n = 0;
					st = st_val;
				}else if (!isspace(c))
					st = st_err;
				break;
			case st_val:
				if (is_quot(c)){
					val[n] = 0;
					st = st_stop;
				}else if (n >= MAX_VAL_LEN)
					st = st_err;
				else if (is_esc(c))
					st = st_escape;
				else
					val[n++] = c;
				break;
			case st_escape:
				val[n++] = c;
				st = st_val;
				break;
		}
	}
	if (st == st_stop)
		return c == EOF ? scan_eof : scan_ok;
	else
		return scan_err;
}

/* Запись */
static bool write_char(int c)
{
	if (wr_index < MAX_CFG_LEN){
		cfg_buf[wr_index++] = c;
		return true;
	}else
		return false;
}

#define write_space() write_char(' ')
#define write_eq() write_char('=')
#define write_quot() write_char('\'')
#define write_nl() write_char('\n')
#define write_escape() write_char('\\')

static bool write_cfg_line(const char *name, const char *val)
{
	if ((name == NULL) || (val == NULL))
		return false;
	for (const char *p = name; *p != 0; p++)
		if (!write_char(*p))
			return false;
	if (!write_space() || !write_eq() || !write_space() || !write_quot())
		return false;
	for (const char *p = val; *p != 0; p++){
		if ((*p == '\\') || (*p == '\'')){
			if (!write_escape())
				return false;
		}
		if (!write_char(*p))
			return false;
	}
	return write_quot() && write_nl();
}

/* Обработка значений параметров файла конфигурации */
static bool str_to_int(const char *val, void *p)
{
	char *endptr;
	*(int *)p = strtol(val, &endptr, 10);
	return *endptr == 0;
}

static void int_to_str(const void *p, char *val)
{
	sprintf(val, "%d", *(int *)p);
}

static bool str_to_uint32(const char *val, void *p)
{
	char *endptr;
	*(uint32_t *)p = strtoul(val, &endptr, 10);
	return *endptr == 0;
}

static void uint32_to_str(const void *p, char *val)
{
	sprintf(val, "%u", *(uint32_t *)p);
}

static bool str_to_uint16(const char *val, void *p)
{
	char *endptr;
	*(uint16_t *)p = strtoul(val, &endptr, 10);
	return *endptr == 0;
}

static void uint16_to_str(const void *p, char *val)
{
	sprintf(val, "%hu", *(uint16_t *)p);
}

static bool str_to_hex_byte(const char *val, void *p)
{
	char *endptr;
	*(uint8_t *)p = (uint8_t)strtoul(val, &endptr, 16);
	return *endptr == 0;
}

static void hex_byte_to_str(const void *p, char *val)
{
	sprintf(val, "0x%hx", (uint16_t)*(uint8_t *)p);
}

static bool str_to_bool(const char *val, void *p)
{
	if (!strcmp(val, "on")){
		*(bool *)p = true;
		return true;
	}else if (!strcmp(val, "off")){
		*(bool *)p = false;
		return true;
	}else
		return false;
}

static void bool_to_str(const void *p, char *val)
{
	if (*(bool *)p)
		sprintf(val, "on");
	else
		sprintf(val, "off");
}

static bool str_to_ip(const char *val, void *p)
{
	struct in_addr addr;
	if (inet_aton(val, &addr)){
		*(uint32_t *)p = addr.s_addr;
		return true;
	}else
		return false;
}

static void ip_to_str(const void *p, char *val)
{
	struct in_addr addr;
	addr.s_addr = *(uint32_t *)p;
	strcpy(val, inet_ntoa(addr));
}

static bool str_to_string(const char *val, void *p)
{
	strncpy((char *)p, val, MAX_VAL_LEN);
	((char *)p)[MAX_VAL_LEN] = 0;
	return true;
}

static void string_to_str(const void *p, char *val)
{
	str_to_string(p, val);
}

/* "Карта" файла настроек */
typedef bool (*fn_read_t)(const char *, void *);
typedef void (*fn_write_t)(const void *, char *);
static struct {
	const char *name;	/* название параметра в файле конфигурации */
	int offs;		/* смещение параметра в структуре конфигурации */
	fn_read_t fn_read;	/* функция для чтения параметра из буфера */
	fn_write_t fn_write;	/* функция для записи параметра в буфер */
} cfg_map[] = {
#define CFG_ENTRY(txt, name, type) \
	{txt, offsetof(struct term_cfg, name), str_to_##type, type##_to_str}
#define CFG_ENTRY_BOOL(txt, name)	CFG_ENTRY(txt, name, bool)
#define CFG_ENTRY_INT(txt, name)	CFG_ENTRY(txt, name, int)
#define CFG_ENTRY_UINT32(txt, name)	CFG_ENTRY(txt, name, uint32)
#define CFG_ENTRY_UINT16(txt, name)	CFG_ENTRY(txt, name, uint16)
#define CFG_ENTRY_HEX_BYTE(txt, name)	CFG_ENTRY(txt, name, hex_byte)
#define CFG_ENTRY_IP(txt, name)		CFG_ENTRY(txt, name, ip)
#define CFG_ENTRY_STRING(txt, name)	CFG_ENTRY(txt, name, string)
/* Система */
	CFG_ENTRY_HEX_BYTE	("gaddr",		gaddr),
	CFG_ENTRY_HEX_BYTE	("iaddr",		iaddr),
#if defined __WATCH_EXPRESS__
	CFG_ENTRY_UINT32	("watch-interval",	watch_interval),
#endif
	CFG_ENTRY_BOOL		("use-iplir",		use_iplir),
	CFG_ENTRY_BOOL		("autorun-local",	autorun_local),
/* Внешние устройства */
	CFG_ENTRY_BOOL		("has-xprn",		has_xprn),
	CFG_ENTRY_STRING	("xprn-number",		xprn_number),
	CFG_ENTRY_BOOL		("has-aprn",		has_aprn),
	CFG_ENTRY_STRING	("aprn-number",		aprn_number),
	CFG_ENTRY_INT		("aprn-tty",		aprn_tty),
	CFG_ENTRY_BOOL		("has-lprn",		has_lprn),
	CFG_ENTRY_BOOL		("sd-card",		has_sd_card),
	CFG_ENTRY_INT		("s0",			s0),
	CFG_ENTRY_INT		("s1",			s1),
	CFG_ENTRY_INT		("s2",			s2),
	CFG_ENTRY_INT		("s3",			s3),
	CFG_ENTRY_INT		("s4",			s4),
	CFG_ENTRY_INT		("s5",			s5),
	CFG_ENTRY_INT		("s6",			s6),
	CFG_ENTRY_INT		("s7",			s7),
	CFG_ENTRY_INT		("s8",			s8),
	CFG_ENTRY_INT		("s9",			s9),
/* TCP/IP (сетевая карта) */
	CFG_ENTRY_BOOL		("use-ppp",		use_ppp),
	CFG_ENTRY_IP		("local-ip",		local_ip),
	CFG_ENTRY_IP		("remote-p-ip",		x3_p_ip),
	CFG_ENTRY_IP		("remote-s-ip",		x3_s_ip),
	CFG_ENTRY_BOOL		("use-p-ip",		use_p_ip),
	CFG_ENTRY_IP		("net-mask",		net_mask),
	CFG_ENTRY_IP		("gateway",		gateway),
	CFG_ENTRY_BOOL		("tcp-cbt",		tcp_cbt),
/* PPP (настройки сети задаются сервером) */
	CFG_ENTRY_INT		("ppp-tty",		ppp_tty),
	CFG_ENTRY_INT		("ppp-speed",		ppp_speed),
	CFG_ENTRY_STRING	("ppp-init",		ppp_init),
	CFG_ENTRY_STRING	("ppp-phone",		ppp_phone),
	CFG_ENTRY_BOOL		("ppp-tone-dial",	ppp_tone_dial),
	CFG_ENTRY_INT		("ppp-auth",		ppp_auth),
	CFG_ENTRY_STRING	("ppp-login",		ppp_login),
	CFG_ENTRY_STRING	("ppp-passwd",		ppp_passwd),
	CFG_ENTRY_BOOL		("ppp-crtscts",		ppp_crtscts),
/* Настройки ИПТ */
	CFG_ENTRY_BOOL		("bank-system",		bank_system),
	CFG_ENTRY_INT		("bank-pos-port",	bank_pos_port),
	CFG_ENTRY_IP		("bank-proc-ip",	bank_proc_ip),
	CFG_ENTRY_BOOL		("ext-pos",		ext_pos),
/* Настройки ККТ */
	CFG_ENTRY_BOOL		("kkt",			has_kkt),
	CFG_ENTRY_BOOL		("fiscal_mode",		fiscal_mode),
	CFG_ENTRY_INT		("fdo-iface",		fdo_iface),
	CFG_ENTRY_IP		("fdo-ip",		fdo_ip),
	CFG_ENTRY_UINT16	("fdo-port",		fdo_port),
	CFG_ENTRY_UINT32	("fdo-poll-period",	fdo_poll_period),
	CFG_ENTRY_IP		("kkt-ip",		kkt_ip),
	CFG_ENTRY_IP		("kkt-netmask",		kkt_netmask),
	CFG_ENTRY_IP		("kkt-gw",		kkt_gw),
	CFG_ENTRY_STRING	("gprs-apn",		kkt_gprs_apn),
	CFG_ENTRY_STRING	("gprs-user",		kkt_gprs_user),
	CFG_ENTRY_STRING	("gprs-passwd",		kkt_gprs_passwd),
	CFG_ENTRY_INT		("tax-system",		tax_system),
	CFG_ENTRY_INT		("kkt-log-level",	kkt_log_level),
	CFG_ENTRY_UINT32	("kkt-log-stream",	kkt_log_stream),
	CFG_ENTRY_INT		("tz-offs",		tz_offs),
	CFG_ENTRY_UINT32	("kkt-base-timeout",	kkt_base_timeout),
	CFG_ENTRY_UINT32	("kkt-brightness",	kkt_brightness),
	CFG_ENTRY_BOOL		("kkt-apc",		kkt_apc),
/* Настройки экрана */
	CFG_ENTRY_UINT32	("blank-time",		blank_time),
	CFG_ENTRY_INT		("color-scheme",	color_scheme),
	CFG_ENTRY_INT		("screen-mode",		scr_mode),
	CFG_ENTRY_BOOL		("screen-hints",	has_hint),
	CFG_ENTRY_BOOL		("simple-calc",		simple_calc),
/* Клавиатура */
	CFG_ENTRY_UINT32	("keyboard-rate",	kbd_rate),
	CFG_ENTRY_UINT32	("keyboard-delay",	kbd_delay),
	CFG_ENTRY_BOOL		("keyboard-beeps",	kbd_beeps),
};

/* Чтение конфигурации терминала */
bool read_cfg(void)
{
	name_str name;
	val_str val;
	int retval, i;
	uint8_t *cfg_base = (uint8_t *)&cfg;
	if (!read_cfg_file(STERM_CFG_NAME))
		return false;
	do {
		retval = scan_cfg_line(name, val);
		if (retval == scan_err)
			return false;
		for (i = 0; i < ASIZE(cfg_map); i++){
			if (!strcmp(name, cfg_map[i].name)){
				cfg_map[i].fn_read(val, cfg_base + cfg_map[i].offs);
				break;
			}
		}
	} while (retval != scan_eof);
	translate_color_scheme(cfg.color_scheme,
		&cfg.rus_color, &cfg.lat_color, &cfg.bg_color);
	return true;
}

/* Запись конфигурации терминала */
bool write_cfg(void)
{
	int i;
	bool ret = false;
	uint8_t *cfg_base = (uint8_t *)&cfg;
	val_str val;
	reset_cfg();
	for (i = 0; i < ASIZE(cfg_map); i++){
		cfg_map[i].fn_write(cfg_base + cfg_map[i].offs, val);
		if (!write_cfg_line(cfg_map[i].name, val))
			return false;
	}
	cfg_len = wr_index;
/*	if (remount_home(true)){*/
		ret = write_cfg_file(STERM_CFG_NAME);
/*		remount_home(false);
	}*/
	return ret;
}

/* Различные вспомогательные функции */
/* Преобразование цветовой схемы в цвета */
void translate_color_scheme(int scheme, Color *rus, Color *lat, Color *bg)
{
	struct {
		Color rus;
		Color lat;
		Color bg;
	} schemes[] = {
		{		/* Цвета-1 */
			RGB(0xff, 0xff, 0xff),
			RGB(0xff, 0xff, 0x00),
			RGB(0x00, 0x00, 0x80)
		},
		{		/* Цвета-2 */
			RGB(0xff, 0xff, 0x00),
			RGB(0x00, 0xff, 0xff),
			RGB(0x00, 0x80, 0x80)
		},
		{		/* Цвета-3 */
			RGB(0xff, 0xff, 0xff),
			RGB(0x00, 0x00, 0xff),
			RGB(0x80, 0x80, 0x80)
		},
		{		/* Цвета-4 */
			RGB(0xff, 0xff, 0x80),
			RGB(0xff, 0x80, 0x80),
			RGB(0x00, 0x70, 0xc0)
		},
		{		/* Цвета-5 */
			RGB(0xff, 0xff, 0x00),
			RGB(0x80, 0xff, 0x80),
			RGB(0x00, 0x00, 0x00)
		},
	};

	if ((scheme < 0) && (scheme >= ASIZE(schemes)))
			scheme = 0;
	*rus = schemes[scheme].rus;
	*lat = schemes[scheme].lat;
	*bg  = schemes[scheme].bg;
}

/* Определение ip хост-ЭВМ */
uint32_t get_x3_ip(void)
{
	return cfg.use_p_ip ? cfg.x3_p_ip : cfg.x3_s_ip;
}
