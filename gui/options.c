/* ����ன�� ��ࠬ��஢ �ନ����. (c) gsr & alex 2000-2004, 2018-2019. */

#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "gui/dialog.h"
#include "gui/exgdi.h"
#include "gui/menu.h"
#include "gui/options.h"
#include "gui/scr.h"
#include "kkt/kkt.h"
#include "log/express.h"
#include "prn/express.h"
#include "prn/local.h"
#include "cfg.h"
#include "devinfo.h"
#include "iplir.h"
#include "paths.h"
#include "sterm.h"
#include "tki.h"

#define OPTN_N_BUTTONS		2

/* ������� ���஢ ��� ।���஢���� */
#define OPTN_EDIT_BUF_LEN	64
#define OPTN_HEX_EDIT_LEN	2
#define OPTN_INT_EDIT_LEN	11
#define OPTN_UINT16_EDIT_LEN	5
#define OPTN_UINT32_EDIT_LEN	10
#define OPTN_IP_EDIT_LEN	15
#define OPTN_STR_EDIT_LEN	MAX_VAL_LEN	/* ���ᨬ���� ࠧ��� */

/* ��� ����� � 䠩�� ���䨣��樨 */
enum {
	ini_void,
	ini_bool,
	ini_hex,
	ini_int,
	ini_uint16,
	ini_uint32,
	ini_ipaddr,
	ini_str,
	ini_psw,
};

/* ��� ����� ����� � ���� ����஥� */
enum {
	optn_str_enum,		/* ᯨ᮪ ��ப */
	optn_int_enum,		/* ᯨ᮪ 楫�� �ᥫ */
	optn_hex_edit,		/* ��⭠������ ���� */
	optn_int_edit,		/* 楫�� �᫮ � ������ */
	optn_uint_edit,		/* 楫�� �᫮ ��� ����� (uint16/uint32) */
	optn_ip_edit,		/* ip ���� */
	optn_str_edit,		/* ���筠� ��ப� ����� */
	optn_lstr_edit,		/* ��ப� ����� ��⨭᪨� �㪢 */
	optn_psw_edit,		/* ��஫� */
	optn_static_txt,	/* ����᪨� ⥪�� (����� ���) */
	optn_delim,		/* ࠧ����⥫� ��㯯 */
};

/* �������� ⨯� ������ � ���� ����� */
union optn_un{
	bool		flag;
	int		i;
	uint8_t		u8;
	uint32_t	u32;
	uint16_t	u16;
	char		ip[OPTN_IP_EDIT_LEN + 1];
	char		s[OPTN_STR_EDIT_LEN + 1];
	const char	*txt;		/* optn_static_txt */
} optn_un;

/* ������� ����� ���� ����஥� */
struct optn_item{
	const char *name;	/* �������� ��ࠬ��� */
	const char *descr;	/* ���ᠭ�� ��ࠬ��� */
	int ot;			/* (option type) -- ⨯ ����� ����� */
	int max_len;		/* ���ᨬ��쭮� �᫮ ������ ��� optn_str/psw_edit */
	union optn_un v, vv;	/* ��室��� � ⥪�饥 ���祭�� ����� */
	const void *d;		/* ᯨ᮪ ���祭�� ����� */
	int n;			/* �᫮ ���祭�� ����᫨���� ⨯� */
	bool enabled;		/* �롮� ����� ࠧ�襭 */
	int at;			/* (argument type) -- ⨯ ���祭�� � 䠩�� ����஥� */
	int offset;		/* ᬥ饭�� � ������� ���䨣��樨 */
	void (*on_change)(struct optn_item *);	/* ��뢠���� �� ��������� ���祭�� */
};

/* ��㯯� ����⮢ ����ன�� */
struct optn_group{
	const char *name;
	struct optn_item *items;
	int n_items;
/* ��뢠���� �� ����⨨ �� ��� �஢�ન �ࠢ��쭮�� ������ */
	bool (*on_exit)(const struct optn_group *group);
};

/* �㭪樨 ���⭮�� �맮�� */
static void on_iplir_change(struct optn_item *item);
static void on_xprn_change(struct optn_item *item);
static void on_aprn_change(struct optn_item *item);
static void on_bank_change(struct optn_item *item);
static void on_kkt_change(struct optn_item *item);
static void on_fdo_iface_change(struct optn_item *item);
static void on_scheme_change(struct optn_item *item);

/* �㭪樨 ���⭮�� �맮�� ��� ��㯯 */
static bool on_exit_devices(const struct optn_group *group);
static bool on_exit_check_ip(const struct optn_group *group);
static bool on_exit_ppp(const struct optn_group *group);
static bool on_exit_bank_system(const struct optn_group *group);

static bool scheme_shown = false;
static bool scheme_changed = false;

/*
 * ��ࠬ���� ��� �� �室� � ���� "���譨� ���ன�⢠" ������ ������
 * ⮫쪮 ���� ࠧ.
 */
bool lprn_params_read = false;
static const char lprn_hdr[] = "���-----------";

/* TCP/IP */
static const char *optn_use_ppp[] = {"��⥢�� ����", "PPP"};
static const char *optn_cbt[]={"����-���", "��ନ���"};

/* PPP */
static const char *optn_dial_mode[] = {"�������", "������"};
static const char *optn_ppp_auth[] = {"Chat", "PAP", "CHAP"};
static const char *optn_ppp_rtscts[] = {"XON/XOFF", "RTS/CTS"};

/* ��� */
static const char *optn_fdo_iface[] = {"USB", "Ethernet", "GPRS", "�����."};
static int optn_fdo_poll_period[] = {0, 3, 5, 10, 15, 30, 60, 90, 180, 600};
static const char *optn_tax_systems[] = 
	{"���", "��� ��室", "��� ��室-��室", "����", "���", "��⥭�"};
static const char *optn_kkt_log_level[] =
	{"���", "���ଠ��", "�।�०�����", "�訡��", "���"};
static const char *optn_kkt_log_stream[] =
	{"��ࠢ�����", "�����", "���", "���"};
static int optn_tz_offs[] = {-12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
static uint32_t optn_kkt_base_timeout[] = {100, 200, 300, 400, 500, 1000};

/* ��࠭ */
static int optn_ssaver[] = {0, 1, 2, 5, 7, 10, 15, 20};
static const char *optn_scheme_names[] = {"����-1", "����-2", "����-3", "����-4", "����-5"};
static const char *optn_scr_mode[] = {"80x20", "32x8"};
static const char *optn_calc[] = {"����襭��", "���⮩"};

/* ��������� */
static int optn_kbd_rate[] = {5, 7, 10, 15, 20, 25, 30};
static int optn_kbd_delay[] = {250, 500, 750, 1000};

/* COM-���� */
static const char *optn_com_port[] = {"COM1", "COM2", "COM3", "COM4"};
static int optn_com_baud[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};

/* �����ਭ� "������" */
#if defined __WATCH_EXPRESS__
static int optn_watches[] = {0, 10, 30, 60, 90, 120, 180, 300, 600};
#endif

/* bool */
static const char *optn_bool1[] = {"���", "��"};
static const char *optn_bool2[] = {"���", "����"};
static const char *optn_bool3[] = {"�몫.", "���."};

/* ��㯯� ����஥� */

#define OPTN_STATIC(nm, text, len) \
	{ \
		.name		= nm, \
		.descr		= NULL, \
		.ot		= optn_static_txt, \
		.max_len	= len, \
		.v		= {.txt = text}, \
		.vv		= {.txt = text}, \
		.d		= NULL, \
		.n		= 0, \
		.enabled	= false, \
		.at		= ini_void, \
		.offset		= -1, \
		.on_change	= NULL \
	}

#define OPTN_ENUM(nm, dsc, et, lst, atype, fld, on_chg) \
	{ \
		.name		= nm, \
		.descr		= dsc, \
		.ot		= et, \
		.max_len	= 0, \
		.v		= {.i = 0}, \
		.vv		= {.i = 0}, \
		.d		= lst, \
		.n		= ASIZE(lst), \
		.enabled	= true, \
		.at		= atype, \
		.offset		= offsetof(struct term_cfg, fld), \
		.on_change	= on_chg \
	}

#define OPTN_INT_ENUM(nm, dsc, lst, at, fld, on_chg) \
		OPTN_ENUM(nm, dsc, optn_int_enum, lst, at, fld, on_chg)

#define OPTN_STR_ENUM(nm, dsc, lst, at, fld, on_chg) \
		OPTN_ENUM(nm, dsc, optn_str_enum, lst, at, fld, on_chg)

#define OPTN_BOOL_BASE(nm, dsc, lst, fld, on_chg) \
	{ \
		.name		= nm, \
		.descr		= dsc, \
		.ot		= optn_str_enum, \
		.max_len	= 0, \
		.v		= {.flag = true}, \
		.vv		= {.flag = true}, \
		.d		= lst, \
		.n		= ASIZE(lst), \
		.enabled	= true, \
		.at		= ini_bool, \
		.offset		= offsetof(struct term_cfg, fld), \
		.on_change	= on_chg \
	}

#define OPTN_BOOL1(nm, dsc, fld, on_chg) \
		OPTN_BOOL_BASE(nm, dsc, optn_bool1, fld, on_chg)

#define OPTN_BOOL2(nm, dsc, fld, on_chg) \
		OPTN_BOOL_BASE(nm, dsc, optn_bool2, fld, on_chg)

#define OPTN_BOOL3(nm, dsc, fld, on_chg) \
		OPTN_BOOL_BASE(nm, dsc, optn_bool3, fld, on_chg)

#define OPTN_INT_EDIT_BASE(nm, dsc, otype, atype, fld, on_chg) \
	{ \
		.name		= nm, \
		.descr		= dsc, \
		.ot		= otype, \
		.max_len	= 0, \
		.v		= {.i = 0}, \
		.vv		= {.i = 0}, \
		.d		= NULL, \
		.n		= 0, \
		.enabled	= true, \
		.at		= atype, \
		.offset		= offsetof(struct term_cfg, fld), \
		.on_change	= on_chg \
	}

#define OPTN_HEX_EDIT(nm, dsc, fld, on_chg) \
		OPTN_INT_EDIT_BASE(nm, dsc, optn_hex_edit, ini_hex, fld, on_chg)

#define OPTN_INT_EDIT(nm, dsc, fld, on_chg) \
		OPTN_INT_EDIT_BASE(nm, dsc, optn_int_edit, ini_int, fld, on_chg)

#define OPTN_UINT16_EDIT(nm, dsc, fld, on_chg) \
		OPTN_INT_EDIT_BASE(nm, dsc, optn_uint_edit, ini_uint16, fld, on_chg)

#define OPTN_UINT32_EDIT(nm, dsc, fld, on_chg) \
		OPTN_INT_EDIT_BASE(nm, dsc, optn_uint_edit, ini_uint32, fld, on_chg)

#define OPTN_IP_EDIT(nm, dsc, fld, on_chg) \
	{ \
		.name		= nm, \
		.descr		= dsc, \
		.ot		= optn_ip_edit, \
		.max_len	= 0, \
		.v		= {.ip = "0.0.0.0"}, \
		.vv		= {.ip = "0.0.0.0"}, \
		.d		= NULL, \
		.n		= 0, \
		.enabled	= true, \
		.at		= ini_ipaddr, \
		.offset		= offsetof(struct term_cfg, fld), \
		.on_change	= on_chg \
	}

#define OPTN_STR_EDIT_BASE(nm, dsc, otype, ml, fld, on_chg) \
	{ \
		.name		= nm, \
		.descr		= dsc, \
		.ot		= otype, \
		.max_len	= ml, \
		.v		= {.s = ""}, \
		.vv		= {.s = ""}, \
		.d		= NULL, \
		.n		= 0, \
		.enabled	= true, \
		.at		= ini_str, \
		.offset		= offsetof(struct term_cfg, fld), \
		.on_change	= on_chg \
	}

#define OPTN_STR_EDIT(nm, dsc, ml, fld, on_chg) \
		OPTN_STR_EDIT_BASE(nm, dsc, optn_str_edit, ml, fld, on_chg)

#define OPTN_LSTR_EDIT(nm, dsc, ml, fld, on_chg) \
		OPTN_STR_EDIT_BASE(nm, dsc, optn_lstr_edit, ml, fld, on_chg)

#define OPTN_PSW_EDIT(nm, dsc, ml, fld, on_chg) \
		OPTN_STR_EDIT_BASE(nm, dsc, optn_psw_edit, ml, fld, on_chg)

/* ���⥬� */
static struct optn_item sys_optn_items[] = {
	OPTN_HEX_EDIT("��㯯���� ����", "��㯯���� ���� �ନ����", gaddr, NULL),
	OPTN_HEX_EDIT("�������㠫�� ����", "�������㠫�� ���� �ନ����", iaddr, NULL),
#if defined __WATCH_EXPRESS__
	OPTN_INT_ENUM("�����ਭ� \"������\"",
		"��ਮ��筮��� (ᥪ) ���뫪� ����ᮢ\r\n"
		"��� �஢�ન ࠡ��ᯮᮡ����\r\n"
		"�����⥬� \"������\" �� ���-���",
		optn_watches, ini_uint32, watch_interval, NULL),
#endif
	OPTN_BOOL3("���஢���� VipNet", "�ᯮ�짮���� �� ࠡ�� �� TCP/IP\r\n"
		"��஢���� ������ � ������� ����� VipNet\r\n���⥪�",
		use_iplir, on_iplir_change),
#if 0
	OPTN_BOOL1("��⮧���� �ਣ�த.", "��⮬���᪨� ����� �ਣ�த����\r\n"
		"�ਫ������ ��᫥ ����祭�� �ନ����", autorun_local, NULL),
#endif
};

/* ���譨� ���ன�⢠ */
static struct optn_item dev_optn_items[] = {
	OPTN_BOOL2("���", "����稥 �᭮����� �ਭ�� � ��⠢�\r\n�ନ����",
		has_xprn, on_xprn_change),
	OPTN_STR_EDIT("����� ���", "�����᪮� ����� �᭮�����\r\n�����饣� ���ன�⢠",
		PRN_NUMBER_LEN, xprn_number, NULL),
	OPTN_BOOL2("���", "����稥 �������⥫쭮�� �ਭ��\r\n"
		"� ��⠢� �ନ����", has_aprn, on_aprn_change),
	OPTN_STR_EDIT("����� ���", "�����᪮� ����� �������⥫쭮��\r\n"
		"�����饣� ���ன�⢠", PRN_NUMBER_LEN, aprn_number, NULL),
	OPTN_STR_ENUM("���� ���", "��᫥����⥫�� ����, � ���஬�\r\n"
		"������砥��� ���", optn_com_port, ini_int, aprn_tty, NULL),
	OPTN_BOOL2("���", "����稥 � ��⠢� �ନ���� �ਭ��\r\n�ਣ�த��� ����⮢",
		has_lprn, NULL),
	OPTN_STATIC("����� ���", (const char *)lprn_number, sizeof(lprn_number)),
	OPTN_BOOL2("���� ����� ���",
		"����稥 � ��⠢� ���\r\n����� ����� ��� �࠭����\r\n"
		"��ࠧ�� �⯥�⠭��� �������", has_sd_card, NULL),
	OPTN_STATIC("------��ࠬ���� ����", lprn_hdr, sizeof(lprn_hdr) - 1),
	OPTN_INT_EDIT("����� ������", "����� ���㬥�� � ��", s0, NULL),
	OPTN_INT_EDIT("��ਭ� ������", "��ਭ� ���㬥�� � ��", s1, NULL),
	OPTN_INT_EDIT("�����. �� ����-����", "�����ﭨ� �� ���뢠�����\r\n"
		"����-���� � ��", s2, NULL),
	OPTN_INT_EDIT("����� �࠭�� ⥪��", "����� �࠭�� ⥪�� � ��", s3, NULL),
	OPTN_INT_EDIT("��ࢠ� ��ப� ⥪��", "������ ��ࢮ� ��ப� ⥪�� � ��",
		s4, NULL),
	OPTN_INT_EDIT("������ ����-���� #1", "������ ���� ����-���� #1 � ��",
		s5, NULL),
	OPTN_INT_EDIT("������ ����-���� #2", "������ ���� ����-���� #2 � ��",
		s6, NULL),
	OPTN_INT_EDIT("�������� ���ࢠ�", "�������� ���ࢠ� � �窠�\r\n"
		"(1 �窠 = 1/8 ��)", s7, NULL),
	OPTN_INT_EDIT("���४�� ��१�", "����⠭� ���४樨 ��१�\r\n"
		"�� ९�୮� ��⪥ � �窠�\r\n(1 �窠 = 1/8 ��)", s8, NULL),
	OPTN_INT_EDIT("���४�� ����� ��. ��", "����⠭� ���४樨 ����� �࠭���\r\n"
		"����஫쭮� ����� � �窠�\r\n(1 �窠 = 1/8 ��)", s9, NULL),
};

/* TCP/IP */
static struct optn_item ip_optn_items[] = {
	OPTN_ENUM("��� ������祭��", "��� ������祭�� �ନ����\r\n� ���",
		optn_str_enum, optn_use_ppp, ini_bool, use_ppp, NULL),
	OPTN_IP_EDIT("IP �ନ����", "IP-���� �ନ����", local_ip, NULL),
	OPTN_IP_EDIT("IP ���-���", "IP-���� ���-���", x3_p_ip, NULL),
	OPTN_IP_EDIT("IP ���-���-2", "����ୠ⨢�� IP-���� ���-���", x3_s_ip, NULL),
	OPTN_IP_EDIT("��᪠ �����","��᪠ �����쭮� ��", net_mask, NULL),
	OPTN_IP_EDIT("IP �","IP-���� �", gateway, NULL),
	OPTN_ENUM("����� ��ᨨ","���樠�� ࠧ�뢠 ��ᨨ",
		optn_str_enum, optn_cbt, ini_bool, tcp_cbt, NULL),
};

/* PPP */
static struct optn_item ppp_optn_items[] = {
	OPTN_STR_ENUM("���� ������", "��᫥����⥫�� ���� � ���஬�\r\n"
		"������砥��� ����� ��� �裡 � ���",
		optn_com_port, ini_int, ppp_tty, NULL),
	OPTN_INT_ENUM("����. ��।�� (baud)", "������� ������ ����묨\r\n"
		"� �������", optn_com_baud, ini_int, ppp_speed, NULL),
	OPTN_LSTR_EDIT("��ப� ���樠����樨", "��ப�, ��।������� � �����\r\n"
		"��� ��� ���樠����樨.", OPTN_STR_EDIT_LEN, ppp_init, NULL),
	OPTN_LSTR_EDIT("����� �������", "����� ⥫�䮭� ISP\r\n"
		"��������: �᫨ �� ࠡ�⠥� �� \r\n"
		"�뤥������ �����, ��⠢�� �� ����\r\n"
		"�����", OPTN_STR_EDIT_LEN, ppp_phone, NULL),
	OPTN_STR_ENUM("����� �����", "��� ����� ����� �� ᮥ�������\r\n"
		"�� �������㥬�� �����", optn_dial_mode, ini_bool, ppp_tone_dial, NULL),
	OPTN_STR_ENUM("��� ���ਧ�樨", "��� ���ਧ�樨 ���짮��⥫� �� ࠡ��\r\n"
		"�� �������㥬�� �����", optn_ppp_auth, ini_int, ppp_auth, NULL),
	OPTN_LSTR_EDIT("��� ���짮��⥫�", "��� ���짮��⥫� �� ࠡ��\r\n"
		"�� �������㥬�� �����", OPTN_STR_EDIT_LEN, ppp_login, NULL),
	OPTN_PSW_EDIT("��஫� ���짮��⥫�", "��஫� ���짮��⥫� �� ࠡ��\r\n"
		"�� �������㥬�� �����", OPTN_STR_EDIT_LEN, ppp_passwd, NULL),
	OPTN_STR_ENUM("���. �������", "���ᮡ �ࠢ����� �������",
		optn_ppp_rtscts, ini_bool, ppp_crtscts, NULL),
};

/* ��� */
static struct optn_item bank_optn_items[] = {
	OPTN_BOOL2("���� � ��� \"������\"", "��⨢��� ��� \"������\"\r\n"
		"��� ������ ����⮢\r\n���।�⢮� ������᪨� ����",
		bank_system, on_bank_change),
	OPTN_STR_ENUM("���� ���", "��᫥����⥫�� ���� � ���஬�\r\n"
		"������砥��� ���", optn_com_port, ini_int, bank_pos_port, NULL),
	OPTN_IP_EDIT("IP �����. 業��","IP-���� ����ᨭ������ 業��",
		bank_proc_ip, NULL),
#if defined __EXT_POS__
	OPTN_BOOL2("���譨� POS", "� ����⢥ ��� �ᯮ������ ���譨�\r\n"
		"POS-�ନ���", ext_pos, NULL),
#endif
};

/* ��� */
#if defined __GNUC__ && (__GNUC__ > 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsizeof-pointer-div"
#endif
static struct optn_item kkt_optn_items[] = {
	OPTN_BOOL2("����稥 ���", "����稥 ��� � ��⠢� �ନ����",
		has_kkt, on_kkt_change),
	OPTN_BOOL1("��᪠��� ०��", "����� ��� � �᪠�쭮� ०���",
		fiscal_mode, NULL),
	OPTN_STR_ENUM("���� � ���", "���ᮡ �裡 ��� � ���",
		optn_fdo_iface, ini_int, fdo_iface, on_fdo_iface_change),
	OPTN_IP_EDIT("IP ���", "IP-���� ���", fdo_ip, NULL),
	OPTN_UINT16_EDIT("���� ���", "TCP-���� ���", fdo_port, NULL),
	OPTN_ENUM("��ਮ� ����", "��ਮ� ���� ��� �� ࠡ�� � ���\r\n�� USB (ᥪ)",
		optn_int_enum, optn_fdo_poll_period, ini_uint32, fdo_poll_period, NULL),
	OPTN_IP_EDIT("IP ���", "IP-���� ���", kkt_ip, NULL),
	OPTN_IP_EDIT("��᪠ �����", "��᪠ �����쭮� �� ���", kkt_netmask, NULL),
	OPTN_IP_EDIT("IP �", "IP-���� �", kkt_gw, NULL),
	OPTN_LSTR_EDIT("APN", "��� �窨 ����㯠 �� ࠡ�� � ���\r\n�� GPRS",
		OPTN_STR_EDIT_LEN, kkt_gprs_apn, NULL),
	OPTN_LSTR_EDIT("��� ���짮��⥫�", "��� ���짮��⥫� �� ࠡ�� � ���\r\n�� GPRS",
		OPTN_STR_EDIT_LEN, kkt_gprs_user, NULL),
	OPTN_PSW_EDIT("��஫�", "��஫� �� ࠡ�� � ��� �� GPRS",
		OPTN_STR_EDIT_LEN, kkt_gprs_passwd, NULL),
	OPTN_STR_ENUM("���������������", "���⥬� ���������������",
		optn_tax_systems, ini_uint32, tax_system, NULL),
	OPTN_STR_ENUM("��⠫����� ��3", "�஢��� ��⠫���樨 �� ࠡ��� � ���",
		optn_kkt_log_level, ini_int, kkt_log_level, NULL),
	OPTN_STR_ENUM("��⮪ � ��3", "��⮪, ��� ���ண� �⮡ࠦ����� �����\r\n"
		"� �� ࠡ��� � ���", optn_kkt_log_stream, ini_uint32, kkt_log_stream, NULL),
	OPTN_INT_ENUM("��ᮢ�� ����", "���饭�� � ��� ���⭮�� �६���\r\n"
		"�⭮�⥫쭮 ��᪮�᪮��", optn_tz_offs, ini_int, tz_offs, NULL),
	OPTN_ENUM("����� ⠩����, ��", "�� ⠩����� ࠡ��� � ��� ���������\r\n"
		"�� �᭮����� ���୮��", optn_int_enum, optn_kkt_base_timeout,
		ini_uint32, kkt_base_timeout, NULL),
	OPTN_INT_ENUM("�મ��� ����", "�મ��� ���� ���.\r\n"
		"��� ����� ���祭��, ⥬ ��� �મ���",
		NULL, ini_uint32, kkt_brightness, NULL),
	OPTN_BOOL1("��⮯���� 祪��", "��⮬���᪠� ����� 祪�� �� ���",
		kkt_apc, NULL),
};
#if defined __GNUC__ && (__GNUC__ > 6)
#pragma GCC diagnostic pop
#endif

/* ��࠭ */
static struct optn_item scr_optn_items[] = {
	OPTN_INT_ENUM("�몫�祭�� �࠭�", "�६� ����⨢���� ��। ��襭���\r\n�࠭� (���)",
		optn_ssaver, ini_uint32, blank_time, NULL),
	OPTN_STR_ENUM("���⮢�� �奬�","���⮢�� �奬� �ନ����",
		optn_scheme_names, ini_int, color_scheme, on_scheme_change),
	OPTN_STR_ENUM("����� �࠭�", "80x20\r\n32x8",
		optn_scr_mode, ini_int, scr_mode, NULL),
	OPTN_BOOL2("����稥 ���᪠���", "�뢮� ���᪠��� � ������ ���\r\n�࠭�",
		has_hint, NULL),
	OPTN_STR_ENUM("����� ��������", "����� �⮡ࠦ���� �࠭����\r\n��������",
		optn_calc, ini_bool, simple_calc, NULL),
};

/* ��������� */
static struct optn_item kbd_optn_items[] = {
	OPTN_INT_ENUM("������� ����������", "������� ��⮯���� ����������",
		optn_kbd_rate, ini_uint32, kbd_rate, NULL),
	OPTN_INT_ENUM("����প� ��⮯����", "����প� ��⮯���� ����������",
		optn_kbd_delay, ini_uint32, kbd_delay, NULL),
	OPTN_BOOL2("��㪮��� ��������", "������� ��㪮���� ᨣ����\r\n"
		"�� ����⨨ ������", kbd_beeps, NULL),
};

/* ��㯯� ��ࠬ��஢ ����ன�� */
enum {
	OPTN_GROUP_MENU = -1,
	OPTN_GROUP_SYSTEM,
	OPTN_GROUP_DEVICES,
	OPTN_GROUP_TCPIP,
	OPTN_GROUP_PPP,
	OPTN_GROUP_BANK,
	OPTN_GROUP_KKT,
	OPTN_GROUP_SCR,
	OPTN_GROUP_KBD,
};

static struct optn_group optn_groups[] = {
	{"���⥬�� ����ன��", sys_optn_items, ASIZE(sys_optn_items), NULL},
	{"���譨� ���ன�⢠", dev_optn_items, ASIZE(dev_optn_items),
		on_exit_devices},
	{"����ன�� TCP/IP", ip_optn_items, ASIZE(ip_optn_items),
		on_exit_check_ip},
	{"����ன�� PPP", ppp_optn_items, ASIZE(ppp_optn_items),
		on_exit_ppp},
	{"��� \"������\"", bank_optn_items, ASIZE(bank_optn_items),
		on_exit_bank_system},
	{"���", kkt_optn_items, ASIZE(kkt_optn_items), NULL},
	{"��࠭", scr_optn_items, ASIZE(scr_optn_items), NULL},
	{"���������", kbd_optn_items, ASIZE(kbd_optn_items), NULL},
};

/* ���᪠��� � ���� ����஥� */
static struct hint_entry optn_hints[] = {
	{"Esc",		"��室",			NULL},
	{"\x18\x19",	"�������� �� ᯨ��",		NULL},
	{"PgUp/PgDn",	"��ॡ�� ���祭��",		NULL},
	{"Tab",		"��६�饭�� ����� ��㯯���",	NULL},
	{"OK",		"��⠭���� ��ࠬ��஢",		NULL},
	{"�⬥��",	"�⪠� �� ��⠭����",		NULL},
};

static struct menu *optn_menu = NULL;
static int optn_active_group = -1;
static int optn_active_item = -1;
static int optn_active_control = -1;
int optn_cm = cmd_none;

static GCPtr optn_gc;
static GCPtr optn_mem_gc;
static FontPtr optn_fnt;
/* ���� ।���஢���� ��ࠬ��஢ */
static char edit_buf[OPTN_EDIT_BUF_LEN];
static int edit_max_len = 0;	/* ���ᨬ��쭠� ����� ������ � ���� ।���஢���� */
static int edit_len = 0;	/* ⥪��� ����� ������ � ���� ।���஢���� */
/* ������ ������ � ���� ।���஢���� (�⭮�⥫쭮 edit_first_pos) */
static int edit_pos = 0;
static int edit_first_pos = 0;	/* ������ ��ࢮ�� �⮡ࠦ������ ᨬ���� */
/* ����� � ᨬ����� ������� ��� ���� ।���஢���� */
#define OPTN_EDIT_VISIBLE_LEN	16
/* �������� ���� ����� (���न���� � ���ᥫ�� �⭮�⥫쭮 optn_gc) */
static struct {
	int first_y;		/* ���न��� y ��ࢮ� ��ப� */
	int name_x;		/* ��砫� ���� �������� ��ࠬ��� */
	int name_w;		/* �ਭ� ���� �������� */
	int h;			/* ���� ���� �������� */
	int edit_x;		/* ��砫� ���� ����� */
	int edit_w;		/* �ਭ� ���� ����� */
	int border_width;	/* ⮫騭� ࠬ�� ���� ����� */
	int gap;		/* ����� ����� �ᥤ���� ��ப��� */
} optn_line_geom;

/* �ணࠬ��� ����� */
#define CURSOR_BLINK_DELAY	50	/* �ᥪ */
#define CURSOR_COLOR		clGray
static int optn_cursor_xor = 0;
static bool optn_cursor_visible = false;

/* ��ᮢ���� ����� */
static void draw_optn_cursor(void)
{
	int x, y, h;
	x = optn_line_geom.edit_x + optn_line_geom.border_width +
		edit_pos * optn_fnt->max_width;
	y = optn_line_geom.first_y + optn_active_item * (optn_line_geom.h + optn_line_geom.gap) +
		optn_line_geom.gap / 2 + optn_line_geom.border_width;
	h = optn_line_geom.h - 2 * optn_line_geom.border_width;
	SetPenColor(optn_gc, CURSOR_COLOR);
	SetRop2Mode(optn_gc, R2_XOR);
	Line(optn_gc, x,     y, x,     y + h - 1);
	Line(optn_gc, x + 1, y, x + 1, y + h - 1);
	SetRop2Mode(optn_gc, R2_COPY);
	optn_cursor_xor ^= 1;
}

/* ��⠭���� �������� ����� */
static void set_optn_cursor_visibility(bool visibility)
{
	optn_cursor_visible = visibility;
	if (optn_cursor_visible){
		if (!optn_cursor_xor)
			draw_optn_cursor();
	}else{
		if (optn_cursor_xor)
			draw_optn_cursor();
	}
}

/* �������� ����� */
static void show_optn_cursor(void)
{
	set_optn_cursor_visibility(true);
}

static struct optn_item *get_optn_item(int n);
static bool optn_is_edit(struct optn_item *item);
static void smart_show_optn_cursor(void)
{
	if (optn_is_edit(get_optn_item(optn_active_item)))
		show_optn_cursor();
}

/* ����� ����� */
static void hide_optn_cursor(void)
{
	set_optn_cursor_visibility(false);
}

/* ������� ����� */
static void do_cursor_blinking(void)
{
	static uint32_t t0 = 0;
	uint32_t t = u_times();
	if ((t - t0) >= CURSOR_BLINK_DELAY){
		if (optn_cursor_visible)
			draw_optn_cursor();
		t0 = t;
	}
}

/* ����祭�� ������ � edit_buf �� �᭮����� ����樨 � ������� ��� ���� */
static int get_edit_index(int pos)
{
	return edit_first_pos + pos;
}

/* ����祭�� ������ �� ���� ।���஢���� */
static char *get_edit_data(void)
{
	static char str[OPTN_EDIT_BUF_LEN + 1];
	if (edit_len > 0){
		memcpy(str, edit_buf, edit_len);
		str[edit_len] = 0;
	}else
		str[0] = 0;
	return str;
}

/* ����祭�� ������� ��� ��ப� �� ���� ।���஢���� */
static char *get_edit_visible_data(void)
{
	static char str[OPTN_EDIT_VISIBLE_LEN + 1];
	int i, j;
	for (i = 0; i < OPTN_EDIT_VISIBLE_LEN; i++){
		j = get_edit_index(i);
		if (j < edit_len)
			str[i] = edit_buf[j];
		else
			break;
	}
	str[i] = 0;
	return str;
}

/* ��⠭���� ������ � ���� ।���஢���� */
static bool set_edit_data(const char *data, int max_len)
{
	int l;
	if ((data == NULL) || (max_len <= 0) || (max_len > OPTN_EDIT_BUF_LEN))
		return false;
	l = strlen(data);
	if (l > max_len)
		return false;
	memcpy(edit_buf, data, l);
	edit_len = l;
	edit_max_len = max_len;
	edit_pos = edit_first_pos = 0;
	return true;
}

/*
 * ��⠭���� ����樨 ����� � ���� ।���஢����. �����頥� true,
 * �᫨ �ॡ���� ����ᮢ�� ⥪�� � ��ப� ।���஢����.
 */
static bool optn_edit_set_pos(int pos)
{
	int index = get_edit_index(pos);
	if ((pos == edit_pos) || (index < 0) || (index > edit_len))
		return false;
	if (pos < 0){
		edit_first_pos = index;
		edit_pos = 0;
		return true;
	}else if (index > (edit_first_pos + OPTN_EDIT_VISIBLE_LEN)){
		edit_first_pos = index - 1;
		edit_pos = 1;
		return true;
	}else
		edit_pos = pos;
	return false;
}

/* ��६�饭�� ����� ����� */
static bool optn_edit_left(void)
{
	return optn_edit_set_pos(edit_pos - 1);
}

/* ��६�饭�� ����� ��ࠢ� */
static bool optn_edit_right(void)
{
	return optn_edit_set_pos(edit_pos + 1);
}

/* ��६�饭�� ����� � ��砫� ��ப� */
static bool optn_edit_home(void)
{
	bool flag = edit_first_pos > 0;
	edit_first_pos = edit_pos = 0;
	return flag;
}

/* ��६�饭�� ����� � ����� ��ப� */
static bool optn_edit_end(void)
{
	if ((edit_first_pos + OPTN_EDIT_VISIBLE_LEN) < edit_len){
		edit_first_pos = edit_len - OPTN_EDIT_VISIBLE_LEN;
		edit_pos = OPTN_EDIT_VISIBLE_LEN;
		return true;
	}else
		return optn_edit_set_pos(edit_len - edit_first_pos);
}

/* �஢�ઠ ���� int_edit */
static bool is_valid_dec(uint8_t c)
{
	return	(0x30 <= c) && (c <= 0x39);
}

/* �஢�ઠ ���� hex_edit */
static bool is_valid_hex(uint8_t c)
{
	return is_valid_dec(c) || ((0x41 <= c) && (c <= 0x46));
}

/* �஢�ઠ ���� ip_edit */
static bool is_valid_ip(uint8_t c)
{
	return is_valid_dec(c) || (c == '.');
}

/* �஢�ઠ ���� str_edit */
static bool is_valid_str(uint8_t c)
{
	return c >= 0x20;
}

/* �஢�ઠ ᨬ���� �� �����⨬�� ���祭�� ��� ������� ���� */
static bool is_valid(int n, uint8_t b)
{
	bool ret = false;
	switch (optn_groups[optn_active_group].items[n].ot){
		case optn_hex_edit:
			ret = is_valid_hex(b);
			break;
		case optn_int_edit:
			if ((b == '-') && (edit_first_pos == 0) &&
					(edit_pos == 0) &&
					((edit_len == 0) || (edit_buf[0] != '-')))
				ret = true;
			else
				ret = is_valid_dec(b);
			break;
		case optn_uint_edit:
			ret = is_valid_dec(b);
			break;
		case optn_ip_edit:
			ret = is_valid_ip(b);
			break;
		case optn_lstr_edit:
		case optn_str_edit:
		case optn_psw_edit:
			ret = is_valid_str(b);
			break;
	}
	return ret;
}

static struct optn_item *get_optn_item(int n)
{
	return optn_groups[optn_active_group].items + n;
}

/* ���� ����� ���� ।����㥬� */
static bool optn_is_edit(struct optn_item *item)
{
	bool ret = false;
	if (item != NULL){
		switch (item->ot){
			case optn_hex_edit:
			case optn_int_edit:
			case optn_uint_edit:
			case optn_ip_edit:
			case optn_str_edit:
			case optn_lstr_edit:
			case optn_psw_edit:
				ret = true;
		}
	}
	return ret;
}

/* ����७�� ��ப� �� ���� ᨬ���, ��稭�� � ⥪�饩 ����樨 */
static bool optn_edit_expand(int index)
{
	int i;
	if (edit_len == edit_max_len)
		return false;
	for (i = edit_len; i >= index; i--)
		edit_buf[i + 1] = edit_buf[i];
	edit_len++;
	return true;
}

/* ��⠢�� ᨬ����. �஢�ઠ ���४⭮�� ��������� ᨬ���� 㦥 �஢����� */
static bool optn_edit_ins_char(uint8_t c)
{
	int index = get_edit_index(edit_pos);
	if (edit_len == edit_max_len)
		return false;
	if (index < edit_len){
		optn_edit_expand(index);
		edit_buf[index] = c;
	}else{
		edit_buf[index] = c;
		edit_len++;
	}
	return true;
}

/* �������� ᨬ���� � ����樨 ����� */
static bool optn_edit_del_char(void)
{
	int i;
	if (edit_pos == edit_len)
		return false;
	for (i = get_edit_index(edit_pos); i < (edit_len - 1); i++)
		edit_buf[i] = edit_buf[i + 1];
	edit_len--;
	return true;
}

/* �������� ᨬ���� � ����樨 ��। ����஬ */
static bool optn_edit_bk_char(void)
{
	if (edit_pos == 0)
		return false;
	else{
		optn_edit_left();
		return optn_edit_del_char();
	}
}

/* ���⪠ ���� ����� */
static bool optn_edit_clear(void)
{
	edit_len = 0;
	edit_pos = 0;
	edit_first_pos = 0;
	return true;
}

/* ��।������ ������ ����᫨���� ⨯� �� ��� ���祭�� */
static int optn_index_by_val(struct optn_item *item, int w)
{
	int ret = 0;
	if (item != NULL){
		for (int i = 0; i < item->n; i++){
			if (((int *)item->d)[i] == w){
				ret = i;
				break;
			}
		}
	}
	return ret;
}

/* ����祭�� ����� ।���஢���� �� ᬥ饭�� ��ࠬ��� � term_cfg */
static struct optn_item *__optn_item_by_offset(int offset)
{
	struct optn_item *ret = NULL;
	for (int i = 0; i < ASIZE(optn_groups); i++){
		for (int j = 0; j < optn_groups[i].n_items; j++){
			if (optn_groups[i].items[j].offset == offset){
				ret = optn_groups[i].items + j;
				break;
			}
		}
	}
	return ret;
}

#define optn_item_by_offset(fld) __optn_item_by_offset(offsetof(struct term_cfg, fld))

static bool __optn_set_item_enable(int offset, bool enable)
{
	bool ret = false;
	struct optn_item *itm = __optn_item_by_offset(offset);
	if (itm != NULL){
		itm->enabled = enable;
		ret = true;
	}
	return ret;
}

#define optn_set_item_enable(fld, en) __optn_set_item_enable(offsetof(struct term_cfg, fld), en)
#define optn_enable_item(fld) optn_set_item_enable(fld, true)
#define optn_disable_item(fld) optn_set_item_enable(fld, false)

/*
 * ����襭��/����饭�� ।���஢���� ��ࠬ��஢ ��� ��� � ����ᨬ���
 * �� ०��� ࠡ���.
 */
static void adjust_lprn_params(bool enable)
{
	struct optn_group *grp = optn_groups + OPTN_GROUP_DEVICES;
	struct optn_item *itm;
	if (enable)
		grp->n_items = ASIZE(dev_optn_items);
	else{
		bool found = false;
		for (grp->n_items = 0, itm = grp->items;
				!found && (grp->n_items < ASIZE(dev_optn_items));
				grp->n_items++, itm++){
			if (itm->offset == -1)
				continue;
			else /*if (cfg.has_lprn)*/
				found = itm->offset >= offsetof(struct term_cfg, has_sd_card);
/*			else
				found = itm->offset >= offsetof(struct term_cfg, has_lprn);*/
		}
	}
}

/* ����� � ���� ����஥� */
static bool optn_create_menu(void)
{
	optn_menu = new_menu(false, false);
	add_menu_item(optn_menu, new_menu_item("���⥬�� ��⠭����", cmd_sys_optn, true));
	add_menu_item(optn_menu, new_menu_item("���譨� ���ன�⢠", cmd_dev_optn, true));
	add_menu_item(optn_menu, new_menu_item("����ன�� TCP/IP", cmd_tcpip_optn, true));
	add_menu_item(optn_menu, new_menu_item("����ன�� PPP", cmd_ppp_optn, true));
	add_menu_item(optn_menu, new_menu_item("��� \"������\"", cmd_bank_optn, bank_ok));
	add_menu_item(optn_menu, new_menu_item("���", cmd_kkt_optn, true));
	add_menu_item(optn_menu, new_menu_item("����ன�� �࠭�", cmd_scr_optn, true));
	add_menu_item(optn_menu, new_menu_item("����ன�� ����������", cmd_kbd_optn, true));
	add_menu_item(optn_menu, new_menu_item("���࠭��� ����ன��", cmd_store_optn, true));
	add_menu_item(optn_menu, new_menu_item("��室 ��� ��࠭����", cmd_exit, true));
	return true;
}

/*
 * �맮� �⮩ �㭪樨 �� �뢮�� ���� 㤮���� �맮�� callback'��
 * ��� �⤥���� ����ᥩ ��㯯� ����஥�.
 */
static void optn_adjust_menu(void)
{
	struct optn_item *item = optn_item_by_offset(use_ppp);
	bool flag = (item != NULL) ? item->vv.i : false;
/* �᫨ flag == true, ࠧ�蠥� ����ன�� PPP */
	enable_menu_item(optn_menu, cmd_ppp_optn, flag);
}

/* ���࠭����/�⬥�� ��������� ��㯯� ����஥� */
static bool optn_commit_group(int group, bool commit)
{
	if ((group < 0) || (group > ASIZE(optn_groups)))
		return false;
	struct optn_item *items = optn_groups[group].items;
	for (int i = 0; i < optn_groups[group].n_items; i++){
		if (items[i].ot != optn_static_txt){
			if (commit)
				items[i].v=items[i].vv;
			else
				items[i].vv=items[i].v;
		}
	}
	return true;
}

#if 0
/* �८�ࠧ������ ��ப� � ��⮬ Esc-��᫥����⥫쭮�⥩ */
static bool make_escape_chars(char *dst, char *src, int n)
{
	int i, j, l;
	if ((src == NULL) || (dst == NULL) || (n <= 0))
		return false;
	l = strlen(src);
	if (l > (n - 1))
		l = n - 1;
	for (i = j = 0; (i < l) && (j < (n - 1)); i++, j++){
/*		if ((src[i] == '\\') || (src[i] == '\''))
			dst[j++] = '\\';
		if (j == (n - 1))
			break;*/
		dst[j] = src[i];
	}
	dst[j] = 0;
	return true;
}

/* ��⠢�� \ ��। ', �᫨ ��� ⠬ �� �뫮 */
static bool do_escape_quote(char *dst, char *src, int n)
{
	int i, j, l;
	bool dle = false;
	if ((src == NULL) || (dst == NULL) || (n <= 0))
		return false;
	l = strlen(src);
	if (l > (n - 1))
		l = n - 1;
	for (i = j = 0; (i < l) && (j < (n - 1)); i++, j++){
		if ((src[i] == '\'') && !dle){
			dst[j++] = '\\';
			if (j == (n - 1))
				break;
		}
		dst[j] = src[i];
		dle = dle ? false : src[i] == '\\';
	}
	dst[j] = 0;
	return true;
}
#endif

/* �⥭�� ��㯯� ����஥� */
static bool optn_read_group(const struct term_cfg *cfg, int group)
{
	if ((cfg == NULL) || (group == OPTN_GROUP_MENU) || (group >= ASIZE(optn_groups)))
		return false;
	struct optn_item *items = optn_groups[group].items;
	for (int i = 0; i < optn_groups[group].n_items; i++){
		if (items[i].offset < 0)
			continue;
		const uint8_t *p = (uint8_t *)cfg + items[i].offset;
		switch (items[i].at){
			case ini_bool:
				items[i].vv.flag = *((const bool *)p);
				break;
			case ini_hex:
				items[i].vv.u8 = *p;
				break;
			case ini_int:{
				int v = *((const int *)p);
				if ((items[i].ot == optn_int_enum) && (items[i].d != NULL))
					items[i].vv.i = optn_index_by_val(items + i, v);
				else
					items[i].vv.i = v;
				break;
			}
			case ini_uint16:{
				uint16_t v = *((const uint16_t *)p);
				if ((items[i].ot == optn_int_enum) && (items[i].d != NULL))
					items[i].vv.u16 = optn_index_by_val(items + i, v);
				else
					items[i].vv.u16 = v;
				break;
			}
			case ini_uint32:{
				uint32_t v = *((const uint32_t *)p);
				if ((items[i].ot == optn_int_enum) && (items[i].d != NULL))
					items[i].vv.u32 = optn_index_by_val(items + i, v);
				else
					items[i].vv.u32 = v;
				break;
			}
			case ini_ipaddr:
				strcpy(items[i].vv.ip, inet_ntoa(*((struct in_addr *)p)));
				break;
			case ini_str:
			case ini_psw:
				snprintf(items[i].vv.s, sizeof(items[i].vv.s),
					"%s", (const char *)p);
				break;
		}
	}
	return optn_commit_group(group, true);
}

/* ������ ��㯯� ����஥� */
static bool optn_write_group(struct term_cfg *cfg, int group)
{
	if ((cfg == NULL) || (group == OPTN_GROUP_MENU) || (group >= ASIZE(optn_groups)))
		return false;
	struct optn_item *items = optn_groups[group].items;
	for (int i = 0; i < optn_groups[group].n_items; i++){
		if (items[i].ot == optn_static_txt)
			continue;
		uint8_t *p = (uint8_t *)cfg + items[i].offset;
		switch (items[i].at){
			case ini_bool:
				*((bool *)p) = items[i].v.flag;
				break;
			case ini_hex:
				*p = items[i].v.u8;
				break;
			case ini_int:{
				int v = items[i].v.i;
				if ((items[i].ot == optn_int_enum) && (items[i].d != NULL))
					*((int *)p) = ((int *)items[i].d)[v];
				else
					*((int *)p) = v;
				break;
			}
			case ini_uint16:{
				uint16_t v = items[i].v.u16;
				if ((items[i].ot == optn_int_enum) && (items[i].d != NULL))
					*((uint16_t *)p) = ((uint16_t *)items[i].d)[v];
				else
					*((uint16_t *)p) = v;
				break;
			}
			case ini_uint32:{
				uint32_t v = items[i].v.u32;
				if ((items[i].ot == optn_int_enum) && (items[i].d != NULL))
					*((uint32_t *)p) = ((uint32_t *)items[i].d)[v];
				else
					*((uint32_t *)p) = v;
				break;
			}
			case ini_ipaddr:
				*((uint32_t *)p) = inet_addr(items[i].v.ip);
				break;
			case ini_str:
			case ini_psw:
				snprintf((char *)p, sizeof(items[i].v.s), "%s",
					items[i].v.s);
				break;
		}
	}
	return true;
}

/* ��⠭���� ⥪�饩 ��㯯� ����஥� */
static bool optn_first_item(void);
static bool optn_set_group(int n)
{
	bool ret = false;
	optn_cm = cmd_none;
	optn_active_group = n;
	optn_active_item = 0;
	if (n == OPTN_GROUP_MENU){
		hide_optn_cursor();
		optn_adjust_menu();
		ClearScreen(clBlack);
		draw_menu(optn_menu);
		ret = true;
	}else if (n < ASIZE(optn_groups)){
		optn_active_control=0;
		for (int i = 0; i < optn_groups[n].n_items; i++)
			if (optn_groups[n].items[i].on_change != NULL)
				optn_groups[n].items[i].on_change(&optn_groups[n].items[i]);
		optn_first_item();
		ret = draw_options();
	}
	return ret;
}

/* ���������� term_cfg �������묨 ��ࠬ��ࠬ� */
bool optn_get_items(struct term_cfg *p)
{
	if (p == NULL)
		return false;
	for (int i = 0; i < ASIZE(optn_groups); i++)
		optn_write_group(p, i);
	translate_color_scheme(p->color_scheme,
		&p->rus_color, &p->lat_color, &p->bg_color);
	return true;
}

/* ��।������ ������ਨ ��ப� ���� ����஥� */
static void set_optn_line_geom(void)
{
	int w = GetCX(optn_mem_gc);
	optn_line_geom.first_y = 4;
	optn_line_geom.name_x = 2;
	optn_line_geom.border_width = 1;
	optn_line_geom.gap = 4;
	optn_line_geom.h = optn_fnt->max_height + 4;
	optn_line_geom.edit_w = OPTN_EDIT_VISIBLE_LEN * optn_fnt->max_width +
		2 * optn_line_geom.border_width + 2;
	optn_line_geom.edit_x = w - optn_line_geom.edit_w - 2;
	optn_line_geom.name_w = w - optn_line_geom.edit_w - 10;
}

void init_options(void)
{
	int i;
	optn_active = true;
	set_term_busy(true);
#if defined __CHECK_HOST_IP__
	load_valid_host_ips();
#endif
	scr_visible = false;
	set_scr_mode(m80x20, true, false);
	optn_cm = cmd_none;
	optn_create_menu();
	hide_cursor();
	optn_gc = CreateGC(2, 2 * titleCY + 3, DISCX / 2 - 5, 500);
	optn_mem_gc = CreateMemGC(GetCX(optn_gc), GetCY(optn_gc));
	optn_fnt = CreateFont(_("fonts/terminal10x18.fnt"), false);
	set_optn_line_geom();
	for (i = 0; i < ASIZE(optn_groups); i++)
		optn_read_group(&cfg, i);
	optn_set_group(OPTN_GROUP_MENU);
	lprn_params_read = false;
}

void release_options(bool need_clear)
{
	release_menu(optn_menu,false);
	if (need_clear)
		ClearScreen(clBtnFace);
	DeleteGC(optn_mem_gc);
	DeleteGC(optn_gc);
	DeleteFont(optn_fnt);
	optn_active=false;
}

/* ��⠭���� ��砫쭮�� ���祭�� � ���� ।���஢���� */
static bool optn_set_edit(void)
{
	int max_len = 0;
	char data[OPTN_EDIT_BUF_LEN + 1];
	struct optn_item *itm = optn_groups[optn_active_group].items + optn_active_item;
	if ((optn_active_control != 0) || !optn_is_edit(itm))
		return false;
	bool ret = false;
	switch (itm->ot){
		case optn_hex_edit:
			max_len = OPTN_HEX_EDIT_LEN;
			snprintf(data, max_len + 1, "%.2X", itm->vv.u8);
			break;
		case optn_int_edit:
			max_len = OPTN_INT_EDIT_LEN;
			snprintf(data, max_len + 1, "%d", itm->vv.i);
			break;
		case optn_uint_edit:
			if (itm->at == ini_uint16){
				max_len = OPTN_UINT16_EDIT_LEN;
				snprintf(data, max_len + 1, "%hu", itm->vv.u16);
			}else{
				max_len = OPTN_UINT32_EDIT_LEN;
				snprintf(data, max_len + 1, "%u", itm->vv.u32);
			}
			break;
		case optn_ip_edit:
			max_len = OPTN_IP_EDIT_LEN;
			snprintf(data, max_len + 1, "%s", itm->vv.ip);
			break;
		case optn_lstr_edit:
		case optn_str_edit:
		case optn_psw_edit:
			if (itm->max_len > OPTN_STR_EDIT_LEN)
				max_len = OPTN_STR_EDIT_LEN;
			else
				max_len = itm->max_len;
			snprintf(data, max_len + 1, "%s", itm->vv.s);
			break;
		case optn_static_txt:
			if (itm->max_len > OPTN_STR_EDIT_LEN)
				max_len = OPTN_STR_EDIT_LEN;
			else
				max_len = itm->max_len;
			snprintf(data, max_len + 1, "%s", itm->vv.txt);
			break;
	}
	if (max_len > 0)
		ret = set_edit_data(data, max_len);
	return ret;
}

/* ��ᮢ���� �������� ��ࠬ��� */
static void draw_optn_item_name(int n)
{
	Color fg;
	struct optn_item *itm = get_optn_item(n);
	int x, y, w, h;
	if (n == optn_active_item)
		fg = clMaroon;
	else if (itm->enabled)
		fg = clBlack;
	else
		fg = clGray;
	x = optn_line_geom.name_x;
	y = optn_line_geom.first_y + n * (optn_line_geom.h + optn_line_geom.gap) +
		optn_line_geom.gap / 2;
	w = optn_line_geom.name_w;
	h = optn_line_geom.h;
	SetTextColor(optn_mem_gc, fg);
	DrawText(optn_mem_gc, x, y, w, h,
		optn_groups[optn_active_group].items[n].name,
		DT_RIGHT | DT_VCENTER);
}

/* ��ᮢ���� ���祭�� ��࠭���� ��ࠬ��� */
static void draw_optn_field(int n)
{
	int x, y, w, h;
	if ((optn_active_control != 0) || (n != optn_active_item))
		return;
	x = optn_line_geom.edit_x;
	y = optn_line_geom.first_y + n * (optn_line_geom.h + optn_line_geom.gap) +
		optn_line_geom.gap / 2;
	w = optn_line_geom.edit_w;
	h = optn_line_geom.h;
	SetBrushColor(optn_mem_gc, clRopnetBrown);
	FillBox(optn_mem_gc, x, y, w, h);
	DrawBorder(optn_mem_gc, x, y, w, h, optn_line_geom.border_width,
		clRopnetDarkBrown, clRopnetDarkBrown);
}

/* ��ᮢ���� ���祭�� ��ப����� ��ࠬ��� */
static void draw_optn_str(int n, const char *val)
{
	int x = optn_line_geom.edit_x + optn_line_geom.border_width + 1;
	int y = optn_line_geom.first_y + n * (optn_line_geom.h + optn_line_geom.gap) +
		optn_line_geom.gap / 2;
	int w = optn_line_geom.edit_w - 2 * (optn_line_geom.border_width + 1);
	int h = optn_line_geom.h;
	DrawText(optn_mem_gc, x, y, w, h, val, DT_LEFT | DT_VCENTER);
}

/* ��ᮢ���� ���祭�� ⨯� str_edit */
static void draw_optn_str_edit(int n)
{
	char v[OPTN_EDIT_BUF_LEN + 1];
	struct optn_item *itm;
	if ((optn_active_control == 0) && (n == optn_active_item))
		snprintf(v, sizeof(v), "%s", get_edit_visible_data());
	else{
		itm = get_optn_item(n);
		snprintf(v, sizeof(v), "%s", itm->vv.s);
	}
	draw_optn_str(n, v);
}

/* ��ᮢ���� ���祭�� ⨯� str_enum */
static void draw_optn_str_enum(int n)
{
	struct optn_item *itm = get_optn_item(n);
	draw_optn_str(n, ((char **)itm->d)[itm->vv.i]);
}

/* ��ᮢ���� ���祭�� ⨯� int_enum */
static void draw_optn_int_enum(int n)
{
	char v[OPTN_EDIT_BUF_LEN + 1];
	struct optn_item *itm = get_optn_item(n);
	if ((itm->offset == offsetof(struct term_cfg, kkt_brightness)) && (itm->vv.i == 0))
		snprintf(v, sizeof(v), "�� 㬮�砭��:%d", kkt_brightness.def);
	else
		snprintf(v, sizeof(v), "%d",
			(itm->d == NULL) ? itm->vv.i : ((int *)itm->d)[itm->vv.i]);
	draw_optn_str(n, v);
}

/* ��ᮢ���� ���祭�� ⨯� hex_edit */
static void draw_optn_hex_edit(int n)
{
	char v[OPTN_EDIT_BUF_LEN + 1];
	if ((optn_active_control == 0) && (n == optn_active_item))
		snprintf(v, sizeof(v), "%s", get_edit_visible_data());
	else{
		struct optn_item *itm = get_optn_item(n);
		snprintf(v, sizeof(v), "%.2X", itm->vv.u8);
	}
	draw_optn_str(n, v);
}

/* ��ᮢ���� ���祭�� ⨯� int_edit */
static void draw_optn_int_edit(int n)
{
	char v[OPTN_EDIT_BUF_LEN + 1];
	if ((optn_active_control == 0) && (n == optn_active_item))
		snprintf(v, sizeof(v), "%s", get_edit_visible_data());
	else{
		struct optn_item *itm = get_optn_item(n);
		snprintf(v, sizeof(v), "%d", itm->vv.i);
	}
	draw_optn_str(n, v);
}

/* ��ᮢ���� ���祭�� ⨯� uint_edit */
static void draw_optn_uint_edit(int n)
{
	char v[OPTN_EDIT_BUF_LEN + 1];
	if ((optn_active_control == 0) && (n == optn_active_item))
		snprintf(v, sizeof(v), "%s", get_edit_visible_data());
	else{
		struct optn_item *itm = get_optn_item(n);
		if (itm->at == ini_uint16)
			snprintf(v, sizeof(v), "%hu", itm->vv.u16);
		else
			snprintf(v, sizeof(v), "%u", itm->vv.u32);
	}
	draw_optn_str(n, v);
}


/* ��ᮢ���� ���祭�� ⨯� ip_edit */
static void draw_optn_ip_edit(int n)
{
	char v[OPTN_EDIT_BUF_LEN + 1];
	if ((optn_active_control == 0) && (n == optn_active_item))
		snprintf(v, sizeof(v), "%s", get_edit_visible_data());
	else{
		struct optn_item *itm = get_optn_item(n);
		snprintf(v, sizeof(v), "%s", itm->vv.ip);
	}
	draw_optn_str(n, v);
}

/* ��ᮢ���� ���祭�� ⨯� psw_edit */
static void draw_optn_psw_edit(int n)
{
	char v[OPTN_EDIT_BUF_LEN + 1];
	struct optn_item *itm = get_optn_item(n);
	int i = 0, l = 0;
	if ((optn_active_control == 0) && (n == optn_active_item))
		l = strlen(get_edit_visible_data());
	else
		l = strlen(itm->vv.s);
	if (l > OPTN_EDIT_BUF_LEN)
		return;
	for (i = 0; i < l; i++)
		v[i] = '*';
	v[i] = 0;
	draw_optn_str(n, v);
}

/* ��ᮢ���� ���祭�� ��ࠬ��� */
static void draw_optn_item_value(int n)
{
	struct optn_item *itm = get_optn_item(n);
	draw_optn_field(n);
	switch (itm->ot){
		case optn_str_enum:
			draw_optn_str_enum(n);
			break;
		case optn_int_enum:
			draw_optn_int_enum(n);
			break;
		case optn_hex_edit:
			draw_optn_hex_edit(n);
			break;
		case optn_int_edit:
			draw_optn_int_edit(n);
			break;
		case optn_uint_edit:
			draw_optn_uint_edit(n);
			break;
		case optn_ip_edit:
			draw_optn_ip_edit(n);
			break;
		case optn_lstr_edit:
		case optn_str_edit:
			draw_optn_str_edit(n);
			break;
		case optn_psw_edit:
			draw_optn_psw_edit(n);
			break;
		case optn_static_txt:
			draw_optn_str(n, itm->v.txt);
			break;
	}
}

/* ��ᮢ���� ��ப� ⨯� <��� ��ࠬ���> <���祭��> */
static void draw_optn_item(int n, bool copy)
{
	draw_optn_item_name(n);
	draw_optn_item_value(n);
	if (copy)
		CopyGC(optn_gc, 0, 0, optn_mem_gc, 0, 0, GetCX(optn_mem_gc), GetCY(optn_mem_gc));
}

/* ��ᮢ���� ��㯯� ��ࠬ��஢ */
static bool draw_optn_descr(void);
static bool draw_optn_items(void)
{
	SetFont(optn_mem_gc, optn_fnt);
	ClearGC(optn_mem_gc, clBtnFace);
	for (int i = 0; i < optn_groups[optn_active_group].n_items; i++)
		draw_optn_item(i, false);
	CopyGC(optn_gc, 0, 0, optn_mem_gc, 0, 0, GetCX(optn_mem_gc), GetCY(optn_mem_gc));
	return draw_optn_descr();
}

/* ��뢠���� �� ��������� ��⨢��� ��ப� */
static void on_active_item_change(void)
{
	char *s;
	struct optn_item *itm = get_optn_item(optn_active_item);
	if (optn_is_edit(itm)){
		s = get_edit_data();
		switch (itm->at){
			case ini_hex:
				itm->vv.u8 = strtoul(s, NULL, 16);
				break;
			case ini_int:
				itm->vv.i = strtol(s, NULL, 10);
				break;
			case ini_uint16:
				itm->vv.u16 = strtoul(s, NULL, 10);
				break;
			case ini_uint32:
				itm->vv.u32 = strtoul(s, NULL, 10);
				break;
			case ini_ipaddr:
				strcpy(itm->vv.ip, s);
				break;
			case ini_str:
			case ini_psw:
				strcpy(itm->vv.s, s);
				break;
		}
	}
}

/* ��ᮢ���� ������ � ���� ����஥� */
static bool draw_optn_buttons(void)
{
#define BTNOFFS		10
	char *str[] = {"��", "�⬥��"};
	GCPtr pGC = CreateGC(1, DISCY-40, DISCX/2-3, 32);
	GCPtr pMemGC = CreateMemGC(GetCX(pGC), GetCY(pGC));
	FontPtr pFont = CreateFont(_("fonts/andale8x15.fnt"), true);
	int bw = (GetCX(pGC)-20)/OPTN_N_BUTTONS - BTNOFFS;
	
	SetFont(pMemGC, pFont);
	ClearGC(pMemGC, clBtnFace);
	
	for (int i = 0; i < OPTN_N_BUTTONS; i++){
		DrawButton(pMemGC, 10+i*(bw+BTNOFFS), 0, bw, 30, str[i], false);
		if (i == (optn_active_control-1)){
			SetPenColor(pMemGC, clNavy);
			DrawRect(pMemGC, 10+i*(bw+BTNOFFS)+4, 4, bw-8, 30-8);
		}
	}
	
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC));
	
	DeleteGC(pMemGC);
	DeleteGC(pGC);
	DeleteFont(pFont);
	return true;
}

#include "build.h"
/* ��ᮢ���� ������ ���᪠��� */
static bool draw_optn_help(void)
{
	GCPtr pMemGC = CreateMemGC(DISCX / 2 - 4, DISCY / 2 - 32);
	GCPtr pGC = CreateGC(DISCX / 2 + 3, 2 * titleCY + 3, DISCX / 2 - 4, DISCY / 2 - 34);
	FontPtr pFont = CreateFont(_("fonts/terminal10x18.fnt"), true);
	term_number tn;
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn);
	
	ClearGC(pMemGC, clBtnFace);
	SetFont(pMemGC, pFont);
	
	static char buf[256];
	int i = 0, tmp = 0;
	for (i = 0; i < ASIZE(optn_hints); i++){
		sprintf(buf, "%s - ",optn_hints[i].key);
		SetTextColor(pMemGC, clGreen);
		TextOut(pMemGC, 20, 20 + i * (pFont->max_height + 4), buf);
		tmp = TextWidth(pFont, buf);
		sprintf(buf, "%s",optn_hints[i].descr);
		SetTextColor(pMemGC, clBlue);
		TextOut(pMemGC, 20 + tmp + 2, 20 + i * (pFont->max_height + 4), buf);
	}
	i++;
	SetTextColor(pMemGC, clGreen);
	
	sprintf(buf, "��ନ��� %s "
			_s(STERM_VERSION_BRANCH) "."
			_s(STERM_VERSION_RELEASE) "."
			_s(STERM_VERSION_PATCH) "."
			_s(STERM_VERSION_BUILD),
			term_string);
	TextOut(pMemGC, 20, 20 + i * (pFont->max_height + 4), buf);
	i++;
	
	const char *ptr = "����⮢�⥫�: �� ��� \"������\"";
	TextOut(pMemGC, 20, 20 + i * (pFont->max_height + 4), ptr);
	tmp = TextWidth(pFont, ptr);
	SetTextColor(pMemGC, clRed);
	i++;
	
	SetTextColor(pMemGC, clGreen);
	sprintf(buf, "�����᪮� �����: %.*s", isizeof(tn), tn);
	TextOut(pMemGC, 20, 20 + i * (pFont->max_height + 4), buf);
	i++;
	
	sprintf(buf, "����஫쭠� �㬬�: %.4X", term_check_sum);
	TextOut(pMemGC, 20, 20 + i * (pFont->max_height + 4), buf);
	
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC));
	
	DeleteFont(pFont);
	DeleteGC(pMemGC);
	DeleteGC(pGC);
	return true;
}

/* ��ᮢ���� ��ࠧ� ��࠭��� 梥⮢�� �奬� */
static void clear_scheme_example(void)
{
#define SCHEME_EXAMPLE_WIDTH	350
	GCPtr pGC = CreateGC(DISCX / 2 + 3, DISCY / 2 + 31, DISCX / 2 - 4, DISCY / 2 - 32);
	SetBrushColor(pGC, clBtnFace);
	FillBox(pGC, 20, 60, SCHEME_EXAMPLE_WIDTH, font32x8->max_height + 6);
	DeleteGC(pGC);
}

static bool draw_scheme_example(const struct optn_item *item)
{
/*	static uint16_t buf[] = {32, 80, 54, 50, 32, 71, 49, 56, 32, 67, 140, 145, 138, 32, 27};*/
	static uint16_t buf[] = {32, 80, 54, 50, 32, 71, 49, 56, 32, 67, 140, 142, 32, 27};
	if (item == NULL)
		return false;
	translate_color_scheme(item->vv.i, &cfg.rus_color, &cfg.lat_color, &cfg.bg_color);
	GCPtr pGC = CreateGC(DISCX / 2 + 3, DISCY / 2 + 31, DISCX / 2 - 6, DISCY / 2 - 62);
	GCPtr pMemGC = CreateMemGC(SCHEME_EXAMPLE_WIDTH, font32x8->max_height + 6);
	
	pMemGC->pFont = font32x8;

	ClearGC(pMemGC,cfg.bg_color);	
	RichTextOut(pMemGC, 0, 3, buf, ASIZE(buf));
	DrawBorder(pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC), 2, clBtnShadow, clBtnHighlight);
/* ��ᮢ���� ����� */
	int x0 = 4 * font32x8->max_width;
	SetPenColor(pMemGC, cfg.lat_color);
	for (int i = 0; i < OVR_MODE_CURSOR_WIDTH-1; i++)
		Line(pMemGC, x0 + i, 5, x0 + i, font32x8->max_height);
	
	CopyGC(pGC, 20, 60, pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC));
	DeleteGC(pMemGC);
	DeleteGC(pGC);

	translate_color_scheme(cfg.color_scheme, &cfg.rus_color, &cfg.lat_color, &cfg.bg_color);
	return true;
}

/* ��ᮢ���� ���ᠭ�� ��ࠬ��� */
static bool draw_optn_descr(void)
{
	const struct optn_item *items = optn_groups[optn_active_group].items;
	bool scheme_active = items[optn_active_item].d == optn_scheme_names;
	const char *descr = items[optn_active_item].descr;
	if (descr == NULL)
		return false;
	static char s[128];
	int l = strlen(descr);
	FontPtr pFont = CreateFont(_("fonts/terminal10x18.fnt"), true);
	GCPtr pMemGC = CreateMemGC(DISCX / 2 - 24, (pFont->max_height + 4) * 12);
	ClearGC(pMemGC, clBtnFace);
	SetFont(pMemGC, pFont);
	SetTextColor(pMemGC, clBlue);
	GCPtr pGC = CreateGC(DISCX / 2 + 3, DISCY / 2 + 31, DISCX / 2 - 6, DISCY / 2 - 62);

	for (int i = 0, k = 0, n = 0; i <= l; i++){
		char c = descr[i];
		if ((c == 0) || (c == '\n')){
			int m = i - k;
			if (m > (sizeof(s) - 1))
				m = sizeof(s) - 1;
			memcpy(s, descr + k, m);
			s[m] = 0;
			
			TextOut(pMemGC, 0, n * (pFont->max_height + 4), s);
			k = i + 1;
			n++;
		}
	}
	CopyGC(pGC, 10, 10, pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC));
	DeleteFont(pFont);
	DeleteGC(pMemGC);

	if (scheme_active && !scheme_changed)
		scheme_changed = true;
	if (scheme_changed){
		draw_scheme_example(optn_item_by_offset(color_scheme));
		scheme_changed = false;
		scheme_shown = true;
	}else if (scheme_shown && !scheme_active){
		clear_scheme_example();
		scheme_shown = false;
	}
	DeleteGC(pGC);
	return true;
}

/* ��ᮢ���� ���� ����஥� */
bool draw_options(void)
{
	if (optn_active_group == OPTN_GROUP_MENU)
		return draw_menu(optn_menu);
	GCPtr pGC = CreateGC(0, 0, DISCX, DISCY);
	FontPtr pFont = CreateFont(_("fonts/terminal10x18.fnt"), true);
	SetFont(pGC, pFont);
	DrawWindow(pGC, 0, 0, GetCX(pGC), GetCY(pGC),
			optn_groups[optn_active_group].name, pGC);
	ClearGC(pGC, clBtnFace);
	int w = GetCX(pGC);
	int h = GetCY(pGC);
	int middle_x = w / 2;
	int middle_y = h / 2;
	int lw = w / 2 - 1;
	int rw = lw;
	DrawBorder(pGC, middle_x - 1, 2, 2, h - 4, borderCX, clBtnShadow, clBtnHighlight);

	SetBrushColor(pGC, clRopnetBlue);
	FillBox(pGC, 2, 2, lw - 4, titleCY);
	DrawBorder(pGC, 2, 2, lw - 4, titleCY, borderCX, clRopnetDarkBlue, clRopnetDarkBlue);
	SetTextColor(pGC, clBlack);
	DrawText(pGC, 2, 2, lw - 4, titleCY, "��ࠬ����", DT_CENTER | DT_VCENTER);

	FillBox(pGC, middle_x + 3, 2, rw - 4, titleCY);
	DrawBorder(pGC, middle_x + 3, 2, rw - 4, titleCY, borderCX, 
			clRopnetDarkBlue, clRopnetDarkBlue);
	DrawText(pGC, middle_x + 3, 2, rw - 4, titleCY, "��ࠢ��", DT_CENTER | DT_VCENTER);

	FillBox(pGC, middle_x + 3, middle_y, rw - 4, titleCY);
	DrawBorder(pGC, middle_x + 3, middle_y, rw - 4, titleCY, borderCX, 
			clRopnetDarkBlue, clRopnetDarkBlue);
	DrawText(pGC, middle_x + 3, middle_y, rw - 4, titleCY,
			"���ᠭ��", DT_CENTER | DT_VCENTER);

	DeleteFont(pFont);
	DeleteGC(pGC);
	
	return draw_optn_buttons() && draw_optn_help() && draw_optn_items();
}

/* ���室 � ᫥���饩 ��ப� */
static bool optn_next_item(void)
{
	bool ret= false;
	int n = optn_active_item;
	struct optn_item *items = optn_groups[optn_active_group].items;
	do {
		n++;
		n %= optn_groups[optn_active_group].n_items;
	} while (!items[n].enabled && (n != optn_active_item));
	if (n != optn_active_item){
		optn_active_item = n;
		optn_set_edit();
		ret = true;
	}
	return ret;
}

/* ���室 � �।��饩 ��ப� */
static bool optn_prev_item(void)
{
	bool ret = false;
	int n = optn_active_item;
	struct optn_item *items = optn_groups[optn_active_group].items;
	do {
		n += (optn_groups[optn_active_group].n_items - 1);
		n %= optn_groups[optn_active_group].n_items;
	} while (!items[n].enabled && (n != optn_active_item));
	if (n != optn_active_item){
		optn_active_item = n;
		optn_set_edit();
		ret = true;
	}
	return ret;
}

/* ���室 � ��ࢮ� ࠧ�襭��� ����� � ��㯯� */
static bool optn_first_item(void)
{
	bool ret = false;
	int n = optn_active_item;
	struct optn_item *items = optn_groups[optn_active_group].items;
	optn_active_item = 0;
	if (!items[0].enabled && !optn_next_item())
		optn_active_item = n;
	else{
		optn_set_edit();
		ret = true;
	}
	return ret;
}

/* ��ࠡ�⪠ ���祭�� ����᫨���� ⨯� */
static bool optn_handle_enum(struct optn_item *item, const struct kbd_event *e)
{
	bool flag = false;
	switch (e->key){
		case KEY_PGDN:		/* �।��饥 ���祭�� */
			item->vv.i += (item->n - 1);
			item->vv.i %= item->n;
			flag = true;
			break;
		case KEY_PGUP:		/* ᫥���饥 ���祭�� */
			item->vv.i++;
			item->vv.i %= item->n;
			flag = true;
			break;
	}
	if (flag)
		draw_optn_item(optn_active_item, true);
	return flag;
}

/* ��� �⮩ ��४���஢�� �� �� ᬮ��� ������� �����쪨� ��⨭᪨� �㪢� */
static char optn_str_recode(char c)
{
	static const char *from = "���������������������������\x9b����";
	static const char *to   = "f,dult;pbqrkvyjghcnea[wxio]sm'.z";
	for (int i = 0; i < 32; i++){
		if (c == from[i]){
			c = to[i];
			break;
		}
	}
	return c;
}

/* ��ࠡ�⪠ ���� ।���஢���� */
static bool optn_handle_edit(const struct kbd_event *e)
{
	bool flag = false;
	char ch = 0;
	if (!e->pressed)
		return false;
	if (is_valid(optn_active_item, e->ch)){
		if (edit_len < edit_max_len){
			hide_optn_cursor();
			switch (optn_groups[optn_active_group].items[optn_active_item].ot){
				case optn_lstr_edit:
				case optn_psw_edit:
					ch = optn_str_recode(e->ch);
					break;
				default:
					ch = e->ch;
			}
			optn_edit_ins_char(ch);
			optn_edit_right();
		}
		flag = true;
	}else{
		switch (e->key){
			case KEY_LEFT:
				hide_optn_cursor();
				optn_edit_left();
				flag = true;
				break;
			case KEY_RIGHT:
				hide_optn_cursor();
				optn_edit_right();
				flag = true;
				break;
			case KEY_DEL:
				hide_optn_cursor();
				if (e->shift_state & SHIFT_CTRL)
					optn_edit_clear();
				else
					optn_edit_del_char();
				flag = true;
				break;
			case KEY_BACKSPACE:
				hide_optn_cursor();
				optn_edit_bk_char();
				flag = true;
				break;
			case KEY_HOME:
				hide_optn_cursor();
				optn_edit_home();
				flag = true;
				break;
			case KEY_END:
				hide_optn_cursor();
				optn_edit_end();
				flag = true;
				break;
		}
	}
	if (flag){
		draw_optn_item(optn_active_item, true);
		show_optn_cursor();
	}
	return flag;
}

/* ��ࠡ�⪠ ��ப� ����஥� */
static bool process_optn_item(const struct kbd_event *e)
{
	bool flag = false;
	if (optn_active_control != 0)
		return true;
	scheme_changed = false;
	struct optn_item *itm = get_optn_item(optn_active_item);
	switch (itm->ot){
		case optn_str_enum:
		case optn_int_enum:
			flag = optn_handle_enum(itm, e);
			break;
		case optn_hex_edit:
		case optn_int_edit:
		case optn_uint_edit:
		case optn_ip_edit:
		case optn_lstr_edit:
		case optn_str_edit:
		case optn_psw_edit:
			flag = optn_handle_edit(e);
			break;
	}
	if (flag && (itm->on_change != NULL)){
		itm->on_change(itm);
		draw_optn_items();
	}
	return true;
}

/* ��ࠡ�⪠ ��㯯� ����஥� */ 
static bool process_optn_group(const struct kbd_event *e)
{
	if (!e->pressed)
		return true;
	bool ret = false;
	switch (e->key){
		case KEY_TAB:
			if (optn_active_control == 0){
				hide_optn_cursor();
				on_active_item_change();
			}
			if (e->shift_state & SHIFT_SHIFT)
				optn_active_control += 2;
			else
				optn_active_control++;
			optn_active_control %= 3;
			ret = draw_optn_items() && draw_optn_buttons();
			break;
		case KEY_UP:
			if (optn_active_control == 0){
				hide_optn_cursor();
				on_active_item_change();
				if (optn_prev_item())
					draw_optn_items();
				smart_show_optn_cursor();
			}
			ret = true;
			break;
		case KEY_DOWN:
			if (optn_active_control == 0){
				hide_optn_cursor();
				on_active_item_change();
				if (optn_next_item())
					draw_optn_items();
				smart_show_optn_cursor();
			}
			ret = true;
			break;
		case KEY_ENTER:
		case KEY_NUMENTER:
			if (optn_active_control == 0){
				hide_optn_cursor();
				on_active_item_change();
			}
/*			optn_commit_group(optn_active_group,
					optn_active_control != 2);*/
			break;
		case KEY_ESCAPE:
		case KEY_F10:
			if (optn_active_control == 0)
				hide_optn_cursor();
/*			optn_commit_group(optn_active_group, false);*/
			break;
		default:
			ret = process_optn_item(e);
	}
	return ret;
}

static int get_ppp_port(void)
{
	int ret = -1;
	struct optn_item *itm = optn_item_by_offset(use_ppp);
	if ((itm != NULL) && itm->vv.flag){
		itm = optn_item_by_offset(ppp_tty);
		if (itm != NULL)
			ret = itm->vv.i;
	}
	return ret;
}

static int get_pos_port(void)
{
	int ret = -1;
	struct optn_item *itm = optn_item_by_offset(bank_system);
	if ((itm != NULL) && itm->vv.flag){
		itm = optn_item_by_offset(bank_pos_port);
		if (itm != NULL)
			ret = itm->vv.i;
	}
	return ret;
}

static int get_aprn_port(void)
{
	struct optn_item *itm =
		optn_item_by_offset(has_aprn);
	if ((itm == NULL) || !itm->vv.flag)
		return -1;
	itm = optn_item_by_offset(aprn_tty);
	return (itm == NULL) ? -1 : itm->vv.i;
}

/* �஢�ઠ ��������� ������ � �ਭ��� */
static bool on_exit_devices(const struct optn_group *group __attribute__((unused)))
{
	char *message = NULL;
	int aprn_port = get_aprn_port();
	int ppp_port = get_ppp_port();
	int pos_port = get_pos_port();
	struct optn_item *itm;
/* �஢�ઠ ���⮢ */
	if (aprn_port != -1){
		if (aprn_port == ppp_port)
			message = "��࠭�� ���� 㦥 �ᯮ������ ��� "
				"������祭�� ������";
		else if (aprn_port == pos_port)
			message = "��࠭�� ���� 㦥 �ᯮ������ � ���";
	}
	if (message != NULL){
		message_box(NULL, message, dlg_yes, 0, al_center);
		return false;
	}
/* �஢�ઠ ������� ����஢ �ਭ�஢ */
	itm = optn_item_by_offset(has_xprn);
	if ((itm != NULL) && itm->vv.flag){
		itm = optn_item_by_offset(xprn_number);
		if ((itm != NULL) && !prn_number_correct((uint8_t *)itm->vv.s))
			message = "������ ����� ���";
	}
	if (message == NULL){
		itm = optn_item_by_offset(has_aprn);
		if ((itm != NULL) && itm->vv.flag){
			itm = optn_item_by_offset(aprn_number);
			if ((itm != NULL) && !prn_number_correct((uint8_t *)itm->vv.s))
				message = "������ ����� ���";
		}
	}
	if (message != NULL){
		message_box(NULL, message, dlg_yes, 0, al_center);
		return false;
	}else
		return true;
}

/* �஢�ઠ ip-���� */
static bool check_ip_addr(const char *val)
{
	int i, j, k, v;
	if (val == NULL)
		return false;
	for (i = k = 0; i < 4; i++, k++){
		v = 0;
		for (j = 0; (j < 3) && isdigit(val[k]); j++, k++){
			v *= 10;
			v += val[k] - 0x30;
		}
		if ((j == 0) || (v > 255) || ((i == 0) && (v == 0)))
			return false;
		else if (val[k] == 0)
			return i == 3;
		else if ((i < 3) && (val[k] != '.'))
			return false;
	}
	return false;
}

static bool on_exit_check_ip(const struct optn_group *group)
{
	struct optn_item *itm = group->items;
	int i;
	bool valid = true, host_ok = true;
	for (i = 0; valid && (i < group->n_items) && (itm != NULL); i++, itm++){
		if (itm->enabled && (itm->ot == optn_ip_edit)){
			if (!check_ip_addr(itm->vv.ip))
				valid = false;
#if defined __CHECK_HOST_IP__
			else if (((itm->offset == xoffsetof(cfg, x3_p_ip)) ||
					(itm->offset == xoffsetof(cfg, x3_s_ip))) &&
					!check_host_ip(ntohl(inet_addr(itm->vv.ip))))
				host_ok = false;
#endif
		}
	}
	if (!valid)
		message_box(NULL, "�������⨬� �ଠ� ip-����", dlg_yes, 0,
				al_center);
	else if (!host_ok)
		message_box(NULL, "�������⨬� ip-���� ���-���", dlg_yes, 0,
				al_center);
	return valid && host_ok;
}

static bool on_exit_ppp(const struct optn_group *group __attribute__((unused)))
{
	int ppp_port = get_ppp_port();
	int pos_port = get_pos_port();
	int aprn_port = get_aprn_port();
	const char *message = NULL;
	if (ppp_port != -1){
		if (ppp_port == pos_port)
			message = "��࠭�� ���� 㦥 �ᯮ������ � ���";
		else if (ppp_port == aprn_port)
			message = "��࠭�� ���� 㦥 �ᯮ������ ��� "
				"������祭�� ���";
	}
	bool ret = true;
	if (message != NULL){
		message_box(NULL, message, dlg_yes, 0, al_center);
		ret = false;
	}
	return ret;
}

static bool on_exit_bank_system(const struct optn_group *group)
{
	if (!on_exit_check_ip(group))
		return false;
	int pos_port = get_pos_port();
	int ppp_port = get_ppp_port();
	int aprn_port = get_aprn_port();
	const char *message = NULL;
	if (pos_port != -1){
		if (pos_port == ppp_port)
			message = "��࠭�� ���� 㦥 �ᯮ������ � PPP";
		else if (pos_port == aprn_port)
			message = "��࠭�� ���� 㦥 �ᯮ������ ��� "
				"������祭�� ���";
	}
	bool ret = true;
	if (message != NULL){
		message_box(NULL, message, dlg_yes, 0, al_center);
		ret = false;
	}
	return ret;
}

/* ����祭�� ����஥� ��� */
static void get_lprn_params(void)
{
	if ((wm != wm_local) || (kt != key_dbg))
		adjust_lprn_params(false);
	else if (lprn_params_read)
		adjust_lprn_params(true);
	else{
		int ret = lprn_get_params(&cfg);
		if (ret == LPRN_RET_OK){
			lprn_params_read = true;
			optn_read_group(&cfg, OPTN_GROUP_DEVICES);
			adjust_lprn_params(true);
		}else if (ret == LPRN_RET_ERR){
			ClearScreen(clBlack);
			err_beep();
			message_box("������ ���", "�� 㤠���� ������� ��ࠬ���� ࠡ��� ���.",
				dlg_yes, DLG_BTN_YES, al_center);
			adjust_lprn_params(false);
		}
	}
}

/* ��ࠡ�⪠ ���� ����஥� */
bool process_options(const struct kbd_event *e)
{
	do_cursor_blinking();
	if (!e->pressed)
		return true;
	if (optn_active_group == OPTN_GROUP_MENU)
		if (process_menu(optn_menu, (struct kbd_event *)e) != cmd_none)
			switch (get_menu_command(optn_menu)){
				case cmd_sys_optn:
					optn_set_group(OPTN_GROUP_SYSTEM);
					break;
				case cmd_dev_optn:
					get_lprn_params();
					optn_set_group(OPTN_GROUP_DEVICES);
					break;
				case cmd_tcpip_optn:
					optn_set_group(OPTN_GROUP_TCPIP);
					break;
				case cmd_ppp_optn:
					optn_set_group(OPTN_GROUP_PPP);
					break;
				case cmd_bank_optn:
					optn_set_group(OPTN_GROUP_BANK);
					break;
				case cmd_kkt_optn:
					optn_set_group(OPTN_GROUP_KKT);
					break;
				case cmd_scr_optn:
					optn_set_group(OPTN_GROUP_SCR);
					break;
				case cmd_kbd_optn:
					optn_set_group(OPTN_GROUP_KBD);
					break;
				case cmd_store_optn:
					optn_cm = cmd_store_optn;	/* fall through */
				default:
					return false;
			}
		else
			return true;
	else if (!process_optn_group(e)){
		bool ok = (((e->key == KEY_ENTER) || (e->key == KEY_NUMENTER)) &&
				(optn_active_control != 2));
		if (optn_groups[optn_active_group].on_exit && ok &&
			!optn_groups[optn_active_group].on_exit(
				optn_groups + optn_active_group))
			optn_set_group(optn_active_group);
		else{
			optn_commit_group(optn_active_group, ok);
			optn_set_group(OPTN_GROUP_MENU);
		}
	}
	return true;
}

/* �㭪樨 ���⭮�� �맮�� */
/* ��뢠���� ��� �஢�ન ���������� ��४��祭�� � ०�� VipNet */
static void on_iplir_change(struct optn_item *item)
{
	if (item != NULL)
		item->enabled = !iplir_disabled;
}

/* ��뢠���� �� ����祭��/�몫�祭�� 䫠�� ������ ��� */
static void on_xprn_change(struct optn_item *item)
{
	if (item != NULL)
		optn_set_item_enable(xprn_number, item->vv.flag);
}

/* ��뢠���� �� ����祭��/�몫�祭�� 䫠�� ������ ��� */
static void on_aprn_change(struct optn_item *item)
{
	if (item == NULL)
		return;
	bool flag = item->vv.flag;
	optn_set_item_enable(aprn_number, flag);
	optn_set_item_enable(aprn_tty, flag);
}

/* ��뢠���� �� ��������� ��ࠬ��� "���� � ���" */
static void on_bank_change(struct optn_item *item)
{
	if (item == NULL)
		return;
	bool flag = item->vv.flag;
	optn_set_item_enable(bank_pos_port, flag);
	optn_set_item_enable(bank_proc_ip, flag);
#if defined __EXT_POS__
	optn_set_item_enable(ext_pos, flag);
#endif
}

/* ��뢠���� �� ��������� ��ࠬ��� "����稥 ���" */
static void on_kkt_change(struct optn_item *item)
{
	if (item == NULL)
		return;
	bool flag = (kkt != NULL) && item->vv.flag;
	optn_set_item_enable(fiscal_mode, flag);
	optn_set_item_enable(fdo_iface, flag);
	optn_set_item_enable(fdo_ip, flag);
	optn_set_item_enable(fdo_port, flag);
	optn_set_item_enable(fdo_poll_period, flag);
	optn_set_item_enable(tax_system, flag);
	optn_set_item_enable(kkt_log_level, flag);
	optn_set_item_enable(kkt_log_stream, flag);
	optn_set_item_enable(tz_offs, flag);
	optn_set_item_enable(kkt_base_timeout, flag);
	optn_set_item_enable(kkt_brightness, flag);
	optn_set_item_enable(kkt_apc, flag);
	on_fdo_iface_change(optn_item_by_offset(fdo_iface));
}

/* ��뢠���� �� ��������� ����䥩� ����������⢨� ��� � ��� */
static void on_fdo_iface_change(struct optn_item *item)
{
	if (item == NULL)
		return;
	struct optn_item *itm = optn_item_by_offset(has_kkt);
	bool kkt = (itm == NULL) ? false : itm->vv.flag;
	int fdo_iface = item->vv.i;
	optn_set_item_enable(kkt_ip, kkt && (fdo_iface == KKT_FDO_IFACE_ETH));
	optn_set_item_enable(kkt_netmask, kkt && (fdo_iface == KKT_FDO_IFACE_ETH));
	optn_set_item_enable(kkt_gw, kkt && (fdo_iface == KKT_FDO_IFACE_ETH));
	optn_set_item_enable(kkt_gprs_apn, kkt && (fdo_iface == KKT_FDO_IFACE_GPRS));
	optn_set_item_enable(kkt_gprs_user, kkt && (fdo_iface == KKT_FDO_IFACE_GPRS));
	optn_set_item_enable(kkt_gprs_passwd, kkt && (fdo_iface == KKT_FDO_IFACE_GPRS));
}

/* ��뢠���� � ��砥 ��������� 梥⮢�� �奬� �� ���ᮢ�� ���� ����ன�� */
static void on_scheme_change(struct optn_item *item)
{
	if ((item != NULL) &&
			(item == &optn_groups[optn_active_group].items[optn_active_item])){
		scheme_changed = true;
	}
}

void adjust_kkt_brightness(uint32_t n)
{
	struct optn_item *itm = optn_item_by_offset(kkt_brightness);
	itm->n = n;
}
