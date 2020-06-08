/*
	sterm status line routines
	(c) 2001 gsr & alex
*/

#ifndef STATUS_H
#define STATUS_H

#include "sysdefs.h"

/* Левая часть строки статуса */
enum {
	st_none,
	st_input,
	st_wait,
	st_transmit,
	st_recv,
	st_wrecv,
	st_ireq,
	st_resp,
	st_fresp,
	st_scr,
	st_prn,
	st_req,
	st_keys,
	st_hash,
	st_array,
	st_raw_resp,
	st_log,
	st_xchange,
	st_help,
	st_wait_f11,
	st_ping,
/* Обработка PPP */
	st_ppp_startup,
	st_ppp_cleanup,
	st_ppp_init,
	st_ppp_dialing,
	st_ppp_login,
	st_ppp_ipcp,
	st_ppp_ready,
/* Работа с VipNet */
	st_stop_iplir,
};

/* Правая часть строки статуса */
enum {
	ast_none,
	ast_noxprn,
	ast_noaprn,
	ast_nolprn,
	ast_lprn_err,
	ast_lprn_ch_media,
	ast_lprn_sd_err,
	ast_rejected,
	ast_repeat,
	ast_resp,
	ast_prn,
	ast_req,
	ast_nokey,
	ast_init,
	ast_finit,
	ast_rkey,
	ast_skey,
	ast_dkey,
	ast_no_log_item,
	ast_illegal,
	ast_error,
	ast_prn_number,
	ast_pos_error,
	ast_pos_need_init,
	ast_no_kkt,
};

extern bool		scr_show_language(bool show);
extern bool		scr_show_mode(bool show);
extern bool		scr_show_log(bool show);
extern bool		scr_set_led(char c);
extern bool		scr_show_key(bool show);
extern bool		scr_draw_rollers(void);
extern bool		scr_set_lstatus(const char *s);
extern bool		scr_set_rstatus(const char *s);

#endif		/* STATUS_H */
