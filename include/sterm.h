/* Основные определения терминала "Экспресс-2А-К". (c) gsr 2000-2020 */

#if !defined STERM_H
#define STERM_H

#include <stdio.h>
#include <time.h>
#include "blimits.h"
#include "cfg.h"
#include "ds1990a.h"
#include "express.h"
#include "gd.h"
#include "hash.h"
#include "kbd.h"
#include "gui/gdi.h"

/* Режим работы терминала */
enum {
	wm_main,		/* основной */
	wm_local,		/* пригородный */
};

extern int wm;

/* Необходима блокировка терминала (ошибка работы с ППУ) */
extern bool need_lock_term;

/* Версия терминала */
struct build_version{
	uint32_t version;
	uint32_t subversion;
	uint32_t modification;
};

extern struct build_version term_build;

extern char *term_string;	/* название терминала */

/* Информация для ИПТ */
extern struct bank_info bi;
extern struct bank_info bi_pos;

extern bool menu_active;
extern bool optn_active;
extern bool calc_active;
extern bool xlog_active;
extern bool plog_active;
extern bool llog_active;
extern bool klog_active;
extern bool xchg_active;
extern bool help_active;
extern bool ssaver_active;
extern bool ping_active;
extern bool pos_active;
extern bool fa_active;

/* Команды терминала */
enum {
	cmd_none,
	cmd_exit,
	cmd_done,
	cmd_reset,
	cmd_enter,
	cmd_print,
	cmd_snap_shot,
	cmd_view_req,
	cmd_view_resp,
	cmd_view_raw_resp,
	cmd_view_xchange,
	cmd_reject,
	cmd_view_rom,
	cmd_view_prom,
	cmd_view_keys,
	cmd_options,
	cmd_calculator,
	cmd_switch_res,
	cmd_help,
	cmd_view_xlog,
	cmd_xlog_menu,
	cmd_print_xlog,
	cmd_print_xlog_rec,
	cmd_print_xlog_aux,
	cmd_print_xlog_range,
	cmd_find_xlog_date,
	cmd_find_xlog_number,
	cmd_blank,
	cmd_wakeup,
	cmd_view_error,
	cmd_sys_optn,
	cmd_dev_optn,
	cmd_tcpip_optn,
	cmd_ppp_optn,
	cmd_bank_optn,
	cmd_kkt_optn,
	cmd_scr_optn,
	cmd_kbd_optn,
	cmd_store_optn,
	cmd_ping,
	cmd_pgup,
	cmd_pgdn,
	cmd_shell,
#if defined __WATCH_EXPRESS__
	cmd_watch_req,
#endif
	cmd_ppp_hangup,
	cmd_use_xprn,
	cmd_use_aprn,
	cmd_use_lprn,
	cmd_view_plog,
	cmd_plog_menu,
	cmd_print_plog,
	cmd_print_plog_rec,
	cmd_print_plog_range,
	cmd_find_plog_date,
	cmd_find_plog_number,
	cmd_pos,
	cmd_switch_wm,
	cmd_term_info,
	cmd_iplir_version,
	cmd_kkt_info,
	cmd_view_llog,
	cmd_llog_menu,
	cmd_print_llog,
	cmd_print_llog_rec,
	cmd_print_llog_express,
	cmd_print_llog_aux,
	cmd_print_llog_range,
	cmd_find_llog_date,
	cmd_find_llog_number,
	cmd_ticket_number,
	cmd_lprn_menu,		/* меню работы с образами бланков ППУ */
	cmd_lprn_snapshots,
	cmd_lprn_erase_sd,
	cmd_continue,		/* используется при показе сообщений об ошибках ППУ */
	cmd_view_klog,
	cmd_klog_menu,
	cmd_print_klog,
	cmd_print_klog_rec,
	cmd_print_klog_range,
	cmd_find_klog_date,
	cmd_find_klog_number,
/* Фискальное приложение */
	cmd_fa,
	cmd_reg_fa,
	cmd_rereg_fa,
	cmd_open_shift_fa,
	cmd_close_shift_fa,
	cmd_cheque_fa,
	cmd_cheque_corr_fa,
	cmd_calc_report_fa,
	cmd_close_fs_fa,
	cmd_del_doc_fa,
	cmd_unmark_reissue_fa,
	cmd_reset_fs_fa,
	cmd_print_last_doc_fa,
	cmd_sales_fa,
	cmd_newcheque_fa,
	cmd_agents_fa,
	cmd_articles_fa,
	cmd_archive_fa,
};

extern int kt;			/* тип ключа DS1990A */
extern ds_number dsn;		/* номер ключа DS1990A */

/* Устанавливается во время автоматической печати чека на ККТ */
extern bool apc;

extern uint16_t	term_check_sum;	/* контрольная сумма терминала */
extern time_t	time_delta;	/* Tхост - Tтерм. */
extern uint8_t	hbyte;		/* H-байт */
extern int	_term_state;
extern int	_term_aux_state;
extern bool	full_resp;	/* флаг прихода ответа от хост-ЭВМ */
extern bool	rejecting_req;	/* был послан запрос с отказом от заказа */
extern bool	online;		/* флаг возможности гашения экрана */
extern bool	into_on_response;	/* используется в show_error */
/* Устанавливается при выводе на экран сообщений об ошибках ППУ */
extern bool	lprn_error_shown;

extern uint8_t	text_buf[TEXT_BUF_LEN];
extern uint8_t *err_ptr;
extern int	ecode;
extern struct para_info map[MAX_PARAS + 1];
extern int	n_paras;
extern int	cur_para;

extern struct hash *rom;
extern struct hash *prom;

extern bool	can_reject;
extern bool	resp_handling;
extern bool	resp_executing;
extern bool	resp_printed;

extern bool	has_bank_data;
extern bool	has_kkt_data;

extern int	astate_for_req;

extern uint8_t	n2hbyte(int n);
extern void	show_error(void);
extern void	show_dest(int dst);
extern void	show_ndest(int n);
extern bool	term_delay(int d);
extern void	show_req(void);
extern void	reject_req(void);
extern void	switch_term_mode(void);
extern void	send_request(void);
extern void	show_llog(void);
extern bool	push_term_info(void);
extern bool	pop_term_info(void);
extern int	get_key_type(void);
extern void	flush_home(void);
extern bool	is_escape(uint8_t c);
extern bool	set_term_busy(bool busy);
extern const char *find_term_state(int st);
extern const char *find_term_astate(int ast);
extern bool	set_term_state(int st);
extern bool	set_term_astate(int ast);
extern bool	set_term_led(char c);
extern void	guess_term_state(void);
extern void	scr_wakeup(void);
extern void	release_garbage(void);
extern void	redraw_term(bool show_text, const char *title);
extern bool	kbd_alive(struct kbd_event *e);
extern int	get_cmd(bool check_scr, bool busy);
extern bool	reset_term(bool force);
extern void	hangup_ppp(void);

#define RET_NORMAL		0
#define RET_SHELL		20
#define RET_VERSION		30
#define RET_SIGTERM		40
#define RET_ERROR		99

#endif		/* STERM_H */
