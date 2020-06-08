/*
 * Чтение/запись файла конфигурации терминала.
 * Формат: name = 'value'
 * (c) gsr 2003-2019.
 */

#if !defined CFG_H
#define CFG_H

#include "sterm.h"
#include "gui/gdi.h"		/* для определения Color */

/* Адрес терминала */
struct term_addr{
	uint8_t gaddr;
	uint8_t iaddr;
} __attribute((__packed__));

/* Тип авторизации в ppp */
enum {
	ppp_chat,		/* с помощью программы chat */
	ppp_pap,		/* PAP */
	ppp_chap,		/* CHAP */
};

/* Ограничения файла конфигурации */
#define MAX_CFG_LEN		4096
#define MAX_NAME_LEN		32
#define MAX_VAL_LEN		64
typedef char name_str[MAX_NAME_LEN + 1];
typedef char val_str[MAX_VAL_LEN + 1];

/* Настройки терминала */
struct term_cfg{
/* Система */
	uint8_t		gaddr;		/* групповой... */
	uint8_t		iaddr;		/* ...и индивидуальный адрес */
#if defined __WATCH_EXPRESS__
	uint32_t	watch_interval;	/* мониторинг подсистемы "Экспресс" на хосте */
#endif
	bool		use_iplir;	/* использовать при работе VipNet клиент */
	bool		autorun_local;	/* автоматический запуск пригородного приложения */
/* Внешние устройства */
	bool		has_xprn;	/* наличие ОПУ */
	val_str 	xprn_number;	/* заводской номер ОПУ */
	bool		has_aprn;	/* наличие ДПУ */
	val_str 	aprn_number;	/* заводской номер ДПУ */
	int		aprn_tty;	/* COM-порт, к которому подключено ДПУ  */
	bool		has_lprn;	/* наличие ППУ */
	bool		has_sd_card;	/* наличие в ППУ карты памяти */
	int		s0;		/* длина документа, мм */
	int		s1;		/* ширина документа, мм */
	int		s2;		/* расстояние до считываемого штрих-кода, мм */
	int		s3;		/* левая граница текста, мм */
	int		s4;		/* позиция первой строки текста, мм */
	int		s5;		/* позиция печати штрих-кода #1, мм */
	int		s6;		/* позиция печати штрих-кода #2, мм */
	int		s7;		/* межстрочный интервал, 1/8 мм */
	int		s8;		/* константа коррекции отреза по реперной метке, 1/8 мм */
	int		s9;		/* константа коррекции левой границы контрольной ленты, 1/8 мм */
/* TCP/IP */
	bool		use_ppp;	/* использовать PPP */
	uint32_t	local_ip;	/* ip терминала */
	uint32_t	x3_p_ip;	/* основной ip хоста */
	uint32_t	x3_s_ip;	/* альтернативный ip хоста */
	bool		use_p_ip;	/* использовать первичный ip хоста (иначе -- альтернативный) */
	uint32_t	net_mask;	/* маска локальной подсети */
	uint32_t	gateway;	/* шлюз локальной сети */
/* FIXME: проверить разрыв сессии по инициативе хоста */
	bool		tcp_cbt;	/* разрыв сессии по инициативе терминала */
/* PPP (настройки сети задаются сервером) */
	int		ppp_tty;	/* COM-порт, на котором стоит модем */
	int		ppp_speed;	/* скорость последовательного порта */
	val_str		ppp_init;	/* строка инициализации модема */
	val_str		ppp_phone;	/* номер дозвона (пусто для выделенной линии) */
	bool		ppp_tone_dial;	/* тоновый/импульсный набор */
	int		ppp_auth;	/* тип авторизации PPP */
	val_str		ppp_login;	/* имя пользователя для сервера PPP */
	val_str		ppp_passwd;	/* пароль (хранится в зашифрованном виде) */
	bool		ppp_crtscts;	/* управление модемом (XON/XOFF или RTS/CTS) */
/* Настройки ИПТ */
	bool 		bank_system;	/* работа с ИПТ */
	int		bank_pos_port;	/* COM-порт, к которому подключен POS-эмулятор */
	uint32_t 	bank_proc_ip;	/* ip-адрес процессингового центра */
	bool		ext_pos;	/* внешний POS-терминал */
/* Настройки ККТ */
	bool		has_kkt;	/* наличие ККТ */
	bool		fiscal_mode;	/* фискальный режим ККТ */
	int		fdo_iface;	/* интерфейс взаимодействия с ОФД */
	uint32_t	fdo_ip;		/* ip-адрес ОФД */
	uint16_t	fdo_port;	/* TCP-порт ОФД */
	uint32_t	fdo_poll_period;/* период опроса (сек) ККТ при работе с ОФД по USB */
	uint32_t	kkt_ip;		/* ip-адрес ККТ */
	uint32_t	kkt_netmask;	/* маска подсети ККТ */
	uint32_t	kkt_gw;		/* шлюз локальной сети ККТ */
	val_str		kkt_gprs_apn;	/* имя точки доступа GPRS */
	val_str		kkt_gprs_user;	/* имя пользователя GPRS */
	val_str		kkt_gprs_passwd;/* пароль GPRS */
	int		tax_system;	/* система налогообложения в ККТ */
	int		kkt_log_level;	/* уровень детализации КЛ ККТ */
	uint32_t	kkt_log_stream;	/* поток для просмотра в КЛ ККТ */
	int		tz_offs;	/* смещение местного времени относительно московского (ч) */
	uint32_t	kkt_base_timeout;	/* опорный таймаут ККТ */
	uint32_t	kkt_brightness;	/* яркость печати */
	bool		kkt_apc;	/* автоматическая печать чеков ККТ */
/* Настройки экрана */
	uint32_t	blank_time;	/* время гашения экрана (мин) (0 -- нет гашения) */
	int		color_scheme;	/* цветовая схема */
/* Следующие три значения вычисляются на основании color_scheme */
	Color		rus_color;	/* цвет русских букв */
	Color		lat_color;	/* цвет латинских букв */
	Color		bg_color;	/* цвет фона */
	int		scr_mode;	/* разрешение экрана (32x8 или 80x20) */
	bool		has_hint;	/* наличие области подсказок */
	bool		simple_calc;	/* простой калькулятор (в строке статуса) */
/* Клавиатура */
	uint32_t	kbd_rate;	/* скорость автоповтора клавиатуры (симв./сек) */
	uint32_t	kbd_delay;	/* задерка перед началом автоповтора (мс) */
	bool		kbd_beeps;	/* звуковая индикация нажатия клавиш */
};

extern struct term_cfg	cfg;		/* параметры терминала */

extern bool read_cfg(void);
extern bool write_cfg(void);

extern void translate_color_scheme(int scheme,
		Color *rus, Color *lat, Color *bg);
/* Определение ip хост-ЭВМ */
extern uint32_t get_x3_ip(void);

#endif		/* CFG_H */
