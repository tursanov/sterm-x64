/* Работа с билетопечатающим устройством (БПУ). (c) gsr 2009, 2026 */

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gui/scr.h"
#include "prn/sprn.h"
#include "blimits.h"
#include "cfg.h"
#include "gd.h"
#include "serial.h"
#include "sterm.h"

/* Статус БПУ */
uint8_t sprn_status = 0;
/* Статус SD-карты памяти */
uint8_t sprn_sd_status = 0;
/* Заводской номер БПУ */
uint8_t sprn_number[SPRN_NUMBER_LEN] = {[0 ... SPRN_NUMBER_LEN - 1] = 0x30};
/* Тип носителя в БПУ */
uint8_t sprn_media = SPRN_MEDIA_UNKNOWN;
/* Номер документа в БПУ */
uint8_t sprn_blank_number[SPRN_BLANK_NUMBER_LEN] = {[0 ... SPRN_BLANK_NUMBER_LEN - 1] = 0x30};
/* Из БПУ получен номер бланка */
bool sprn_has_blank_number = false;

/* Номер параметра конфигурации БПУ (0x1d Sx) */
static uint8_t sprn_param_number;
/* Знак параметра конфигурации БПУ (1 или -1) */
static int sprn_param_sign = 1;
/* Значение параметра конфигурации БПУ */
static int sprn_param_value = 0;
/* Значения параметров печати, получаемые по команде 0x1d R */
static uint8_t sprn_params[41];
/* В строке параметров должно быть 10 символов ';' */
static int nr_semicolons;
/* Флаг получения всех параметров БПУ */
static bool sprn_params_received = false;

/* Имя файла устройства для работы с БПУ */
#define SPRN_DEV_NAME		"/dev/ttyUSB0"
/* Устройство для работы с БПУ */
static int sprn_dev = -1;

/* Данные для передачи БПУ */
static uint8_t snd_data[PRN_BUF_LEN];
/* Размер данных для передачи БПУ */
static size_t snd_data_len;
/* Размер данных, переданных БПУ */
static size_t sent_len;
/* Код команды, отправленной в БПУ */
static uint8_t sent_cmd = SPRN_NUL;

/* Время начала очередной транзакции с БПУ */
static uint32_t t0;
/* Таймаут текущей операции. 0 -- бесконечный таймаут. */
static uint32_t timeout = SPRN_INFINITE_TIMEOUT;
/* Флаг превышения таймаута при выполнении операции */
bool sprn_timeout = false;

/* Установка начала текущей операции */
static void sprn_mark_operation(void)
{
	t0 = u_times();
}

/* Установка таймаута в соответствии с командой */
static void sprn_set_timeout(uint8_t cmd)
{
	static struct {
		uint8_t cmd;
		uint32_t timeout;
	} map[] = {
		{SPRN_RD_BCODE,		SPRN_RD_BCODE_TIMEOUT},
		{SPRN_MEDIA,		SPRN_MEDIA_TIMEOUT},
		{SPRN_ID,		SPRN_ID_TIMEOUT},
		{SPRN_STATUS,		SPRN_STATUS_TIMEOUT},
		{SPRN_LOG,		SPRN_LOG_TIMEOUT},
		{SPRN_LOG1,		SPRN_LOG_TIMEOUT},
		{SPRN_RD_BCODE,		SPRN_BCODE_CTL_TIMEOUT},
		{SPRN_NO_BCODE,		SPRN_TEXT_TIMEOUT},
	};
	int i;
	timeout = SPRN_INFINITE_TIMEOUT;
	for (i = 0; i < ASIZE(map); i++){
		if (map[i].cmd == cmd){
			timeout = map[i].timeout;
			break;
		}
	}
}

/* Состояния конечного автомата приёма */
enum {
	rcv_start,		/* ожидание Ар1 */
	rcv_cmd,		/* ожидание кода команды */
	rcv_number_status,	/* ожидание кода завершения чтения номера бланка */
	rcv_number,		/* получение номера бланка */
	rcv_media_status,	/* ожидание кода завершения команды получения типа носителя */
	rcv_media_type,		/* ожидание кода типа носителя */
	rcv_id_status,	/* ожидание кода завершения инициализации БПУ */
	rcv_id,			/* получение заводского номера БПУ */
	rcv_xstatus,		/* ожидание байта статуса БПУ */
	rcv_sd_status,		/* ожидание байта статуса SD-карты */
	rcv_status,		/* ожидание байта завершения команды */
	rcv_number_ctl_status,	/* ожидание кода завершения печати с контролем штрих-кода */
	rcv_wr_param_prefix,	/* ожидание S (установка параметра) */
	rcv_wr_param_number,	/* ожидание номера параметра */
	rcv_wr_param_sign,	/* ожидание знака числового значения параметра (+/-) */
	rcv_wr_param_val,	/* получение числового значения параметра */
	rcv_rd_param_prefix,	/* ожидание R (получение значений всех параметров) */
	rcv_rd_param_vals,	/* получение значений всех параметров */
	rcv_time_sync,		/* ожидание кода завершения команды синхронизации времени */ 
	rcv_end,		/* окончание приёма данных от БПУ (в т.ч. по таймауту) */
	rcv_idle,		/* игнорирование данных от БПУ */
};

static int rcv_st = rcv_idle;

/* Счётчик для некоторых состояний конечного автомата приёма */
static off_t rcv_idx;

/* Установка состояния конечного автомата приёма */
static void sprn_set_rcv_st(int st)
{
	if ((st == rcv_end) || (st == rcv_idle))
		timeout = SPRN_INFINITE_TIMEOUT;
	rcv_st = st;
}

/* Сброс буферов и таймаутов */
static void sprn_reset(void)
{
/*	sprn_status = 0;
	sprn_sd_status = 0;
	memset(sprn_number, 0x30, sizeof(sprn_number));
	sprn_media = SPRN_MEDIA_UNKNOWN;*/
	snd_data_len = sent_len = 0;
	sent_cmd = SPRN_NUL;
	timeout = SPRN_INFINITE_TIMEOUT;
	sprn_set_rcv_st(rcv_idle);
	rcv_idx = 0;
#if defined __LOG_SPRN__
	sprn_flush_log_data();
#endif
}

/* Закрытие устройства для работы с БПУ */
void sprn_close(void)
{
/*	printf("%s: sprn_dev = %d.\n", __func__, sprn_dev);*/
	if (sprn_dev != -1){
		serial_close(sprn_dev);
		sprn_dev = -1;
	}
	sprn_reset();
}

/* Открытие устройства для работы с БПУ */
static bool sprn_open(void)
{
	struct serial_settings ss = {
		.csize		= CS8,
		.parity		= SERIAL_PARITY_NONE,
		.stop_bits	= SERIAL_STOPB_1,
		.control	= SERIAL_FLOW_RTSCTS,
		.baud		= B115200,
	};
	sprn_close();
	sprn_dev = serial_open(SPRN_DEV_NAME, &ss, O_RDWR);
	return sprn_dev != -1;
}

/* Запись в буфер передачи простой команды */
static bool sprn_write_cmd(uint8_t cmd)
{
	if (sizeof(snd_data) >= 3){
		sprn_reset();
		sprn_has_blank_number = false;
		snd_data[0] = SPRN_NUL;
		snd_data[1] = SPRN_DLE;
		snd_data[2] = cmd;
		snd_data_len = 3;
		sent_len = 0;
		sent_cmd = cmd;
		sprn_mark_operation();
		sprn_set_timeout(cmd);
#if defined __LOG_SPRN__
		sprn_log_rcv = false;
#endif
		return true;
	}else
		return false;
}

/* Запись в буфер передачи текста */
static bool sprn_write_text(const uint8_t *txt, size_t len, bool log)
{
	static uint8_t bcode_tmpl[] = {SPRN_DLE, SPRN_RD_BCODE, 0x3b, 0x3b};
	if ((txt == NULL) || (len == 0) || ((len + 1) > sizeof(snd_data)))
		return false;
/*	printf("%s: txt = [%.*s].\n", __func__, len - 1, txt + 1);*/
	sprn_reset();
	sprn_has_blank_number = false;
	snd_data[0] = SPRN_NUL;
	memcpy(snd_data + 1, txt, len);
	snd_data_len = len + 1;
	sent_len = 0;
	sprn_mark_operation();
	if (log)
		sent_cmd = SPRN_LOG;
	else if ((len > sizeof(bcode_tmpl)) && (memcmp(txt, bcode_tmpl,
			sizeof(bcode_tmpl)) == 0))
		sent_cmd = SPRN_RD_BCODE;
	else
		sent_cmd = SPRN_NO_BCODE;
	sprn_set_timeout(sent_cmd);
#if defined __LOG_SPRN__
	sprn_log_rcv = false;
#endif
	return true;
}

/* Запись в буфер передачи текста для печати на контрольной ленте (Ар2 Э) */
static bool sprn_write_text_log1(const uint8_t *data, size_t len)
{
	static uint8_t prefix[] = {SPRN_NUL, SPRN_DLE, SPRN_LOG1, SPRN_STX};
	size_t l = sizeof(prefix) + len + 1;
	bool need_ff;
	if ((data == NULL) || (len == 0))
		return false;
	need_ff = data[len - 1] != SPRN_FORM_FEED;
	if (need_ff)
		l++;
	if (l > sizeof(snd_data))
		return false;
	sprn_reset();
	sprn_has_blank_number = false;
	memcpy(snd_data, prefix, sizeof(prefix));
	memcpy(snd_data + sizeof(prefix), data, len);
	snd_data_len = sizeof(prefix) + len;
	if (need_ff)
		snd_data[snd_data_len++] = SPRN_FORM_FEED;
	snd_data[snd_data_len++] = SPRN_ETX;
	sent_len = 0;
	sprn_mark_operation();
	sent_cmd = SPRN_LOG1;
	sprn_set_timeout(sent_cmd);
#if defined __LOG_SPRN__
	sprn_log_rcv = false;
#endif
	return true;
}

/* Передача данных в БПУ. В случае ошибки возвращает false */
static bool sprn_do_snd(void)
{
	bool ret = true;
	if (sprn_dev == -1)
		ret = false;
	else if (sent_len < snd_data_len){
		ssize_t len = write(sprn_dev, snd_data + sent_len,
			snd_data_len - sent_len);
		if (len < 0){
			if (errno != EWOULDBLOCK){
				fprintf(stderr, "Ошибка записи в %s: %s.\n",
					fd2name(sprn_dev), strerror(errno));
#if defined __LOG_SPRN__
				sprn_log_byte(errno | 0x80);
				sprn_flush_log_data();
#endif
				ret = false;
			}
		}else if (len > 0){
#if defined __LOG_SPRN__
			sprn_log_data(snd_data + sent_len, len);
#endif
			sent_len += len;
		}
	}
	return ret;
}

/* Обработка состояний конечного автомата приёма */
static void on_rcv_start(uint8_t b)
{
	if (b == SPRN_DLE1)
		sprn_set_rcv_st(rcv_cmd);
	else if (b == SPRN_DLE2){
		if (sent_cmd == 'S')
			sprn_set_rcv_st(rcv_wr_param_prefix);
		else if (sent_cmd == 'R')
			sprn_set_rcv_st(rcv_rd_param_prefix);
		else{
			fprintf(stderr, "От БПУ получен символ 0x1d, недопустимый в даном контексте.\n");
			sprn_set_rcv_st(rcv_idle);
		}
	}
}

static void on_rcv_cmd(uint8_t b)
{
	if (b != sent_cmd){
		fprintf(stderr, "БПУ отправлена команда 0x%.2hhx, а в ответ пришла 0x%.2hhx.\n",
			sent_cmd, b);
		sprn_set_rcv_st(rcv_idle);
	}else{
		switch (b){
			case SPRN_NUMBER:
				sprn_set_rcv_st(rcv_number_status);
				break;
			case SPRN_MEDIA:
				sprn_set_rcv_st(rcv_media_status);
				break;
			case SPRN_ID:
				sprn_set_rcv_st(rcv_id_status);
				break;
			case SPRN_STATUS:
				sprn_set_rcv_st(rcv_xstatus);
				break;
			case SPRN_LOG:
			case SPRN_LOG1:
			case SPRN_NO_BCODE:
				sprn_set_rcv_st(rcv_status);
				break;
			case SPRN_RD_BCODE:
				sprn_set_rcv_st(rcv_number_ctl_status);
				break;
			default:
				fprintf(stderr, "От БПУ получен неизвестный код команды: 0x%.2hhx.\n", b);
				sprn_set_rcv_st(rcv_idle);
		}
	}
}

static void on_rcv_number_status(uint8_t b)
{
	sprn_status = b;
	if (b == 0){
		sprn_set_rcv_st(rcv_number);
		rcv_idx = 0;
	}else
		sprn_set_rcv_st(rcv_end);
}

static void on_rcv_number(uint8_t b)
{
	if ((b >= 0x30) && (b <= 0x39)){
		sprn_blank_number[rcv_idx++] = b;
		if (rcv_idx >= sizeof(sprn_blank_number)){
			sprn_has_blank_number = true;
			sprn_set_rcv_st(rcv_end);
		}
	}else{
		fprintf(stderr, "Неверный символ штрих-кода: 0x%.2hhx.\n", b);
		sprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_media_status(uint8_t b)
{
	sprn_status = b;
	if (b == 0)
		sprn_set_rcv_st(rcv_media_type);
	else
		sprn_set_rcv_st(rcv_end);
}

static void on_rcv_media_type(uint8_t b)
{
	switch (b){
		case SPRN_MEDIA_BLANK:
		case SPRN_MEDIA_PAPER:
		case SPRN_MEDIA_BOTH:
			sprn_media = b;
			sprn_set_rcv_st(rcv_end);
			break;
		default:
			fprintf(stderr, "Неизвестный тип носителя в БПУ: 0x%.2hhx.\n", b);
			sprn_set_rcv_st(rcv_idle);
			break;
	}
}

static void on_rcv_id_status(uint8_t b)
{
	sprn_status = b;
	if (b == 0){
		rcv_idx = 0;
		sprn_set_rcv_st(rcv_id);
	}else
		sprn_set_rcv_st(rcv_end);
}

static void on_rcv_id(uint8_t b)
{
	if ((b >= 0x20) && (b <= 0x7e)){
		if (rcv_idx < sizeof(sprn_number))
			sprn_number[rcv_idx++] = b;
		if (rcv_idx >= sizeof(sprn_number))
			sprn_set_rcv_st(rcv_end);
	}else{
		fprintf(stderr, "Неверный символ в номере БПУ: 0x%.2hhx.\n", b);
		sprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_status(uint8_t b)
{
	sprn_status = b;
	sprn_set_rcv_st(rcv_end);
}

static void on_rcv_xstatus(uint8_t b)
{
	sprn_status = b;
	sprn_set_rcv_st(rcv_sd_status);
}

static void on_rcv_sd_status(uint8_t b)
{
	sprn_sd_status = b;
	sprn_set_rcv_st(rcv_end);
}

static void on_rcv_number_ctl_status(uint8_t b)
{
	sprn_status = b;
	rcv_idx = 0;
	sprn_set_rcv_st(rcv_number);
}

static void on_rcv_wr_param_prefix(uint8_t b)
{
	if (b == 'S')
		sprn_set_rcv_st(rcv_wr_param_number);
	else{
		fprintf(stderr, "При установке параметра S%c вместо 'S' "
			"получен 0x%.2hhx.\n", sprn_param_number, b);
		sprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_wr_param_number(uint8_t b)
{
	if (b == sprn_param_number){
		if (b == 0x41)
			sprn_set_rcv_st(rcv_time_sync);
		else{
			sprn_param_value = 0;
			sprn_param_sign = 1;
			rcv_idx = 0;
			sprn_set_rcv_st(rcv_wr_param_sign);
		}
	}else{
		fprintf(stderr, "Устанавливаем параметр S%c, а в ответ пришёл номер %c.\n",
			sprn_param_number, b);
		sprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_wr_param_val(uint8_t b)
{
	if ((b >= 0x30) && (b <= 0x39)){
		sprn_param_value *= 10;
		sprn_param_value += b - 0x30;
		if (++rcv_idx == 3){
			sprn_param_value *= sprn_param_sign;
			sprn_set_rcv_st(rcv_end);
		}
	}else{
		fprintf(stderr, "Неверное значение параметра S%c.\n",
			sprn_param_number);
		sprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_wr_param_sign(uint8_t b)
{
	if (b == '-'){
		sprn_param_sign = -1;
		sprn_set_rcv_st(rcv_wr_param_val);
	}else{
		sprn_set_rcv_st(rcv_wr_param_val);
		on_rcv_wr_param_val(b);
	}
}

static void on_rcv_rd_param_prefix(uint8_t b)
{
	if (b == 'R'){
		rcv_idx = nr_semicolons = 0;
		sprn_set_rcv_st(rcv_rd_param_vals);
	}else{
		fprintf(stderr, "При чтении параметров вместо 'R' получен "
			"0x%.2hhx.\n", b);
		sprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_rd_param_vals(uint8_t b)
{
	if (((b >= 0x30) && (b <= 0x39)) || (b == '-') || (b == ';')){
		if (rcv_idx < sizeof(sprn_params))
			sprn_params[rcv_idx++] = b;
		if (b == ';'){
			nr_semicolons++;
			if (nr_semicolons == 10){
				sprn_params_received = true;
				sprn_set_rcv_st(rcv_end);
			}
		}
	}else{
		fprintf(stderr, "В ответ на запрос параметров пришёл "
			"неверный символ: 0x%.2hhx.\n", b);
		sprn_set_rcv_st(rcv_idle);
	}
}

/*
 * Чтение трёхзначного десятичного числа. Возвращает указатель на следующий
 * после последнего прочитанного байт в буфере или NULL в случае ошибки.
 */
static const uint8_t *sprn_read_val(const uint8_t *buf, bool has_sign, int *val)
{
	int idx = 0, n, sign = 1;
	*val = 0;
	if (has_sign && (buf[0] == '-')){
		sign = -1;
		idx++;
	}
	for (n = 0; n < 3; n++, idx++){
		if ((buf[idx] >= 0x30) && (buf[idx] <= 0x39)){
			*val *= 10;
			*val += buf[idx] - 0x30;
		}else
			break;
	}
	if (n == 3){
		*val *= sign;
		return buf + idx;
	}else
		return NULL;
}

/* Занесение полученных параметров в структуру конфигурации терминала */
static bool sprn_translate_params(struct term_cfg *cfg)
{
	static struct {
		int offs;
		bool has_sign;
	} map[] = {
		{offsetof(struct term_cfg, s0), false},
		{offsetof(struct term_cfg, s1), false},
		{offsetof(struct term_cfg, s2), false},
		{offsetof(struct term_cfg, s3), false},
		{offsetof(struct term_cfg, s4), false},
		{offsetof(struct term_cfg, s5), false},
		{offsetof(struct term_cfg, s6), false},
		{offsetof(struct term_cfg, s7), false},
		{offsetof(struct term_cfg, s8), true},
		{offsetof(struct term_cfg, s9), false},
	};
	int i, *p;
	const uint8_t *pp = sprn_params;
	if ((cfg == NULL) || !sprn_params_received)
		return false;
	for (i = 0; i < ASIZE(map); i++){
		p = (int *)(((uint8_t *)cfg) + map[i].offs);
		pp = sprn_read_val(pp, map[i].has_sign, p);
		if ((pp == NULL) || (*pp != ';'))
			break;
		pp++;
	}
	return i == ASIZE(map);
}

static void on_rcv_time_sync(uint8_t b)
{
	sprn_sd_status = b;
	sprn_set_rcv_st(rcv_end);
}

static bool sprn_do_rcv(void)
{
	bool ret = true;
	uint8_t b;
	ssize_t len;
	len = read(sprn_dev, &b, 1);
	if ((len == -1) && (errno != EWOULDBLOCK)){
		fprintf(stderr, "Ошибка чтения из %s: %s.\n",
			fd2name(sprn_dev), strerror(errno));
#if defined __LOG_SPRN__
		sprn_log_byte(errno | 0x80);
		sprn_flush_log_data();
#endif
		ret = false;
	}else if (len == 1){
/*		printf("%s: b = 0x%.2hhx; rcv_st = %d.\n", __func__,
			b, rcv_st);*/
#if defined __LOG_SPRN__
		sprn_log_byte(b);
#endif
		if ((b == 0x11) || (b == 0x13))
			return ret;
		switch (rcv_st){
			case rcv_start:
				on_rcv_start(b);
				break;
			case rcv_cmd:
				on_rcv_cmd(b);
				break;
			case rcv_number_status:
				on_rcv_number_status(b);
				break;
			case rcv_number:
				on_rcv_number(b);
				break;
			case rcv_media_status:
				on_rcv_media_status(b);
				break;
			case rcv_media_type:
				on_rcv_media_type(b);
				break;
			case rcv_id_status:
				on_rcv_id_status(b);
				break;
			case rcv_id:
				on_rcv_id(b);
				break;
			case rcv_xstatus:
				on_rcv_xstatus(b);
				break;
			case rcv_sd_status:
				on_rcv_sd_status(b);
				break;
			case rcv_status:
				on_rcv_status(b);
				break;
			case rcv_number_ctl_status:
				on_rcv_number_ctl_status(b);
				break;
			case rcv_wr_param_prefix:
				on_rcv_wr_param_prefix(b);
				break;
			case rcv_wr_param_number:
				on_rcv_wr_param_number(b);
				break;
			case rcv_wr_param_sign:
				on_rcv_wr_param_sign(b);
				break;
			case rcv_wr_param_val:
				on_rcv_wr_param_val(b);
				break;
			case rcv_rd_param_prefix:
				on_rcv_rd_param_prefix(b);
				break;
			case rcv_rd_param_vals:
				on_rcv_rd_param_vals(b);
				break;
			case rcv_time_sync:
				on_rcv_time_sync(b);
				break;
		}
	}
	return ret;
}

/* Обработка подсистемы печати на БПУ */
static bool sprn_process(void)
{
	bool ret = true;
	sprn_timeout = (timeout != SPRN_INFINITE_TIMEOUT) &&
		((u_times() - t0) > timeout);
	if (sprn_timeout){
		sprn_set_rcv_st(rcv_idle);
		ret = false;
#if defined __LOG_SPRN__
		sprn_flush_log_data();
#endif
	}else if (sent_len < snd_data_len){
		if (!sprn_do_snd()){
			sprn_reset();
			ret = false;
		}else if (sent_len >= snd_data_len){
			sprn_set_rcv_st(rcv_start);
#if defined __LOG_SPRN__
			sprn_flush_log_data();
			sprn_log_rcv = true;
#endif
		}
	}else if (!sprn_do_rcv()){
		sprn_reset();
		ret = false;
	}
	return ret;
}

/* Ожидание завершения выполнения команды БПУ */
static int sprn_wait_op(bool show_status)
{
	int ret = SPRN_RET_ERR;
/*	set_term_astate(ast_none);*/
	while ((sent_len < snd_data_len) ||
			((rcv_st != rcv_end) && (rcv_st != rcv_idle))){
		if (sprn_process()){
			if (((get_cmd(false, true) == cmd_reset) &&
					reset_term(false)) || (kt == key_none)){
				sprn_reset();
				ret = SPRN_RET_RST;
			}
		}else
			break;
	}
	if (rcv_st == rcv_end)
		ret = SPRN_RET_OK;
	else if (ret != SPRN_RET_RST){
		if (show_status)
			set_term_astate(ast_nosprn);
	}
	return ret;
}

/* Начало передачи в БПУ команды */
static int sprn_do_cmd(uint8_t cmd)
{
	int ret = SPRN_RET_ERR;
	sprn_status = sprn_sd_status = 0;
	if ((sprn_dev == -1) && !sprn_open())
		set_term_astate(ast_nosprn);
	else{
		if (sprn_write_cmd(cmd))
			ret = sprn_wait_op(true);
		else{
			set_term_astate(ast_nosprn);
			sprn_reset();
		}
	}
	return ret;
}

/* Инициализация БПУ и получение его заводского номера */
int sprn_init(void)
{
	int ret;
	memset(sprn_number, 0x30, sizeof(sprn_number));
	ret = sprn_do_cmd(SPRN_ID);
	if ((ret != SPRN_RET_OK) || (sprn_status != 0))
		memset(sprn_number, 0x30, sizeof(sprn_number));
	return ret;
}

/* Возвращает true, если заводской номер БПУ состоит из одних нулей */
bool sprn_is_zero_number(void)
{
	int i;
	for (i = 0; i < sizeof(sprn_number); i++){
		if (sprn_number[i] != 0x30)
			break;
	}
	return i == sizeof(sprn_number);
}

/* Получение статуса БПУ */
int sprn_get_status(void)
{
	return sprn_do_cmd(SPRN_STATUS);
}

/* Получение типа носителя в БПУ */
int sprn_get_media_type(void)
{
	return sprn_do_cmd(SPRN_MEDIA);
}

/* Получение номера документа в БПУ */
int sprn_get_blank_number(void)
{
	return sprn_do_cmd(SPRN_RD_BCODE);
}

/* Печать абзаца контрольной ленты */
int sprn_print_log(const uint8_t *data, size_t len)
{
	int ret = SPRN_RET_ERR;
	if ((sprn_dev == -1) && !sprn_open())
		set_term_astate(ast_nosprn);
	else{
		if (sprn_write_text(data, len, true))
			ret = sprn_wait_op(true);
		else{
			set_term_astate(ast_nosprn);
			sprn_reset();
		}
	}
	return ret;
}

/* Печать абзаца контрольной ленты, пришедшего из "Экспресс" (Ар2 Э) */
int sprn_print_log1(const uint8_t *data, size_t len)
{
	int ret = SPRN_RET_ERR;
	if ((sprn_dev == -1) && !sprn_open())
		set_term_astate(ast_nosprn);
	else{
		if (sprn_write_text_log1(data, len))
			ret = sprn_wait_op(true);
		else{
			set_term_astate(ast_nosprn);
			sprn_reset();
		}
	}
	sprn_close();
	return ret;
}

/* Печать текста на БСО */
int sprn_print_ticket(const uint8_t *data, size_t len, bool *sent_to_prn)
{
	int ret = SPRN_RET_ERR;
	*sent_to_prn = false;
/* Ожидание готовности принтера */
	if ((ret = sprn_get_status()) != SPRN_RET_OK)
		;
	else if (sprn_status != 0)
		;
	else if ((ret = sprn_get_media_type()) != SPRN_RET_OK)
		;
	else if (sprn_status != 0)
		;
	else if ((sprn_media != SPRN_MEDIA_BLANK) &&
			(sprn_media != SPRN_MEDIA_BOTH))
		;
	else{
/* Печать на БСО */
		set_term_astate(ast_none);
		if (sprn_write_text(data, len, false)){
			*sent_to_prn = true;
			ret = sprn_wait_op(true);
			if (ret == SPRN_RET_OK){
				if (sprn_status == 0){
					bool has_number = sprn_has_blank_number;
					if ((ret = sprn_get_status()) == SPRN_RET_OK)
						sprn_has_blank_number = has_number;
				}
			}
		}
	}
	sprn_close();
	return ret;
}

/* Запрос параметров работы БПУ */
int sprn_get_params(struct term_cfg *cfg)
{
	int ret = SPRN_RET_ERR;
	if ((sprn_dev != -1) || sprn_open()){
		sprn_reset();
		sprn_params_received = false;
		snd_data[0] = SPRN_NUL;
		snd_data[1] = SPRN_DLE2;
		snd_data[2] = 'R';
		snd_data_len = 3;
		sent_len = 0;
		sent_cmd = 'R';
		sprn_mark_operation();
		timeout = SPRN_RD_PARAMS_TIMEOUT;
#if defined __LOG_SPRN__
		sprn_log_rcv = false;
#endif
		ret = sprn_wait_op(false);
		sprn_close();
		if ((ret == SPRN_RET_OK) && !sprn_translate_params(cfg))
			ret = SPRN_RET_ERR;
	}
	return ret;
}

/* Запись одного параметра работы БПУ */
static int sprn_set_param(int n, int val)
{
	sprn_reset();
	snd_data[0] = SPRN_NUL;
	snd_data[1] = SPRN_DLE2;
	snd_data[2] = 'S';
	snd_data[3] = n + 0x30;
	snd_data_len = 4;
	if (val < 0){
		snd_data[snd_data_len++] = '-';
		val *= -1;
	}
	snd_data[snd_data_len++] = ((val / 100) % 100) + 0x30;
	snd_data[snd_data_len++] = ((val / 10) % 10) + 0x30;
	snd_data[snd_data_len++] = (val % 10) + 0x30;
	sent_len = 0;
	sent_cmd = 'S';
	sprn_param_number = n + 0x30;
	sprn_mark_operation();
	timeout = SPRN_WR_PARAM_TIMEOUT;
#if defined __LOG_SPRN__
	sprn_log_rcv = false;
#endif
	return sprn_wait_op(false);
}

/* Запись параметров работы БПУ */
int sprn_set_params(struct term_cfg *cfg)
{
	int ret = SPRN_RET_ERR, i, *p = &cfg->s0;
	if ((sprn_dev != -1) || sprn_open()){
		for (i = 0; i < 10; i++){
			ret = sprn_set_param(i, p[i]);
			if (ret == SPRN_RET_OK)
				p[i] = sprn_param_value;
			else
				break;
		}
	}
	sprn_close();
	return ret;
}

/* Синхронизация времени БПУ с терминалом */
int sprn_sync_time(void)
{
	int ret = SPRN_RET_ERR;
	if ((sprn_dev != -1) || sprn_open()){
		time_t t = time(NULL) + time_delta;
		struct tm *tm = localtime(&t);
		sprn_reset();
		snd_data[0] = SPRN_NUL;
		snd_data[1] = SPRN_DLE2;
		snd_data[2] = 0x53;		/* S */
		snd_data[3] = 0x41;		/* A */
		snprintf((char *)snd_data + 4, sizeof(snd_data) - 4,
			"%.2d%.2d%.2d%.2d%.2d%.2d",
			tm->tm_mday, tm->tm_mon + 1, tm->tm_year % 100,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
		snd_data_len = 16;
		sent_len = 0;
		sent_cmd = 0x53;		/* S */
		sprn_param_number = 0x41;	/* A */
		sprn_sd_status = 0;
		sprn_mark_operation();
		timeout = SPRN_TIME_SYNC_TIMEOUT;
#if defined __LOG_SPRN__
		sprn_log_rcv = false;
#endif
		ret = sprn_wait_op(false);
	}
	sprn_close();
	return ret;
}

/* Получение информации об ошибке БПУ на основании её кода */
struct sprn_error_txt *sprn_get_error_txt(uint8_t code)
{
	static struct {
		uint8_t code;
		int len;
		char *txt;
	} err[] = {
		{
			.code		= 0x01,
			.len		= -1,
			.txt		= "НОМЕР БПУ НЕ ПРОПИСАН В ПАМЯТИ"
		},
		{
			.code		= 0x30,
			.len		= -1,
			.txt		= "ШТРИХОВОЙ КОД ОТСУТСТВУЕТ"
		},
		{
			.code		= 0x36,
			.len		= -1,
			.txt		= "ДЛИНА ШТРИХОВОГО КОДА НЕ РАВНА 13"
		},
		{
			.code		= 0x37,
			.len		= -1,
			.txt		= "НЕСОВПАДЕНИЕ НОМЕРА БЛАНКА"
		},
		{
			.code		= 0x38,
			.len		= -1,
			.txt		= "ОШИБКА КОНТРОЛЬНОЙ СУММ\x9b"
		},
		{
			.code		= 0x39,
			.len		= -1,
			.txt		= "НОМЕР БЛАНКА XXXXXXX000000",
		},
		{
			.code		= 0x41,
			.len		= -1,
			.txt		= "КОНЕЦ БУМАГИ"
		},
		{
			.code		= 0x42,
			.len		= -1,
			.txt		= "КР\x9bШКА ОТКР\x9bТА",
		},
		{
			.code		= 0x43,
			.len		= -1,
			.txt		= "БУМАГА ЗАСТРЯЛА НА В\x9bХОДЕ",
		},
		{
			.code		= 0x44,
			.len		= -1,
			.txt		= "БУМАГА ЗАМЯЛАСЬ",
		},
		{
			.code		= 0x45,
			.len		= -1,
			.txt		= "ОШИБКА ЧТЕНИЯ РЕПЕРНОЙ МЕТКИ",
		},
		{
			.code		= 0x46,
			.len		= -1,
			.txt		= "АППАРАТНАЯ ОШИБКА СКАНЕРА ШТРИХ-КОДА",
		},
		{
			.code		= 0x47,
			.len		= -1,
			.txt		= "ОШИБКА НОСИТЕЛЯ",
		},
		{
			.code		= 0x4a,
			.len		= -1,
			.txt		= "ПЕРЕКОС БЛАНКА",
		},
		{
			.code		= 0x4f,
			.len		= -1,
			.txt		= "ОБЩАЯ АППАРАТНАЯ ОШИБКА ПРИНТЕРА",
		},
		{
			.code		= 0x50,
			.len		= -1,
			.txt		= "НЕТ КОМАНД\x9b ОТРЕЗКИ БЛАНКА",
		},
		{
			.code		= 0x51,
			.len		= -1,
			.txt		= "ПРЕВ\x9bШЕНИЕ ОБ'ЕМА ТЕКСТА ПО ВЕРТИКАЛЬН\x9bМ ПОЗИЦИЯМ",
		},
		{
			.code		= 0x52,
			.len		= -1,
			.txt		= "ПРЕВ\x9bШЕНИЕ ОБ'ЕМА ТЕКСТА ПО ГОРИЗОНТАЛЬН\x9bМ ПОЗИЦИЯМ",
		},
		{
			.code		= 0x53,
			.len		= -1,
			.txt		= "НАРУШЕНИЕ СТРУКТУР\x9b ИНФОРМАЦИИ ПРИ ПЕЧАТИ КЛ",
		},
		{	.code		= 0x54,
			.len		= -1,
			.txt		= "НАРУШЕНИЕ ФОРМАТА КОМАНД\x9b АНАЛИЗА ШТРИХОВОГО КОДА",
		},
		{
			.code		= 0x70,
			.len		= -1,
			.txt		= "ОШИБКА РАБОТ\x9b СО ШТРИХОВ\x9bМ КОДОМ",
		},
		{
			.code		= 0xff,
			.len		= -1,
			.txt		= "НЕИЗВЕСТНАЯ ОШИБКА"
		},
	};
	int i;
	for (i = 0; i < ASIZE(err); i++){
		if (err[i].len == -1)
			err[i].len = strlen(err[i].txt);
		if ((err[i].code == 0xff) || (code == err[i].code))
			return (struct sprn_error_txt *)&err[i].len;
	}
	return NULL;
}

/* Получение информации об ошибке карты памяти на основании её кода */
struct sprn_error_txt *sprn_get_sd_error_txt(uint8_t code)
{
	static struct {
		uint8_t code;
		int len;
		char *txt;
	} err[] = {
		{
			.code		= 0x04,
			.len		= -1,
			.txt		= "ОШИБКА СИНХРОНИЗАЦИИ ВРЕМЕНИ С БПУ"
		},
		{
			.code		= 0x60,
			.len		= -1,
			.txt		= "НЕТ МЕСТА В КАРТЕ ПАМЯТИ"
		},
		{
			.code		= 0x61,
			.len		= -1,
			.txt		= "КАРТА ПАМЯТИ ОТСУТСТВУЕТ"
		},
		{
			.code		= 0x62,
			.len		= -1,
			.txt		= "ОШИБКА ПРИ РАСПЕЧАТКЕ ИЗОБРАЖЕНИЙ"
		},
		{
			.code		= 0x63,
			.len		= -1,
			.txt		= "ОШИБКА ПРИ УДАЛЕНИИ ИЗОБРАЖЕНИЙ"
		},
		{
			.code		= 0xff,
			.len		= -1,
			.txt		= "НЕИЗВЕСТНАЯ ОШИБКА"
		},
	};
	int i;
	for (i = 0; i < ASIZE(err); i++){
		if (err[i].len == -1)
			err[i].len = strlen(err[i].txt);
		if ((err[i].code == 0xff) || (code == err[i].code))
			return (struct sprn_error_txt *)&err[i].len;
	}
	return NULL;
}
