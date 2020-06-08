/*
 * Вывод на экран информации циклической контрольной ленты (ЦКЛ).
 * (c) gsr, Alex 2001-2004, 2009, 2018-2020.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "gui/log/express.h"
#include "gui/scr.h"
#include "log/express.h"
#include "prn/aux.h"
#include "prn/express.h"
#include "cfg.h"
#include "express.h"
#include "genfunc.h"
#include "kbd.h"
#include "sterm.h"

/* Запись в экранный буфер записи ЦКЛ типа XLRT_NORMAL/XLRT_AUX */
static void xlog_fill_scr_normal(struct log_gui_context *ctx)
{
	int i, j = 1, k = 0, col = 0, n = 0, m = 1;
	bool dle = false, nl = false, line_begin = true;
	uint8_t b = 0;
	char prefix[80] = "";
	ctx->scr_data_len = ctx->scr_data_lines = 0;
	if (xlog_rec_hdr.type == XLRT_AUX)
		snprintf(prefix, sizeof(prefix), "ДОПОЛНИТЕЛЬН\x9bЙ ПРИНТЕР");
	if (prefix[0] == 0)
		ctx->scr_data[0] = 0;
	else{
		log_add_scr_str(ctx, true, "%s", prefix);
		if (ctx->scr_data_len < ctx->scr_data_size)
			ctx->scr_data[ctx->scr_data_len++] = 0;
		j = ctx->scr_data_len;
	}
	for (i = 0; (i < xlog_data_len) && (j < ctx->scr_data_size); i++){
		b = xlog_data[i];
		if (dle){
			nl = (b == XPRN_WR_BCODE) || (b == XPRN_NO_BCODE) ||
				(b == XPRN_RD_BCODE);
			if (nl && !line_begin){
				nl = dle = false;
				col = 0;
				ctx->scr_data_lines++;
				ctx->scr_data[j - 1] = 0;
				ctx->scr_data[j++] = 0;
				i -= 2;
				line_begin = true;
				continue;
			}
			dle = line_begin = false;
			n = log_get_cmd_len(xlog_data, xlog_data_len, i);
		}else{
			dle = is_escape(b);
			if (!dle)
				line_begin = false;
		}
		switch (b){
			case 0x0a:
				ctx->scr_data_lines++;
				nl = false;
				j = m;
				ctx->scr_data[j++] = 0;
				k = j++;
				if (k < ctx->scr_data_size)
					ctx->scr_data[k] = col;
				break;
			case 0x0d:
				col = 0;
				ctx->scr_data[k] = col;
				m = j;
				j = k + 1;
				line_begin = true;
				break;
			default:
				if (nl && (n == 0)){
					nl = false;
					col = 0;
					ctx->scr_data_lines++;
					ctx->scr_data[j++] = 0;
					if (j < ctx->scr_data_size)
						ctx->scr_data[j++] = 0;
					line_begin = true;
				}else if (col == LOG_SCREEN_COLS){
					col = 0;
					ctx->scr_data_lines++;
					ctx->scr_data[j++] = 0;
					if (j < ctx->scr_data_size)
						ctx->scr_data[j++] = 0;
					line_begin = true;
				}
				if (dle)
					b = 0xdf;	/* инверсный _ */
				else if (n > 0){
					n--;
					if (b < 0x20)
						b += 0x40;
					b |= 0x80;
				}else if ((xlog_rec_hdr.type == XLRT_AUX) &&
						(b == APRN_DLE))
					b = '_';
				else if (b < 0x20)
					b += 0xc0;
				if (j < ctx->scr_data_size)
					ctx->scr_data[j++] = b;
				col++;
				break;
		}
		if (j > m)
			m = j;
	}
	if (b != 0x0a)
		ctx->scr_data_lines++;
	ctx->scr_data_len = j;
}

/* Занесение в экранный буфер записи типа XLRT_NOTIFY */
static void xlog_fill_scr_notify(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "ОТВЕТ БЕЗ ПЕЧАТИ");
}

/* Занесение в экранный буфер записи типа XLRT_INIT */
static void xlog_fill_scr_init(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "ИНИЦИАЛИЗАЦИЯ");
}

/* Занесение в экранный буфер записи типа XLRT_FOREIGN */
static void xlog_fill_scr_foreign(struct log_gui_context *ctx)
{
	char msg[80];
	struct term_addr *addr = (struct term_addr *)xlog_data;
	if (xlog_data_len == sizeof(*addr))
		snprintf(msg, sizeof(msg), "ПОЛУЧЕН ОТВЕТ ДЛЯ ДРУГОГО "
			"ТЕРМИНАЛА (%.2hhX:%.2hhX)",
			addr->gaddr, addr->iaddr);
	else
		snprintf(msg, sizeof(msg), "ПОЛУЧЕН ОТВЕТ ДЛЯ ДРУГОГО "
			"ТЕРМИНАЛА (**:**)");
	log_fill_scr_str(ctx, "%s", msg);
}

/* Занесение в экранный буфер записи типа XLRT_SPECIAL */
static void xlog_fill_scr_special(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "ОШИБКА КОНТРОЛЬНОЙ СУММ\x9b С:%.2s", xlog_data);
}

/* Занесение в экранный буфер записи типа XLRT_BANK */
static void xlog_fill_scr_bank(struct log_gui_context *ctx)
{
	static char *bank_msg = "bank";
	static int bank_msg_len = -1;
	if (bank_msg_len == -1)
		bank_msg_len = strlen(bank_msg);
	ctx->scr_data_len = 0;
	ctx->scr_data[ctx->scr_data_len++] = 0;
	memcpy(ctx->scr_data + ctx->scr_data_len, bank_msg, bank_msg_len + 1);
	ctx->scr_data_len += bank_msg_len + 1;
	ctx->scr_data[ctx->scr_data_len++] = 0;
/* Номер заказа в системе */
	memcpy(ctx->scr_data + ctx->scr_data_len, xlog_data, 7);
	ctx->scr_data_len += 7;
	ctx->scr_data[ctx->scr_data_len++] = 0;
	ctx->scr_data[ctx->scr_data_len++] = 0;
/* Технологический номер кассы */
	memcpy(ctx->scr_data + ctx->scr_data_len, xlog_data + 7, 5);
	ctx->scr_data_len += 5;
	ctx->scr_data[ctx->scr_data_len++] = 0;
	ctx->scr_data[ctx->scr_data_len++] = 0;
/* Сумма заказа */
	memcpy(ctx->scr_data + ctx->scr_data_len, xlog_data + 12, 9);
	ctx->scr_data_len += 9;
	ctx->scr_data[ctx->scr_data_len++] = 0;
}

/* Занесение в экранный буфер записи типа XLRT_IPCHANGE */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
static void xlog_fill_scr_ipchange(struct log_gui_context *ctx)
{
	uint32_t *p = (uint32_t *)xlog_data;
	char old_ip[20], new_ip[20];
	strncpy(old_ip, inet_ntoa(dw2ip(p[0])), sizeof(old_ip) - 1);
	old_ip[sizeof(old_ip) - 1] = 0;
	strncpy(new_ip, inet_ntoa(dw2ip(p[1])), sizeof(new_ip) - 1);
	new_ip[sizeof(new_ip) - 1] = 0;
	log_fill_scr_str(ctx, "ЗАМЕНА ОСНОВНОГО IP ХОСТ-ЭВМ \"ЭКСПРЕСС\" С %s НА %s",
		old_ip, new_ip);
}

/* Занесение в экранный буфер записи типа XLRT_REJECT */
static void xlog_fill_scr_reject(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "ОТКАЗ ОТ ЗАКАЗА");
}

/* Занесение в экранный буфер записи неизвестного типа */
static void xlog_fill_scr_unknown(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "НЕИЗВЕСТН\x9bЙ ТИП ЗАПИСИ: %.4X", xlog_rec_hdr.type);
}

/* Заполнение экранного буфера */
static void xlog_fill_scr_data(struct log_gui_context *ctx)
{
	log_clear_ctx(ctx);
	switch (xlog_rec_hdr.type){
		case XLRT_NOTIFY:
			xlog_fill_scr_notify(ctx);
			break;
		case XLRT_INIT:
			xlog_fill_scr_init(ctx);
			break;
		case XLRT_FOREIGN:
			xlog_fill_scr_foreign(ctx);
			break;
		case XLRT_NORMAL:
		case XLRT_AUX:
		case XLRT_KKT:
			xlog_fill_scr_normal(ctx);
			break;
		case XLRT_SPECIAL:
			xlog_fill_scr_special(ctx);
			break;
		case XLRT_BANK:
			xlog_fill_scr_bank(ctx);
			break;
		case XLRT_IPCHANGE:
			xlog_fill_scr_ipchange(ctx);
			break;
		case XLRT_REJECT:
			xlog_fill_scr_reject(ctx);
			break;
		default:
			xlog_fill_scr_unknown(ctx);
			break;
	}
}

/* Первая строка заголовка записи */
static const char *xlog_get_head_line1(char *buf)
{
	if (hxlog->hdr->n_recs == 0)
		*buf = 0;
	else
		sprintf(buf, "%.2hhx%.2hhx (%u.%u.%u %.4hX %.*s) ОПУ=%.*s ДПУ=%.*s",
			xlog_rec_hdr.addr.gaddr, xlog_rec_hdr.addr.iaddr,
			VERSION_BRANCH(xlog_rec_hdr.term_version),
			VERSION_RELEASE(xlog_rec_hdr.term_version),
			VERSION_PATCH(xlog_rec_hdr.term_version),
			xlog_rec_hdr.term_check_sum,
			isizeof(xlog_rec_hdr.tn), xlog_rec_hdr.tn,
			isizeof(xlog_rec_hdr.xprn_number), xlog_rec_hdr.xprn_number,
			isizeof(xlog_rec_hdr.aprn_number), xlog_rec_hdr.aprn_number);
	return buf;
}

/* Вторая строка заголовка */
static const char *xlog_get_head_line2(char *buf)
{
	if (hxlog->hdr->n_recs == 0)
		sprintf(buf, "                                  На контрольной ленте нет записей");
	else{
		sprintf(buf, "Запись %u/%u от %.2d.%.2d.%d %.2d:%.2d:%.2d "
			"жетон %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX",
			xlog_rec_hdr.number + 1, xlog_rec_hdr.n_para + 1,
			xlog_rec_hdr.dt.day + 1, xlog_rec_hdr.dt.mon + 1,
			xlog_rec_hdr.dt.year + 2000,
			xlog_rec_hdr.dt.hour, xlog_rec_hdr.dt.min, xlog_rec_hdr.dt.sec,
			ds_key_char(xlog_rec_hdr.ds_type),
			xlog_rec_hdr.dsn[7], xlog_rec_hdr.dsn[0],
			xlog_rec_hdr.dsn[6], xlog_rec_hdr.dsn[5],
			xlog_rec_hdr.dsn[4], xlog_rec_hdr.dsn[3],
			xlog_rec_hdr.dsn[2], xlog_rec_hdr.dsn[1]);
		if ((xlog_rec_hdr.type == XLRT_NORMAL) && (xlog_rec_hdr.flags & XLOG_REC_PRINTED))
			strcat(buf, " [+]");
		if (xlog_rec_hdr.type == XLRT_KKT){
			if (xlog_rec_hdr.flags & XLOG_REC_APC)
				strcat(buf, " [АПЧ]");
			if (xlog_rec_hdr.flags & XLOG_REC_CPC)
				strcat(buf, " [ЧвКФП]");
		}
	}
	return buf;
}

/* Третья строка заголовка */
static const char *xlog_get_head_line3(char *buf)
{
	if (hxlog->hdr->n_recs == 0)
		*buf = 0;
	else
		sprintf(buf, "ZNтз: %.3hX  ZNпо: %.3hX  Zбп: %.2hX      "
			"ONтз: %.3hX  ONпо: %.3hX  Oбп: %.2hX    "
			"Время реакции %.3d сек.",
			xlog_rec_hdr.ZNtz, xlog_rec_hdr.ZNpo, (uint16_t)xlog_rec_hdr.ZBp,
			xlog_rec_hdr.ONtz, xlog_rec_hdr.ONpo, (uint16_t)xlog_rec_hdr.OBp,
			xlog_rec_hdr.reaction_time);
	return buf;
}

/* Получение заданной строки заголовка записи ЦКЛ */
static const char *xlog_get_head_line(uint32_t n)
{
	static char buf[128];
	const char *(*fn[])(char *) = {
		xlog_get_head_line1,
		xlog_get_head_line2,
		xlog_get_head_line3,
	};
	return (n < ASIZE(fn)) ? fn[n](buf) : NULL;
}

/* Получение строки подсказки */
static const char *xlog_get_hint_line(uint32_t n)
{
	static char *hints[] = {
		"\x1b/\x1a - пред./след. запись  Ctrl+\x1b/\x1a - быстрое перемещение",
		"Ctrl+Home/End - начало/конец записи  PgUp/PgDn - пред./след. стр.",
		"Home/End - первая/последняя запись  F2 - меню  Esc - выход",
	};
	return (n < ASIZE(hints)) ? hints[n] : NULL;
}

/* Печать текущей записи */
bool xlog_print_current(struct log_gui_context *ctx)
{
	bool flag = false;
	if (!ctx->active)
		return false;
	ctx->modal = true;
	if (!xlog_print_header())
		goto lpc_exit;
	if (xprn_print((char *)log_prn_buf, log_prn_data_len)){
		if (!xlog_print_rec())
			goto lpc_exit;
		if (xprn_print((char *)log_prn_buf, log_prn_data_len)){
			if (!xlog_print_footer())
				goto lpc_exit;
			flag = xprn_print((char *)log_prn_buf, log_prn_data_len);
		}
	}
lpc_exit:
	ctx->modal = false;
	return flag;
}

/* Печать текущей записи на ДПУ */
bool xlog_print_aux(struct log_gui_context *ctx)
{
	bool flag;
	if (!ctx->active)
		return false;
	ctx->modal = true;
	flag = aprn_print((char *)xlog_data, xlog_data_len);
	ctx->modal = false;
	return flag;
}

/* Печать диапазона записей */
bool xlog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to)
{
	bool flag = false;
	uint32_t n, index1 = xlog_first_for_number(hxlog, from - 1);
	uint32_t index2 = xlog_last_for_number(hxlog, to - 1);
/*	bool all = log_get_interval_len(hxlog, index1, index2) == hxlog->hdr->n_recs;*/
	if ((index1 == -1) || (index2 == -1)){
		set_term_astate(ast_no_log_item);
		return false;
	}
	ctx->modal = true;
	for (n = 0; !flag; n++, index1++){
		flag = index1 == index2;	/* последняя запись диапазона */
		if (!xlog_read_rec(hxlog, index1))
			break;
		ctx->cur_rec_index = index1;
		xlog_fill_scr_data(ctx);
		log_draw(ctx);
/* Печать заголовка */
		if ((n == 0) && (!xlog_print_header() ||
				!xprn_print((char *)log_prn_buf, log_prn_data_len)))
			break;
/* Печать записей */
		if (xlog_rec_hdr.type == XLRT_AUX){
			if (!aprn_print((char *)xlog_data, xlog_data_len))
				break;
		}else if (!xlog_print_rec() || !xprn_print((char *)log_prn_buf,
				log_prn_data_len))
			break;
/* Печать концевика */
		if (flag && (!xlog_print_footer() ||
				!xprn_print((char *)log_prn_buf, log_prn_data_len)))
			break;
	}
	ctx->modal = false;
	return flag;
}

/* Полная печать контрольной ленты */
bool xlog_print_all(struct log_gui_context *ctx)
{
	uint32_t n0, n1;
	if ((hxlog->hdr->n_recs == 0) || !log_get_boundary_numbers(hxlog, &n0, &n1))
		return false;
	return xlog_print_range(ctx, n0 + 1, n1 + 1);
}

/* Обработчик событий клавиатуры */
static int xlog_handle_kbd(struct log_gui_context *ctx __attribute__((unused)),
	const struct kbd_event *e)
{
	if (e->pressed && (e->shift_state == 0) && (e->key == KEY_F2))
		return cmd_xlog_menu;
	else
		return cmd_none;
}

/* Поздняя инициализация структуры */
static void xlog_init_gui_ctx(struct log_gui_context *ctx)
{
	ctx->hlog = hxlog;
}

static uint8_t xlog_scr_data[LOG_BUF_LEN];

static struct log_gui_context _xlog_gui_ctx = {
	.hlog		= NULL,
	.active		= &xlog_active,
	.title		= MAIN_TITLE " (КЛ - основной режим)",
	.cur_rec_index	= 0,
	.nr_head_lines	= 3,
	.nr_hint_lines	= 3,
	.scr_data	= xlog_scr_data,
	.scr_data_size	= sizeof(xlog_scr_data),
	.scr_data_len	= 0,
	.scr_data_lines	= 0,
	.first_line	= 0,
	.gc		= NULL,
	.mem_gc		= NULL,
	.modal		= false,
	.asis		= false,
	.init		= xlog_init_gui_ctx,
	.filter		= NULL,
	.read_rec	= xlog_read_rec,
	.get_head_line	= xlog_get_head_line,
	.get_hint_line	= xlog_get_hint_line,
	.fill_scr_data	= xlog_fill_scr_data,
	.handle_kbd	= xlog_handle_kbd
};

struct log_gui_context *xlog_gui_ctx = &_xlog_gui_ctx;
