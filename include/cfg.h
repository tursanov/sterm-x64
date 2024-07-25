/*
 * �⥭��/������ 䠩�� ���䨣��樨 �ନ����.
 * ��ଠ�: name = 'value'
 * (c) gsr 2003-2024
 */

#if !defined CFG_H
#define CFG_H

#if defined __cplusplus
extern "C" {
#endif

#include "sterm.h"
#include "gui/gdi.h"		/* ��� ��।������ Color */

/* ���� �ନ���� */
struct term_addr{
	uint8_t gaddr;
	uint8_t iaddr;
} __attribute((__packed__));

/* ��� ���ਧ�樨 � ppp */
enum {
	ppp_chat,		/* � ������� �ணࠬ�� chat */
	ppp_pap,		/* PAP */
	ppp_chap,		/* CHAP */
};

/* ��࠭�祭�� 䠩�� ���䨣��樨 */
#define MAX_CFG_LEN		4096
#define MAX_NAME_LEN		32
#define MAX_VAL_LEN		64
typedef char name_str[MAX_NAME_LEN + 1];
typedef char val_str[MAX_VAL_LEN + 1];

/* ����ன�� �ନ���� */
struct term_cfg{
/* ���⥬� */
	uint8_t		gaddr;		/* ��㯯����... */
	uint8_t		iaddr;		/* ...� �������㠫�� ���� */
#if defined __WATCH_EXPRESS__
	uint32_t	watch_interval;	/* �����ਭ� �����⥬� "������" �� ��� */
#endif
	bool		use_iplir;	/* �ᯮ�짮���� �� ࠡ�� VipNet ������ */
/* ���譨� ���ன�⢠ */
	bool		has_xprn;	/* ����稥 ��� */
	val_str 	xprn_number;	/* �����᪮� ����� ��� */
	bool		has_aprn;	/* ����稥 ��� */
	val_str 	aprn_number;	/* �����᪮� ����� ��� */
	int		aprn_tty;	/* COM-����, � ���஬� ������祭� ���  */
	bool		has_sprn;	/* ����稥 ��� */
	int		s0;		/* ����� ���㬥��, �� */
	int		s1;		/* �ਭ� ���㬥��, �� */
	int		s2;		/* ����ﭨ� �� ���뢠����� ����-����, �� */
	int		s3;		/* ����� �࠭�� ⥪��, �� */
	int		s4;		/* ������ ��ࢮ� ��ப� ⥪��, �� */
	int		s5;		/* ������ ���� ����-���� #1, �� */
	int		s6;		/* ������ ���� ����-���� #2, �� */
	int		s7;		/* �������� ���ࢠ�, 1/8 �� */
	int		s8;		/* ����⠭� ���४樨 ��१� �� ९�୮� ��⪥, 1/8 �� */
	int		s9;		/* ����⠭� ���४樨 ����� �࠭��� ����஫쭮� �����, 1/8 �� */
/* TCP/IP */
	bool		use_ppp;	/* �ᯮ�짮���� PPP */
	uint32_t	local_ip;	/* ip �ନ���� */
	uint32_t	x3_p_ip;	/* �᭮���� ip ��� */
	uint32_t	x3_s_ip;	/* ����ୠ⨢�� ip ��� */
	bool		use_p_ip;	/* �ᯮ�짮���� ��ࢨ�� ip ��� (���� -- ����ୠ⨢��) */
	uint32_t	net_mask;	/* ��᪠ �����쭮� ����� */
	uint32_t	gateway;	/* �� �����쭮� �� */
/* FIXME: �஢���� ࠧ�� ��ᨨ �� ���樠⨢� ��� */
	bool		tcp_cbt;	/* ࠧ�� ��ᨨ �� ���樠⨢� �ନ���� */
/* PPP (����ன�� �� �������� �ࢥ஬) */
	int		ppp_tty;	/* COM-����, �� ���஬ �⮨� ����� */
	int		ppp_speed;	/* ᪮���� ��᫥����⥫쭮�� ���� */
	val_str		ppp_init;	/* ��ப� ���樠����樨 ������ */
	val_str		ppp_phone;	/* ����� ������� (���� ��� �뤥������ �����) */
	bool		ppp_tone_dial;	/* ⮭���/������� ����� */
	int		ppp_auth;	/* ⨯ ���ਧ�樨 PPP */
	val_str		ppp_login;	/* ��� ���짮��⥫� ��� �ࢥ� PPP */
	val_str		ppp_passwd;	/* ��஫� (�࠭���� � ����஢����� ����) */
	bool		ppp_crtscts;	/* �ࠢ����� ������� (XON/XOFF ��� RTS/CTS) */
/* ����ன�� ��� */
	bool 		bank_system;	/* ࠡ�� � ��� */
	int		bank_pos_port;	/* COM-����, � ���஬� ������祭 POS-����� */
	uint32_t 	bank_proc_ip;	/* ip-���� ����ᨭ������ 業�� */
	bool		ext_pos;	/* ���譨� POS-�ନ��� */
/* ����ன�� ��� */
	bool		has_kkt;	/* ����稥 ��� */
	bool		fiscal_mode;	/* �᪠��� ०�� ��� */
	int		fdo_iface;	/* ����䥩� ����������⢨� � ��� */
	uint32_t	fdo_ip;		/* ip-���� ��� */
	uint16_t	fdo_port;	/* TCP-���� ��� */
	uint32_t	fdo_poll_period;/* ��ਮ� ���� (ᥪ) ��� �� ࠡ�� � ��� �� USB */
	uint32_t	kkt_ip;		/* ip-���� ��� */
	uint32_t	kkt_netmask;	/* ��᪠ ����� ��� */
	uint32_t	kkt_gw;		/* �� �����쭮� �� ��� */
	val_str		kkt_gprs_apn;	/* ��� �窨 ����㯠 GPRS */
	val_str		kkt_gprs_user;	/* ��� ���짮��⥫� GPRS */
	val_str		kkt_gprs_passwd;/* ��஫� GPRS */
	int		tax_system;	/* ��⥬� ��������������� � ��� */
	int		kkt_log_level;	/* �஢��� ��⠫���樨 �� ��� */
	uint32_t	kkt_log_stream;	/* ��⮪ ��� ��ᬮ�� � �� ��� */
	int		tz_offs;	/* ᬥ饭�� ���⭮�� �६��� �⭮�⥫쭮 ��᪮�᪮�� (�) */
	uint32_t	kkt_base_timeout;	/* ����� ⠩���� ��� */
	uint32_t	kkt_brightness;	/* �મ��� ���� */
	bool		kkt_apc;	/* ��⮬���᪠� ����� 祪�� ��� */
/* ����ன�� �࠭� */
	uint32_t	blank_time;	/* �६� ��襭�� �࠭� (���) (0 -- ��� ��襭��) */
	int		color_scheme;	/* 梥⮢�� �奬� */
/* ������騥 �� ���祭�� ��������� �� �᭮����� color_scheme */
	Color		rus_color;	/* 梥� ���᪨� �㪢 */
	Color		lat_color;	/* 梥� ��⨭᪨� �㪢 */
	Color		bg_color;	/* 梥� 䮭� */
	int		scr_mode;	/* ࠧ�襭�� �࠭� (32x8 ��� 80x20) */
	bool		has_hint;	/* ����稥 ������ ���᪠��� */
	bool		simple_calc;	/* ���⮩ �������� (� ��ப� �����) */
/* ��������� */
	uint32_t	kbd_rate;	/* ᪮���� ��⮯���� ���������� (ᨬ�./ᥪ) */
	uint32_t	kbd_delay;	/* ����ઠ ��। ��砫�� ��⮯���� (��) */
	bool		kbd_beeps;	/* ��㪮��� �������� ������ ������ */
};

extern struct term_cfg	cfg;		/* ��ࠬ���� �ନ���� */

extern bool read_cfg(void);
extern bool write_cfg(void);

extern void translate_color_scheme(int scheme,
		Color *rus, Color *lat, Color *bg);
/* ��।������ ip ���-��� */
extern uint32_t get_x3_ip(void);

#if defined __cplusplus
}
#endif

#endif		/* CFG_H */
