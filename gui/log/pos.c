/* Вывод на экран информации БКЛ. (c) gsr, Alex 2001-2004, 2009 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "gui/log/pos.h"
#include "gui/scr.h"
#include "log/pos.h"
#include "prn/express.h"
#include "cfg.h"
#include "express.h"
#include "genfunc.h"
#include "kbd.h"
#include "sterm.h"

/* Запись буфера контрольной ленты в экранный буфер: plog_data => ctx->scr_data */
static void plog_fill_scr_normal(struct log_gui_context *ctx)
{
	int i, j, k = 0, col = 0, n = 0, m = 1;
	bool dle = false, nl = false, line_begin = true;
	uint8_t b = 0;
	ctx->scr_data[0] = 0;
	for (i = 0, j = 1; (i < plog_data_len) && (j < ctx->scr_data_size); i++){
		b = plog_data[i];
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
			n = log_get_cmd_len(plog_data, plog_data_len, i);
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
				}else if (b < 0x20)
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

/* Занесение в экранный буфер записи неизвестного типа */
static void plog_fill_scr_unknown(struct log_gui_context *ctx,
	uint32_t id __attribute__((unused)))
{
	log_fill_scr_str(ctx, "НЕИЗВЕСТН\x9bЙ ТИП ЗАПИСИ: %.4X", plog_rec_hdr.type);
}

/* Заполнение экранного буфера */
static void plog_fill_scr_data(struct log_gui_context *ctx)
{
	log_clear_ctx(ctx);
	switch (plog_rec_hdr.type){
		case PLRT_NORMAL:
		case PLRT_ERROR:
			plog_fill_scr_normal(ctx);
			break;
		default:
			plog_fill_scr_unknown(ctx, plog_rec_hdr.type);
			break;
	}
}

/* Первая строка заголовка записи */
static const char *plog_get_head_line1(char *buf)
{
	if (hplog->hdr->n_recs == 0)
		*buf = 0;
	else
		sprintf(buf, "%.2hhx%.2hhx (%u.%u.%u %.4hX %.*s) ОПУ=%.*s БПУ=%.*s",
			plog_rec_hdr.addr.gaddr, plog_rec_hdr.addr.iaddr,
			VERSION_MAJOR(plog_rec_hdr.term_version),
			VERSION_MINOR(plog_rec_hdr.term_version),
			VERSION_RELEASE(plog_rec_hdr.term_version),
			plog_rec_hdr.term_check_sum,
			isizeof(plog_rec_hdr.tn), plog_rec_hdr.tn,
			isizeof(plog_rec_hdr.xprn_number), plog_rec_hdr.xprn_number,
			isizeof(plog_rec_hdr.lprn_number), plog_rec_hdr.lprn_number);
	return buf;
}

/* Вторая строка заголовка */
static const char *plog_get_head_line2(char *buf)
{
	if (hplog->hdr->n_recs == 0)
		sprintf(buf, "                                  На контрольной ленте нет записей");
	else{
		sprintf(buf, "Запись %u от %.2d.%.2d.%d %.2d:%.2d:%.2d "
			"жетон %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX",
			plog_rec_hdr.number + 1,
			plog_rec_hdr.dt.day + 1, plog_rec_hdr.dt.mon + 1,
			plog_rec_hdr.dt.year + 2000,
			plog_rec_hdr.dt.hour, plog_rec_hdr.dt.min, plog_rec_hdr.dt.sec,
			ds_key_char(plog_rec_hdr.ds_type),
			plog_rec_hdr.dsn[7], plog_rec_hdr.dsn[0],
			plog_rec_hdr.dsn[6], plog_rec_hdr.dsn[5],
			plog_rec_hdr.dsn[4], plog_rec_hdr.dsn[3],
			plog_rec_hdr.dsn[2], plog_rec_hdr.dsn[1]);
	}
	return buf;
}

/* Получение заданной строки заголовка записи БКЛ */
static const char *plog_get_head_line(uint32_t n)
{
	static char buf[128];
	const char *(*fn[])(char *) = {
		plog_get_head_line1,
		plog_get_head_line2,
	};
	return (n < ASIZE(fn)) ? fn[n](buf) : NULL;
}

/* Получение строки подсказки */
static const char *plog_get_hint_line(uint32_t n)
{
	static char *hints[] = {
		"\x1b/\x1a - пред./след. запись  Ctrl+\x1b/\x1a - быстрое перемещение",
		"Ctrl+Home/End - начало/конец записи  PgUp/PgDn - пред./след. стр.",
		"Home/End - первая/последняя запись  F2 - меню  Esc - выход",
	};
	return (n < ASIZE(hints)) ? hints[n] : NULL;
}

/* Печать текущей записи */
bool plog_print_current(struct log_gui_context *ctx)
{
	bool flag = false;
	if (!ctx->active)
		return false;
	ctx->modal = true;
	if (!plog_print_header())
		goto exit;
	if (xprn_print((char *)log_prn_buf, log_prn_data_len)){
		if (!plog_print_rec())
			goto exit;
		if (xprn_print((char *)log_prn_buf, log_prn_data_len)){
			if (!plog_print_footer())
				goto exit;
			flag = xprn_print((char *)log_prn_buf, log_prn_data_len);
		}
	}
exit:
	ctx->modal = false;
	return flag;
}

/* Печать диапазона записей */
bool plog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to)
{
	bool flag = false;
	uint32_t n, index1 = plog_index_for_number(hplog, from - 1);
	uint32_t index2 = plog_index_for_number(hplog, to - 1);
	if ((index1 == -1) || (index2 == -1)){
		set_term_astate(ast_no_log_item);
		return false;
	}
	ctx->modal = true;
	for (n = 0; !flag; n++, index1++){
		flag = index1 == index2;	/* последняя запись диапазона */
		if (!plog_read_rec(hplog, index1))
			break;
		ctx->cur_rec_index = index1;
		plog_fill_scr_data(ctx);
		log_draw(ctx);
/* Печать заголовка */
		if ((n == 0) && (!plog_print_header() ||
				!xprn_print((char *)log_prn_buf, log_prn_data_len)))
			break;
/* Печать записей */
		if (!plog_print_rec() || !xprn_print((char *)log_prn_buf,
				log_prn_data_len))
			break;
/* Печать концевика */
		if (flag && (!plog_print_footer() ||
				!xprn_print((char *)log_prn_buf, log_prn_data_len)))
			break;
	}
	ctx->modal = false;
	return flag;
}

/* Полная печать контрольной ленты */
bool plog_print_all(struct log_gui_context *ctx)
{
	uint32_t n0, n1;
	if ((hplog->hdr->n_recs == 0) || !log_get_boundary_numbers(hplog, &n0, &n1))
		return false;
	return plog_print_range(ctx, n0 + 1, n1 + 1);
}

/* Обработчик событий клавиатуры */
static int plog_handle_kbd(struct log_gui_context *ctx __attribute__((unused)),
	const struct kbd_event *e)
{
	if (e->pressed && (e->shift_state == 0) && (e->key == KEY_F2))
		return cmd_plog_menu;
	else
		return cmd_none;
}

/* Поздняя инициализация структуры */
static void plog_init_gui_ctx(struct log_gui_context *ctx)
{
	ctx->hlog = hplog;
}

static uint8_t plog_scr_data[LOG_BUF_LEN];

static struct log_gui_context _plog_gui_ctx = {
	.hlog		= NULL,
	.active		= &plog_active,
	.title		= MAIN_TITLE " (КЛ2 - работа с ИПТ)",
	.cur_rec_index	= 0,
	.nr_head_lines	= 2,
	.nr_hint_lines	= 3,
	.scr_data	= plog_scr_data,
	.scr_data_size	= sizeof(plog_scr_data),
	.scr_data_len	= 0,
	.scr_data_lines	= 0,
	.first_line	= 0,
	.gc		= NULL,
	.mem_gc		= NULL,
	.modal		= false,
	.asis		= false,
	.init		= plog_init_gui_ctx,
	.filter		= NULL,
	.read_rec	= plog_read_rec,
	.get_head_line	= plog_get_head_line,
	.get_hint_line	= plog_get_hint_line,
	.fill_scr_data	= plog_fill_scr_data,
	.handle_kbd	= plog_handle_kbd
};

struct log_gui_context *plog_gui_ctx = &_plog_gui_ctx;
