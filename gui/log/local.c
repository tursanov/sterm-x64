/*
 * Вывод на экран информации контрольной ленты ППУ (ПКЛ).
 * (c) gsr, Alex 2001-2004, 2009
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "gui/log/express.h"
#include "gui/log/local.h"
#include "gui/scr.h"
#include "log/local.h"
#include "prn/aux.h"
#include "prn/express.h"
#include "prn/local.h"
#include "cfg.h"
#include "express.h"
#include "genfunc.h"
#include "kbd.h"
#include "sterm.h"

/* Запись в экранный буфер записи ЦКЛ типа LLRT_NORMAL/LLRT_AUX */
static void llog_fill_scr_normal(struct log_gui_context *ctx)
{
	int i, j = 1, k = 0, col = 0, n = 0, m = 1;
	bool dle = false, nl = false, line_begin = true;
	uint8_t b = 0;
	char prefix[80] = "";
	ctx->scr_data_len = ctx->scr_data_lines = 0;
	if (llog_rec_hdr.type == LLRT_EXPRESS)
		snprintf(prefix, sizeof(prefix), "ОСНОВНОЙ ПРИНТЕР");
	else if (llog_rec_hdr.type == LLRT_AUX)
		snprintf(prefix, sizeof(prefix), "ДОПОЛНИТЕЛЬН\x9bЙ ПРИНТЕР");
	if (prefix[0] == 0)
		ctx->scr_data[0] = 0;
	else{
		log_add_scr_str(ctx, true, "%s", prefix);
		if (ctx->scr_data_len < ctx->scr_data_size)
			ctx->scr_data[ctx->scr_data_len++] = 0;
		j = ctx->scr_data_len;
	}
	for (i = 0; (i < llog_data_len) && (j < ctx->scr_data_size); i++){
		b = llog_data[i];
		if (dle){
			nl = (b == LPRN_WR_BCODE1) || (b == LPRN_WR_BCODE2) ||
				(b == LPRN_NO_BCODE) || (b == LPRN_RD_BCODE);
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
			n = log_get_cmd_len(llog_data, llog_data_len, i);
		}else{
			dle = (b == LPRN_DLE);
			if (!dle)
				line_begin = false;
		}
		switch (b){
			case LPRN_LF:
				ctx->scr_data_lines++;
				nl = false;
				j = m;
				ctx->scr_data[j++] = 0;
				k = j++;
				if (k < ctx->scr_data_size)
					ctx->scr_data[k] = col;
				break;
			case LPRN_CR:
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
				}else if ((llog_rec_hdr.type == LLRT_AUX) &&
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

/* Занесение в экранный буфер записи типа LLRT_NOTIFY */
static void llog_fill_scr_notify(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "ОТВЕТ БЕЗ ПЕЧАТИ");
}

/* Занесение в экранный буфер записи типа LLRT_INIT */
static void llog_fill_scr_init(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "ИНИЦИАЛИЗАЦИЯ");
}

/* Занесение в экранный буфер записи типа LLRT_BANK */
static void llog_fill_scr_bank(struct log_gui_context *ctx)
{
	static const char *bank_msg = "bank";
	static int bank_msg_len = -1;
	if (bank_msg_len == -1)
		bank_msg_len = strlen(bank_msg);
	ctx->scr_data_len = 0;
	ctx->scr_data[ctx->scr_data_len++] = 0;
	memcpy(ctx->scr_data + ctx->scr_data_len, bank_msg, bank_msg_len + 1);
	ctx->scr_data_len += bank_msg_len + 1;
	ctx->scr_data[ctx->scr_data_len++] = 0;
/* Номер заказа в системе */
	memcpy(ctx->scr_data + ctx->scr_data_len, llog_data, 7);
	ctx->scr_data_len += 7;
	ctx->scr_data[ctx->scr_data_len++] = 0;
	ctx->scr_data[ctx->scr_data_len++] = 0;
/* Технологический номер кассы */
	memcpy(ctx->scr_data + ctx->scr_data_len, llog_data + 7, 5);
	ctx->scr_data_len += 5;
	ctx->scr_data[ctx->scr_data_len++] = 0;
	ctx->scr_data[ctx->scr_data_len++] = 0;
/* Сумма заказа */
	memcpy(ctx->scr_data + ctx->scr_data_len, llog_data + 12, 9);
	ctx->scr_data_len += 9;
	ctx->scr_data[ctx->scr_data_len++] = 0;
}

/* Занесение в экранный буфер записи типа LLRT_ERROR */
static void llog_fill_scr_error(struct log_gui_context *ctx)
{
	if (llog_data_len == 1){
		char *s = (llog_data[0] == LLRT_ERROR_MEDIA) ? "БСО" : "ППУ";
		log_add_scr_str(ctx, true, "ОШИБКА РАБОТ\x9b С %s: %s",
			s, llog_get_llrt_error_txt(llog_data[0]));
	}
}

/*
 * Занесение в экранный буфер кода ошибки ППУ. Возвращает false в случае
 * переполнения экранного буфера.
 */
static bool llog_fill_scr_err_code(struct log_gui_context *ctx,
		const char *prefix, uint8_t code,
		struct lprn_error_txt *(*getter)(uint8_t))
{
	bool ret = true;
	struct lprn_error_txt *err = getter(code);
	if (prefix != NULL)
		ret = log_add_scr_str(ctx, true, "%s" , prefix);
	if (ret && (err != NULL))
		ret = log_add_scr_str(ctx, true, "%.2hhX: %s", llog_data[0], err->txt);
	return ret;
}

/* Занесение в экранный буфер записи типа LLRT_RD_ERROR */
static void llog_fill_scr_rd_error(struct log_gui_context *ctx)
{
	if (llog_data_len == 1){
		log_add_scr_str(ctx, true, "ЗАПРОС");
		llog_fill_scr_err_code(ctx, "ОШИБКА ЧТЕНИЯ НОМЕРА БЛАНКА",
			llog_data[0], lprn_get_error_txt);
	}
}

/* Занесение в экранный буфер записи типа LLRT_PR_ERROR_BCODE */
static void llog_fill_scr_pr_error_bcode(struct log_gui_context *ctx)
{
	if (llog_data_len == (LPRN_BLANK_NUMBER_LEN + 1)){
		llog_fill_scr_err_code(ctx, "ОШИБКА ПЕЧАТИ НА БСО С КОНТРОЛЕМ "
			"ШТРИХ-КОДА", llog_data[0], lprn_get_error_txt);
		log_add_scr_str(ctx, true, "НОМЕР БЛАНКА %.*s", LPRN_BLANK_NUMBER_LEN,
			llog_data + 1);
	}
}

/* Занесение в экранный буфер записи типа LLRT_PR_ERROR */
static void llog_fill_scr_pr_error(struct log_gui_context *ctx)
{
	if (llog_data_len == 1){
		llog_fill_scr_err_code(ctx, "ОШИБКА ПЕЧАТИ НА БСО БЕЗ КОНТРОЛЯ "
			"ШТРИХ-КОДА", llog_data[0], lprn_get_error_txt);
	}
}

/* Занесение в экранный буфер записи типа LLRT_FOREIGN */
static void llog_fill_scr_foreign(struct log_gui_context *ctx)
{
	char msg[80];
	struct term_addr *addr = (struct term_addr *)llog_data;
	if (llog_data_len == sizeof(*addr))
		snprintf(msg, sizeof(msg), "ПОЛУЧЕН ОТВЕТ ДЛЯ ДРУГОГО "
			"ТЕРМИНАЛА (%.2hhX:%.2hhX)",
			addr->gaddr, addr->iaddr);
	else
		snprintf(msg, sizeof(msg), "ПОЛУЧЕН ОТВЕТ ДЛЯ ДРУГОГО "
			"ТЕРМИНАЛА (**:**)");
	log_fill_scr_str(ctx, "%s", msg);
}

/* Занесение в экранный буфер записи типа LLRT_SPECIAL */
static void llog_fill_scr_special(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "ОШИБКА КОНТРОЛЬНОЙ СУММ\x9b С:%.2s", llog_data);
}

/* Занесение в экранный буфер записи типа LLRT_ERASE_SD */
static void llog_fill_scr_erase_sd(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "КАРТА ПАМЯТИ ОЧИЩЕНА");
}

/* Занесение в экранный буфер записи типа LLRT_SD_ERROR */
static void llog_fill_scr_sd_error(struct log_gui_context *ctx)
{
	if (llog_data_len == 1){
		llog_fill_scr_err_code(ctx, "ОШИБКА РАБОТ\x9b С КАРТОЙ ПАМЯТИ",
			llog_data[0], lprn_get_sd_error_txt);
	}
}

/* Занесение в экранный буфер записи типа LLRT_REJECT */
static void llog_fill_scr_reject(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "ОТКАЗ ОТ ЗАКАЗА");
}

/* Занесение в экранный буфер записи типа LLRT_SYNTAX_ERROR */
static void llog_fill_scr_syntax_error(struct log_gui_context *ctx)
{
	const char *err_msg, *slice;
	uint8_t code = llog_data[0];
	int i;
	log_add_scr_str(ctx, true, "СИНТАКСИЧЕСКАЯ ОШИБКА В ОТВЕТЕ -- П:%.2hhd", code);
	err_msg = get_syntax_error_txt(code);
	if (err_msg != NULL){
		for (slice = err_msg; *slice != 0;){
			log_add_scr_str(ctx, true, "%s", slice);
			slice += strlen(slice) + 1;
		}
	}
	log_add_scr_str(ctx, true, "***ЗАПРОС***");
	for (i = 1; i < llog_data_len; i += LOG_SCREEN_COLS){
		int len = llog_data_len - i;
		if (len > LOG_SCREEN_COLS)
			len = LOG_SCREEN_COLS;
		log_add_scr_str(ctx, false, "%.*s", len, llog_data + i);
	}
}

/* Занесение в экранный буфер записи неизвестного типа */
static void llog_fill_scr_unknown(struct log_gui_context *ctx)
{
	log_fill_scr_str(ctx, "НЕИЗВЕСТН\x9bЙ ТИП ЗАПИСИ: %.4X", llog_rec_hdr.type);
}

/* Заполнение экранного буфера */
static void llog_fill_scr_data(struct log_gui_context *ctx)
{
	log_clear_ctx(ctx);
	switch (llog_rec_hdr.type){
		case LLRT_NORMAL:
		case LLRT_EXPRESS:
		case LLRT_AUX:
			llog_fill_scr_normal(ctx);
			break;
		case LLRT_NOTIFY:
			llog_fill_scr_notify(ctx);
			break;
		case LLRT_INIT:
			llog_fill_scr_init(ctx);
			break;
		case LLRT_BANK:
			llog_fill_scr_bank(ctx);
			break;
		case LLRT_ERROR:
			llog_fill_scr_error(ctx);
			break;
		case LLRT_RD_ERROR:
			llog_fill_scr_rd_error(ctx);
			break;
		case LLRT_PR_ERROR_BCODE:
			llog_fill_scr_pr_error_bcode(ctx);
			break;
		case LLRT_PR_ERROR:
			llog_fill_scr_pr_error(ctx);
			break;
		case LLRT_FOREIGN:
			llog_fill_scr_foreign(ctx);
			break;
		case LLRT_SPECIAL:
			llog_fill_scr_special(ctx);
			break;
		case LLRT_ERASE_SD:
			llog_fill_scr_erase_sd(ctx);
			break;
		case LLRT_SD_ERROR:
			llog_fill_scr_sd_error(ctx);
			break;
		case LLRT_REJECT:
			llog_fill_scr_reject(ctx);
			break;
		case LLRT_SYNTAX_ERROR:
			llog_fill_scr_syntax_error(ctx);
			break;
		default:
			llog_fill_scr_unknown(ctx);
			break;
	}
}

/* Первая строка заголовка записи */
static const char *llog_get_head_line1(char *buf)
{
	if (hllog->hdr->n_recs == 0)
		*buf = 0;
	else
		sprintf(buf, "%.2hhx%.2hhx (%u.%u.%u %.4hX %.*s) "
			"ОПУ=%.*s ДПУ=%.*s ППУ=%.*s",
			llog_rec_hdr.addr.gaddr, llog_rec_hdr.addr.iaddr,
			VERSION_MAJOR(llog_rec_hdr.term_version),
			VERSION_MINOR(llog_rec_hdr.term_version),
			VERSION_RELEASE(llog_rec_hdr.term_version),
			llog_rec_hdr.term_check_sum,
			isizeof(llog_rec_hdr.tn), llog_rec_hdr.tn,
			isizeof(llog_rec_hdr.xprn_number), llog_rec_hdr.xprn_number,
			isizeof(llog_rec_hdr.aprn_number), llog_rec_hdr.aprn_number,
			isizeof(llog_rec_hdr.lprn_number), llog_rec_hdr.lprn_number);
	return buf;
}

/* Вторая строка заголовка */
static const char *llog_get_head_line2(char *buf)
{
	if (hllog->hdr->n_recs == 0)
		sprintf(buf, "                                  На контрольной ленте нет записей");
	else{
		sprintf(buf, "Запись %u/%u от %.2d.%.2d.%d %.2d:%.2d:%.2d "
			"жетон %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX",
			llog_rec_hdr.number + 1, llog_rec_hdr.n_para + 1,
			llog_rec_hdr.dt.day + 1, llog_rec_hdr.dt.mon + 1,
			llog_rec_hdr.dt.year + 2000,
			llog_rec_hdr.dt.hour, llog_rec_hdr.dt.min, llog_rec_hdr.dt.sec,
			ds_key_char(llog_rec_hdr.ds_type),
			llog_rec_hdr.dsn[7], llog_rec_hdr.dsn[0],
			llog_rec_hdr.dsn[6], llog_rec_hdr.dsn[5],
			llog_rec_hdr.dsn[4], llog_rec_hdr.dsn[3],
			llog_rec_hdr.dsn[2], llog_rec_hdr.dsn[1]);
		if (llog_rec_hdr.printed)
			strcat(buf, " [+]");
	}
	return buf;
}

/* Третья строка заголовка */
static const char *llog_get_head_line3(char *buf)
{
	if (hllog->hdr->n_recs == 0)
		*buf = 0;
	else
		sprintf(buf, "ZNтз: %.3hX  ZNпо: %.3hX  Zбп: %.2hX      "
			"ONтз: %.3hX  ONпо: %.3hX  Oбп: %.2hX    "
			"Время реакции %.3d сек.",
			llog_rec_hdr.ZNtz, llog_rec_hdr.ZNpo, (uint16_t)llog_rec_hdr.ZBp,
			llog_rec_hdr.ONtz, llog_rec_hdr.ONpo, (uint16_t)llog_rec_hdr.OBp,
			llog_rec_hdr.reaction_time);
	return buf;
}

/* Получение заданной строки заголовка записи ПКЛ */
static const char *llog_get_head_line(uint32_t n)
{
	static char buf[128];
	const char *(*fn[])(char *) = {
		llog_get_head_line1,
		llog_get_head_line2,
		llog_get_head_line3,
	};
	return (n < ASIZE(fn)) ? fn[n](buf) : NULL;
}

/* Получение строки подсказки */
static const char *llog_get_hint_line(uint32_t n)
{
	static char *hints[] = {
		"\x1b/\x1a - пред./след. запись  Ctrl+\x1b/\x1a - быстрое перемещение",
		"Ctrl+Home/End - начало/конец записи  PgUp/PgDn - пред./след. стр.",
		"Home/End - первая/последняя запись  F2 - меню  Esc - выход",
	};
	return (n < ASIZE(hints)) ? hints[n] : NULL;
}

/* Подготовка к печати записи контрольной ленты */
static bool llog_prepare_print(void)
{
	bool ret = false;
	if (lprn_get_status() != LPRN_RET_OK){
		fprintf(stderr, "%s: lprn_get_status failed.\n", __func__);
	}else if (lprn_status != 0){
		fprintf(stderr, "%s: lprn_status after lprn_get_status = 0x%.2hhx.\n",
			__func__, lprn_status);
		set_term_astate(ast_lprn_err);
	}else if (lprn_get_media_type() != LPRN_RET_OK){
		fprintf(stderr, "%s: lprn_get_media failed.\n", __func__);
	}else if (lprn_status != 0){
		fprintf(stderr, "%s: lprn_status after lprn_get_media_type = 0x%.2hhx.\n",
			__func__, lprn_status);
		set_term_astate(ast_lprn_err);
	}else if ((lprn_media != LPRN_MEDIA_PAPER) &&
			(lprn_media != LPRN_MEDIA_BOTH)){
		fprintf(stderr, "%s: lprn_media = 0x%.2hhx.\n", __func__, lprn_media);
		set_term_astate(ast_lprn_ch_media);
	}else
		ret = true;
	if (!ret)
		lprn_close();
	return ret;
}

/* Печать текущей записи на ППУ */
bool llog_print_current(struct log_gui_context *ctx)
{
	bool ret = false;
	if (!ctx->active)
		;
	else if (!llog_prepare_print())
		err_beep();
	else{
		int lprn_ret = LPRN_RET_OK;
		set_term_astate(ast_none);
		ctx->modal = true;
		ret =	llog_begin_print() && llog_print_header(false, 0) &&
			llog_print_rec_header() &&
			((lprn_ret = lprn_print_log(log_prn_buf, log_prn_data_len)) ==
				LPRN_RET_OK) &&
			(lprn_status == 0) &&
			llog_print_rec(true, false) &&
			((lprn_ret = lprn_print_log(log_prn_buf, log_prn_data_len)) ==
				LPRN_RET_OK) &&
			(lprn_status == 0) &&
			log_reset_prn_buf() &&
			llog_print_footer() &&
			llog_end_print() &&
			((lprn_ret = lprn_print_log(log_prn_buf, log_prn_data_len)) ==
				LPRN_RET_OK) &&
			(lprn_status == 0);
		if (!ret && (lprn_ret != LPRN_RET_RST) && (lprn_status != 0))
			set_term_astate(ast_lprn_err);
		ctx->modal = false;
	}
	lprn_close();
	return ret;
}

/* Печать текущей записи на ОПУ */
bool llog_print_xprn(struct log_gui_context *ctx)
{
	bool flag = false;
	if (!ctx->active)
		return false;
	set_term_astate(ast_none);
	ctx->modal = true;
	if (!llog_print_header_xprn())
		goto lpc_exit;
	if (xprn_print((char *)log_prn_buf, log_prn_data_len)){
		if (!llog_print_rec(true, true))
			goto lpc_exit;
		if (xprn_print((char *)log_prn_buf, log_prn_data_len)){
			if (!llog_print_footer_xprn())
				goto lpc_exit;
			flag = xprn_print((char *)log_prn_buf, log_prn_data_len);
		}
	}
lpc_exit:
	ctx->modal = false;
	return flag;
}

/* Печать текущей записи на ДПУ */
bool llog_print_aux(struct log_gui_context *ctx)
{
	set_term_astate(ast_none);
	return xlog_print_aux(ctx);
}

/* Подсчёт количества записей типа LLRT_RD_ERROR */
static uint32_t llog_count_rd_errors(uint32_t index1, uint32_t index2)
{
	uint32_t i, n = 0;
	for (i = index1; i <= index2; i++){
		if (!llog_read_rec(hllog, i))
			break;
		else if (llog_rec_hdr.type == LLRT_RD_ERROR)
			n++;
	}
	return n;
}

/* Печать диапазона записей на ППУ */
static bool llog_print_range_local(struct log_gui_context *ctx,
		uint32_t from, uint32_t to)
{
	bool ret = false;
	uint32_t n, m = 0;
	uint32_t index1 = llog_first_for_number(hllog, from - 1);
	uint32_t index2 = llog_last_for_number(hllog, to - 1);
	uint32_t k = llog_count_rd_errors(index1, index2);
	for (n = index1; n <= index2; n++){
		if (!llog_read_rec(hllog, n))
			break;
		else if ((llog_rec_hdr.type == LLRT_EXPRESS) ||
				(llog_rec_hdr.type == LLRT_AUX))
			continue;
/* Вывод записи на экран */
		ctx->cur_rec_index = n;
		llog_fill_scr_data(ctx);
		log_draw(ctx);
/* Подготовка к печати */
		if (m == 0){
			if (!llog_prepare_print()){
				err_beep();
				break;
			}
/* Печать заголовка контрольной ленты */
			if (!llog_begin_print() || !llog_print_header(true, k) ||
					!llog_print_rec_footer() ||
					(lprn_print_log(log_prn_buf,
						log_prn_data_len) != LPRN_RET_OK))
				break;
			else if	(lprn_status == 0)
				log_reset_prn_buf();
			else{
				set_term_astate(ast_lprn_err);
				break;
			}
		}else
			log_reset_prn_buf();
/* Печать текущей записи */
		if (!llog_print_rec_header_short() ||
				(lprn_print_log(log_prn_buf, log_prn_data_len) !=
					LPRN_RET_OK))
			break;
		else if (lprn_status == 0){
			if (!llog_print_rec(true, false) ||
				(lprn_print_log(log_prn_buf,
					log_prn_data_len) != LPRN_RET_OK))
			break;
		}
		if (lprn_status != 0){
			set_term_astate(ast_lprn_err);
			break;
		}
		m++;
	}
/* Печать концевика */
	if ((m > 0) && (n > index2)){
		log_reset_prn_buf();
		if (llog_print_footer() && llog_end_print() &&
				(lprn_print_log(log_prn_buf,
					log_prn_data_len) == LPRN_RET_OK)){
			ret = lprn_status == 0;
			if (!ret)
				set_term_astate(ast_lprn_err);
		}
	}
	lprn_close();
	return ret;
}

/* Печать диапазона записей на ОПУ */
static bool llog_print_range_xprn(struct log_gui_context *ctx,
		uint32_t from, uint32_t to)
{
	bool ret;
	uint32_t n, m = 0, index1 = llog_first_for_number(hllog, from - 1);
	uint32_t index2 = llog_last_for_number(hllog, to - 1);
	for (n = index1; n <= index2; n++){
		if (!llog_read_rec(hllog, n))
			break;
		else if (llog_rec_hdr.type != LLRT_EXPRESS)
			continue;
/* Вывод записи на экран */
		ctx->cur_rec_index = n;
		llog_fill_scr_data(ctx);
		log_draw(ctx);
/* Печать заголовка */
		if ((m == 0) && (!llog_print_header_xprn() ||
				!xprn_print((char *)log_prn_buf, log_prn_data_len)))
			break;
/* Печать записи */
		if (!llog_print_rec(true, true) ||
				!xprn_print((char *)log_prn_buf, log_prn_data_len))
			break;
		m++;
	}
	ret = n > index2;
/* Печать концевика */
	if ((m > 0) && (n > index2)){
		if (m > 0)
			ret = llog_print_footer_xprn() &&
				xprn_print((char *)log_prn_buf, log_prn_data_len);
	}
	return ret;
}

/* Печать диапазона записей на ДПУ */
static bool llog_print_range_aux(struct log_gui_context *ctx,
		uint32_t from, uint32_t to)
{
	uint32_t n, index1 = llog_first_for_number(hllog, from - 1);
	uint32_t index2 = llog_last_for_number(hllog, to - 1);
	for (n = index1; n <= index2; n++){
		if (!llog_read_rec(hllog, n))
			break;
		else if (llog_rec_hdr.type != LLRT_AUX)
			continue;
/* Вывод записи на экран */
		ctx->cur_rec_index = n;
		llog_fill_scr_data(ctx);
		log_draw(ctx);
/* Печать записи */
		if (!aprn_print((char *)llog_data, llog_data_len))
			break;
	}
	return n > index2;
}

/* Печать диапазона записей */
bool llog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to)
{
	bool ret = false;
	uint32_t index1 = llog_first_for_number(hllog, from - 1);
	uint32_t index2 = llog_last_for_number(hllog, to - 1);
	if (!ctx->active)
		return false;
	else if ((index1 == -1) || (index2 == -1)){
		set_term_astate(ast_no_log_item);
		return false;
	}else
		set_term_astate(ast_none);
	ctx->modal = true;
	ret =	llog_print_range_local(ctx, from, to) &&
		llog_print_range_xprn(ctx, from, to) &&
		llog_print_range_aux(ctx, from, to);
	ctx->modal = false;
	return ret;
}

/* Полная печать контрольной ленты */
bool llog_print_all(struct log_gui_context *ctx)
{
	uint32_t n0, n1;
	if ((hllog->hdr->n_recs == 0) || !log_get_boundary_numbers(hllog, &n0, &n1))
		return false;
	return llog_print_range(ctx, n0 + 1, n1 + 1);
}

/* Обработчик событий клавиатуры */
static int llog_handle_kbd(struct log_gui_context *ctx __attribute__((unused)),
	const struct kbd_event *e)
{
	if (e->pressed && (e->shift_state == 0) && (e->key == KEY_F2))
		return cmd_llog_menu;
	else
		return cmd_none;
}

/* Поздняя инициализация структуры */
static void llog_init_gui_ctx(struct log_gui_context *ctx)
{
	ctx->hlog = hllog;
}

static uint8_t llog_scr_data[LOG_BUF_LEN];

static struct log_gui_context _llog_gui_ctx = {
	.hlog		= NULL,
	.active		= &llog_active,
	.title		= MAIN_TITLE " (КЛ - пригородное приложение)",
	.cur_rec_index	= 0,
	.nr_head_lines	= 3,
	.nr_hint_lines	= 3,
	.scr_data	= llog_scr_data,
	.scr_data_size	= sizeof(llog_scr_data),
	.scr_data_len	= 0,
	.scr_data_lines	= 0,
	.first_line	= 0,
	.gc		= NULL,
	.mem_gc		= NULL,
	.modal		= false,
	.asis		= false,
	.init		= llog_init_gui_ctx,
	.filter		= NULL,
	.read_rec	= llog_read_rec,
	.get_head_line	= llog_get_head_line,
	.get_hint_line	= llog_get_hint_line,
	.fill_scr_data	= llog_fill_scr_data,
	.handle_kbd	= llog_handle_kbd
};

struct log_gui_context *llog_gui_ctx = &_llog_gui_ctx;
