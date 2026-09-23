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
#include "termlog.h"

/* Статус БПУ */
uint8_t sprn_status = 0;
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

const struct dev_info *sprn = NULL;
/* Устройство для работы с БПУ */
static int sprn_dev = -1;

/* Данные для передачи БПУ */
static uint8_t sprn_tx[SPRN_TX_BUF_LEN];
/* Размер данных для передачи БПУ */
static size_t sprn_tx_len;
/* Размер данных, переданных БПУ */
static size_t sprn_sent_len;
/* Код команды, отправленной в БПУ */
static uint8_t sprn_sent_cmd = SPRN_NUL;

/* Время начала очередной транзакции с БПУ */
static uint32_t sprn_t0;
/* Таймаут текущей операции. 0 -- бесконечный таймаут. */
static uint32_t sprn_op_timeout = SPRN_INFINITE_TIMEOUT;
/* Флаг превышения таймаута при выполнении операции */
bool sprn_timeout = false;

/* Установка начала текущей операции */
static void sprn_mark_operation(void)
{
	sprn_t0 = u_times();
}

/* Установка таймаута в соответствии с командой */
static void sprn_set_timeout(uint8_t cmd)
{
	static struct {
		uint8_t cmd;
		uint32_t sprn_op_timeout;
	} map[] = {
		{SPRN_NUMBER,		SPRN_NUMBER_TIMEOUT},
		{SPRN_MEDIA,		SPRN_MEDIA_TIMEOUT},
		{SPRN_ID,		SPRN_ID_TIMEOUT},
		{SPRN_STATUS,		SPRN_STATUS_TIMEOUT},
		{SPRN_RD_BCODE,		SPRN_RD_BCODE_TIMEOUT},
		{SPRN_LOG,		SPRN_LOG_TIMEOUT},
		{SPRN_LOG1,		SPRN_LOG_TIMEOUT},
		{SPRN_RD_BCODE,		SPRN_BCODE_CTL_TIMEOUT},
		{SPRN_NO_BCODE,		SPRN_TEXT_TIMEOUT},
	};
	int i;
	sprn_op_timeout = SPRN_INFINITE_TIMEOUT;
	for (i = 0; i < ASIZE(map); i++){
		if (map[i].cmd == cmd){
			sprn_op_timeout = map[i].sprn_op_timeout;
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
	rcv_status,		/* ожидание байта завершения команды */
	rcv_number_ctl_status,	/* ожидание кода завершения печати с контролем штрих-кода */
	rcv_wr_param_prefix,	/* ожидание S (установка параметра) */
	rcv_wr_param_number,	/* ожидание номера параметра */
	rcv_wr_param_sign,	/* ожидание знака числового значения параметра (+/-) */
	rcv_wr_param_val,	/* получение числового значения параметра */
	rcv_rd_param_prefix,	/* ожидание R (получение значений всех параметров) */
	rcv_rd_param_vals,	/* получение значений всех параметров */
	rcv_end,		/* окончание приёма данных от БПУ (в т.ч. по таймауту) */
	rcv_idle,		/* игнорирование данных от БПУ */
};

static int rcv_st = rcv_idle;

/* Буфер приёма (для записи данных в журнал) */
static uint8_t sprn_rx[SPRN_RX_BUF_LEN];
/* Длина принятых данных */
static size_t sprn_rx_len = 0;

/* Счётчик для некоторых состояний конечного автомата приёма */
static off_t rcv_idx;

/* Установка состояния конечного автомата приёма */
static void sprn_set_rcv_st(int st)
{
	if (st == rcv_start)
		sprn_rx_len = 0;
	else if ((st == rcv_end) || (st == rcv_idle)){
		if (sprn_rx_len > 0){
			log_data_sprn("БПУ --> ТМ", sprn_rx, sprn_rx_len);
			sprn_rx_len = 0;
		}
		sprn_op_timeout = SPRN_INFINITE_TIMEOUT;
	}
	rcv_st = st;
}

/* Сброс буферов и таймаутов */
static void sprn_reset(void)
{
	sprn_status = SPRN_STATUS_OK;
	memset(sprn_number, 0x30, sizeof(sprn_number));
	sprn_media = SPRN_MEDIA_UNKNOWN;
	sprn_tx_len = sprn_sent_len = 0;
	sprn_sent_cmd = SPRN_NUL;
	sprn_op_timeout = SPRN_INFINITE_TIMEOUT;
	sprn_set_rcv_st(rcv_idle);
	rcv_idx = 0;
}

/* Закрытие устройства для работы с БПУ */
void sprn_close(void)
{
	if (sprn_dev != -1){
		serial_close(sprn_dev);
		sprn_dev = -1;
	}
	sprn_reset();
}

/* Открытие устройства для работы с БПУ */
static bool sprn_open(void)
{
	bool ret = false;
	sprn_close();
	if (sprn != NULL){
		sprn_dev = serial_open(sprn->ttyS_name, &sprn->ss, O_RDWR);
		ret = sprn_dev != -1;
	}
	return ret;
}

/* Запись в буфер передачи простой команды */
static bool sprn_write_cmd(uint8_t cmd)
{
	bool ret = false;
	if (sizeof(sprn_tx) >= 3){
		sprn_reset();
		sprn_has_blank_number = false;
		sprn_tx[0] = SPRN_NUL;
		sprn_tx[1] = SPRN_DLE;
		sprn_tx[2] = cmd;
		sprn_tx_len = 3;
		sprn_sent_len = 0;
		sprn_sent_cmd = cmd;
		sprn_mark_operation();
		sprn_set_timeout(cmd);
		ret = true;
	}
	return ret;
}

/* Запись в буфер передачи текста */
static bool sprn_write_text(const uint8_t *txt, size_t len, bool log)
{
	static uint8_t bcode_tmpl[] = {SPRN_DLE, SPRN_RD_BCODE, 0x3b, 0x3b};
	if ((txt == NULL) || (len == 0) || ((len + 1) > sizeof(sprn_tx)))
		return false;
	sprn_reset();
	sprn_has_blank_number = false;
	sprn_tx[0] = SPRN_NUL;
	memcpy(sprn_tx + 1, txt, len);
	sprn_tx_len = len + 1;
	sprn_sent_len = 0;
	sprn_mark_operation();
	if (log)
		sprn_sent_cmd = SPRN_LOG;
	else if ((len > sizeof(bcode_tmpl)) && (memcmp(txt, bcode_tmpl,
			sizeof(bcode_tmpl)) == 0))
		sprn_sent_cmd = SPRN_RD_BCODE;
	else
		sprn_sent_cmd = SPRN_NO_BCODE;
	sprn_set_timeout(sprn_sent_cmd);
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
	if (l > sizeof(sprn_tx))
		return false;
	sprn_reset();
	sprn_has_blank_number = false;
	memcpy(sprn_tx, prefix, sizeof(prefix));
	memcpy(sprn_tx + sizeof(prefix), data, len);
	sprn_tx_len = sizeof(prefix) + len;
	if (need_ff)
		sprn_tx[sprn_tx_len++] = SPRN_FORM_FEED;
	sprn_tx[sprn_tx_len++] = SPRN_ETX;
	sprn_sent_len = 0;
	sprn_mark_operation();
	sprn_sent_cmd = SPRN_LOG1;
	sprn_set_timeout(sprn_sent_cmd);
	return true;
}

/* Передача данных в БПУ. В случае ошибки возвращает false */
static bool sprn_do_snd(void)
{
	bool ret = true;
	if (sprn_dev == -1)
		ret = false;
	else if (sprn_sent_len < sprn_tx_len){
		ssize_t len = write(sprn_dev, sprn_tx + sprn_sent_len,
			sprn_tx_len - sprn_sent_len);
		if (len < 0){
			if (errno != EWOULDBLOCK){
				log_sys_err("Ошибка записи в %s:", sprn->ttyS_name);
				ret = false;
			}
		}else if (len > 0){
			log_data_sprn("ТМ --> БПУ", sprn_tx + sprn_sent_len, len);
			sprn_sent_len += len;
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
		if (sprn_sent_cmd == 'S')
			sprn_set_rcv_st(rcv_wr_param_prefix);
		else if (sprn_sent_cmd == 'R')
			sprn_set_rcv_st(rcv_rd_param_prefix);
		else{
			log_err("От БПУ получен символ 0x1d, недопустимый в даном контексте.");
			sprn_set_rcv_st(rcv_idle);
		}
	}
}

static void on_rcv_cmd(uint8_t b)
{
	if (b != sprn_sent_cmd){
		log_err("БПУ отправлена команда 0x%.2hhx, а в ответ пришла 0x%.2hhx.", sprn_sent_cmd, b);
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
				log_err("От БПУ получен неизвестный код команды: 0x%.2hhx.", b);
				sprn_set_rcv_st(rcv_idle);
		}
	}
}

static void on_rcv_number_status(uint8_t b)
{
	sprn_status = b;
	if (sprn_ok(b)){
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
		log_err("Неверный символ штрих-кода: 0x%.2hhx.", b);
		sprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_media_status(uint8_t b)
{
	sprn_status = b;
	if (sprn_ok(b))
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
			log_err("Неизвестный тип носителя в БПУ: 0x%.2hhx.", b);
			sprn_set_rcv_st(rcv_idle);
			break;
	}
}

static void on_rcv_id_status(uint8_t b)
{
	sprn_status = b;
	if (sprn_ok(b)){
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
		log_err("Неверный символ в номере БПУ: 0x%.2hhx.", b);
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
		log_err("При установке параметра S%c вместо 'S' получен 0x%.2hhx.",
			sprn_param_number, b);
		sprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_wr_param_number(uint8_t b)
{
	if (b == sprn_param_number){
		sprn_param_value = 0;
		sprn_param_sign = 1;
		rcv_idx = 0;
		sprn_set_rcv_st(rcv_wr_param_sign);
	}else{
		log_err("Устанавливаем параметр S%c, а в ответ пришёл номер %c.",
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
		log_err("Неверное значение параметра S%c.", sprn_param_number);
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
		log_err("При чтении параметров вместо 'R' получен 0x%.2hhx.", b);
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
		log_err("В ответ на запрос параметров пришёл неверный символ: 0x%.2hhx.", b);
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

static bool sprn_do_rcv(void)
{
	bool ret = true;
	uint8_t b;
	ssize_t len = read(sprn_dev, &b, 1);
	if ((len == -1) && (errno != EWOULDBLOCK)){
		log_sys_err("Ошибка чтения из %s: ", sprn->ttyS_name);
		ret = false;
	}else if (len == 1){
		if (sprn_rx_len < sizeof(sprn_rx))
			sprn_rx[sprn_rx_len++] = b;
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
		}
	}
	return ret;
}

/* Обработка подсистемы печати на БПУ */
static bool sprn_process(void)
{
	bool ret = true;
	sprn_timeout = (sprn_op_timeout != SPRN_INFINITE_TIMEOUT) &&
		((u_times() - sprn_t0) > sprn_op_timeout);
	if (sprn_timeout){
		sprn_set_rcv_st(rcv_idle);
		ret = false;
	}else if (sprn_sent_len < sprn_tx_len){
		if (!sprn_do_snd()){
			sprn_reset();
			ret = false;
		}else if (sprn_sent_len >= sprn_tx_len)
			sprn_set_rcv_st(rcv_start);
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
	while ((sprn_sent_len < sprn_tx_len) ||
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
	sprn_status = SPRN_STATUS_OK;
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
	return sprn_do_cmd(SPRN_NUMBER);
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
				if (sprn_ok(sprn_status)){
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
		sprn_tx[0] = SPRN_NUL;
		sprn_tx[1] = SPRN_DLE2;
		sprn_tx[2] = 'R';
		sprn_tx_len = 3;
		sprn_sent_len = 0;
		sprn_sent_cmd = 'R';
		sprn_mark_operation();
		sprn_op_timeout = SPRN_RD_PARAMS_TIMEOUT;
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
	sprn_tx[0] = SPRN_NUL;
	sprn_tx[1] = SPRN_DLE2;
	sprn_tx[2] = 'S';
	sprn_tx[3] = n + 0x30;
	sprn_tx_len = 4;
	if (val < 0){
		sprn_tx[sprn_tx_len++] = '-';
		val *= -1;
	}
	sprn_tx[sprn_tx_len++] = ((val / 100) % 100) + 0x30;
	sprn_tx[sprn_tx_len++] = ((val / 10) % 10) + 0x30;
	sprn_tx[sprn_tx_len++] = (val % 10) + 0x30;
	sprn_sent_len = 0;
	sprn_sent_cmd = 'S';
	sprn_param_number = n + 0x30;
	sprn_mark_operation();
	sprn_op_timeout = SPRN_WR_PARAM_TIMEOUT;
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

/* Получение информации об ошибке БПУ на основании её кода */
const struct sprn_error_txt *sprn_get_error_txt(uint8_t code)
{
	static struct {
		uint8_t code;
		int len;
		const char *txt;
	} err[] = {
		{
			.code		= SPRN_STATUS_NO_NUMBER,
			.len		= -1,
			.txt		= "НОМЕР БПУ НЕ ПРОПИСАН В ПАМЯТИ"
		},
		{
			.code		= SPRN_STATUS_NO_BCODE,
			.len		= -1,
			.txt		= "ШТРИХОВОЙ КОД ОТСУТСТВУЕТ"
		},
		{
			.code		= SPRN_STATUS_BCODE_LEN,
			.len		= -1,
			.txt		= "ДЛИНА ШТРИХОВОГО КОДА НЕ РАВНА 13"
		},
		{
			.code		= SPRN_STATUS_BLANK_NR,
			.len		= -1,
			.txt		= "НЕСОВПАДЕНИЕ НОМЕРА БЛАНКА"
		},
		{
			.code		= SPRN_STATUS_BCODE_CRC,
			.len		= -1,
			.txt		= "ОШИБКА КОНТРОЛЬНОЙ СУММ\x9b"
		},
		{
			.code		= SPRN_STATUS_ZERO_NR,
			.len		= -1,
			.txt		= "НОМЕР БЛАНКА XXXXXXX000000",
		},
		{
			.code		= SPRN_STATUS_PAPER_END,
			.len		= -1,
			.txt		= "КОНЕЦ БУМАГИ"
		},
		{
			.code		= SPRN_STATUS_COVER_OPEN,
			.len		= -1,
			.txt		= "КР\x9bШКА ОТКР\x9bТА",
		},
		{
			.code		= SPRN_STATUS_PAPER_LOCK,
			.len		= -1,
			.txt		= "БУМАГА ЗАСТРЯЛА НА В\x9bХОДЕ",
		},
		{
			.code		= SPRN_STATUS_PAPER_WRACK,
			.len		= -1,
			.txt		= "БУМАГА ЗАМЯЛАСЬ",
		},
		{
			.code		= SPRN_STATUS_NOTCH_ERR,
			.len		= -1,
			.txt		= "ОШИБКА ЧТЕНИЯ РЕПЕРНОЙ МЕТКИ",
		},
		{
			.code		= SPRN_STATUS_SCANNER_ERR,
			.len		= -1,
			.txt		= "АППАРАТНАЯ ОШИБКА СКАНЕРА ШТРИХ-КОДА",
		},
		{
			.code		= SPRN_STATUS_MEDIA_ERR,
			.len		= -1,
			.txt		= "ОШИБКА НОСИТЕЛЯ",
		},
		{
			.code		= SPRN_STATUS_BLANK_SKEW,
			.len		= -1,
			.txt		= "ПЕРЕКОС БЛАНКА",
		},
		{
			.code		= SPRN_STATUS_HW_ERR,
			.len		= -1,
			.txt		= "ОБЩАЯ АППАРАТНАЯ ОШИБКА ПРИНТЕРА",
		},
		{
			.code		= SPRN_STATUS_NO_FFEED,
			.len		= -1,
			.txt		= "НЕТ КОМАНД\x9b ОТРЕЗКИ БЛАНКА",
		},
		{
			.code		= SPRN_STATUS_VPOS_OVER,
			.len		= -1,
			.txt		= "ПРЕВ\x9bШЕНИЕ ОБ'ЕМА ТЕКСТА ПО ВЕРТИКАЛЬН\x9bМ ПОЗИЦИЯМ",
		},
		{
			.code		= SPRN_STATUS_HPOS_OVER,
			.len		= -1,
			.txt		= "ПРЕВ\x9bШЕНИЕ ОБ'ЕМА ТЕКСТА ПО ГОРИЗОНТАЛЬН\x9bМ ПОЗИЦИЯМ",
		},
		{
			.code		= SPRN_STATUS_LOG_ERR,
			.len		= -1,
			.txt		= "НАРУШЕНИЕ СТРУКТУР\x9b ИНФОРМАЦИИ ПРИ ПЕЧАТИ КЛ",
		},
		{	.code		= SPRN_STATUS_BC_CMD_ERR,
			.len		= -1,
			.txt		= "НАРУШЕНИЕ ФОРМАТА КОМАНД\x9b АНАЛИЗА ШТРИХОВОГО КОДА",
		},
		{	.code		= SPRN_STATUS_GRID_ERROR,
			.len		= -1,
			.txt		= "НАРУШЕНИЕ ПАРАМЕТРОВ НАНЕСЕНИЯ МАКЕТО",
		},
		{
			.code		= SPRN_STATUS_INVALID_ARG,
			.len		= -1,
			.txt		= "НЕВЕРН\x9bЙ ПАРАМЕТР",
		},
		{
			.code		= SPRN_STATUS_NO_ICON,
			.len		= -1,
			.txt		= "ПИКТОГРАММА НЕ НАЙДЕНА В БПУ",
		},
		{
			.code		= SPRN_STATUS_GRID_WIDTH,
			.len		= -1,
			.txt		= "ШИРИНА СЕТКИ БОЛЬШЕ ШИРИН\x9b БЛАНКА В УСТАНОВКАХ",
		},
		{
			.code		= SPRN_STATUS_GRID_HEIGHT,
			.len		= -1,
			.txt		= "В\x9bСОТА СЕТКИ БОЛЬШЕ В\x9bСОТ\x9b БЛАНКА В УСТАНОВКАХ",
		},
		{
			.code		= SPRN_STATUS_GRID_NM_FMT,
			.len		= -1,
			.txt		= "НЕПРАВИЛЬН\x9bЙ ФОРМАТ ИМЕНИ СЕТКИ",
		},
		{
			.code		= SPRN_STATUS_GRID_NM_LEN,
			.len		= -1,
			.txt		= "ДЛИНА ИМЕНИ СЕТКИ БОЛЬШЕ ДОПУСТИМОЙ",
		},
		{
			.code		= SPRN_STATUS_GRID_NR,
			.len		= -1,
			.txt		= "НЕВЕРН\x9bЙ ФОРМАТ НОМЕРА СЕТКИ",
		},
		{
			.code		= 0xff,
			.len		= -1,
			.txt		= "НЕИЗВЕСТНАЯ ОШИБКА"
		},
	};
	for (int i = 0; i < ASIZE(err); i++){
		if (err[i].len == -1)
			err[i].len = strlen(err[i].txt);
		if ((err[i].code == 0xff) || (code == err[i].code))
			return (struct sprn_error_txt *)&err[i].len;
	}
	return NULL;
}
