/* Вывод на экран информации ККЛ. (c) gsr 2018-2019 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gui/log/kkt.h"
#include "gui/scr.h"
#include "log/kkt.h"
#include "prn/express.h"
#include "cfg.h"
#include "express.h"
#include "genfunc.h"
#include "kbd.h"
#include "sterm.h"

static inline bool is_print(uint8_t b)
{
	return (b >= 0x20) && (b < 0x7f);
}

/* Занесение в экранный буфер части данных записи ККЛ */
static bool klog_show_data(struct log_gui_context *ctx, const uint8_t *data, size_t len,
	const char *title)
{
	if ((title != NULL) && !log_add_scr_str(ctx, !ctx->asis, "%s", title))
		return false;
	else if (len == 0)
		return true;
	bool ret = false;
	uint32_t begin = 0, end = ((len + 15) / 16) * 16;
	static char line[LOG_SCREEN_COLS + 1];
	off_t offs = 0;
	for (int i = 0; i <= end; i++){
		if ((i % 16) == 0){
			if (i > 0){
				sprintf(line + offs, "   ");
				offs += 3;
				for (uint32_t j = 0, k = begin; j < 16; j++, k++, offs++){
					if (k < len){
						uint8_t b = data[k];
						sprintf(line + offs, "%c", is_print(b) ? b : '.');
					}else
						sprintf(line + offs, " ");
				}
				if (!log_add_scr_str(ctx, false, "%s", line))
					break;
				begin += 16;
			}
			if (i == end){
				ret = true;
				break;
			}
			sprintf(line, "%.4X:", begin);
			offs = 5;
		}else if ((i % 8) == 0){
			sprintf(line + offs, " --");
			offs += 3;
		}
		if (i < len)
			sprintf(line + offs, " %.2hhX", data[i]);
		else
			sprintf(line + offs, "   ");
		offs += 3;
	}
	return ret;
}

/* Занесение в экранный буфер записи ККЛ */
static bool klog_fill_scr_normal(struct log_gui_context *ctx)
{
	ctx->asis = KLOG_STREAM(klog_rec_hdr.stream) != KLOG_STREAM_PRN;
	static char title[LOG_SCREEN_COLS + 1];
	if (klog_rec_hdr.cmd != KKT_NUL){
		snprintf(title, sizeof(title), "ЗАПРОС (%.2hhX):", klog_rec_hdr.cmd);
		if (!klog_show_data(ctx, klog_data, klog_rec_hdr.req_len, title))
			return false;
	}
	snprintf(title, sizeof(title), "ОТВЕТ (СТАТУС = %.2hhX):", klog_rec_hdr.status);
	return klog_show_data(ctx, klog_data + klog_rec_hdr.req_len, klog_rec_hdr.resp_len, title);
}

/* Заполнение экранного буфера */
static void klog_fill_scr_data(struct log_gui_context *ctx)
{
	if (pthread_mutex_lock(&klog_mtx) == 0){
		log_clear_ctx(ctx);
		klog_fill_scr_normal(ctx);
		pthread_mutex_unlock(&klog_mtx);
	}
}

/* Первая строка заголовка записи */
static const char *klog_get_head_line1(char *buf)
{
	if ((hklog->hdr->n_recs == 0) ||
			!klog_gui_ctx->filter(klog_gui_ctx->hlog, klog_gui_ctx->cur_rec_index))
		*buf = 0;
	else
		sprintf(buf, "%.2hhX%.2hhX (\"ЭКСПРЕСС-2А-К\" "
			"%hhu.%hhu.%hhu %.4hX %.*s) ККТ=%.*s",
			klog_rec_hdr.addr.gaddr, klog_rec_hdr.addr.iaddr,
			VERSION_BRANCH(klog_rec_hdr.term_version),
			VERSION_RELEASE(klog_rec_hdr.term_version),
			VERSION_PATCH(klog_rec_hdr.term_version),
			klog_rec_hdr.term_check_sum,
			isizeof(klog_rec_hdr.tn), klog_rec_hdr.tn,
			isizeof(klog_rec_hdr.kkt_nr), klog_rec_hdr.kkt_nr);
	return buf;
}

/* Вторая строка заголовка */
static const char *klog_get_head_line2(char *buf)
{
	if ((hklog->hdr->n_recs == 0) ||
			!klog_gui_ctx->filter(klog_gui_ctx->hlog, klog_gui_ctx->cur_rec_index)){
		static char msg[LOG_SCREEN_COLS + 1];
		snprintf(msg, sizeof(msg), "На контрольной ленте нет записей [%s]",
			klog_get_stream_name(cfg.kkt_log_stream));
		size_t msg_len = strlen(msg);
		sprintf(buf, "%*s", (int)(LOG_SCREEN_COLS + msg_len) / 2, msg);
/*		sprintf(buf, "                             "
			"На контрольной ленте нет записей [%s]",
			klog_get_stream_name(cfg.kkt_log_stream));*/
	}else{
		char rep[20], dt[128];
		uint32_t n = klog_rec_hdr.stream >> 3;
		if (n > 0){
			snprintf(rep, sizeof(rep), ":%u", n + 1);
			uint64_t interval = date_time_to_time_t(&klog_rec_hdr.dt);
			interval = interval * 1000 + klog_rec_hdr.ms + klog_rec_hdr.op_time;
			time_t t1 = interval / 1000;
			struct tm *tm = localtime(&t1);
			snprintf(dt, sizeof(dt), "%.2d.%.2d.%.4d %.2d:%.2d:%.2d -- "
				"%.2d.%.2d.%.4d %.2d:%.2d:%.2d",
				klog_rec_hdr.dt.day + 1, klog_rec_hdr.dt.mon + 1,
				klog_rec_hdr.dt.year + 2000,
				klog_rec_hdr.dt.hour, klog_rec_hdr.dt.min, klog_rec_hdr.dt.sec,
				tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
				tm->tm_hour, tm->tm_min, tm->tm_sec);
		}else{
			rep[0] = 0;
			snprintf(dt, sizeof(dt), "%.2d.%.2d.%.4d %.2d:%.2d:%.2d.%.3d",
				klog_rec_hdr.dt.day + 1, klog_rec_hdr.dt.mon + 1,
				klog_rec_hdr.dt.year + 2000,
				klog_rec_hdr.dt.hour, klog_rec_hdr.dt.min, klog_rec_hdr.dt.sec,
				klog_rec_hdr.ms);
		}
		sprintf(buf, "Запись %u [%s%s] от %s "
			"жетон %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%s",
			klog_rec_hdr.number + 1,
			klog_get_stream_name(KLOG_STREAM(klog_rec_hdr.stream)), rep, dt,
			ds_key_char(klog_rec_hdr.ds_type),
			klog_rec_hdr.dsn[7], klog_rec_hdr.dsn[0],
			klog_rec_hdr.dsn[6], klog_rec_hdr.dsn[5],
			klog_rec_hdr.dsn[4], klog_rec_hdr.dsn[3],
			klog_rec_hdr.dsn[2], klog_rec_hdr.dsn[1],
			(klog_rec_hdr.len & KLOG_REC_APC) ? " [АПЧ]" : "");
	}
	return buf;
}

/* Получение заданной строки заголовка записи ККЛ */
static const char *klog_get_head_line(uint32_t n)
{
	const char *ret = NULL;
	if (pthread_mutex_lock(&klog_mtx) == 0){
		static char buf[128];
		const char *(*fn[])(char *) = {
			klog_get_head_line1,
			klog_get_head_line2,
		};
		if (n < ASIZE(fn))
			ret = fn[n](buf);
		pthread_mutex_unlock(&klog_mtx);
	}
	return ret;
}

/* Получение строки подсказки */
static const char *klog_get_hint_line(uint32_t n)
{
	static const char *hints[] = {
		"\x1b/\x1a - пред./след. запись  Ctrl+\x1b/\x1a - быстрое перемещение",
		"Ctrl+Home/End - начало/конец записи  PgUp/PgDn - пред./след. стр.",
		"Home/End - первая/последняя запись  F2 - меню  Esc - выход",
	};
	const char *ret = NULL;
	if (pthread_mutex_lock(&klog_mtx) == 0){
		if (n < ASIZE(hints))
			ret = hints[n];
		pthread_mutex_unlock(&klog_mtx);
	}
	return ret;
}

/* Печать текущей записи */
bool klog_print_current(struct log_gui_context *ctx)
{
	bool ret = false;
	if (!ctx->active)
		return false;
	ctx->modal = true;
	ret =	klog_print_header() &&
		xprn_print((char *)log_prn_buf, log_prn_data_len) &&
		klog_print_rec() &&
		xprn_print((char *)log_prn_buf, log_prn_data_len) &&
		klog_print_footer() &&
		xprn_print((char *)log_prn_buf, log_prn_data_len);
	ctx->modal = false;
	return ret;
}

/* Печать диапазона записей */
bool klog_print_range(struct log_gui_context *ctx, uint32_t from, uint32_t to)
{
	uint32_t index1 = klog_index_for_number(hklog, from - 1);
	uint32_t index2 = klog_index_for_number(hklog, to - 1);
	if ((index1 == -1) || (index2 == -1)){
		set_term_astate(ast_no_log_item);
		return false;
	}
	bool ret = false;
	ctx->modal = true;
	for (int n = 0; !ret; n++, index1++){
		ret = index1 == index2;		/* последняя запись диапазона */
		if (!klog_read_rec(hklog, index1))
			break;
		ctx->cur_rec_index = index1;
		klog_fill_scr_data(ctx);
		log_draw(ctx);
/* Печать заголовка */
		if ((n == 0) && (!klog_print_header() ||
				!xprn_print((char *)log_prn_buf, log_prn_data_len)))
			break;
/* Печать записей */
		else if (!klog_print_rec() || !xprn_print((char *)log_prn_buf,
				log_prn_data_len))
			break;
/* Печать концевика */
		else if (ret && (!klog_print_footer() ||
				!xprn_print((char *)log_prn_buf, log_prn_data_len))){
			ret = false;
			break;
		}
	}
	ctx->modal = false;
	return ret;
}

/* Полная печать контрольной ленты */
bool klog_print_all(struct log_gui_context *ctx)
{
	uint32_t n0, n1;
	if ((hklog->hdr->n_recs == 0) ||
			!klog_gui_ctx->filter(klog_gui_ctx->hlog, klog_gui_ctx->cur_rec_index) ||
			!log_get_boundary_numbers(hklog, &n0, &n1))
		return false;
	return klog_print_range(ctx, n0 + 1, n1 + 1);
}

/* Обработчик событий клавиатуры */
static int klog_handle_kbd(struct log_gui_context *ctx __attribute__((unused)),
	const struct kbd_event *e)
{
	if (e->pressed && (e->shift_state == 0) && (e->key == KEY_F2))
		return cmd_klog_menu;
	else
		return cmd_none;
}

/* Поздняя инициализация структуры */
static void klog_init_gui_ctx(struct log_gui_context *ctx)
{
	ctx->hlog = hklog;
}

/* Можно ли показывать данную запись */
static bool klog_can_show_rec(struct log_handle *hlog, uint32_t index)
{
	return	(cfg.kkt_log_stream == KLOG_STREAM_ALL) ||
		(hlog->map[(hlog->map_head + index) % hlog->map_size].tag == cfg.kkt_log_stream);
}

static uint8_t klog_scr_data[GUI_KLOG_BUF_LEN];

static struct log_gui_context _klog_gui_ctx = {
	.hlog		= NULL,
	.active		= &klog_active,
	.title		= MAIN_TITLE " (КЛ3 - работа с ККТ)",
	.cur_rec_index	= 0,
	.nr_head_lines	= 2,
	.nr_hint_lines	= 3,
	.scr_data	= klog_scr_data,
	.scr_data_size	= sizeof(klog_scr_data),
	.scr_data_len	= 0,
	.scr_data_lines	= 0,
	.first_line	= 0,
	.gc		= NULL,
	.mem_gc		= NULL,
	.modal		= false,
	.asis		= false,
	.init		= klog_init_gui_ctx,
	.filter		= klog_can_show_rec,
	.read_rec	= klog_read_rec,
	.get_head_line	= klog_get_head_line,
	.get_hint_line	= klog_get_hint_line,
	.fill_scr_data	= klog_fill_scr_data,
	.handle_kbd	= klog_handle_kbd
};

struct log_gui_context *klog_gui_ctx = &_klog_gui_ctx;
