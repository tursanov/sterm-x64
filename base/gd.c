/* �஢��� ��࠭�஢����� ���⠢�� �ନ����. (c) gsr 2000, 2004-2018 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "gui/scr.h"
#include "kkt/kkt.h"
#include "log/express.h"
#include "prn/express.h"
#include "prn/local.h"
#include "cfg.h"
#include "genfunc.h"
#include "numbers.h"
#include "ppp.h"
#include "sterm.h"
#include "transport.h"
#include "tki.h"
#include "xml2data.h"

int s_state = ss_ready;			/* ���ﭨ� ᥠ�ᮢ��� �஢�� */
int session_error = SLAYER_OK;		/* �訡�� ᥠ�ᮢ��� �஢�� */
bool delayed_init = false;		/* 䫠� �⫮������ ���樠����樨 */

uint32_t init_t0 = 0;			/* �६� ��砫� ���樠����樨 */
static int init_tries;			/* �᫮ ����⮪ ���樠����樨 */

/* �����䨪��� �ନ���� (��ன ���� ��������� � ����ᨬ��� �� ���䨣��樨) */
static uint8_t term_id[3] = {0x6c, 0x00, 0x30};
/* ���稪� ��࠭�஢����� ���⠢�� */
uint16_t ZNtz	= 0;
uint16_t oldZNtz= 0;	/* �ᯮ������ �� ���樠����樨 */
uint16_t ZNpo	= 0;
uint16_t oldZNpo= 0;
uint8_t ZBp	= 0;
uint8_t oldZBp	= 0;
uint16_t ONpo	= 0;
uint16_t ONtz	= 0;
uint8_t OBp	= 0;
static uint16_t Ocrc;
static uint8_t Ogaddr;
static uint8_t Oiaddr;

/*
 * ��� ᨭ⠪��᪮� �訡�� �⢥� � ��᫥���� ��।���� �����.
 * �ᯮ������ �� �ନ஢���� ����� ��� ⨯� LLRT_SYNTAX_ERROR
 */
static struct {
	uint8_t err;
	char req[OUT_BUF_LEN + 1];
} __attribute__((__packed__)) _last_request;

char *last_request = _last_request.req;

/* ���� ���� ��᫥����� �⢥� */
static void reset_last_request(void)
{
	_last_request.err = 0;
	_last_request.req[0] = 0;
}

/* ���樠������ �஢�� ��࠭�஢����� ���⠢�� */
void init_gd(void)
{
	open_transport();
	ZBp = GDF_REQ_INIT | GDF_REQ_FIRST;
	OBp = 0;
	delayed_init = false;
	srand(time(NULL));
}

/* �᢮�������� �஢�� ��࠭�஢����� ���⠢�� */
void release_gd(void)
{
	close_transport();
}

/* ��ॢ�� �஢�� ��࠭�஢����� ���⠢�� � ��砫쭮� ���ﭨ� */
void reset_gd(bool need_init)
{
	ZNpo = ONpo = ONtz = 0;
	ZBp = GDF_REQ_INIT | GDF_REQ_FIRST;
	if (s_state != ss_finit)
		ZNtz = RANDOM_VAL;
	session_error = SLAYER_OK;
	init_tries = 0;
	init_t0 = 0;
	if (need_init){
		_term_aux_state = ast_init;
		s_state = ss_new;
		release_term_socket();
		delayed_init = false;
	}else{
		s_state = ss_ready;
		delayed_init = true;	/* �ந��樠�����㥬�� �� ��ࢮ� ����� */
	}
}

/* ���४�஢�� 䫠��� ��࠭�஢����� ���⠢�� */
static void adjust_flags(void)
{
	CLR_FLAG(ZBp, GDF_REQ_TYPE);
	if (hbyte == HB_INIT)
		SET_FLAG(ZBp, GDF_REQ_INIT);
	else
		SET_FLAG(ZBp, GDF_REQ_APP);
}

/* ���᫥��� ����஫쭮� �㬬� �� �������� x^0 + x^13 + x^15 */
uint16_t x3_crc(uint8_t *p, int l)
{
	uint16_t div = 1 | (1 << 13) | (1 << 15);
	uint32_t s = 0;
	int i;
	if (p != NULL){
		for (i = 0; i < l; i++){
			s <<= 8;
			s |= p[i];
			s %= div;
		}
	}
	return (uint16_t)s;
}

/* ��।������ ᬥ饭�� ��砫� ����� (����� ���������) */
int get_req_offset(void)
{
	return 6;
}

/* ��।������ ���ᨬ��쭮� ����� ����� */
int get_max_req_len(int offset)
{
	return sizeof(req_buf) - offset - 45;
}

/*
 * ���������� � ⥪��� ����� ��ࠬ����� ��࠭�஢����� ���⠢��.
 * ����� ��室���� � ����, ��। ��� ���� ���� ��� ���������,
 * req_len -- ����� ����� ����� � ����������.
 */
/* ������ � ⥪�� ������ ���ଠ樨 � ᥠ�� */
static void write_session_info(void)
{
	req_buf[req_len++] = SESSION_INFO_MARK;
	if (s_state == ss_ready)
		INC_VAL(ZNtz);
	write_hex(req_buf + req_len, ZNtz, 3);
	req_len += 3;
	write_hex(req_buf + req_len, ZNpo, 3);
	req_len += 3;
	adjust_flags();
	req_buf[req_len++] = ZBp;
}

/* ��������� �����䨪��� �ନ���� � ����ᨬ��� �� ��� ���������⥩ */
static uint8_t get_tcap_byte(void)
{
	static uint8_t map[] = {
/* 00 -- 07 */	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,	/* 0 1 2 3 4 5 6 7 */
/* 08 -- 0f */	0x38, 0x39, 0x61, 0x62, 0x77, 0x67, 0x64, 0x65,	/* 8 9 � � � � � � */
/* 10 -- 17 */	0x76, 0x7a, 0x00, 0x00, 0x6b, 0x6c, 0x6d, 0x6e,	/* � � . . � � � � */
/* 18 -- 1f */	0x6f, 0x70, 0x72, 0x73, 0x74, 0x75, 0x66, 0x68,	/* � � � � � � � � */
/* 20 -- 27 */	0x63, 0x7e, 0x7b, 0x7d, 0x79, 0x78, 0x7c, 0x60,	/* � � � � � � � � */
/* 28 -- 2f */	0x71, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,	/* � A B C D E F G */
/* 30 -- 37 */	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,	/* H I J K L M N O */
/* 38 -- 3f */	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,	/* P Q R S T U V W */
/* 40 -- 47 */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* . . . . . . . . */
/* 48 -- 4f */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* . . . . . . . . */
/* 50 -- 57 */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* . . . . . . . . */
/* 58 -- 5f */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* . . . . . . . . */
/* 60 -- 67 */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* . . . . . . . . */
/* 68 -- 6f */	0x00, 0x00, 0x58, 0x69, 0x00, 0x00, 0x59, 0x6a,	/* . X � . . . Y � */
/* 70 -- 77 */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* . . . . . . . . */
/* 77 -- 7f */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* . . . . . . . . */
	};
	uint8_t ret = TCAP_TERM | TCAP_BNK2 | TCAP_EX_BCODE | TCAP_UNIBLANK;
	if (cfg.has_kkt && (kkt != NULL) && cfg.fiscal_mode){
		ret |= TCAP_KKT;
		if (kkt_has_param("SUPPORT_ESC_R"))
			ret |= TCAP_XSLT;
	}
	if (cfg.has_sprn)
		ret |= TCAP_EX_BCODE;
	if (!cfg.bank_system)
		ret |= TCAP_NO_POS;
	return map[ret];
}

/* ������ � ⥪�� ������ ���ଠ樨 � �ନ���� */
static void write_term_info(void)
{
	term_number tn;
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn);
	req_buf[req_len++] = TERM_INFO_MARK;
/* �����䨪��� �ନ���� */
	req_buf[req_len++] = term_id[0];
	req_buf[req_len++] = get_tcap_byte();
	req_buf[req_len++] = term_id[2];
/* �����᪮� ����� �ନ���� */
	memcpy(req_buf + req_len, tn, sizeof(tn));
	req_len += sizeof(tn);
/* ����� ��⮭� DS1990A */
	static int ind[] = {7, 0, 6, 5, 4, 3, 2, 1};
	for (int i = 0; i < ASIZE(ind); i++){
		write_hex_byte(req_buf + req_len, dsn[ind[i]]);
		req_len += 2;
	}
}

/* ������ � ⥪�� ������ ���ଠ樨 � �ਭ��� */
static void write_prn_info(void)
{
	int i;
	static uint8_t zero_number[PRN_NUMBER_LEN] = {[0 ... PRN_NUMBER_LEN - 1] = 0x30};
	uint8_t *number;
	req_buf[req_len++] = PRN_INFO_MARK;
/* ����� �᭮����� �ਭ�� (���) */
	if (cfg.has_xprn && prn_number_correct((uint8_t *)cfg.xprn_number))
		number = (uint8_t *)cfg.xprn_number;
	else
		number = zero_number;
	for (i = 0; i < PRN_NUMBER_LEN; i++)
		req_buf[req_len++] = recode(number[i]);
/* ����� �������⥫쭮�� �ਭ�� (���) */
	if (cfg.has_aprn && prn_number_correct((uint8_t *)cfg.aprn_number))
		number = (uint8_t *)cfg.aprn_number;
	else
		number = zero_number;
	for (i = 0; i < PRN_NUMBER_LEN; i++)
		req_buf[req_len++] = recode(number[i]);
/* ����� ����⮯����饣� ���ன�⢠ (���) */
	if (cfg.has_sprn)
		memcpy(req_buf + req_len, lprn_number, sizeof(lprn_number));
	else
		memset(req_buf + req_len, 0x30, sizeof(lprn_number));
	req_len += sizeof(lprn_number);
}

/* ������ � ⥪�� ������ ���ଠ樨 � ���ᨨ �ନ���� */
static void write_version_info(void)
{
	req_buf[req_len++] = VERSION_INFO_MARK;
	write_hex_word(req_buf + req_len, term_check_sum);
	req_len += 4;
	sprintf((char *)(req_buf + req_len), "%.3d.%.3d.%.3d",
		STERM_VERSION_MAJOR, STERM_VERSION_MINOR, STERM_VERSION_RELEASE);
	req_len += 11;
}

/* ������ � ⥪�� ������ ���ଠ樨 � �६��� ॠ�樨 �ନ���� */
static void write_reaction_info(void)
{
	req_buf[req_len++] = REACTION_INFO_MARK;
	write_hex_byte(req_buf + req_len, (reaction_time + 50) / 100);
	req_len += 2;
}

/* ������ � ⥪�� ������ ����஫쭮� �㬬� */
static void write_crc(void)
{
	write_hex_word(req_buf + req_len, x3_crc(req_buf, req_len));
	req_len += 4;
}

/* ���࠭���� ��।�������� ����� � ���� */
static bool store_text_tcpip(void)
{
	size_t txt_len;
	if (req_len < REQ_PREFIX_LEN)
		return false;
	txt_len = req_len - REQ_PREFIX_LEN;
	if ((txt_len + 1) > sizeof(_last_request.req))
		txt_len = sizeof(_last_request.req) - 1;
	if (txt_len > 0)
		memcpy(_last_request.req, req_buf + REQ_PREFIX_LEN, txt_len);
	_last_request.req[txt_len] = 0;
	return true;
}

static bool wrap_text_tcp_ip(void)
{
	int l;
	l = sizeof(req_buf) - req_len;
	if (l < REQ_SUFFIX_LEN)
		return false;
	store_text_tcpip();
	req_buf[4] = cfg.gaddr;
	req_buf[5] = cfg.iaddr;
	write_prn_info();
	write_version_info();
	write_reaction_info();
	write_session_info();
	write_term_info();
	write_hex_word(req_buf, req_len + 4);
	write_crc();
	oldZNtz = ZNtz;
	oldZNpo = ZNpo;
	oldZBp = ZBp;		/* ��� ����஫쭮� ����� */
	req_time = u_times();
	reaction_time = 0;
	return true;
}

extern uint8_t *err_ptr;

bool wrap_text(void)
{
	err_ptr = NULL;
	reset_last_request();
	if (wrap_text_tcp_ip()){
		clr_xml_data();
		begin_transmit();
		return true;
	}else
		return false;
}

/* �⪠� �� ������ */
void reject(void)
{
	SET_FLAG(ZBp, GDF_REQ_REJECT);
	req_len = get_req_offset();
	wrap_text();
}

/* ��।������ ⨯� �訡�� ��࠭�஢����� ���⠢�� */
static int gd_error_type(int e)
{
	if ((e >= SERR_BASE) && (e < TCPERR_BASE))
		return gde_session;
	else if ((e >= TCPERR_BASE) && (e < PPPERR_BASE))
		return gde_tcp;
	else if (e >= PPPERR_BASE)
		return gde_ppp;
	else
		return gde_noerror;
}

/* �㦭� �� ᫥����� ���樠������ */
static bool need_next_init(int e)
{
	switch (gd_error_type(e)){
		case gde_session:
			return init_tries < MAX_INIT_TRIES_SESSION;
		case gde_tcp:
			return init_tries < MAX_INIT_TRIES_TCP;
		case gde_ppp:
			return init_tries < MAX_INIT_TRIES_PPP;
		default:
			return false;
	}
}

void slayer_error(int e)
{
	session_error = e;
	if (s_state == ss_initializing){
		init_tries++;
		if (need_next_init(e))
			s_state = ss_winit;
		else{
			set_term_astate(ast_finit);
			init_tries = 0;
			init_t0 = 0;
			s_state = ss_finit;
			c_state = cs_nc;
			set_term_state(st_none);
			release_term_socket();
		}
	}else{
		guess_term_state();
		switch (c_state){
			case cs_wconnect:
			case cs_sending:
				set_term_astate(ast_repeat);
				break;
			default:
				set_term_astate(ast_rejected);
				break;
		}
		c_state = cs_nc;
	}
	if (cfg.tcp_cbt || (gd_error_type(e) == gde_tcp))
		release_term_socket();
}

/* ��砫� ���樠����樨 */
void begin_initialization(void)
{
	init_t0 = u_times();
	set_term_state(st_none);
	set_term_astate(ast_init);
	hbyte = HB_INIT;
	s_state = ss_initializing;
	req_len = get_req_offset();
	req_type = req_regular;
	wrap_text();
}

/* �஢�ઠ �������쭮� ����� �⢥� */
bool check_min_resp_len(void)
{
	if (resp_len < 15){
		SET_FLAG(ZBp, GDF_REQ_CRC);
		slayer_error(SERR_CHKSUM);
		resp_len = 0;
		return false;
	}else
		return true;
}

/* �஢�ઠ ����஫쭮� �㬬� �⢥� */
bool check_crc(void)
{
	Ocrc = read_hex_word(resp_buf + resp_len - 4);
	if (Ocrc == x3_crc(resp_buf, resp_len - 4))
		return true;
	else{
		SET_FLAG(ZBp, GDF_REQ_CRC);
		if ((Ocrc == 0) && (resp_len == 19)){	/* ᯥ樠��� �⢥� */
			xlog_write_rec(hxlog, resp_buf + 13, 2,
				XLRT_SPECIAL, 0);
			slayer_error(SERR_SPECIAL);
		}else
			slayer_error(SERR_CHKSUM);
		return false;
	}
}

/* ����祭�� ���稪�� ��࠭�஢����� ���⠢�� � 䫠��� */
static bool read_resp_gd_info(void)
{
	Ogaddr = resp_buf[4];
	Oiaddr = resp_buf[5];
	ONpo = read_hex(resp_buf + 6, 3);
	if (!number_error){
		ONtz = read_hex(resp_buf + 9, 3);
		if (!number_error)
			OBp = resp_buf[12];
	}
	return !number_error;
}

/* �஢�ઠ ���稪�� � 䫠��� ��� ���筮�� �⢥� */
static bool check_gd_counters(uint8_t flags_to_clear)
{
/* �⢥� �� ���樠������ ����� �ਪ������� ⥪�� */
	if (TST_FLAG(OBp, GDF_RESP_INIT) || !TST_FLAG(OBp, GDF_RESP_APP)){
		slayer_error(SERR_HANDSHAKE);
		return false;
	}
/* ����祭 �⢥� �� �� �� ��᫠��� ����� ��� �⢥� 㦥 ��ࠡ�⠭ */
	if ((DELTA(ONpo, ZNtz) > 0) || (DELTA(ONpo, ZNpo) <= 0)){
		slayer_error(SERR_COUNTER);
		return false;
	}
/* ����뢠�� ��� ���樠����樨 */
	SET_FLAG(flags_to_clear, GDF_REQ_FIRST);
	ZNpo = ONpo;
	SET_FLAG(flags_to_clear, GDF_REQ_REJECT);
	CLR_FLAG(ZBp, flags_to_clear);
	return true;
}

/* �஢�ઠ ���稪�� � 䫠��� ��� �⢥� �� ���樠������ */
static bool check_gd_counters_init(uint8_t flags_to_clear)
{
/* �ਪ������ ⥪�� � �⢥� �� ���樠������ */
	if (TST_FLAG(OBp, GDF_RESP_APP) || !TST_FLAG(OBp, GDF_RESP_INIT)){
		slayer_error(SERR_APPONINIT);
		return false;
	}
/* ��ᮢ������� ���� �㪮������ */
	if (ONtz != ZNtz){
		slayer_error(SERR_COUNTER);
		return false;
	}
	ZNtz = ZNpo = ONpo;
	SET_FLAG(flags_to_clear, GDF_REQ_REJECT);
	CLR_FLAG(ZBp, flags_to_clear);
	return true;
}

/* �஢�ઠ �ࠢ�� ��࠭�஢����� ���⠢�� */
bool check_gd_rules(void)
{
	uint8_t flags_to_clear = 0;
	if (!read_resp_gd_info()){
		SET_FLAG(ZBp, GDF_REQ_CRC);
		slayer_error(SERR_CHKSUM);
		return false;
	}
	SET_FLAG(flags_to_clear, GDF_REQ_CRC);
/* �஢�ઠ ���� �ନ���� */
	if ((Ogaddr != cfg.gaddr) || (Oiaddr != cfg.iaddr)){
		SET_FLAG(ZBp, GDF_REQ_FOREIGN);
		xlog_write_foreign(hxlog, Ogaddr, Oiaddr);
		slayer_error(SERR_FOREIGN);
		return false;
	}
	SET_FLAG(flags_to_clear, GDF_REQ_FOREIGN);
/* �஢�ઠ ���稪�� � 䫠��� */
	if (s_state == ss_ready){
		guess_term_state();
		return check_gd_counters(flags_to_clear);
	}else
		return check_gd_counters_init(flags_to_clear);
}

/* ��⠭���� ᬥ饭�� � ����� �ਪ������� ⥪�� �⢥� */
void set_text_len(void)
{
	text_offset = 13;
	text_len = resp_len - text_offset - 4;
}
