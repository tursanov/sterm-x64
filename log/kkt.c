/* Функции для работы с контрольной лентой ККТ (ККЛ, КЛ3). (c) gsr 2018-2019 */

#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "kkt/kkt.h"
#include "log/kkt.h"
#include "log/logdbg.h"
#include "cfg.h"
#include "express.h"
#include "paths.h"
#include "sterm.h"
#include "tki.h"

/* Данные текущей записи контрольной ленты */
/* NB: данный буфер используется как для чтения, так и для записи */
uint8_t klog_data[LOG_BUF_LEN] = {[0 ... LOG_BUF_LEN - 1] = 0};
/* Длина данных */
uint32_t klog_data_len = 0;
/* Индекс текущего обрабатываемого байта в log_data */
uint32_t klog_data_index = 0;

/* Используется для разграничения доступа к ККЛ */
pthread_mutex_t klog_mtx = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

#define KLOG_MAP_MAX_SIZE	(KLOG_MAX_SIZE / sizeof(struct klog_rec_header))
static struct map_entry_t klog_map[KLOG_MAP_MAX_SIZE];

/* Заголовок контрольной ленты */
struct klog_header klog_hdr;
/* Заголовок текущей записи контрольной ленты */
struct klog_rec_header klog_rec_hdr;

/*
 * Запись на ККЛ может происходить как из основного потока, так и из потока
 * работы с ОФД. При этом необходимо сохранить данные, находящиеся в klog_data
 * на момент начала записи. Для этого они помещаются в буфер, а по окончании
 * записи восстанавливаются.
 */
static struct klog_rec_header klog_rec_hdr_orig;
static uint8_t klog_data_orig[sizeof(klog_data)];
static uint32_t klog_data_len_orig = 0;
static uint32_t klog_data_index_orig = 0;

static bool klog_data_stored = false;

static void klog_push_data(void)
{
	klog_rec_hdr_orig = klog_rec_hdr;
	if (klog_data_len > 0)
		memcpy(klog_data_orig, klog_data, klog_data_len);
	klog_data_len_orig = klog_data_len;
	klog_data_index_orig = klog_data_index;
	klog_data_stored = true;
}

static void klog_pop_data(void)
{
	if (klog_data_stored){
		klog_rec_hdr = klog_rec_hdr_orig;
		if (klog_data_len_orig > 0)
			memcpy(klog_data, klog_data_orig, klog_data_len_orig);
		klog_data_len = klog_data_len_orig;
		klog_data_index = klog_data_index_orig;
		klog_data_stored = false;
	}
}

/* Инициализация заголовка контрольной ленты (используется при ее создании) */
static void klog_init_hdr(struct log_handle *hlog)
{
	struct klog_header *hdr = (struct klog_header *)hlog->hdr;
	hdr->tag = KLOG_TAG;
	hdr->len = KLOG_MAX_SIZE;
	hdr->n_recs = 0;
	hdr->head = sizeof(*hdr);
	hdr->tail = sizeof(*hdr);
	hdr->cur_number = -1U;
}

/* Вызывается при очистке контрольной ленты */
static void klog_clear(struct log_handle *hlog)
{
	struct klog_header *hdr = (struct klog_header *)hlog->hdr;
	hdr->head = hdr->tail;
	hdr->n_recs = 0;
	hdr->cur_number = -1U;
}

/* CRC32 вычисляется по полиному 0x04c11db7 (IEEE 802.3). Для вычисления используется таблица */

static uint32_t crc32_tbl[256];
static bool crc32_tbl_ready = false;

static void init_crc32_tbl(void)
{
	for (uint32_t i = 0; i < 256; i++){
		uint32_t crc = i;
		for (int j = 0; j < 8; j++)
			crc = (crc & 1) ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
		crc32_tbl[i] = crc;
	}
	crc32_tbl_ready = true;
}

static inline void init_crc32_tbl_if_need(void)
{
	if (!crc32_tbl_ready)
		init_crc32_tbl();
}

static uint32_t klog_rec_crc32(void)
{
	init_crc32_tbl_if_need();
	uint32_t crc32 = 0xffffffff;
	size_t len = sizeof(klog_rec_hdr) + KLOG_REC_LEN(klog_rec_hdr.len);
	for (int i = 0; i < len; i++){
		uint8_t b = (i < sizeof(klog_rec_hdr)) ?
			*((uint8_t *)&klog_rec_hdr + i) :
			klog_data[i - sizeof(klog_rec_hdr)];
		crc32 = crc32_tbl[(crc32 ^ b) & 0xff] ^ (crc32 >> 8);
	}
	return crc32 ^= 0xffffffff;
}

/* Последняя запись ККЛ -- пустая инструкция ОФД */
static bool last_is_fdo_empty = false;

/* Является ли запись пустой инструкцией ОФД */
static inline bool is_fdo_empty(uint32_t stream, uint8_t cmd, uint8_t status)
{
	return	(stream == KLOG_STREAM_FDO) && (cmd == KKT_SRV_FDO_REQ) &&
		(status == FDO_REQ_NOP);
}

/* Код предыдущей операции в пустой инструкции ОФД */
static uint8_t fdo_empty_prev_op = UINT8_MAX;
/* Код завершения предыдущей операции в пустой инструкции ОФД */
static uint16_t fdo_empty_prev_op_status = 0;

/* Чтение данных инструкции ОФД */
static bool fdo_read_empty_data(const uint8_t *data, uint8_t *op, uint16_t *status)
{
	assert(data != NULL);
	assert(op != NULL);
	assert(status != NULL);
	bool ret = true, dash = (*data == 0x2d);
	*op = 0;
	*status = 0;
	uint8_t _op = 0;
	uint16_t _status = 0;
	for (int i = 0; i < 6; i++){
		uint8_t b = data[i];
		if (i == 0)
			_op = dash ? FDO_REQ_NOP : b;
		else if (i == 1){
			if (b != 0x3a){
				ret = false;
				break;
			}
		}else if (dash){
			if (b != 0x2d){
				ret = false;
				break;
			}
		}else if ((b >= 0x30) && (b <= 0x39)){
			_status *= 10;
			_status += b - 0x30;
		}else{
			ret = false;
			break;
		}
	}
	if (ret){
		*op = _op;
		*status = _status;
	}
	return ret;
}

/* Заполнение карты контрольной ленты */
static bool klog_fill_map(struct log_handle *hlog)
{
	struct klog_header *hdr = (struct klog_header *)hlog->hdr;
	memset(hlog->map, 0, hlog->map_size * sizeof(*hlog->map));
	hlog->map_head = 0;
	last_is_fdo_empty = false;
	if (hdr->n_recs > KLOG_MAP_MAX_SIZE){
		logdbg("%s: Слишком много записей на %s; "
			"должно быть не более " _s(KLOG_MAP_MAX_SIZE) ".\n", __func__,
			hlog->log_type);
		return false;
	}
	for (uint32_t i = 0, offs = hdr->head; i < hdr->n_recs; i++){
		uint32_t tail = offs;
		try_fn(log_read(hlog, offs, (uint8_t *)&klog_rec_hdr, sizeof(klog_rec_hdr)));
		if (klog_rec_hdr.tag != KLOG_REC_TAG){
			logdbg("%s: Неверный формат заголовка записи %s #%u (%.8x).\n",
				__func__, hlog->log_type, i, klog_rec_hdr.tag);
			return log_truncate(hlog, i, tail);
		}else if (KLOG_REC_LEN(klog_rec_hdr.len) > LOG_BUF_LEN){
			logdbg("%s: Слишком длинная запись %s #%u: %u байт (max %u).\n",
				__func__, hlog->log_type, i,
				KLOG_REC_LEN(klog_rec_hdr.len), LOG_BUF_LEN);
			return log_truncate(hlog, i, tail);
		}else if (KLOG_REC_LEN(klog_rec_hdr.len) !=
				(klog_rec_hdr.req_len + klog_rec_hdr.resp_len)){
			logdbg("%s: Неверные данные о длине записи %s #%u: %u != %u + %u.\n",
				__func__, hlog->log_type, i, KLOG_REC_LEN(klog_rec_hdr.len),
				klog_rec_hdr.req_len, klog_rec_hdr.resp_len);
			return log_truncate(hlog, i, tail);
		}
		hlog->map[i].offset = offs;
		hlog->map[i].number = klog_rec_hdr.number;
		hlog->map[i].dt = klog_rec_hdr.dt;
		hlog->map[i].tag = KLOG_STREAM(klog_rec_hdr.stream);
		offs = log_inc_index(hlog, offs, sizeof(klog_rec_hdr));
		klog_data_len = KLOG_REC_LEN(klog_rec_hdr.len);
		if (klog_data_len > 0)
			try_fn(log_read(hlog, offs, klog_data, klog_data_len));
		uint32_t crc = klog_rec_hdr.crc32;
		klog_rec_hdr.crc32 = 0;
		if (klog_rec_crc32() != crc){
			logdbg("%s: Несовпадение контрольной суммы для записи %s #%u (0x%.8x).\n",
				__func__, hlog->log_type, i, offs);
			return log_truncate(hlog, i, tail);
		}
		klog_rec_hdr.crc32 = crc;
		offs = log_inc_index(hlog, offs, KLOG_REC_LEN(klog_rec_hdr.len));
		if ((i + 1) == hdr->n_recs){
			last_is_fdo_empty = is_fdo_empty(KLOG_STREAM(klog_rec_hdr.stream),
					klog_rec_hdr.cmd, klog_rec_hdr.status) &&
				(klog_rec_hdr.req_len == 9) && 
				fdo_read_empty_data(klog_data + 3, &fdo_empty_prev_op,
					&fdo_empty_prev_op_status);
/*			logdbg("%s: последняя запись %s -- пустой опрос ОФД "
				"(op = %.2hhx; status = %.4u).\n", __func__,
				hlog->log_type, fdo_empty_prev_op, fdo_empty_prev_op_status);*/
		}
	}
	return true;
}

/* Чтение заголовка контрольной ленты */
static bool klog_read_header(struct log_handle *hlog)
{
	bool ret = false;
	if (log_atomic_read(hlog, 0, (uint8_t *)hlog->hdr, sizeof(struct klog_header))){
		if (hlog->hdr->tag != KLOG_TAG)
			logdbg("%s: Неверный формат заголовка %s.\n", __func__,
				hlog->log_type);
		else if (hlog->hdr->len > KLOG_MAX_SIZE)
			logdbg("%s: Неверный размер %s.\n", __func__, hlog->log_type);
		else{
			off_t len = lseek(hlog->rfd, 0, SEEK_END);
			if (len == (off_t)-1)
				logdbg("%s: Ошибка определения размера файла %s.\n",
					__func__, hlog->log_type);
			else{
				hlog->full_len = hlog->hdr->len + sizeof(struct klog_header);
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
static bool klog_write_header(struct log_handle *hlog)
{
	return (hlog->wfd == -1) ? false : log_atomic_write(hlog, 0, (uint8_t *)hlog->hdr,
		sizeof(struct klog_header));
}

/* Структура для работы с контрольной лентой ККТ (ККЛ) */
static struct log_handle _hklog = {
	.hdr		= (struct log_header *)&klog_hdr,
	.hdr_len	= sizeof(klog_hdr),
	.full_len	= sizeof(klog_hdr),
	.log_type	= "ККЛ",
	.log_name	= STERM_KLOG_NAME,
	.rfd		= -1,
	.wfd		= -1,
	.map		= klog_map,
	.map_size	= KLOG_MAP_MAX_SIZE,
	.map_head	= 0,
	.on_open	= NULL,
	.on_close	= NULL,
	.init_log_hdr	= klog_init_hdr,
	.clear		= klog_clear,
	.fill_map	= klog_fill_map,
	.read_header	= klog_read_header,
	.write_header	= klog_write_header
};

struct log_handle *hklog = &_hklog;

/*
 * Определение индекса записи с заданным номером. Возвращает -1UL,
 * если запись не найдена.
 */
uint32_t klog_index_for_number(struct log_handle *hlog, uint32_t number)
{
	uint32_t i;
	for (i = 0; i < hlog->hdr->n_recs; i++){
		if (hlog->map[(hlog->map_head + i) % hlog->map_size].number == number)
			return i;
	}
	return -1U;
}

/* Можно ли отпечатать диапазон записей контрольной ленты */
bool klog_can_print_range(struct log_handle *hlog)
{
	return cfg.has_xprn && (cfg.kkt_log_stream == KLOG_STREAM_ALL) &&
		(hlog->hdr->n_recs > 0);
}

/* Можно ли распечатать контрольную ленту полностью */
bool klog_can_print(struct log_handle *hlog)
{
	return klog_can_print_range(hlog);
}

/* Можно ли проводить поиск по контрольной ленте */
bool klog_can_find(struct log_handle *hlog)
{
	return (cfg.kkt_log_stream == KLOG_STREAM_ALL) && (hlog->hdr->n_recs > 0);
}

/*
 * Добавление новой записи в конец ККЛ. Данные находятся в klog_data,
 * заголовок -- в klog_rec_hdr.
 */
static bool klog_add_rec(struct log_handle *hlog)
{
	uint32_t offs;
	uint32_t rec_len = KLOG_REC_LEN(klog_rec_hdr.len) + sizeof(klog_rec_hdr);
	if (rec_len > hlog->hdr->len){
		logdbg("%s: Длина записи (%u байт) превышает длину %s (%u байт).\n",
			__func__, rec_len, hlog->log_type, hlog->hdr->len);
		return false;
	}
/* Если при записи на циклическую ККЛ не хватает места, удаляем записи в начале */
	uint32_t free_len = log_free_space(hlog), tail = 0, n, m = hlog->map_head;
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
		logdbg("%s: Превышено максимальное число записей на %s;\n"
			"должно быть не более %u.\n", __func__, hlog->log_type, hlog->map_size);
		return false;
	}
/* Начинаем запись на контрольную ленту */
	bool ret = false;
	if (log_begin_write(hlog)){
		hlog->map_head = m;
		tail = (hlog->map_head + n) % hlog->map_size;
		hlog->map[tail].offset = hlog->hdr->tail;
		hlog->map[tail].number = klog_rec_hdr.number;
		hlog->map[tail].dt = klog_rec_hdr.dt;
		hlog->hdr->n_recs = n + 1;
		hlog->hdr->head = hlog->map[hlog->map_head].offset;
		offs = hlog->hdr->tail;
		hlog->hdr->tail = log_inc_index(hlog, hlog->hdr->tail,
				sizeof(klog_rec_hdr) + KLOG_REC_LEN(klog_rec_hdr.len));
		ret = hlog->write_header(hlog) &&
			log_write(hlog, offs, (uint8_t *)&klog_rec_hdr, sizeof(klog_rec_hdr)) &&
			((KLOG_REC_LEN(klog_rec_hdr.len) == 0) ||
			log_write(hlog, log_inc_index(hlog, offs, sizeof(klog_rec_hdr)),
				klog_data, KLOG_REC_LEN(klog_rec_hdr.len)));
	}
	log_end_write(hlog);
	return ret;
}

static inline uint64_t timeb_to_ms(const struct timeb *t)
{
	return t->time * 1000 + t->millitm;
}

static uint32_t get_op_time(const struct timeb *t0)
{
	struct timeb t;
	ftime(&t);
	return (uint32_t)(timeb_to_ms(&t) - timeb_to_ms(t0));
}

/* Заполнение заголовка записи контрольной ленты */
static void klog_init_rec_hdr(struct log_handle *hlog, struct klog_rec_header *hdr,
	const struct timeb *t0)
{
	hdr->tag = KLOG_REC_TAG;
	hlog->hdr->cur_number++;
	hdr->number = hlog->hdr->cur_number;
	hdr->stream = KLOG_STREAM_CTL;
	hdr->len = 0;
	hdr->req_len = hdr->resp_len = 0;
	time_t_to_date_time(t0->time, &hdr->dt);
	hdr->ms = t0->millitm;
	hdr->op_time = get_op_time(t0);
	hdr->addr.gaddr = cfg.gaddr;
	hdr->addr.iaddr = cfg.iaddr;
	hdr->term_version = STERM_VERSION;
	hdr->term_check_sum = term_check_sum;
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)hdr->tn);
	memset(hdr->kkt_nr, 0, sizeof(hdr->kkt_nr));
	strncpy(hdr->kkt_nr, kkt_nr, sizeof(hdr->kkt_nr));
	memcpy(hdr->dsn, dsn, DS_NUMBER_LEN);
	hdr->ds_type = kt;
	hdr->cmd = KKT_NUL;
	hdr->status = KKT_STATUS_OK;
	hdr->crc32 = 0;
}

static bool status_ok(uint8_t prefix, uint8_t cmd, uint8_t status)
{
	bool ret = false;
	switch (prefix){
		case KKT_POLL:
			switch (cmd){
				case KKT_POLL_ENQ:
				case KKT_POLL_PARAMS:
					ret = status == KKT_STATUS_OK;
					break;
			}
			break;
		case KKT_SRV:
			switch (cmd){
				case KKT_SRV_FDO_IFACE:
					ret = status == FDO_IFACE_STATUS_OK;
					break;
				case KKT_SRV_FDO_ADDR:
					ret = status == FDO_ADDR_STATUS_OK;
					break;
				case KKT_SRV_FDO_REQ:
					ret = status < KKT_STATUS_INTERNAL_BASE;
					break;
				case KKT_SRV_FDO_DATA:
					ret = status == FDO_DATA_STATUS_OK;
					break;
				case KKT_SRV_FLOW_CTL:
					ret = status == FLOW_CTL_STATUS_OK;
					break;
				case KKT_SRV_RST_TYPE:
					ret = status == RST_TYPE_STATUS_OK;
					break;
				case KKT_SRV_RTC_GET:
					ret = status == RTC_GET_STATUS_OK;
					break;
				case KKT_SRV_RTC_SET:
					ret = status == RTC_SET_STATUS_OK;
					break;
				case KKT_SRV_LAST_DOC_INFO:
				case KKT_SRV_PRINT_LAST:
				case KKT_SRV_BEGIN_DOC:
				case KKT_SRV_SEND_DOC:
				case KKT_SRV_END_DOC:
					ret = status == KKT_STATUS_OK;
					break;
				case KKT_SRV_NET_SETTINGS:
					ret = status == NET_SETTINGS_STATUS_OK;
					break;
				case KKT_SRV_GPRS_SETTINGS:
					ret = status == GPRS_CFG_STATUS_OK;
					break;
			}
			break;
		case KKT_FS:
			switch (cmd){
				case KKT_FS_STATUS:
				case KKT_FS_NR:
				case KKT_FS_LIFETIME:
				case KKT_FS_VERSION:
				case KKT_FS_LAST_ERROR:
				case KKT_FS_SHIFT_PARAMS:
				case KKT_FS_TRANSMISSION_STATUS:
				case KKT_FS_FIND_DOC:
				case KKT_FS_FIND_FDO_ACK:
				case KKT_FS_UNCONFIRMED_DOC_CNT:
				case KKT_FS_REGISTRATION_RESULT:
				case KKT_FS_REGISTRATION_RESULT2:
				case KKT_FS_REGISTRATION_PARAM:
				case KKT_FS_REGISTRATION_PARAM2:
				case KKT_FS_GET_DOC_STLV:
				case KKT_FS_READ_DOC_TLV:
				case KKT_FS_READ_REGISTRATION_TLV:
				case KKT_FS_LAST_REG_DATA:
				case KKT_FS_RESET:
					ret = status == KKT_STATUS_OK;
					break;
			}
			break;
		case KKT_NUL:
			case KKT_LOG:
			case KKT_VF:
			case KKT_CHEQUE:
				ret = status == KKT_STATUS_OK;
				break;
	}
	return ret;
}

static int get_kkt_stream(uint8_t prefix, uint8_t cmd)
{
	int ret = KLOG_STREAM_CTL;
	switch (prefix){
		case KKT_POLL:
		case KKT_FS:
			break;
		case KKT_SRV:
			switch (cmd){
				case KKT_SRV_FDO_REQ:
				case KKT_SRV_FDO_DATA:
					ret = KLOG_STREAM_FDO;
			}
			break;
		case 0:
			if (cmd != KKT_ECHO)
				ret = KLOG_STREAM_PRN;
			break;
	}
	return ret;
}

static uint32_t adjust_last_empty_rec(struct log_handle *hlog)
{
	uint32_t ret = -1U;
	uint32_t rec_idx = hlog->hdr->n_recs - 1;
	if (!klog_read_rec(hlog, rec_idx)){
		logdbg("%s: Ошибка чтения записи #%u.\n", __func__, rec_idx);
		return ret;
	}else if (KLOG_STREAM(klog_rec_hdr.stream) == KLOG_STREAM_FDO){
		struct timeb t0 = {
			.time		= date_time_to_time_t(&klog_rec_hdr.dt),
			.millitm	= klog_rec_hdr.ms
		};
		uint32_t op_time = get_op_time(&t0);
		if ((op_time > klog_rec_hdr.op_time) &&
				((klog_rec_hdr.stream & ~KLOG_STREAM_MASK) != ~KLOG_STREAM_MASK)){
			klog_rec_hdr.stream += KLOG_STREAM_MASK + 1;
			klog_rec_hdr.op_time = op_time;
			klog_rec_hdr.crc32 = 0;
			klog_rec_hdr.crc32 = klog_rec_crc32();
			if (log_begin_write(hlog)){
				uint32_t offs = hlog->map[(hlog->map_head + rec_idx) %
					hlog->map_size].offset;
				if (log_write(hlog, offs, (uint8_t *)&klog_rec_hdr,
						sizeof(klog_rec_hdr)))
					ret = klog_rec_hdr.number;
				else
					logdbg("%s: Ошибка записи по смещению 0x%.8x.\n",
						__func__, offs);
			}else
				logdbg("%s: Невозможно начать запись на %s.\n",
					__func__, hlog->log_type);
			log_end_write(hlog);
		}else
			logdbg("%s: op_time = %u; rec_op_time = %u,\n",
				__func__, op_time, klog_rec_hdr.op_time);
	}else
			logdbg("%s: stream = 0x%.8x.\n", __func__, klog_rec_hdr.stream);
	return ret;
}

/* Добавление на ККЛ информации о пустом опросе ОФД */
static uint32_t klog_write_fdo_empty(struct log_handle *hlog, const uint8_t *req)
{
	uint8_t prev_op = 0;
	uint16_t prev_op_status = 0;
	if (fdo_read_empty_data(req + 3, &prev_op, &prev_op_status)){
		if (last_is_fdo_empty && (prev_op == fdo_empty_prev_op) &&
				(prev_op_status == fdo_empty_prev_op_status)){
//			logdbg("%s: Обнаружен повторный пустой опрос ОФД.\n", __func__);
			uint32_t n_rec = adjust_last_empty_rec(hlog);
			if (n_rec == -1U)
/*				logdbg("%s: Не удалось скорректировать "
					"заголовок последней записи.\n", __func__)*/;
			else{
/*				logdbg("%s: Скорректирован заголовок "
					"последней записи.\n", __func__);*/
				return n_rec;
			}
		}
		last_is_fdo_empty = true;
		fdo_empty_prev_op = prev_op;
		fdo_empty_prev_op_status = prev_op_status;
	}
	return -1U;
}

/* Занесение записи на ККЛ. Возвращает номер записи */
uint32_t klog_write_rec(struct log_handle *hlog, const struct timeb *t0,
	const uint8_t *req, uint16_t req_len,
	uint8_t status, const uint8_t *resp, uint16_t resp_len, uint32_t flags)
{
	uint32_t ret = -1U;
	if ((hlog == NULL) || (t0 == NULL) || (req == NULL) || (req_len < 2))
		return ret;
	int rc = pthread_mutex_lock(&klog_mtx);
	if (rc != 0){
		logdbg("%s: Невозможно получить доступ к %s: %s.\n",
			__func__, hlog->log_type, strerror(rc));
		return ret;
	}
	klog_push_data();
	do {
		uint32_t len = req_len + resp_len;
		if (len > sizeof(klog_data)){
			logdbg("%s: Переполнение буфера данных при записи на %s (%u байт).\n",
				__func__, hlog->log_type, len);
			break;
		}else if (cfg.kkt_log_level == KLOG_LEVEL_OFF)
			break;
		uint8_t cmd = req[1], prefix = KKT_NUL;
		if ((cmd == KKT_POLL) || (cmd == KKT_SRV) || (cmd == KKT_FS)){
			if (req_len < 3)
				break;
			else{
				prefix = cmd;
				cmd = req[2];
			}
		}
		if (status_ok(prefix, cmd, status) &&
				((cfg.kkt_log_level == KLOG_LEVEL_WARN) ||
				 (cfg.kkt_log_level == KLOG_LEVEL_ERR)))
			break;
		uint32_t stream = get_kkt_stream(prefix, cmd);
		if (is_fdo_empty(stream, cmd, status) && (req_len == 9)){
			ret = klog_write_fdo_empty(hlog, req);
			if (ret != -1U)
				break;
		}else
			last_is_fdo_empty = false;
		klog_data_len = klog_data_index = 0;
		klog_init_rec_hdr(hlog, &klog_rec_hdr, t0);
		if (cfg.kkt_log_level == KLOG_LEVEL_ALL){
			memcpy(klog_data, req, req_len);
			if ((resp != NULL) && (resp_len > 0))
				memcpy(klog_data + req_len, resp, resp_len);
			klog_data_len = len;
			klog_rec_hdr.len = len | (flags & KLOG_REC_FLAG_MASK);
			klog_rec_hdr.req_len = req_len;
			klog_rec_hdr.resp_len = resp_len;
		}
		klog_rec_hdr.stream = stream;
		klog_rec_hdr.cmd = (cfg.kkt_log_level == KLOG_LEVEL_ERR) ? KKT_NUL : cmd;
		klog_rec_hdr.status = status;
		klog_rec_hdr.crc32 = klog_rec_crc32();
		if (klog_add_rec(hlog))
			ret = klog_rec_hdr.number;
		if (ret == -1U)
			last_is_fdo_empty = false;
	} while (false);
	klog_pop_data();
	if ((rc = pthread_mutex_unlock(&klog_mtx)) != 0)
		logdbg("%s: Ошибка разблокировки %s: %s.\n", __func__,
			hlog->log_type, strerror(rc));
	return ret;
}

/* Чтение заданной записи контрольной ленты */
bool klog_read_rec(struct log_handle *hlog, uint32_t index)
{
	bool ret = false;
	int rc = pthread_mutex_lock(&klog_mtx);
	if (rc != 0){
		logdbg("%s: Невозможно получить доступ к %s: %s.\n",
			__func__, hlog->log_type, strerror(rc));
		return ret;
	}
	do {
		if (index >= hlog->hdr->n_recs)
			break;
		klog_data_len = klog_data_index = 0;
		uint32_t offs = hlog->map[(hlog->map_head + index) % hlog->map_size].offset;
		if (!log_read(hlog, offs, (uint8_t *)&klog_rec_hdr, sizeof(klog_rec_hdr)))
			break;
		if (KLOG_REC_LEN(klog_rec_hdr.len) > sizeof(klog_data)){
			logdbg("%s: Слишком длинная запись %s #%u (%u байт).\n", __func__,
				hlog->log_type, index, KLOG_REC_LEN(klog_rec_hdr.len));
			break;
		}
		offs = log_inc_index(hlog, offs, sizeof(klog_rec_hdr));
		if ((KLOG_REC_LEN(klog_rec_hdr.len) > 0) &&
				!log_read(hlog, offs, klog_data, KLOG_REC_LEN(klog_rec_hdr.len)))
			break;
		uint32_t crc = klog_rec_hdr.crc32;
		klog_rec_hdr.crc32 = 0;
		if (klog_rec_crc32() != crc){
			logdbg("%s: Несовпадение контрольной суммы для записи %s #%u.\n",
				__func__, hlog->log_type, index);
			break;
		}
		klog_rec_hdr.crc32 = crc;
		klog_data_len = KLOG_REC_LEN(klog_rec_hdr.len);
		ret = true;
	} while (false);
	if ((rc = pthread_mutex_unlock(&klog_mtx)) != 0)
		logdbg("%s: Ошибка разблокировки %s: %s.\n", __func__,
			hlog->log_type, strerror(rc));
	return ret;
}

const char *klog_get_stream_name(int stream)
{
	const char *ret = "???";
	switch (stream){
		case KLOG_STREAM_CTL:
			ret = "УПР";
			break;
		case KLOG_STREAM_PRN:
			ret = "ПЕЧ";
			break;
		case KLOG_STREAM_FDO:
			ret = "ОФД";
			break;
	}
	return ret;
}

/* Вывод заголовка распечатки ККЛ */
bool klog_print_header(void)
{
	bool ret = false;
	int rc = pthread_mutex_lock(&klog_mtx);
	if (rc == 0){
		log_reset_prn_buf();
		try_fn(prn_write_str("\x1eНАЧАЛО ПЕЧАТИ КОНТРОЛЬНОЙ ЛЕНТЫ "
			"(РАБОТА С ККТ) "));
		try_fn(prn_write_cur_date_time());
		ret = prn_write_str("\x1c\x0b") && prn_write_eol();
		if ((rc = pthread_mutex_unlock(&klog_mtx)) != 0)
			logdbg("%s: Ошибка разблокировки %s: %s.\n", __func__,
				hklog->log_type, strerror(rc));
	}else
		logdbg("%s: Невозможно получить доступ к %s: %s.\n",
			__func__, hklog->log_type, strerror(rc));
	return ret;
}

/* Вывод концевика распечатки контрольной ленты */
bool klog_print_footer(void)
{
	bool ret = false;
	int rc = pthread_mutex_lock(&klog_mtx);
	if (rc == 0){
		log_reset_prn_buf();
		try_fn(prn_write_str("\x1e\x10\x0bКОНЕЦ ПЕЧАТИ КОНТРОЛЬНОЙ ЛЕНТЫ "
			"(РАБОТА С ККТ) "));
		try_fn(prn_write_cur_date_time());
		ret = prn_write_str("\x1c\x0b") && prn_write_eol();
		if ((rc = pthread_mutex_unlock(&klog_mtx)) != 0)
			logdbg("%s: Ошибка разблокировки %s: %s.\n", __func__,
				hklog->log_type, strerror(rc));
	}else
		logdbg("%s: Невозможно получить доступ к %s: %s.\n",
			__func__, hklog->log_type, strerror(rc));
	return ret;
}

/* Вывод заголовка записи на печать */
static bool klog_print_rec_header(void)
{
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

	return 	prn_write_str_fmt("\x1e%.2hhX%.2hhX (\"ЭКСПРЕСС-2А-К\" "
			"%hhu.%hhu.%hhu %.4hX %.*s) ККТ=%.*s",
			klog_rec_hdr.addr.gaddr, klog_rec_hdr.addr.iaddr,
			VERSION_BRANCH(klog_rec_hdr.term_version),
			VERSION_RELEASE(klog_rec_hdr.term_version),
			VERSION_PATCH(klog_rec_hdr.term_version),
			klog_rec_hdr.term_check_sum,
			isizeof(klog_rec_hdr.tn), klog_rec_hdr.tn,
			isizeof(klog_rec_hdr.kkt_nr), klog_rec_hdr.kkt_nr) &&
		prn_write_eol() &&
		prn_write_str_fmt("ЗАПИСЬ %u [%s%s] ОТ %s ", klog_rec_hdr.number + 1,
			klog_get_stream_name(KLOG_STREAM(klog_rec_hdr.stream)), rep, dt) &&
		prn_write_str_fmt("ЖЕТОН %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX",
			ds_key_char(klog_rec_hdr.ds_type),
			klog_rec_hdr.dsn[7], klog_rec_hdr.dsn[0],
			klog_rec_hdr.dsn[6], klog_rec_hdr.dsn[5],
			klog_rec_hdr.dsn[4], klog_rec_hdr.dsn[3],
			klog_rec_hdr.dsn[2], klog_rec_hdr.dsn[1]) &&
		(!(klog_rec_hdr.len & KLOG_REC_APC) || prn_write_str(" [АПЧ]")) &&
		prn_write_eol();
}

static inline bool is_print(uint8_t b)
{
	return (b >= 0x20) && (b <= 0x7f);
}

/* Вывод на печать строки контрольной ленты */
static bool klog_print_line(bool recode)
{
	if (klog_data_index >= KLOG_REC_LEN(klog_rec_hdr.len))
		return false;
	uint32_t begin = klog_data_index, end = klog_rec_hdr.req_len, addr = begin;
	if (klog_data_index >= klog_rec_hdr.req_len){
		end = KLOG_REC_LEN(klog_rec_hdr.len);
		addr -= klog_rec_hdr.req_len;
	}
	static char ln[128];
	sprintf(ln, "%.4X:", addr);
	off_t offs = 5;
	for (uint32_t i = 0, idx = begin; i < 16; i++, idx++, offs += 3){
		if ((i != 0) && ((i % 8) == 0)){
			sprintf(ln + offs, " --");
			offs += 3;
		}
		if (idx < end)
			sprintf(ln + offs, " %.2hhX", klog_data[idx]);
		else
			sprintf(ln + offs, "   ");
	}
	sprintf(ln + offs, "   ");
	offs += 3;
	for (uint32_t i = 0, idx = begin; i < 16; i++, idx++, offs++){
		if (idx < end){
			uint8_t b = klog_data[idx];
			sprintf(ln + offs, "%c", is_print(b) ? b : '.');
		}else
			sprintf(ln + offs, " ");
	}
	bool ret = recode ? prn_write_str(ln) : prn_write_chars_raw(ln, -1);
	return ret && prn_write_eol();
}

static bool klog_print_delim(size_t len)
{
	bool ret = true;
	for (int i = 0; i < len; i++){
		ret = prn_write_char_raw('=');
		if (!ret)
			break;
	}
	return ret && prn_write_eol();
}

static bool klog_print_data_header(const char *title)
{
	bool ret = true;
	if (title != NULL)
		ret = prn_write_str(title) && prn_write_eol();
	return ret && klog_print_delim(76);
}

/* Вывод записи контрольной ленты на печать */
bool klog_print_rec(void)
{
	bool ret = false;
	int rc = pthread_mutex_lock(&klog_mtx);
	if (rc == 0){
		log_reset_prn_buf();
		try_fn(klog_print_rec_header());
		bool recode = false; //klog_rec_hdr.stream != KLOG_STREAM_PRN;
		if (klog_rec_hdr.req_len > 0){
			try_fn(klog_print_data_header("ЗАПРОС:"));
			for (klog_data_index = 0; klog_data_index < klog_rec_hdr.req_len;
					klog_data_index += 16)
				try_fn(klog_print_line(recode));
		}
		if (klog_rec_hdr.resp_len > 0){
			try_fn(klog_print_data_header("ОТВЕТ:"));
			for (klog_data_index = klog_rec_hdr.req_len; klog_data_index < klog_data_len;
					klog_data_index += 16)
				try_fn(klog_print_line(recode));
		}
		ret = prn_write_eol();
		if ((rc = pthread_mutex_unlock(&klog_mtx)) != 0)
			logdbg("%s: Ошибка разблокировки %s: %s.\n", __func__,
				hklog->log_type, strerror(rc));
	}else
		logdbg("%s: Невозможно получить доступ к %s: %s.\n",
			__func__, hklog->log_type, strerror(rc));
	return ret;
}
