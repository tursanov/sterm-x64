/* Функции для работы с контрольной лентой ИПТ (БКЛ). (c) gsr 2004,2008 */

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
#include "log/logdbg.h"
#include "log/pos.h"
#include "prn/express.h"
#include "prn/local.h"
#include "cfg.h"
#include "express.h"
#include "gd.h"
#include "genfunc.h"
#include "paths.h"
#include "sterm.h"
#include "tki.h"

/* Данные текущей записи контрольной ленты */
/* NB: данный буфер используется как для чтения, так и для записи */
uint8_t plog_data[LOG_BUF_LEN];
/* Длина данных */
uint32_t plog_data_len;
/* Индекс текущего обрабатываемого байта в plog_data */
uint32_t plog_data_index;

#define PLOG_MAP_MAX_SIZE	(PLOG_MAX_SIZE / sizeof(struct plog_rec_header))
static struct map_entry_t plog_map[PLOG_MAP_MAX_SIZE];

/* Заголовок контрольной ленты */
struct plog_header plog_hdr;
/* Заголовок текущей записи контрольной ленты */
struct plog_rec_header plog_rec_hdr;

/* Инициализация заголовка контрольной ленты (используется при ее создании) */
static void plog_init_hdr(struct log_handle *hlog)
{
	struct plog_header *hdr = (struct plog_header *)hlog->hdr;
	hdr->tag = PLOG_TAG;
	hdr->len = PLOG_MAX_SIZE;
	hdr->n_recs = 0;
	hdr->head = sizeof(*hdr);
	hdr->tail = sizeof(*hdr);
	hdr->cur_number = -1U;
}

/* Вызывается при очистке контрольной ленты */
static void plog_clear(struct log_handle *hlog)
{
	struct plog_header *hdr = (struct plog_header *)hlog->hdr;
	hdr->head = hdr->tail;
	hdr->n_recs = 0;
	hdr->cur_number = -1U;
}

/* Вычисление контрольной суммы CRC32 x^30 + x^27 + x^18 + x^5 + x^1 записи КЛ */
static uint32_t plog_rec_crc32(void)
{
	uint8_t b;
	uint32_t i, j, s = 0, l = sizeof(plog_rec_hdr) + plog_rec_hdr.len;
	for (i = 0; i < l; i++){
		if (i < sizeof(plog_rec_hdr))
			b = *((uint8_t *)&plog_rec_hdr + i);
		else
			b = plog_data[i - sizeof(plog_rec_hdr)];
		for (j = 0; j < 8; j++){
			s >>= 1;
			s |=	(((b & 1) ^
				((s & (1 << 1))  >> 1)  ^
				((s & (1 << 5))  >> 5)  ^
				((s & (1 << 18)) >> 18) ^
				((s & (1 << 27)) >> 27) ^
				((s & (1 << 30)) >> 30)) << 31);
		}
	}
	return s;
}

/* Заполнение карты контрольной ленты */
static bool plog_fill_map(struct log_handle *hlog)
{
	struct plog_header *hdr = (struct plog_header *)hlog->hdr;
	uint32_t i, offs = hdr->head, tail, crc;
	memset(hlog->map, 0, hlog->map_size * sizeof(*hlog->map));
	hlog->map_head = 0;
	if (hdr->n_recs > PLOG_MAP_MAX_SIZE){
		logdbg("%s: Слишком много записей на %s; "
			"должно быть не более " _s(PLOG_MAP_MAX_SIZE) ".\n",
			__func__, hlog->log_type);
		return false;
	}
	for (i = 0; i < hdr->n_recs; i++){
		tail = offs;
		try_fn(log_read(hlog, offs, (uint8_t *)&plog_rec_hdr, sizeof(plog_rec_hdr)));
		hlog->map[i].offset = offs;
		hlog->map[i].number = plog_rec_hdr.number;
		hlog->map[i].dt = plog_rec_hdr.dt;
		hlog->map[i].tag = 0;
		offs = log_inc_index(hlog, offs, sizeof(plog_rec_hdr));
		if (plog_rec_hdr.tag != PLOG_REC_TAG){
			logdbg("%s: Неверный формат заголовка записи %s #%u.\n",
				__func__, hlog->log_type, i);
			return log_truncate(hlog, i, tail);
		}else if (plog_rec_hdr.len > LOG_BUF_LEN){
			logdbg("%s: Слишком длинная запись %s #%u: %u байт (max %u).\n",
				__func__, hlog->log_type, i, plog_rec_hdr.len, LOG_BUF_LEN);
			return log_truncate(hlog, i, tail);
		}
		plog_data_len = plog_rec_hdr.len;
		if (plog_data_len > 0)
			try_fn(log_read(hlog, offs, plog_data, plog_data_len));
		crc = plog_rec_hdr.crc32;
		plog_rec_hdr.crc32 = 0;
		if (plog_rec_crc32() != crc){
			logdbg("%s: Несовпадение контрольной суммы для записи %s #%u.\n",
				__func__, hlog->log_type, i);
			return log_truncate(hlog, i, tail);
		}
		plog_rec_hdr.crc32 = crc;
		offs = log_inc_index(hlog, offs, plog_rec_hdr.len);
	}
	return true;
}

/* Чтение заголовка контрольной ленты */
static bool plog_read_header(struct log_handle *hlog)
{
	bool ret = false;
	if (log_atomic_read(hlog, 0, (uint8_t *)hlog->hdr, sizeof(struct plog_header))){
		if (hlog->hdr->tag != PLOG_TAG)
			logdbg("%s: Неверный формат заголовка %s.\n", __func__, hlog->log_type);
		else if (hlog->hdr->len > PLOG_MAX_SIZE)
			logdbg("%s: Неверный размер %s.\n", __func__, hlog->log_type);
		else{
			off_t len = lseek(hlog->rfd, 0, SEEK_END);
			if (len == (off_t)-1)
				logdbg("%s: Ошибка определения размера файла %s.\n",
					__func__, hlog->log_type);
			else{
				hlog->full_len = hlog->hdr->len + sizeof(struct plog_header);
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
static bool plog_write_header(struct log_handle *hlog)
{
	if (hlog->wfd == -1)
		return false;
	return log_atomic_write(hlog, 0, (uint8_t *)hlog->hdr,
		sizeof(struct plog_header));
}

/* Структура для работы с контрольной лентой ИПТ (БКЛ) */
static struct log_handle _hplog = {
	.hdr		= (struct log_header *)&plog_hdr,
	.hdr_len	= sizeof(plog_hdr),
	.full_len	= sizeof(plog_hdr),
	.log_type	= "БКЛ",
	.log_name	= STERM_PLOG_NAME,
	.rfd		= -1,
	.wfd		= -1,
	.map		= plog_map,
	.map_size	= PLOG_MAP_MAX_SIZE,
	.map_head	= 0,
	.on_open	= NULL,
	.on_close	= NULL,
	.init_log_hdr	= plog_init_hdr,
	.clear		= plog_clear,
	.fill_map	= plog_fill_map,
	.read_header	= plog_read_header,
	.write_header	= plog_write_header
};

struct log_handle *hplog = &_hplog;

/*
 * Определение индекса записи с заданным номером. Возвращает -1UL,
 * если запись не найдена.
 */
uint32_t plog_index_for_number(struct log_handle *hlog, uint32_t number)
{
	uint32_t i;
	for (i = 0; i < hlog->hdr->n_recs; i++){
		if (hlog->map[(hlog->map_head + i) % hlog->map_size].number == number)
			return i;
	}
	return -1U;
}

/* Можно ли отпечатать диапазон записей контрольной ленты */
bool plog_can_print_range(struct log_handle *hlog)
{
	return cfg.has_xprn && (hlog->hdr->n_recs > 0);
}

/* Можно ли распечатать контрольную ленту полностью */
bool plog_can_print(struct log_handle *hlog)
{
	return plog_can_print_range(hlog);
}

/* Можно ли проводить поиск по контрольной ленте */
bool plog_can_find(struct log_handle *hlog)
{
	return hlog->hdr->n_recs > 0;
}

/*
 * Добавление новой записи в конец БКЛ. Данные находятся в plog_data,
 * заголовок -- в plog_rec_hdr.
 */
static bool plog_add_rec(struct log_handle *hlog)
{
	uint32_t rec_len, free_len, m, n, tail, offs;
	bool ret = false;
	rec_len = plog_rec_hdr.len + sizeof(plog_rec_hdr);
	if (rec_len > hlog->hdr->len){
		logdbg("%s: Длина записи (%u байт) превышает длину КЛ (%u байт).\n",
			__func__, rec_len, hlog->hdr->len);
		return false;
	}
	free_len = log_free_space(hlog);
/* Если при записи на циклическую БКЛ не хватает места, удаляем записи в начале */
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
		hlog->map[tail].number = plog_rec_hdr.number;
		hlog->map[tail].dt = plog_rec_hdr.dt;
		hlog->hdr->n_recs = n + 1;
		hlog->hdr->head = hlog->map[hlog->map_head].offset;
		offs = hlog->hdr->tail;
		hlog->hdr->tail = log_inc_index(hlog, hlog->hdr->tail,
				sizeof(plog_rec_hdr) + plog_rec_hdr.len);
		ret = hlog->write_header(hlog) &&
			log_write(hlog, offs, (uint8_t *)&plog_rec_hdr, sizeof(plog_rec_hdr)) &&
			((plog_rec_hdr.len == 0) ||
			log_write(hlog, log_inc_index(hlog, offs,
					sizeof(plog_rec_hdr)),
				plog_data, plog_rec_hdr.len));
	}
	log_end_write(hlog);
	return ret;
}

/*
 * Заполнение заголовка записи контрольной ленты. Поля типа, длины
 * и контрольной суммы заполняются значениями по умолчанию.
 */
static void plog_init_rec_hdr(struct log_handle *hlog,
		struct plog_rec_header *hdr)
{
	hdr->tag = PLOG_REC_TAG;
	hlog->hdr->cur_number++;
	hdr->number = hlog->hdr->cur_number;
	hdr->type = PLRT_NORMAL;
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
	fill_prn_number(hdr->lprn_number,
		((wm == wm_local) && cfg.has_lprn) ? (const char *)lprn_number : "",
		sizeof(hdr->lprn_number));
	memcpy(hdr->dsn, dsn, DS_NUMBER_LEN);
	hdr->ds_type = kt;
	hdr->crc32 = 0;
}

/* Занесение на БКЛ записи заданного типа. Возвращает номер записи */
uint32_t plog_write_rec(struct log_handle *hlog, uint8_t *data, uint32_t len,
		uint32_t type)
{
	plog_data_len = plog_data_index = 0;
	if (data != NULL){
		if (len > sizeof(plog_data)){
			logdbg("%s: Переполнение буфера данных при записи "
				"на %s (%u байт).\n", __func__, hlog->log_type, len);
			return -1U;
		}
		memcpy(plog_data, data, len);
		plog_data_len = len;
		if (type == PLRT_ERROR)
			recode_str((char *)plog_data, plog_data_len);
	}
	plog_init_rec_hdr(hlog, &plog_rec_hdr);
	plog_rec_hdr.type = type;
	plog_rec_hdr.len = len;
	plog_rec_hdr.crc32 = plog_rec_crc32();
	return plog_add_rec(hlog) ? plog_rec_hdr.number : -1UL;
}

/* Чтение заданной записи контрольной ленты */
bool plog_read_rec(struct log_handle *hlog, uint32_t index)
{
	uint32_t offs, crc;
	if (index >= hlog->hdr->n_recs)
		return false;
	plog_data_len = 0;
	plog_data_index = 0;
	offs = hlog->map[(hlog->map_head + index) % hlog->map_size].offset;
	if (!log_read(hlog, offs, (uint8_t *)&plog_rec_hdr, sizeof(plog_rec_hdr)))
		return false;
	if (plog_rec_hdr.len > sizeof(plog_data)){
		logdbg("%s: Слишком длинная запись %s #%u (%u байт).\n", __func__,
			hlog->log_type, index, plog_rec_hdr.len);
		return false;
	}
	offs = log_inc_index(hlog, offs, sizeof(plog_rec_hdr));
	if ((plog_rec_hdr.len > 0) && !log_read(hlog, offs,
			plog_data, plog_rec_hdr.len))
		return false;
	crc = plog_rec_hdr.crc32;
	plog_rec_hdr.crc32 = 0;
	if (plog_rec_crc32() != crc){
		logdbg("%s: Несовпадение контрольной суммы для записи %s #%u.\n",
			__func__, hlog->log_type, index);
		return false;
	}
	plog_rec_hdr.crc32 = crc;
	plog_data_len = plog_rec_hdr.len;
	return true;
}

/* Вывод заголовка распечатки БКЛ */
bool plog_print_header(void)
{
	log_reset_prn_buf();
	try_fn(prn_write_str("\x1e\x1eНАЧАЛО ПЕЧАТИ КОНТРОЛЬНОЙ ЛЕНТЫ "
		"(РАБОТА С ИПТ) "));
	try_fn(prn_write_cur_date_time());
	return prn_write_str("\x1c\x0b") && prn_write_eol();
}

/* Вывод концевика распечатки контрольной ленты */
bool plog_print_footer(void)
{
	log_reset_prn_buf();
	try_fn(prn_write_str("\x1e\x1e\x10\x0bКОНЕЦ ПЕЧАТИ КОНТРОЛЬНОЙ ЛЕНТЫ "
		"(РАБОТА С ИПТ) "));
	try_fn(prn_write_cur_date_time());
	return prn_write_str("\x1c\x0b") && prn_write_eol();
}

/* Вывод заголовка записи на печать */
static bool plog_print_rec_header(void)
{
	char s[128];
	snprintf(s, sizeof(s), "\x1e%.2hX%.2hX (\"ЭКСПРЕСС-2А-К\" %.4hX %.*s) "
			"ОПУ=%.*s ППУ=%.*s",
		(uint16_t)plog_rec_hdr.addr.gaddr, (uint16_t)plog_rec_hdr.addr.iaddr,
		plog_rec_hdr.term_check_sum,
		isizeof(plog_rec_hdr.tn), plog_rec_hdr.tn,
		isizeof(plog_rec_hdr.xprn_number), plog_rec_hdr.xprn_number,
		isizeof(plog_rec_hdr.lprn_number), plog_rec_hdr.lprn_number);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "ЗАПИСЬ %u ОТ ", plog_rec_hdr.number + 1);
	try_fn(prn_write_str(s));
	try_fn(prn_write_date_time(&plog_rec_hdr.dt));
	snprintf(s, sizeof(s), " ЖЕТОН %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX",
		ds_key_char(plog_rec_hdr.ds_type),
		plog_rec_hdr.dsn[7], plog_rec_hdr.dsn[0],
		plog_rec_hdr.dsn[6], plog_rec_hdr.dsn[5],
		plog_rec_hdr.dsn[4], plog_rec_hdr.dsn[3],
		plog_rec_hdr.dsn[2], plog_rec_hdr.dsn[1]);
	return prn_write_str(s) && prn_write_eol();
}

/* Можно ли выводить символ при печати БКЛ */
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
	int k = plog_data_index, l = 0;
	bool flag = true;	/* команду можно вывести на печать */
	bool loop_flag = true;
	if (cmd == XPRN_PRNOP){
		for (; loop_flag && (plog_data_index < plog_data_len); plog_data_index++){
			b = plog_data[plog_data_index];
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
	}else if ((cmd == XPRN_VPOS) && (plog_data_index < plog_data_len))
		flag = plog_data[plog_data_index++] >= 0x38;
	if (!flag)
		l = plog_data_index - k;
	plog_data_index = k;
	return l;
}

/* Вывод на печать команды работы со штрих-кодом */
static bool plog_print_bctl(uint8_t cmd)
{
	switch (cmd){
		case XPRN_WR_BCODE:
			return	prn_write_str("ПЧ ШТРИХ: ") &&
				log_print_bcode(plog_data, plog_data_len, &plog_data_index);
		case XPRN_RD_BCODE:
			return	prn_write_str("ЧТ ШТРИХ: ") &&
				log_print_bcode(plog_data, plog_data_len, &plog_data_index);
		case XPRN_NO_BCODE:
			plog_data_index--;
			return	prn_write_str("БЕЗ ШТРИХ");
		default:
			return false;
	}
}

/* Вывод строки БКЛ на печать */
static bool plog_print_line(void)
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
	int st = st_start, l;
	uint8_t b, cmd = 0;
	for (; (plog_data_index < plog_data_len) && (st != st_stop); plog_data_index++){
		b = plog_data[plog_data_index];
		switch (st){
			case st_start:
				try_fn(prn_write_char_raw('*'));
				st = st_regular;
				plog_data_index--;
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
				}else if (is_printable_char(b))
					try_fn(prn_write_char_raw(b));
				break;
			case st_dle:
				cmd = b;
				switch (b){
					case XPRN_WR_BCODE:
					case XPRN_RD_BCODE:
					case XPRN_NO_BCODE:
						st = st_bctl;
						break;
					default:
						st = st_cmd;
				}
				break;
			case st_bctl:
				try_fn(plog_print_bctl(cmd));
				if ((plog_data_index >= (plog_data_len - 1)) ||
						((plog_data[plog_data_index + 1] != 0x0a) &&
						 (plog_data[plog_data_index + 1] != 0x0d))){
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
				}
				plog_data_index += l - 1;
				st = st_regular;
				break;
			case st_wcr:
				if (b == 0x0d){
					try_fn(prn_write_char_raw(b));
					st = st_stop;
				}else{
					plog_data_index--;
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
					plog_data_index--;
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
static bool plog_print_plrt_normal(void)
{
	for (plog_data_index = 0; plog_data_index < plog_data_len;){
		if (!plog_print_line())
			break;
	}
	return plog_data_index == plog_data_len;
}

/* Вывод записи контрольной ленты на печать */
bool plog_print_rec(void)
{
	log_reset_prn_buf();
	try_fn(plog_print_rec_header());
	switch (plog_rec_hdr.type){
		case PLRT_NORMAL:
		case PLRT_ERROR:
			return plog_print_plrt_normal();
		default:
			return false;
	}
}
