/* Функции для работы с циклической контрольной лентой. (c) gsr 2004, 2008, 2018-2019 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "log/express.h"
#include "log/logdbg.h"
#include "prn/aux.h"
#include "prn/express.h"
#include "cfg.h"
#include "express.h"
#include "gd.h"
#include "genfunc.h"
#include "paths.h"
#include "sterm.h"
#include "tki.h"
#include "transport.h"

/* Данные текущей записи контрольной ленты */
/* NB: данный буфер используется как для чтения, так и для записи */
uint8_t xlog_data[LOG_BUF_LEN] = {[0 ... LOG_BUF_LEN - 1] = 0};
/* Длина данных */
uint32_t xlog_data_len = 0;
/* Индекс текущего обрабатываемого байта в log_data */
uint32_t xlog_data_index = 0;

#define XLOG_MAP_MAX_SIZE	(XLOG_MAX_SIZE / sizeof(struct xlog_rec_header))
static struct map_entry_t xlog_map[XLOG_MAP_MAX_SIZE];

/* Заголовок контрольной ленты */
static struct xlog_header xlog_hdr;
/* Заголовок текущей записи контрольной ленты */
struct xlog_rec_header xlog_rec_hdr;

/* Инициализация заголовка контрольной ленты (используется при ее создании) */
static void xlog_init_hdr(struct log_handle *hlog)
{
	struct xlog_header *hdr = (struct xlog_header *)hlog->hdr;
	hdr->tag = XLOG_TAG;
	hdr->len = XLOG_MAX_SIZE;
	hdr->n_recs = 0;
	hdr->head = sizeof(*hdr);
	hdr->tail = sizeof(*hdr);
	hdr->cur_number = -1U;
	hdr->printed = false;
}

/* Вызывается при очистке контрольной ленты */
static void xlog_clear(struct log_handle *hlog)
{
	struct xlog_header *hdr = (struct xlog_header *)hlog->hdr;
	hdr->head = hdr->tail;
	hdr->n_recs = 0;
	hdr->cur_number = -1U;
}

/* Вычисление контрольной суммы CRC32 x^30 + x^27 + x^18 + x^4 + x^1 записи ЦКЛ */
static uint32_t xlog_rec_crc32(void)
{
	uint8_t b;
	uint32_t i, j, s = 0, l = sizeof(xlog_rec_hdr) + xlog_rec_hdr.len;
	for (i = 0; i < l; i++){
		if (i < sizeof(xlog_rec_hdr))
			b = *((uint8_t *)&xlog_rec_hdr + i);
		else
			b = xlog_data[i - sizeof(xlog_rec_hdr)];
		for (j = 0; j < 8; j++){
			s >>= 1;
			s |=	(((b & 1) ^
				((s & (1 << 1))  >> 1)  ^
				((s & (1 << 4))  >> 4)  ^
				((s & (1 << 18)) >> 18) ^
				((s & (1 << 27)) >> 27) ^
				((s & (1 << 30)) >> 30)) << 31);
		}
	}
	return s;
}

/* Заполнение карты контрольной ленты */
static bool xlog_fill_map(struct log_handle *hlog)
{
	struct xlog_header *hdr = (struct xlog_header *)hlog->hdr;
	uint32_t i, offs = hdr->head, tail, crc;
	memset(hlog->map, 0, hlog->map_size * sizeof(*hlog->map));
	hlog->map_head = 0;
	if (hdr->n_recs > XLOG_MAP_MAX_SIZE){
		logdbg("%s: Слишком много записей на %s; "
			"должно быть не более " _s(XLOG_MAP_MAX_SIZE) ".\n",
			__func__, hlog->log_type);
		return false;
	}
	for (i = 0; i < hdr->n_recs; i++){
		tail = offs;
		try_fn(log_read(hlog, offs, (uint8_t *)&xlog_rec_hdr, sizeof(xlog_rec_hdr)));
		hlog->map[i].offset = offs;
		hlog->map[i].number = xlog_rec_hdr.number;
		hlog->map[i].dt = xlog_rec_hdr.dt;
		hlog->map[i].tag = 0;
		offs = log_inc_index(hlog, offs, sizeof(xlog_rec_hdr));
		if (xlog_rec_hdr.tag != XLOG_REC_TAG){
			logdbg("%s: Неверный формат заголовка записи %s #%u.\n",
				__func__, hlog->log_type, i);
			return log_truncate(hlog, i, tail);
		}
		switch (xlog_rec_hdr.type){
			case XLRT_NORMAL:
			case XLRT_NOTIFY:
			case XLRT_INIT:
			case XLRT_FOREIGN:
			case XLRT_AUX:
			case XLRT_SPECIAL:
			case XLRT_BANK:
			case XLRT_IPCHANGE:
			case XLRT_REJECT:
			case XLRT_KKT:
				break;
			default:
				logdbg("%s: Запись %s #%u имеет неизвестный тип: %.4x.\n",
					__func__, hlog->log_type, i, xlog_rec_hdr.type);
				return log_truncate(hlog, i, tail);
		}
		if (xlog_rec_hdr.len > LOG_BUF_LEN){
			logdbg("%s: Слишком длинная запись %s #%u: %u байт (max %u).\n",
				__func__, hlog->log_type, i, xlog_rec_hdr.len, LOG_BUF_LEN);
			return log_truncate(hlog, i, tail);
		}
		xlog_data_len = xlog_rec_hdr.len;
		if (xlog_data_len > 0)
			try_fn(log_read(hlog, offs, xlog_data, xlog_data_len));
		crc = xlog_rec_hdr.crc32;
		xlog_rec_hdr.crc32 = 0;
		if (xlog_rec_crc32() != crc){
			logdbg("%s: Несовпадение контрольной суммы для записи %s #%u.\n",
				__func__, hlog->log_type, i);
			return log_truncate(hlog, i, tail);
		}
		xlog_rec_hdr.crc32 = crc;
		offs = log_inc_index(hlog, offs, xlog_rec_hdr.len);
	}
	return true;
}

/* Чтение заголовка контрольной ленты */
static bool xlog_read_header(struct log_handle *hlog)
{
	bool ret = false;
	if (log_atomic_read(hlog, 0, (uint8_t *)hlog->hdr, sizeof(struct xlog_header))){
		if (hlog->hdr->tag != XLOG_TAG)
			logdbg("%s: Неверный формат заголовка %s.\n", __func__,
				hlog->log_type);
		else if (hlog->hdr->len > XLOG_MAX_SIZE)
			logdbg("%s: Неверный размер %s.\n", __func__, hlog->log_type);
		else{
			off_t len = lseek(hlog->rfd, 0, SEEK_END);
			if (len == (off_t)-1)
				logdbg("%s: Ошибка определения размера файла %s.\n",
					__func__, hlog->log_type);
			else{
				hlog->full_len = hlog->hdr->len + sizeof(struct xlog_header);
				if (hlog->full_len == len)
					ret = true;
				else
					logdbg("%s: Размер %s, указанный в заголовке, не совпадает с реальным размером.\n",
						__func__, hlog->log_type);
			}
		}
	}
	return ret;
}

/* Запись заголовка контрольной ленты */
static bool xlog_write_header(struct log_handle *hlog)
{
	if (hlog->wfd == -1)
		return false;
	return log_atomic_write(hlog, 0, (uint8_t *)hlog->hdr,
		sizeof(struct xlog_header));
}

/* Структура для работы с циклической контрольной лентой */
static struct log_handle _hxlog = {
	.hdr		= (struct log_header *)&xlog_hdr,
	.hdr_len	= sizeof(xlog_hdr),
	.full_len	= sizeof(xlog_hdr),
	.log_type	= "ЦКЛ",
	.log_name	= STERM_XLOG_NAME,
	.rfd		= -1,
	.wfd		= -1,
	.map		= xlog_map,
	.map_size	= XLOG_MAP_MAX_SIZE,
	.map_head	= 0,
	.on_open	= NULL,
	.on_close	= NULL,
	.init_log_hdr	= xlog_init_hdr,
	.clear		= xlog_clear,
	.fill_map	= xlog_fill_map,
	.read_header	= xlog_read_header,
	.write_header	= xlog_write_header
};

struct log_handle *hxlog = &_hxlog;

/*
 * Определение индекса первой записи с заданным номером. Возвращает -1UL,
 * если запись не найдена.
 */
uint32_t xlog_first_for_number(struct log_handle *hlog, uint32_t number)
{
	for (uint32_t i = 0; i < hlog->hdr->n_recs; i++){
		if (hlog->map[(hlog->map_head + i) % hlog->map_size].number == number)
			return i;
	}
	return -1U;
}

/*
 * Определение индекса последней записи с заданным номером. Возвращает -1UL,
 * если запись не найдена.
 */
uint32_t xlog_last_for_number(struct log_handle *hlog, uint32_t number)
{
	uint32_t index = xlog_first_for_number(hlog, number);
	if (index != -1U){
		for (; index < hlog->hdr->n_recs; index++){
			if (hlog->map[(hlog->map_head + index) % hlog->map_size].number != number)
				break;
		}
		return index - 1;
	}
	return -1U;
}

/* Определение индекса записи по номерам записи и абзаца */
uint32_t xlog_index_for_number(struct log_handle *hlog, uint32_t number, uint32_t n_para)
{
	uint32_t index = xlog_first_for_number(hlog, number);
	if (index != -1U){
		for (; (index < hlog->hdr->n_recs) && n_para; index++, n_para--){
			if (hlog->map[(hlog->map_head + index) % hlog->map_size].number != number)
				return -1U;
		}
	}
	return index;
}

/* Можно ли записать на контрольную ленту */
bool xlog_can_write(struct log_handle *hlog __attribute__((unused)))
{
	return true;
/*	return log_hdr.printed || (log_free_space() >= MIN_LOG_FREE);*/
}

/* Можно ли очистить контрольную ленту */
bool xlog_can_clear(struct log_handle *hlog __attribute__((unused)))
{
	return false;
/*	return (log_hdr.n_recs > 0) && !cfg.cyclic_log && log_hdr.printed;*/
}

/* Можно ли отпечатать диапазон записей контрольной ленты */
bool xlog_can_print_range(struct log_handle *hlog)
{
	return cfg.has_xprn && (hlog->hdr->n_recs > 0);
}

/* Можно ли распечатать контрольную ленту полностью */
bool xlog_can_print(struct log_handle *hlog __attribute__((unused)))
{
	return false;
/*	return cfg.has_xprn && (log_hdr.n_recs > 0) && !cfg.cyclic_log;*/
}

/* Можно ли проводить поиск по контрольной ленте */
bool xlog_can_find(struct log_handle *hlog)
{
	return hlog->hdr->n_recs > 0;
}

/*
 * Добавление новой записи в конец КЛ. Данные находятся в xlog_data,
 * заголовок -- в xlog_rec_hdr.
 */
static bool xlog_add_rec(struct log_handle *hlog)
{
	uint32_t rec_len, free_len, m, n, tail, offs;
	bool ret = false;
/*	if (!cfg.cyclic_log && log_hdr.printed)
		log_clear(false);*/
	rec_len = xlog_rec_hdr.len + sizeof(xlog_rec_hdr);
	if (rec_len > hlog->hdr->len){
		logdbg("%s: Длина записи (%u байт) превышает длину КЛ (%u байт).\n",
			__func__, rec_len, hlog->hdr->len);
		return false;
	}
	free_len = log_free_space(hlog);
/*	if (!cfg.cyclic_log &&
			((free_len < MIN_LOG_FREE) || (rec_len > free_len))){
		logdbg("%s: Слишком мало свободного места на КЛ при записи (%u байт)\n",
			__func__, free_len);
		return false;
	}*/
/* Если при записи на циклическую КЛ не хватает места, удаляем записи в начале */
	m = hlog->map_head;
	for (n = hlog->hdr->n_recs; (n > 0) && (free_len < rec_len); n--){
		if (n == 1)
			tail = hlog->hdr->tail;
		else
			tail = hlog->map[(m + 1) % hlog->map_size].offset;
		free_len += log_delta(hlog, tail, hlog->map[m].offset);
		m++;
		m %= hlog->map_size;
	}
	if (free_len < rec_len){
		logdbg("%s: Не удалось освободить место под новую запись.\n", __func__);
		return false;
	}
/* n -- оставшееся число записей на контрольной ленте */
	if ((n + 1) > hlog->map_size){
		logdbg("%s: Превышено максимальное число записей на КЛ;\n"
			"должно быть не более %u.\n", __func__, hlog->map_size);
		return false;
	}
/* Начинаем запись на контрольную ленту */
	if (log_begin_write(hlog)){
		hlog->map_head = m;
		tail = (hlog->map_head + n) % hlog->map_size;
		hlog->map[tail].offset = hlog->hdr->tail;
		hlog->map[tail].number = xlog_rec_hdr.number;
		hlog->map[tail].dt = xlog_rec_hdr.dt;
		hlog->hdr->n_recs = n + 1;
		hlog->hdr->head = hlog->map[hlog->map_head].offset;
		offs = hlog->hdr->tail;
		hlog->hdr->tail = log_inc_index(hlog, hlog->hdr->tail,
				sizeof(xlog_rec_hdr) + xlog_rec_hdr.len);
		ret = hlog->write_header(hlog) &&
			log_write(hlog, offs, (uint8_t *)&xlog_rec_hdr, sizeof(xlog_rec_hdr)) &&
			((xlog_rec_hdr.len == 0) ||
			log_write(hlog, log_inc_index(hlog, offs,
					sizeof(xlog_rec_hdr)),
				xlog_data, xlog_rec_hdr.len));
	}
	log_end_write(hlog);
	return ret;
}

/*
 * Заполнение заголовка записи циклической контрольной ленты. Поля типа,
 * длины и контрольной суммы заполняются значениями по умолчанию.
 */
static void xlog_init_rec_hdr(struct log_handle *hlog,
		struct xlog_rec_header *hdr, uint32_t n_para)
{
	hdr->tag = XLOG_REC_TAG;
	if (n_para == 0)
		hlog->hdr->cur_number++;
	hdr->number = hlog->hdr->cur_number;
	hdr->n_para = n_para;
	hdr->type = XLRT_NORMAL;
	hdr->code = 0;
	hdr->len = 0;
	time_t_to_date_time(time(NULL) + time_delta, &hdr->dt);
	hdr->addr.gaddr = cfg.gaddr;
	hdr->addr.iaddr = cfg.iaddr;
	hdr->term_version = STERM_VERSION;
	hdr->term_check_sum = term_check_sum;
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)hdr->tn);
	fill_prn_number(hdr->xprn_number, cfg.has_xprn ? cfg.xprn_number : "",
		sizeof(hdr->xprn_number));
	fill_prn_number(hdr->aprn_number, cfg.has_aprn ? cfg.aprn_number : "",
		sizeof(hdr->aprn_number));
	memcpy(hdr->dsn, dsn, DS_NUMBER_LEN);
	hdr->ds_type = kt;
	hdr->ZNtz = oldZNtz;
	hdr->ZNpo = oldZNpo;
	hdr->ZBp  = oldZBp;
	hdr->ONpo = ONpo;
	hdr->ONtz = ONtz;
	hdr->OBp  = OBp;
	hdr->reaction_time = (reaction_time + 50) / 100;
	hdr->flags = 0;
	hdr->crc32 = 0;
}

/* Занесение на КЛ записи заданного типа. Возвращает номер записи */
uint32_t xlog_write_rec(struct log_handle *hlog, uint8_t *data, uint32_t len,
		uint32_t type, uint32_t n_para)
{
	xlog_data_len = xlog_data_index = 0;
	if (data != NULL){
		if (len > sizeof(xlog_data)){
			logdbg("%s: Переполнение буфера данных при записи "
				"на %s (%u байт).\n", __func__, hlog->log_type, len);
			return -1U;
		}
		if ((data != xlog_data) && (len > 0)){	/* см. xlog_write_foreign */
			memcpy(xlog_data, data, len);
			xlog_data_len = len;
		}
	}
	xlog_init_rec_hdr(hlog, &xlog_rec_hdr, n_para);
	xlog_rec_hdr.type = type;
/* FIXME: код контрольной ленты всегда 0 */
	xlog_rec_hdr.code = 0;	/* только для наглядности, т.к. это поле уже обнулено в log_init_rec_hdr */
	xlog_rec_hdr.len = len;
	xlog_rec_hdr.crc32 = xlog_rec_crc32();
	return xlog_add_rec(hlog) ? xlog_rec_hdr.number : -1UL;
}

/* Занесение на контрольную ленту записи о чужом ответе */
bool xlog_write_foreign(struct log_handle *hlog, uint8_t gaddr, uint8_t iaddr)
{
	struct term_addr *addr = (struct term_addr *)xlog_data;
	addr->gaddr = gaddr;
	addr->iaddr = iaddr;
	return xlog_write_rec(hlog, xlog_data, sizeof(*addr), XLRT_FOREIGN, 0) != -1U;
}

/* Занесение на контрольную ленту записи об изменении основного ip хост-ЭВМ */
bool xlog_write_ipchange(struct log_handle *hlog, uint32_t old, uint32_t new)
{
	uint32_t *p = (uint32_t *)xlog_data;
	p[0] = old;
	p[1] = new;
	return xlog_write_rec(hlog, xlog_data, 2 * sizeof(uint32_t), XLRT_IPCHANGE, 0) != -1U;
}

/* Чтение заданной записи контрольной ленты */
bool xlog_read_rec(struct log_handle *hlog, uint32_t index)
{
	uint32_t offs, crc;
	if (index >= hlog->hdr->n_recs)
		return false;
	xlog_data_len = 0;
	xlog_data_index = 0;
	offs = hlog->map[(hlog->map_head + index) % hlog->map_size].offset;
	if (!log_read(hlog, offs, (uint8_t *)&xlog_rec_hdr, sizeof(xlog_rec_hdr)))
		return false;
	else if (xlog_rec_hdr.len > sizeof(xlog_data)){
		logdbg("%s: Слишком длинная запись %s #%u (%u байт).\n",
			__func__, hlog->log_type, index, xlog_rec_hdr.len);
		return false;
	}
	offs = log_inc_index(hlog, offs, sizeof(xlog_rec_hdr));
	if ((xlog_rec_hdr.len > 0) && !log_read(hlog, offs, xlog_data,
			xlog_rec_hdr.len))
		return false;
	crc = xlog_rec_hdr.crc32;
	xlog_rec_hdr.crc32 = 0;
	if (xlog_rec_crc32() != crc){
		logdbg("%s: Несовпадение контрольной суммы для записи %s #%u.\n",
			__func__, hlog->log_type, index);
		return false;
	}
	xlog_rec_hdr.crc32 = crc;
	xlog_data_len = xlog_rec_hdr.len;
	return true;
}

/* Установка флагов записи на КЛ */
bool xlog_set_rec_flags(struct log_handle *hlog, uint32_t number, uint32_t n_para,
	uint32_t flags)
{
	bool ret = false;
	uint32_t index = xlog_index_for_number(hlog, number, n_para);
	if ((index != -1U) && xlog_read_rec(hlog, index)){
		xlog_rec_hdr.flags |= flags;
		xlog_rec_hdr.crc32 = 0;
		xlog_rec_hdr.crc32 = xlog_rec_crc32();
		if (log_begin_write(hlog))
			ret = log_write(hlog,
				hlog->map[(hlog->map_head + index) % hlog->map_size].offset,
				(uint8_t *)&xlog_rec_hdr, sizeof(xlog_rec_hdr));
		log_end_write(hlog);
	}
	return ret;
}

/* Вывод заголовка распечатки КЛ */
bool xlog_print_header(void)
{
	log_reset_prn_buf();
	try_fn(prn_write_str("\x1e\x0bНАЧАЛО ПЕЧАТИ КОНТРОЛЬНОЙ ЛЕНТЫ "
		"(ОСНОВНОЙ РЕЖИМ) "));
	try_fn(prn_write_cur_date_time());
	return prn_write_str("\x1c\x0b") && prn_write_eol();
}

/* Вывод концевика распечатки контрольной ленты */
bool xlog_print_footer(void)
{
	log_reset_prn_buf();
	try_fn(prn_write_str("\x1e\x0bКОНЕЦ ПЕЧАТИ КОНТРОЛЬНОЙ ЛЕНТЫ "
		"(ОСНОВНОЙ РЕЖИМ) "));
	try_fn(prn_write_cur_date_time());
	return prn_write_str("\x1c\x0b") && prn_write_eol();
}

/* Вывод заголовка записи на печать */
static bool xlog_print_rec_header(void)
{
	char s[128];
	snprintf(s, sizeof(s), "\x1e\x0b%.2hhX%.2hhX "
			"(\"ЭКСПРЕСС-2А-К\" %.4hX %.*s) ОПУ=%.*s ДПУ=%.*s",
		xlog_rec_hdr.addr.gaddr, xlog_rec_hdr.addr.iaddr,
		xlog_rec_hdr.term_check_sum,
		isizeof(xlog_rec_hdr.tn), xlog_rec_hdr.tn,
		isizeof(xlog_rec_hdr.xprn_number), xlog_rec_hdr.xprn_number,
		isizeof(xlog_rec_hdr.aprn_number), xlog_rec_hdr.aprn_number);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "ЗАПИСЬ %u/%u ОТ ",
		xlog_rec_hdr.number + 1, xlog_rec_hdr.n_para + 1);
	try_fn(prn_write_str(s));
	try_fn(prn_write_date_time(&xlog_rec_hdr.dt));
	snprintf(s, sizeof(s), " ЖЕТОН %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX",
		ds_key_char(xlog_rec_hdr.ds_type),
		xlog_rec_hdr.dsn[7], xlog_rec_hdr.dsn[0],
		xlog_rec_hdr.dsn[6], xlog_rec_hdr.dsn[5],
		xlog_rec_hdr.dsn[4], xlog_rec_hdr.dsn[3],
		xlog_rec_hdr.dsn[2], xlog_rec_hdr.dsn[1]);
	if ((xlog_rec_hdr.type == XLRT_NORMAL) && (xlog_rec_hdr.flags & XLOG_REC_PRINTED))
		strcat(s, " [+]");
	if (xlog_rec_hdr.type == XLRT_KKT){
		if (xlog_rec_hdr.flags & XLOG_REC_APC)
			strcat(s, " [АПЧ]");
		if (xlog_rec_hdr.flags & XLOG_REC_CPC)
			strcat(s, " [ЧвКФП]");
	}
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "ZNТЗ: %.3hX  ZNПО: %.3hX  ZБП: %.2hhX      "
			"ONПО: %.3hX  ONТЗ: %.3hX  OБП: %.2hhX    "
			"ВРЕМЯ РЕАКЦИИ %.3d СЕК.\x1c\x0b",
		xlog_rec_hdr.ZNtz, xlog_rec_hdr.ZNpo, xlog_rec_hdr.ZBp,
		xlog_rec_hdr.ONpo, xlog_rec_hdr.ONtz, xlog_rec_hdr.OBp,
		xlog_rec_hdr.reaction_time);
	return prn_write_str(s) && prn_write_eol();
}

/* Вывод на печать записи типа XLRT_NOTIFY */
static bool xlog_print_xlrt_notify(void)
{
	return prn_write_str("*ОТВЕТ БЕЗ ПЕЧАТИ*") && prn_write_eol();
}

/* Вывод на печать записи типа XLRT_INIT */
static bool xlog_print_xlrt_init(void)
{
	return prn_write_str("*ИНИЦИАЛИЗАЦИЯ*") && prn_write_eol();
}

/* Вывод на печать записи типа XLRT_FOREIGN */
static bool xlog_print_xlrt_foreign(void)
{
	struct term_addr *addr = (struct term_addr *)xlog_data;
	char s[128];
	if (xlog_data_len == sizeof(*addr))
		snprintf(s, sizeof(s), "*ПОЛУЧЕН ОТВЕТ ДЛЯ ДРУГОГО ТЕРМИНАЛА "
				"(%.2hhX:%2hhX)*",
			addr->gaddr, addr->iaddr);
	else
		snprintf(s, sizeof(s), "*ПОЛУЧЕН ОТВЕТ ДЛЯ ДРУГОГО ТЕРМИНАЛА (**:**)*");
	return prn_write_str(s) && prn_write_eol();
}

/* Вывод на печать записи типа XLRT_SPECIAL */
static bool xlog_print_xlrt_special(void)
{
	char s[128];
	xlog_data_index = 0;
	snprintf(s, sizeof(s), "*ОШИБКА КОНТРОЛЬНОЙ СУММ\x9b С:");
	return	prn_write_str(s) &&
		prn_write_char_raw(xlog_data[xlog_data_index++]) &&
		prn_write_char_raw(xlog_data[xlog_data_index]) &&
		prn_write_char_raw('*') &&
		prn_write_eol();
}

/* Вывод на печать записи типа XLRT_BANK */
static bool xlog_print_xlrt_bank(void)
{
	return	prn_write_str("*БАНК*") && prn_write_eol() &&
/* Номер заказа в системе */
		prn_write_char_raw('*') &&
		prn_write_chars_raw((char *)xlog_data, 7) &&
		prn_write_char_raw('*') && prn_write_eol() &&
/* Технологический номер кассы */
		prn_write_char_raw('*') &&
		prn_write_chars_raw((char *)xlog_data + 7, 5) &&
		prn_write_char_raw('*') && prn_write_eol() &&
/* Сумма заказа */
		prn_write_char_raw('*') &&
		prn_write_chars_raw((char *)xlog_data + 12, 9) &&
		prn_write_char_raw('*') && prn_write_eol();
}

/* Вывод на печать записи типа XLRT_IPCHANGE */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
static bool xlog_print_xlrt_ipchange(void)
{
	uint32_t *p = (uint32_t *)xlog_data;
	char s[128], old_ip[20], new_ip[20];
	strncpy(old_ip, inet_ntoa(dw2ip(p[0])), sizeof(old_ip) - 1);
	old_ip[sizeof(old_ip) - 1] = 0;
	strncpy(new_ip, inet_ntoa(dw2ip(p[1])), sizeof(new_ip) - 1);
	new_ip[sizeof(new_ip) - 1] = 0;
	snprintf(s, sizeof(s), "*ЗАМЕНА ОСНОВНОГО IP ХОСТ-ЭВМ \"ЭКСПРЕСС\" "
		"С %s НА %s*", old_ip, new_ip);
	return prn_write_str(s) && prn_write_eol();
}

/* Вывод на печать записи типа XLRT_REJECT */
static bool xlog_print_xlrt_reject(void)
{
	return prn_write_str("*ОТКАЗ ОТ ЗАКАЗА*") && prn_write_eol();
}

/* Можно ли выводить символ при печати контрольной ленты */
static bool is_printable_char(uint8_t c)
{
	switch (c){
		case 0x00:
		case 0x0c:
		case 0x7f:
			return false;
		default:
			return true;
	}
}

/*
 * Можно ли выводить команду при печати контрольной ленты.
 * Возвращает количество символов, которые не надо печатать. Если команду
 * можно напечатать, возвращает 0.
 */
static int get_unprintable_len(uint8_t cmd)
{
	uint8_t b;
	int k = xlog_data_index, l = 0;
	bool flag = true;	/* команду можно вывести на печать */
	bool loop_flag = true;
	if (cmd == XPRN_PRNOP){
		for (; loop_flag && (xlog_data_index < xlog_data_len); xlog_data_index++){
			b = xlog_data[xlog_data_index];
			switch (b){
				case XPRN_PRNOP_VPOS_BK:
				case XPRN_PRNOP_VPOS_ABS:
					flag = false;		/* fall through */
				case XPRN_PRNOP_HPOS_RIGHT:
				case XPRN_PRNOP_HPOS_LEFT:
				case XPRN_PRNOP_HPOS_ABS:
				case XPRN_PRNOP_VPOS_FW:
				case XPRN_PRNOP_INP_TICKET:
				case XPRN_PRNOP_LINE_FW:
					loop_flag = false;
					break;
			}
		}
	}else if ((cmd == XPRN_VPOS) && (xlog_data_index < xlog_data_len))
		flag = xlog_data[xlog_data_index++] >= 0x38;
	if (!flag)
		l = xlog_data_index - k;
	xlog_data_index = k;
	return l;
}

/* Вывод на печать команды работы со штрих-кодом */
static bool xlog_print_bctl(uint8_t cmd)
{
	switch (cmd){
		case XPRN_WR_BCODE:
			return	prn_write_str("ПЧ ШТРИХ: ") &&
				log_print_bcode(xlog_data, xlog_data_len, &xlog_data_index);
		case XPRN_RD_BCODE:
			return	prn_write_str("ЧТ ШТРИХ: ") &&
				log_print_bcode(xlog_data, xlog_data_len, &xlog_data_index);
		case XPRN_NO_BCODE:
			xlog_data_index--;
			return	prn_write_str("БЕЗ ШТРИХ");
		default:
			return false;
	}
}

/* Вывод строки контрольной ленты на печать */
static bool xlog_print_line(void)
{
	enum {
		st_start,
		st_regular,
		st_dle,
		st_bctl,
		st_cmd,
		st_wcr,
		st_wlf,
		st_stop,
	};
	int st = st_start, l, m = 0;
	uint8_t b, cmd = 0;
	for (; (xlog_data_index < xlog_data_len) && (st != st_stop); xlog_data_index++){
		b = xlog_data[xlog_data_index];
		switch (st){
			case st_start:
				try_fn(prn_write_char_raw('*'));
				st = st_regular;
				xlog_data_index--;
				break;
			case st_regular:
				if (is_escape(b))
					st = st_dle;
				else if (b == 0x0a){
					try_fn(prn_write_str("*\x0a"));
					st = st_wcr;
				}else if (b == 0x0d){
					try_fn(prn_write_str("*\x0d"));
					st = st_wlf;
				}else{
					if (is_printable_char(b)){
						try_fn(prn_write_char_raw(b));
						m++;
					}
				}
				break;
			case st_dle:
				cmd = b;
				switch (b){
					case XPRN_WR_BCODE:
					case XPRN_RD_BCODE:
					case XPRN_NO_BCODE:
						if (m == 0)
							st = st_bctl;
						else{
							try_fn(prn_write_char_raw('*'));
							try_fn(prn_write_eol());
							xlog_data_index -= 2;
							st = st_stop;
						}
						break;
					default:
						st = st_cmd;
				}
				break;
			case st_bctl:
				try_fn(xlog_print_bctl(cmd));
				if ((xlog_data_index >= (xlog_data_len - 1)) ||
						((xlog_data[xlog_data_index + 1] != 0x0a) &&
						 (xlog_data[xlog_data_index + 1] != 0x0d))){
					try_fn(prn_write_char_raw('*'));
					try_fn(prn_write_eol());
					st = st_stop;
				}else
					st = st_regular;
				break;
			case st_cmd:
				l = get_unprintable_len(cmd);
				if (l == 0){
					try_fn(prn_write_char_raw(X_DLE));
					try_fn(prn_write_char_raw(cmd));
/*					xlog_data_index--;*/
				}
				xlog_data_index += l - 1;
				st = st_regular;
				break;
			case st_wcr:
				if (b == 0x0d){
					try_fn(prn_write_char_raw(b));
					st = st_stop;
				}else{
					xlog_data_index--;
					st = st_stop;
				}
				break;
			case st_wlf:
				if (b == 0x0a){
					try_fn(prn_write_char_raw(b));
					st = st_stop;
				}else if (b == 0x0d)
					try_fn(prn_write_char_raw(b));
				else{
					xlog_data_index--;
					st = st_stop;
				}
				break;
		}
	}
	if (st == st_stop)
		return true;
	else
		return	prn_write_char_raw('*') &&
			prn_write_eol();
}

/* Вывод на печать обычной записи */
static bool xlog_print_xlrt_normal(void)
{
	for (xlog_data_index = 0; xlog_data_index < xlog_data_len;){
		if (!xlog_print_line())
			break;
	}
	return xlog_data_index == xlog_data_len;
}

/* Вывод на печать записи ПКЛ для ОПУ */
bool xlog_print_llrt_normal(void)
{
	return xlog_print_xlrt_normal();
}

/* Вывод записи контрольной ленты на печать */
bool xlog_print_rec(void)
{
	log_reset_prn_buf();
	try_fn(xlog_print_rec_header());
	switch (xlog_rec_hdr.type){
		case XLRT_NORMAL:
		case XLRT_AUX:
		case XLRT_KKT:
			return xlog_print_xlrt_normal();
		case XLRT_NOTIFY:
			return xlog_print_xlrt_notify();
		case XLRT_INIT:
			return xlog_print_xlrt_init();
		case XLRT_FOREIGN:
			return xlog_print_xlrt_foreign();
		case XLRT_SPECIAL:
			return xlog_print_xlrt_special();
		case XLRT_BANK:
			return xlog_print_xlrt_bank();
		case XLRT_IPCHANGE:
			return xlog_print_xlrt_ipchange();
		case XLRT_REJECT:
			return xlog_print_xlrt_reject();
		default:
			return false;
	}
}
