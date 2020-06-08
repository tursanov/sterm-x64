/*
 * �㭪樨 ��� ࠡ��� � ����஫쭮� ���⮩ ��� (���).
 * (c) gsr 2004, 2008, 2009.
 */

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
#include "log/local.h"
#include "prn/local.h"
#include "cfg.h"
#include "express.h"
#include "gd.h"
#include "genfunc.h"
#include "paths.h"
#include "sterm.h"
#include "tki.h"
#include "transport.h"

/* ����� ⥪�饩 ����� ����஫쭮� ����� */
/* NB: ����� ���� �ᯮ������ ��� ��� �⥭��, ⠪ � ��� ����� */
uint8_t llog_data[LOG_BUF_LEN];
/* ����� ������ */
uint32_t llog_data_len;
/* ������ ⥪�饣� ��ࠡ��뢠����� ���� � llog_data */
uint32_t llog_data_index;

#define LLOG_MAP_MAX_SIZE	(LLOG_MAX_SIZE / sizeof(struct llog_rec_header))
static struct map_entry_t llog_map[LLOG_MAP_MAX_SIZE];

/* ��������� ����஫쭮� ����� */
static struct llog_header llog_hdr;
/* ��������� ⥪�饩 ����� ����஫쭮� ����� */
struct llog_rec_header llog_rec_hdr;

/* ���樠������ ��������� ����஫쭮� ����� (�ᯮ������ �� �� ᮧ�����) */
static void llog_init_hdr(struct log_handle *hlog)
{
	struct llog_header *hdr = (struct llog_header *)hlog->hdr;
	hdr->tag = LLOG_TAG;
	hdr->len = LLOG_MAX_SIZE;
	hdr->n_recs = 0;
	hdr->head = sizeof(*hdr);
	hdr->tail = sizeof(*hdr);
	hdr->cur_number = -1U;
}

/* ��뢠���� �� ���⪥ ����஫쭮� ����� */
static void llog_clear(struct log_handle *hlog)
{
	struct llog_header *hdr = (struct llog_header *)hlog->hdr;
	hdr->head = hdr->tail;
	hdr->n_recs = 0;
	hdr->cur_number = -1U;
}

/* ���᫥��� ����஫쭮� �㬬� CRC32 x^30 + x^27 + x^18 + x^3 + x^1 ����� ��1 */
static uint32_t llog_rec_crc32(void)
{
	uint8_t b;
	uint32_t i, j, s = 0, l = sizeof(llog_rec_hdr) + llog_rec_hdr.len;
	for (i = 0; i < l; i++){
		if (i < sizeof(llog_rec_hdr))
			b = *((uint8_t *)&llog_rec_hdr + i);
		else
			b = llog_data[i - sizeof(llog_rec_hdr)];
		for (j = 0; j < 8; j++){
			s >>= 1;
			s |=	(((b & 1) ^
				((s & (1 << 1))  >> 1)  ^
				((s & (1 << 3))  >> 3)  ^
				((s & (1 << 18)) >> 18) ^
				((s & (1 << 27)) >> 27) ^
				((s & (1 << 30)) >> 30)) << 31);
		}
	}
	return s;
}

/* ���������� ����� ����஫쭮� ����� */
static bool llog_fill_map(struct log_handle *hlog)
{
	struct llog_header *hdr = (struct llog_header *)hlog->hdr;
	uint32_t i, offs = hdr->head, tail, crc;
	memset(hlog->map, 0, hlog->map_size * sizeof(*hlog->map));
	hlog->map_head = 0;
	if (hdr->n_recs > LLOG_MAP_MAX_SIZE){
		fprintf(stderr, "���誮� ����� ����ᥩ �� %s; "
			"������ ���� �� ����� " _s(LLOG_MAP_MAX_SIZE) ".\n",
			hlog->log_type);
		return false;
	}
	for (i = 0; i < hdr->n_recs; i++){
		tail = offs;
		try_fn(log_read(hlog, offs, (uint8_t *)&llog_rec_hdr, sizeof(llog_rec_hdr)));
		hlog->map[i].offset = offs;
		hlog->map[i].number = llog_rec_hdr.number;
		hlog->map[i].dt = llog_rec_hdr.dt;
		hlog->map[i].tag = 0;
		offs = log_inc_index(hlog, offs, sizeof(llog_rec_hdr));
		if (llog_rec_hdr.tag != LLOG_REC_TAG){
			fprintf(stderr, "������ �ଠ� ��������� ����� %s #%u.\n",
				hlog->log_type, i);
			return log_truncate(hlog, i, tail);
		}
		switch (llog_rec_hdr.type){
			case LLRT_NORMAL:
			case LLRT_NOTIFY:
			case LLRT_INIT:
			case LLRT_BANK:
			case LLRT_ERROR:
			case LLRT_RD_ERROR:
			case LLRT_PR_ERROR_BCODE:
			case LLRT_PR_ERROR:
			case LLRT_EXPRESS:
			case LLRT_AUX:
			case LLRT_FOREIGN:
			case LLRT_SPECIAL:
			case LLRT_ERASE_SD:
			case LLRT_SD_ERROR:
			case LLRT_REJECT:
			case LLRT_SYNTAX_ERROR:
				break;
			default:
				fprintf(stderr, "������ %s #%u ����� ��������� ⨯: %.8x.\n",
					hlog->log_type, i, llog_rec_hdr.type);
				return log_truncate(hlog, i, tail);
		}
		if (llog_rec_hdr.len > LOG_BUF_LEN){
			fprintf(stderr, "���誮� ������� ������ %s #%u: %u ���� (max %u).\n",
				hlog->log_type, i, llog_rec_hdr.len, LOG_BUF_LEN);
			return log_truncate(hlog, i, tail);
		}
		llog_data_len = llog_rec_hdr.len;
		if (llog_data_len > 0)
			try_fn(log_read(hlog, offs, llog_data, llog_data_len));
		crc = llog_rec_hdr.crc32;
		llog_rec_hdr.crc32 = 0;
		if (llog_rec_crc32() != crc){
			fprintf(stderr, "��ᮢ������� ����஫쭮� �㬬� ��� ����� %s #%u.\n",
				hlog->log_type, i);
			return log_truncate(hlog, i, tail);
		}
		llog_rec_hdr.crc32 = crc;
		offs = log_inc_index(hlog, offs, llog_rec_hdr.len);
	}
	return true;
}

/* �⥭�� ��������� ����஫쭮� ����� */
static bool llog_read_header(struct log_handle *hlog)
{
	bool ret = false;
	if (log_atomic_read(hlog, 0, (uint8_t *)hlog->hdr, sizeof(struct llog_header))){
		if (hlog->hdr->tag != LLOG_TAG)
			fprintf(stderr, "������ �ଠ� ��������� %s.\n",
				hlog->log_type);
		else if (hlog->hdr->len > LLOG_MAX_SIZE)
			fprintf(stderr, "������ ࠧ��� %s.\n", hlog->log_type);
		else{
			off_t len = lseek(hlog->rfd, 0, SEEK_END);
			if (len == (off_t)-1)
				fprintf(stderr, "�訡�� ��।������ ࠧ��� 䠩�� %s.\n",
					hlog->log_type);
			else{
				hlog->full_len = hlog->hdr->len + sizeof(struct llog_header);
				if (hlog->full_len == len)
					ret = true;
				else
					fprintf(stderr, "������ %s, 㪠����� � ���������, �� ᮢ������ � ॠ��� ࠧ��஬.\n",
						hlog->log_type);
			}
		}
	}
	return ret;
}

/* ������ ��������� ����஫쭮� ����� */
static bool llog_write_header(struct log_handle *hlog)
{
	if (hlog->wfd == -1)
		return false;
	return log_atomic_write(hlog, 0, (uint8_t *)hlog->hdr,
		sizeof(struct llog_header));
}

/* ������� ��� ࠡ��� � 横���᪮� ����஫쭮� ���⮩ */
static struct log_handle _hllog = {
	.hdr		= (struct log_header *)&llog_hdr,
	.hdr_len	= sizeof(llog_hdr),
	.full_len	= sizeof(llog_hdr),
	.log_type	= "���",
	.log_name	= STERM_LLOG_NAME,
	.rfd		= -1,
	.wfd		= -1,
	.map		= llog_map,
	.map_size	= LLOG_MAP_MAX_SIZE,
	.map_head	= 0,
	.on_open	= NULL,
	.on_close	= NULL,
	.init_log_hdr	= llog_init_hdr,
	.clear		= llog_clear,
	.fill_map	= llog_fill_map,
	.read_header	= llog_read_header,
	.write_header	= llog_write_header
};

struct log_handle *hllog = &_hllog;

/*
 * ��।������ ������ ��ࢮ� ����� � ������� ����஬. �����頥� -1UL,
 * �᫨ ������ �� �������.
 */
uint32_t llog_first_for_number(struct log_handle *hlog, uint32_t number)
{
	uint32_t i;
	for (i = 0; i < hlog->hdr->n_recs; i++){
		if (hlog->map[(hlog->map_head + i) % hlog->map_size].number == number)
			return i;
	}
	return -1U;
}

/*
 * ��।������ ������ ��᫥���� ����� � ������� ����஬. �����頥� -1UL,
 * �᫨ ������ �� �������.
 */
uint32_t llog_last_for_number(struct log_handle *hlog, uint32_t number)
{
	uint32_t index = llog_first_for_number(hlog, number);
	if (index != -1U){
		for (; index < hlog->hdr->n_recs; index++){
			if (hlog->map[(hlog->map_head + index) % hlog->map_size].number != number)
				break;
		}
		return index - 1;
	}
	return -1U;
}

/* ��।������ ������ ����� �� ����ࠬ ����� � ����� */
uint32_t llog_index_for_number(struct log_handle *hlog, uint32_t number, uint32_t n_para)
{
	uint32_t index = llog_first_for_number(hlog, number);
	if (index != -1U){
		for (; (index < hlog->hdr->n_recs) && n_para; index++, n_para--){
			if (hlog->map[(hlog->map_head + index) % hlog->map_size].number != number)
				return -1U;
		}
	}
	return index;
}

/* ����� �� �⯥���� �������� ����ᥩ ����஫쭮� ����� */
bool llog_can_print_range(struct log_handle *hlog)
{
	return hlog->hdr->n_recs > 0;
}

/* ����� �� �ᯥ���� ����஫��� ����� ��������� */
bool llog_can_print(struct log_handle *hlog)
{
	return llog_can_print_range(hlog);
}

/* ����� �� �஢����� ���� �� ����஫쭮� ���� */
bool llog_can_find(struct log_handle *hlog)
{
	return hlog->hdr->n_recs > 0;
}

/*
 * ���������� ����� ����� � ����� ���. ����� ��室���� � llog_data,
 * ��������� -- � llog_rec_hdr.
 */
static bool llog_add_rec(struct log_handle *hlog)
{
	uint32_t rec_len, free_len, m, n, tail, offs;
	bool ret = false;
	rec_len = llog_rec_hdr.len + sizeof(llog_rec_hdr);
	if (rec_len > hlog->hdr->len){
		fprintf(stderr, "����� ����� (%u ����) �ॢ�蠥� ����� %s (%u ����).\n",
				rec_len, hlog->log_type, hlog->hdr->len);
		return false;
	}
	free_len = log_free_space(hlog);
/* �᫨ �� ����� �� 横������ �� �� 墠⠥� ����, 㤠�塞 ����� � ��砫� */
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
		fprintf(stderr, "�� 㤠���� �᢮������ ���� �� %s ��� ����� ������.\n",
				hlog->log_type);
		return false;
	}
/* n -- ��⠢襥�� �᫮ ����ᥩ �� ����஫쭮� ���� */
	if ((n + 1) > hlog->map_size){
		fprintf(stderr, "�ॢ�襭� ���ᨬ��쭮� �᫮ ����ᥩ �� %s;\n"
				"������ ���� �� ����� %u.\n",
			hlog->log_type, hlog->map_size);
		return false;
	}
/* ��稭��� ������ �� ����஫��� ����� */
	if (log_begin_write(hlog)){
		hlog->map_head = m;
		tail = (hlog->map_head + n) % hlog->map_size;
		hlog->map[tail].offset = hlog->hdr->tail;
		hlog->map[tail].number = llog_rec_hdr.number;
		hlog->map[tail].dt = llog_rec_hdr.dt;
		hlog->hdr->n_recs = n + 1;
		hlog->hdr->head = hlog->map[hlog->map_head].offset;
		offs = hlog->hdr->tail;
		hlog->hdr->tail = log_inc_index(hlog, hlog->hdr->tail,
				sizeof(llog_rec_hdr) + llog_rec_hdr.len);
		ret = hlog->write_header(hlog) &&
			log_write(hlog, offs, (uint8_t *)&llog_rec_hdr, sizeof(llog_rec_hdr)) &&
			((llog_rec_hdr.len == 0) ||
			log_write(hlog, log_inc_index(hlog, offs,
					sizeof(llog_rec_hdr)),
				llog_data, llog_rec_hdr.len));
	}
	log_end_write(hlog);
	return ret;
}

/*
 * ���������� ��������� ����� ���. ���� ⨯�, ����� � ����஫쭮� �㬬�
 * ����������� ���祭�ﬨ �� 㬮�砭��.
 */
static void llog_init_rec_hdr(struct log_handle *hlog,
		struct llog_rec_header *hdr, uint32_t n_para)
{
	hdr->tag = LLOG_REC_TAG;
	if (n_para == 0)
		hlog->hdr->cur_number++;
	hdr->number = hlog->hdr->cur_number;
	hdr->n_para = n_para;
	hdr->type = LLRT_NORMAL;
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
		cfg.has_lprn ? (const char *)lprn_number : "",
		sizeof(hdr->lprn_number));
	memcpy(hdr->dsn, dsn, DS_NUMBER_LEN);
	hdr->ds_type = kt;
	hdr->ZNtz = oldZNtz;
	hdr->ZNpo = oldZNpo;
	hdr->ZBp  = oldZBp;
	hdr->ONpo = ONpo;
	hdr->ONtz = ONtz;
	hdr->OBp  = OBp;
	hdr->reaction_time = (reaction_time + 50) / 100;
	hdr->printed = false;
	hdr->crc32 = 0;
}

/* ����ᥭ�� �� ��� ����� ��������� ⨯�. �����頥� ����� ����� */
uint32_t llog_write_rec(struct log_handle *hlog, uint8_t *data, uint32_t len,
		uint32_t type, uint32_t n_para)
{
	llog_data_len = llog_data_index = 0;
	if (data != NULL){
		if (len > sizeof(llog_data)){
			fprintf(stderr, "��९������� ���� ������ �� ����� "
				"�� %s (%u ����).\n", hlog->log_type, len);
			return -1U;
		}
		if (len > 0){
			memcpy(llog_data, data, len);
			llog_data_len = len;
		}
	}
	llog_init_rec_hdr(hlog, &llog_rec_hdr, n_para);
	llog_rec_hdr.type = type;
	llog_rec_hdr.len = len;
	llog_rec_hdr.crc32 = llog_rec_crc32();
	return llog_add_rec(hlog) ? llog_rec_hdr.number : -1UL;
}

/* ����ᥭ�� �� ��� ����� �� �訡�� ࠡ��� � ��� */
bool llog_write_error(struct log_handle *hlog, uint8_t code, uint32_t n_para)
{
	return llog_write_rec(hlog, &code, 1, LLRT_ERROR, n_para) != -1U;
}

/* ����ᥭ�� �� ��� ����� �� �訡�� �⥭�� ����� ������ */
bool llog_write_rd_error(struct log_handle *hlog, uint8_t code)
{
	CLR_FLAG(oldZBp, GDF_REQ_REJECT);
	return llog_write_rec(hlog, &code, 1, LLRT_RD_ERROR, 0) != -1U;
}

/* ����ᥭ�� �� ��� ����� �� �訡�� ���� �� ��� � ����஫�� ����-���� */
bool llog_write_pr_error_bcode(struct log_handle *hlog, uint8_t code, uint8_t *number,
		uint32_t n_para)
{
	uint8_t data[PRN_NUMBER_LEN + 1];
	data[0] = code;
	memcpy(data + 1, number, PRN_NUMBER_LEN);
	return llog_write_rec(hlog, data, sizeof(data), LLRT_PR_ERROR_BCODE, n_para) != -1U;
}

/* ����ᥭ�� �� ��� ����� �� �訡�� ���� �� ��� ��� ����஫� ����-���� */
bool llog_write_pr_error(struct log_handle *hlog, uint8_t code, uint32_t n_para)
{
	return llog_write_rec(hlog, &code, 1, LLRT_PR_ERROR, n_para) != -1U;
}

/* ����ᥭ�� �� ��� ����� � �㦮� �⢥� */
bool llog_write_foreign(struct log_handle *hlog, uint8_t gaddr, uint8_t iaddr)
{
	struct term_addr addr = {
		.gaddr	= gaddr,
		.iaddr	= iaddr
	};
	return llog_write_rec(hlog, (uint8_t *)&addr, sizeof(addr), LLRT_FOREIGN, 0) != -1U;
}

/* ����ᥭ�� �� ��� ����� �� ���⪥ ����� ����� */
bool llog_write_erase_sd(struct log_handle *hlog)
{
	return llog_write_rec(hlog, NULL, 0, LLRT_ERASE_SD, 0) != -1U;
}

/* ����ᥭ�� �� ��� ����� �� �訡�� ࠡ��� � ���⮩ ����� ��� */
bool llog_write_sd_error(struct log_handle *hlog, uint8_t code, uint32_t n_para)
{
	return llog_write_rec(hlog, &code, 1, LLRT_SD_ERROR, n_para) != -1U;
}

/* ����ᥭ�� �� ��� ����� � ᨭ⠪��᪮� �訡�� � �⢥� */
bool llog_write_syntax_error(struct log_handle *hlog, uint8_t code)
{
	size_t len = strlen(last_request);
	last_request[-1] = code;
	return llog_write_rec(hlog, (uint8_t *)last_request - 1, len + 1,
		LLRT_SYNTAX_ERROR, 0) != -1U;
}

/* �⥭�� �������� ����� ����஫쭮� ����� */
bool llog_read_rec(struct log_handle *hlog, uint32_t index)
{
	uint32_t offs, crc;
	if (index >= hlog->hdr->n_recs)
		return false;
	llog_data_len = 0;
	llog_data_index = 0;
	offs = hlog->map[(hlog->map_head + index) % hlog->map_size].offset;
	if (!log_read(hlog, offs, (uint8_t *)&llog_rec_hdr, sizeof(llog_rec_hdr)))
		return false;
	if (llog_rec_hdr.len > sizeof(llog_data)){
		fprintf(stderr, "���誮� ������� ������ %s #%u (%u ����).\n",
				hlog->log_type, index, llog_rec_hdr.len);
		return false;
	}
	offs = log_inc_index(hlog, offs, sizeof(llog_rec_hdr));
	if ((llog_rec_hdr.len > 0) && !log_read(hlog, offs, llog_data,
			llog_rec_hdr.len))
		return false;
	crc = llog_rec_hdr.crc32;
	llog_rec_hdr.crc32 = 0;
	if (llog_rec_crc32() != crc){
		fprintf(stderr, "��ᮢ������� ����஫쭮� �㬬� ��� ����� %s #%u.\n",
				hlog->log_type, index);
		return false;
	}
	llog_rec_hdr.crc32 = crc;
	llog_data_len = llog_rec_hdr.len;
	return true;
}

/* ��⠭���� �⬥⪨ � ���� ����� �� �� */
bool llog_mark_rec_printed(struct log_handle *hlog, uint32_t number, uint32_t n_para)
{
	bool ret = false;
	uint32_t index = llog_index_for_number(hlog, number, n_para);
	if ((index != -1U) && llog_read_rec(hlog, index)){
		llog_rec_hdr.printed = true;
		llog_rec_hdr.crc32 = 0;
		llog_rec_hdr.crc32 = llog_rec_crc32();
		if (log_begin_write(hlog))
			ret = log_write(hlog,
				hlog->map[(hlog->map_head + index) % hlog->map_size].offset,
				(uint8_t *)&llog_rec_hdr, sizeof(llog_rec_hdr));
		log_end_write(hlog);
	}
	return ret;
}

/* ��砫� ���� ��� */
bool llog_begin_print(void)
{
	static char data[] = {LPRN_DLE, LPRN_LOG, LPRN_STX};
	log_reset_prn_buf();
	return prn_write_chars_raw(data, sizeof(data));
}

/* ����� ���� ��� */
bool llog_end_print(void)
{
	return prn_write_char(LPRN_ETX);
}

/*
 * �뢮� ��������� �ᯥ�⪨ ��� �� ���.
 * full ��⠭���������� �� ���� ���������. � �⮬ ��砥 nr_rd_err --
 * ������⢮ ����ᥩ ⨯� LLRT_RD_ERROR � ���������.
 */
bool llog_print_header(bool full, uint32_t nr_rd_err)
{
	char s[128];
	try_fn(prn_write_str("\x19\x0b����������� ����� - ����������� ����������"));
	try_fn(prn_write_eol());
	try_fn(prn_write_chars_raw("\x1b\13306\x60\x07\x1e", -1));
	try_fn(prn_write_cur_date_time());
	try_fn(prn_write_eol());
	try_fn(prn_write_str("\x1e\x0b"));
	if (full){
		snprintf(s, sizeof(s), "%.2hhX%.2hhX (\"��������-2�-�\" "
				"%.4hX %.3u.%.3u.%.3u %.*s)",
			llog_rec_hdr.addr.gaddr, llog_rec_hdr.addr.iaddr,
			llog_rec_hdr.term_check_sum,
			VERSION_BRANCH(llog_rec_hdr.term_version),
			VERSION_RELEASE(llog_rec_hdr.term_version),
			VERSION_PATCH(llog_rec_hdr.term_version),
			isizeof(llog_rec_hdr.tn), llog_rec_hdr.tn);
		try_fn(prn_write_str(s));
		try_fn(prn_write_eol());
		snprintf(s, sizeof(s), "���=%.*s",
			isizeof(llog_rec_hdr.lprn_number), llog_rec_hdr.lprn_number);
		try_fn(prn_write_str(s));
		try_fn(prn_write_eol());
		if (nr_rd_err != 0){
			snprintf(s, sizeof(s), "���������� �������, "
				"���������\x9b� �� �������: %u.", nr_rd_err);
			try_fn(prn_write_str(s));
			try_fn(prn_write_eol());
		}
		return true;
	}else
		return prn_write_eol();
/*	snprintf(s, sizeof(s), "����� %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX"
			"%.2hhX%.2hhX%.2hhX",
		ds_key_char(llog_rec_hdr.ds_type),
		llog_rec_hdr.dsn[7], llog_rec_hdr.dsn[0],
		llog_rec_hdr.dsn[6], llog_rec_hdr.dsn[5],
		llog_rec_hdr.dsn[4], llog_rec_hdr.dsn[3],
		llog_rec_hdr.dsn[2], llog_rec_hdr.dsn[1]);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "������ %lu/%lu �� ",
		llog_rec_hdr.number + 1, llog_rec_hdr.n_para + 1);
	try_fn(prn_write_str(s));
	try_fn(prn_write_date_time(&llog_rec_hdr.dt));
	return prn_write_str("\x19\x0b") && prn_write_eol() && prn_write_eol();*/
}

/* �뢮� ��������� �ᯥ�⪨ ��� �� ��� */
bool llog_print_header_xprn(void)
{
	log_reset_prn_buf();
	try_fn(prn_write_str("\x1e\x0b������ ������ ����������� ����\x9b "
		"(����������� ����������) "));
	try_fn(prn_write_cur_date_time());
	return prn_write_str("\x1c\x0b") && prn_write_eol();
}

/* �뢮� ���楢��� �ᯥ�⪨ ����஫쭮� ����� �� ��� */
bool llog_print_footer(void)
{
	return	prn_write_str("\x1e\x0b����� ������ ����������� ����\x9b "
			"(����������� ����������)\x19\x0b") &&
		prn_write_eol() &&
		prn_write_chars_raw("\x1b\13306\x60\x07\x1e", -1) &&
		prn_write_cur_date_time() &&
		prn_write_eol() &&
		prn_write_char_raw(LPRN_FORM_FEED);
}

/* �뢮� ���楢��� �ᯥ�⪨ ����஫쭮� ����� �� ��� */
bool llog_print_footer_xprn(void)
{
	log_reset_prn_buf();
	try_fn(prn_write_str("\x1e\x0b����� ������ ����������� ����\x9b "
		"(����������� ����������) "));
	try_fn(prn_write_cur_date_time());
	return prn_write_str("\x1c\x0b") && prn_write_eol();
}

/* �뢮� ��������� ����� �� ��� */
bool llog_print_rec_header(void)
{
	char s[128];
	snprintf(s, sizeof(s), "\x1e\x0b%.2hhX%.2hhX (\"��������-2�-�\" "
			"%.4hX %.*s) ���=%.*s",
		llog_rec_hdr.addr.gaddr, llog_rec_hdr.addr.iaddr,
		llog_rec_hdr.term_check_sum,
		isizeof(llog_rec_hdr.tn), llog_rec_hdr.tn,
		isizeof(llog_rec_hdr.lprn_number), llog_rec_hdr.lprn_number);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "����� %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX"
			"%.2hhX%.2hhX%.2hhX",
		ds_key_char(llog_rec_hdr.ds_type),
		llog_rec_hdr.dsn[7], llog_rec_hdr.dsn[0],
		llog_rec_hdr.dsn[6], llog_rec_hdr.dsn[5],
		llog_rec_hdr.dsn[4], llog_rec_hdr.dsn[3],
		llog_rec_hdr.dsn[2], llog_rec_hdr.dsn[1]);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "ZN��: %.3hX  ZN��: %.3hX  Z��: %.2hhX  "
			"ON��: %.3hX  ON��: %.3hX  O��: %.2hhX",
		llog_rec_hdr.ZNtz, llog_rec_hdr.ZNpo, llog_rec_hdr.ZBp,
		llog_rec_hdr.ONpo, llog_rec_hdr.ONtz, llog_rec_hdr.OBp);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "����� ������� %.3d ���.",
			llog_rec_hdr.reaction_time);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "������ %u/%u �� ",
		llog_rec_hdr.number + 1, llog_rec_hdr.n_para + 1);
	return	prn_write_str(s) &&
		prn_write_date_time(&llog_rec_hdr.dt) &&
		prn_write_str(llog_rec_hdr.printed ? " [+]" : "") &&
		prn_write_str("\x19\x0b") &&
		prn_write_eol() &&
		prn_write_char_raw(LPRN_FORM_FEED);
}

/* �뢮� ���⪮�� ��������� ����� �� ��� �� ���� ��������� */
bool llog_print_rec_header_short(void)
{
	char s[128];
	snprintf(s, sizeof(s), "\x1e\x0b%.2hhX:%.2hhX  ������ %u/%u �� ",
		llog_rec_hdr.addr.gaddr, llog_rec_hdr.addr.iaddr,
		llog_rec_hdr.number + 1, llog_rec_hdr.n_para + 1);
	return	prn_write_str(s) &&
		prn_write_date_time(&llog_rec_hdr.dt) &&
		prn_write_str(llog_rec_hdr.printed ? " [+]" : "") &&
		prn_write_str("\x19\x0b") &&
		prn_write_eol() &&
		prn_write_char_raw(LPRN_FORM_FEED);
}

/* �뢮� ��������� ����� �� ��� */
static bool llog_print_rec_header_xprn(void)
{
	char s[128];
	snprintf(s, sizeof(s), "\x1e\x0b%.2hhX%.2hhX "
			"(\"��������-2�-�\" %.4hX %.*s) ���=%.*s ���=%.*s",
		llog_rec_hdr.addr.gaddr, llog_rec_hdr.addr.iaddr,
		llog_rec_hdr.term_check_sum,
		isizeof(llog_rec_hdr.tn), llog_rec_hdr.tn,
		isizeof(llog_rec_hdr.xprn_number), llog_rec_hdr.xprn_number,
		isizeof(llog_rec_hdr.aprn_number), llog_rec_hdr.aprn_number);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "������ %u/%u �� ",
		llog_rec_hdr.number + 1, llog_rec_hdr.n_para + 1);
	try_fn(prn_write_str(s));
	try_fn(prn_write_date_time(&llog_rec_hdr.dt));
	snprintf(s, sizeof(s), " ����� %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX",
		ds_key_char(llog_rec_hdr.ds_type),
		llog_rec_hdr.dsn[7], llog_rec_hdr.dsn[0],
		llog_rec_hdr.dsn[6], llog_rec_hdr.dsn[5],
		llog_rec_hdr.dsn[4], llog_rec_hdr.dsn[3],
		llog_rec_hdr.dsn[2], llog_rec_hdr.dsn[1]);
	if (llog_rec_hdr.printed)
		strcat(s, " [+]");
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "ZN��: %.3hX  ZN��: %.3hX  Z��: %.2hhX      "
			"ON��: %.3hX  ON��: %.3hX  O��: %.2hhX    "
			"����� ������� %.3d ���.\x1c\x0b",
		llog_rec_hdr.ZNtz, llog_rec_hdr.ZNpo, llog_rec_hdr.ZBp,
		llog_rec_hdr.ONpo, llog_rec_hdr.ONtz, llog_rec_hdr.OBp,
		llog_rec_hdr.reaction_time);
	return prn_write_str(s) && prn_write_eol();
}

/* �뢮� ���楢��� ����� �� ��� */
bool llog_print_rec_footer(void)
{
	return prn_write_char_raw(LPRN_FORM_FEED);
}

/* �뢮� �� ����� ����� ⨯� LLRT_NOTIFY */
static bool llog_print_llrt_notify(void)
{
	return prn_write_str("����� ��� ������") && prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_INIT */
static bool llog_print_llrt_init(void)
{
	return prn_write_str("�������������") && prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_BANK */
static bool llog_print_llrt_bank(void)
{
	return	prn_write_str("����") && prn_write_eol() &&
/* ����� ������ � ��⥬� */
		prn_write_chars_raw((char *)llog_data, 7) &&
		prn_write_eol() &&
/* ��孮�����᪨� ����� ����� */
		prn_write_chars_raw((char *)llog_data + 7, 5) &&
		prn_write_eol() &&
/* �㬬� ������ */
		prn_write_chars_raw((char *)llog_data + 12, 9) &&
		prn_write_eol();
}

/* ����祭�� ���ᠭ�� ���� �訡�� LLRT_ERROR �� �� ���� */
const char *llog_get_llrt_error_txt(uint8_t code)
{
	static struct {
		uint8_t code;
		const char *txt;
	} map[] = {
		{LLRT_ERROR_TIMEOUT,	"�������"},
		{LLRT_ERROR_MEDIA,	"������\x9b� ��� ��������"},
		{LLRT_ERROR_GENERIC,	"����� ������"},
		{0,			"����������� ������"},
	};
	int i;
	for (i = 0; i < ASIZE(map); i++){
		if (code == map[i].code)
			return map[i].txt;
	}
	return map[ASIZE(map) - 1].txt;
}

/* �뢮� �� ����� ����� ⨯� LLRT_ERROR */
static bool llog_print_llrt_error(void)
{
	if (llog_data_len == 1){
		if (llog_data[0] == LLRT_ERROR_MEDIA)
			try_fn(prn_write_str("\x1e\x0b������ �����\x9b � ���:\x19\x0b"));
		else
			prn_write_str("\x1e\x0b������ �����\x9b � ���:\x19\x0b");
		return	prn_write_eol() &&
			prn_write_str(llog_get_llrt_error_txt(llog_data[0])) &&
			prn_write_eol();
	}else
		return false;
}

/* �뢮� �� ����� ����� ⨯� LLRT_RD_ERROR */
static bool llog_print_llrt_rd_error(void)
{
	char s[128];
	struct lprn_error_txt *err;
	if (llog_data_len != 1)
		return false;
	try_fn(prn_write_str("������"));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "\x1e\x0b������ ������ ������ ������: %.2hhX\x19\x0b",
		llog_data[0]);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	err = lprn_get_error_txt(llog_data[0]);
	if (err == NULL)
		return false;
	return prn_write_str(err->txt) && prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_PR_ERROR_BCODE */
static bool llog_print_llrt_pr_error_bcode(void)
{
	char s[128];
	struct lprn_error_txt *err;
	if (llog_data_len != (LPRN_BLANK_NUMBER_LEN + 1))
		return false;
	snprintf(s, sizeof(s), "\x1e\x0b������ ������ �� ��� � ��������� "
		"�����-����: %.2hhX\x19\x0b", llog_data[0]);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	err = lprn_get_error_txt(llog_data[0]);
	if (err == NULL)
		return false;
	try_fn(prn_write_str(err->txt));
	try_fn(prn_write_eol());
	snprintf(s, sizeof(s), "����� ������ %.*s", LPRN_BLANK_NUMBER_LEN,
		llog_data + 1);
	return prn_write_str(s) && prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_PR_ERROR */
static bool llog_print_llrt_pr_error(void)
{
	char s[128];
	struct lprn_error_txt *err;
	if (llog_data_len != 1)
		return false;
	snprintf(s, sizeof(s), "\x1e\x0b������ ������ �� ��� ��� �������� "
		"�����-����: %.2hhX\x19\x0b", llog_data[0]);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	err = lprn_get_error_txt(llog_data[0]);
	if (err == NULL)
		return false;
	return prn_write_str(err->txt) && prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_FOREIGN */
static bool llog_print_llrt_foreign(void)
{
	struct term_addr *addr = (struct term_addr *)llog_data;
	char s[128];
	if (llog_data_len == sizeof(*addr))
		snprintf(s, sizeof(s), "\x1e\x0b������� ����� ��� ������� "
			"��������� (%.2hhX:%2hhX)\x19\x0b",
			addr->gaddr, addr->iaddr);
	else
		snprintf(s, sizeof(s), "\x1e\x0b������� ����� ��� ������� "
			"��������� (**:**)\x19\x0b");
	return prn_write_str(s) && prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_SPECIAL */
static bool llog_print_llrt_special(void)
{
	char s[128];
	llog_data_index = 0;
	snprintf(s, sizeof(s), "������ ����������� ����\x9b �:");
	return	prn_write_str(s) &&
		prn_write_char_raw(llog_data[llog_data_index++]) &&
		prn_write_char_raw(llog_data[llog_data_index]) &&
		prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_ERASE_SD */
static bool llog_print_llrt_erase_sd(void)
{
	return	prn_write_str("����� ������ �������") &&
		prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_SD_ERROR */
static bool llog_print_llrt_sd_error(void)
{
	char s[128];
	struct lprn_error_txt *err;
	if (llog_data_len != 1)
		return false;
	snprintf(s, sizeof(s), "\x1e\x0b������ �����\x9b � ������ ������: "
		"%.2hhX\x19\x0b", llog_data[0]);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	err = lprn_get_sd_error_txt(llog_data[0]);
	if (err == NULL)
		return false;
	return prn_write_str(err->txt) && prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_REJECT */
static bool llog_print_llrt_reject(void)
{
	return prn_write_str("����� �� ������") && prn_write_eol();
}

/* �뢮� �� ����� ����� ⨯� LLRT_SYNTAX_ERROR */
static bool llog_print_llrt_syntax_error(void)
{
	char s[128];
	uint8_t code;
	const char *err_msg, *slice;
	int i;
	if (llog_data_len == 0)
		return false;
	code = llog_data[0];
	snprintf(s, sizeof(s), "�������������� ������ � ������ -- �:%.2hhd", code);
	try_fn(prn_write_str(s));
	try_fn(prn_write_eol());
	err_msg = get_syntax_error_txt(code);
	if (err_msg != NULL){
		for (slice = err_msg; *slice != 0;){
			try_fn(prn_write_str(slice));
			try_fn(prn_write_eol());
			slice += strlen(slice) + 1;
		}
	}
	try_fn(prn_write_str("***������***"));
	try_fn(prn_write_eol());
#define LPRN_MAX_COLS		40
	for (i = 1; i < llog_data_len; i += LPRN_MAX_COLS){
		int len = llog_data_len - i;
		if (len > LPRN_MAX_COLS)
			len = LPRN_MAX_COLS;
		try_fn(prn_write_chars_raw((char *)llog_data + i, len));
		try_fn(prn_write_eol());
	}
	return true;
}

/* ����� �� �뢮���� ᨬ��� �� ���� ����஫쭮� ����� */
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
 * ����� �� �뢮���� ������� �� ���� ����஫쭮� �����.
 * �����頥� ������⢮ ᨬ�����, ����� �� ���� ������. �᫨ �������
 * ����� ��������, �����頥� 0.
 */
static int get_unprintable_len(uint8_t cmd)
{
	int k = llog_data_index, l = 0;
	bool flag = true;	/* ������� ����� �뢥�� �� ����� */
	bool loop_flag = true;
	if (cmd == LPRN_POS){
		for (; loop_flag && (llog_data_index < llog_data_len); llog_data_index++){
			switch (llog_data[llog_data_index]){
				case LPRN_PRNOP_VPOS_ABS:
					flag = false;		/* fall through */
				case LPRN_PRNOP_HPOS_ABS:
					loop_flag = false;
					break;
			}
		}
	}
	if (!flag)
		l = llog_data_index - k;
	llog_data_index = k;
	return l;
}

/* �뢮� �� ����� ������� ����ᥭ�� ����-���� LPRN_WR_BCODE2 */
static bool llog_print_bcode2(void)
{
	enum {
		st_type,
		st_x,
		st_y,
		st_number,
		st_len,
		st_data,
		st_stop,
	};
	int st = st_type, n = 0, data_len = 0;
	uint8_t b;
	for (; (llog_data_index < llog_data_len) && (st != st_stop); llog_data_index++){
		b = llog_data[llog_data_index];
		switch (st){
			case st_type:
				if (isdigit(b)){
					n = 0;
					st = st_x;
				}else
					return false;
				break;
			case st_x:
				if ((n == 0) && (b == 0x3b))
					st = st_number;
				else if (!isdigit(b))
					return false;
				else if (++n == 3){
					n = 0;
					st = st_y;
				}
				break;
			case st_y:
				if (!isdigit(b))
					return false;
				else if (++n == 3){
					n = 0;
					st = st_len;
				}
				break;
			case st_number:
				if ((b != 0x31) && (b != 0x32) && (b != 0x33))
					return false;
				n = 0;
				st = st_len;
				break;
			case st_len:
				if (!isdigit(b))
					return false;
				data_len *= 10;
				data_len += b - 0x30;
				if (++n == 3){
					if (data_len == 0)
						return false;
					else{
						n = 0;
						st = st_data;
					}
				}
				break;
			case st_data:
				if (++n == data_len)
					st = st_stop;
				break;
		}
		try_fn(prn_write_char_raw(b));
	}
	if (st == st_stop){
		llog_data_index--;
		return true;
	}else
		return false;
}

/* �뢮� �� ����� ������� ࠡ��� � ����-����� */
static bool llog_print_bctl(uint8_t cmd)
{
	switch (cmd){
		case LPRN_WR_BCODE1:
			return	prn_write_str("�� �����1: ") &&
				log_print_bcode(llog_data, llog_data_len, &llog_data_index);
		case LPRN_WR_BCODE2:
			return	prn_write_str("�� �����2: ") &&
				llog_print_bcode2();
		case LPRN_RD_BCODE:
			return	prn_write_str("�� �����: ") &&
				log_print_bcode(llog_data, llog_data_len, &llog_data_index);
		case LPRN_NO_BCODE:
			llog_data_index--;
			return	prn_write_str("��� �����");
		default:
			return false;
	}
}

/* �뢮� ��ப� ����஫쭮� ����� �� ����� */
static bool llog_print_line(void)
{
	enum {
		st_regular,
		st_dle,
		st_bctl,
		st_cmd,
		st_wcr,
		st_wlf,
		st_stop,
	};
	int st = st_regular, m = 0, l;
	uint8_t b, cmd = 0;
	for (; (llog_data_index < llog_data_len) && (st != st_stop); llog_data_index++){
		b = llog_data[llog_data_index];
		switch (st){
			case st_regular:
				if (b == LPRN_DLE)
					st = st_dle;
				else if (b == LPRN_FORM_FEED){
					if (m > 0)
						try_fn(prn_write_eol());
					st = st_stop;
				}else{
					bool flag = is_printable_char(b);
					if (b == 0x0a)
						st = st_wcr;
					else if (b == 0x0d)
						st = st_wlf;
					else if (flag)
						m++;
					if (flag)
						try_fn(prn_write_char_raw(b));
				}
				break;
			case st_dle:
				cmd = b;
				switch (b){
					case LPRN_WR_BCODE1:
					case LPRN_WR_BCODE2:
					case LPRN_RD_BCODE:
					case LPRN_NO_BCODE:
						if (m == 0)
							st = st_bctl;
						else{
							try_fn(prn_write_eol());
							llog_data_index -= 2;
							st = st_stop;
						}
						break;
					default:
						st = st_cmd;
				}
				break;
			case st_bctl:
				try_fn(llog_print_bctl(cmd));
				if ((llog_data_index >= (llog_data_len - 1)) ||
						((llog_data[llog_data_index + 1] != 0x0a) &&
						 (llog_data[llog_data_index + 1] != 0x0d))){
					try_fn(prn_write_eol());
					st = st_stop;
				}else
					st = st_regular;
				break;
			case st_cmd:
				l = get_unprintable_len(cmd);
				if (l == 0){
					try_fn(prn_write_char_raw(LPRN_DLE));
					try_fn(prn_write_char_raw(cmd));
				}
				llog_data_index += l - 1;
				st = st_regular;
				break;
			case st_wcr:
				if (b == 0x0d){
					try_fn(prn_write_char_raw(b));
					st = st_stop;
				}else{
					llog_data_index--;
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
					llog_data_index--;
					st = st_stop;
				}
				break;
		}
	}
	return (st == st_stop) ? true : prn_write_eol();
}

/* �뢮� �� ����� ���筮� ����� */
static bool llog_print_llrt_normal(void)
{
	for (llog_data_index = 0; llog_data_index < llog_data_len;){
		if (!llog_print_line())
			break;
	}
	return llog_data_index == llog_data_len;
}

/* �뢮� ����� ����஫쭮� ����� �� ����� */
bool llog_print_rec(bool reset, bool need_header)
{
	bool ret = false;
/*	bool native = (llog_rec_hdr.type != LLRT_EXPRESS) &&
		(llog_rec_hdr.type != LLRT_AUX);*/
	if (reset)
		log_reset_prn_buf();
	if (need_header){
		if (llog_rec_hdr.type == LLRT_EXPRESS)
			try_fn(llog_print_rec_header_xprn());
		else
			try_fn(llog_print_rec_header());
	}/*else if (native)
		try_fn(llog_print_rec_header_short());*/
	switch (llog_rec_hdr.type){
		case LLRT_NORMAL:
			ret = llog_print_llrt_normal();
			break;
		case LLRT_NOTIFY:
			ret = llog_print_llrt_notify();
			break;
		case LLRT_INIT:
			ret = llog_print_llrt_init();
			break;
		case LLRT_BANK:
			ret = llog_print_llrt_bank();
			break;
		case LLRT_ERROR:
			ret = llog_print_llrt_error();
			break;
		case LLRT_RD_ERROR:
			ret = llog_print_llrt_rd_error();
			break;
		case LLRT_PR_ERROR_BCODE:
			ret = llog_print_llrt_pr_error_bcode();
			break;
		case LLRT_PR_ERROR:
			ret = llog_print_llrt_pr_error();
			break;
		case LLRT_EXPRESS:
			return xlog_print_llrt_normal();
		case LLRT_AUX:
			return true;
		case LLRT_FOREIGN:
			ret = llog_print_llrt_foreign();
			break;
		case LLRT_SPECIAL:
			ret = llog_print_llrt_special();
			break;
		case LLRT_ERASE_SD:
			ret = llog_print_llrt_erase_sd();
			break;
		case LLRT_SD_ERROR:
			ret = llog_print_llrt_sd_error();
			break;
		case LLRT_REJECT:
			ret = llog_print_llrt_reject();
			break;
		case LLRT_SYNTAX_ERROR:
			ret = llog_print_llrt_syntax_error();
			break;
	}
	return ret ? llog_print_rec_footer() : ret;
}
