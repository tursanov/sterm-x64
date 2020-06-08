/* Работа с пригородным печатающим устройством (ППУ). (c) gsr 2009 */

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gui/scr.h"
#include "log/local.h"
#include "prn/local.h"
#include "blimits.h"
#include "cfg.h"
#include "gd.h"
#include "serial.h"
#include "sterm.h"

/* Статус ППУ */
uint8_t lprn_status = 0;
/* Статус SD-карты памяти */
uint8_t lprn_sd_status = 0;
/* Заводской номер ППУ */
uint8_t lprn_number[LPRN_NUMBER_LEN] = {[0 ... LPRN_NUMBER_LEN - 1] = 0x30};
/* Тип носителя в ППУ */
uint8_t lprn_media = LPRN_MEDIA_UNKNOWN;
/* Номер документа в ППУ */
uint8_t lprn_blank_number[LPRN_BLANK_NUMBER_LEN] = {[0 ... LPRN_BLANK_NUMBER_LEN - 1] = 0x30};
/* Из ППУ получен номер бланка */
bool lprn_has_blank_number = false;

/* Номер параметра конфигурации ППУ (0x1d Sx) */
static uint8_t lprn_param_number;
/* Знак параметра конфигурации ППУ (1 или -1) */
static int lprn_param_sign = 1;
/* Значение параметра конфигурации ППУ */
static int lprn_param_value = 0;
/* Значения параметров печати, получаемые по команде 0x1d R */
static uint8_t lprn_params[41];
/* В строке параметров должно быть 10 символов ';' */
static int nr_semicolons;
/* Флаг получения всех параметров ППУ */
static bool lprn_params_received = false;

/* Имя файла устройства для работы с ППУ */
#define LPRN_DEV_NAME		"/dev/ttyUSB0"
/* Устройство для работы с ППУ */
static int lprn_dev = -1;

/* Данные для передачи ППУ */
static uint8_t snd_data[PRN_BUF_LEN];
/* Размер данных для передачи ППУ */
static size_t snd_data_len;
/* Размер данных, переданных ППУ */
static size_t sent_len;
/* Код команды, отправленной в ППУ */
static uint8_t sent_cmd = LPRN_NUL;

/* Время начала очередной транзакции с ППУ */
static uint32_t t0;
/* Таймаут текущей операции. 0 -- бесконечный таймаут. */
static uint32_t timeout = LPRN_INFINITE_TIMEOUT;
/* Флаг превышения таймаута при выполнении операции */
bool lprn_timeout = false;

/* Установка начала текущей операции */
static void lprn_mark_operation(void)
{
	t0 = u_times();
}

/* Установка таймаута в соответствии с командой */
static void lprn_set_timeout(uint8_t cmd)
{
	static struct {
		uint8_t cmd;
		uint32_t timeout;
	} map[] = {
		{LPRN_BCODE,		LPRN_BCODE_TIMEOUT},
		{LPRN_MEDIA,		LPRN_MEDIA_TIMEOUT},
		{LPRN_INIT,		LPRN_INIT_TIMEOUT},
		{LPRN_STATUS,		LPRN_STATUS_TIMEOUT},
		{LPRN_LOG,		LPRN_LOG_TIMEOUT},
		{LPRN_LOG1,		LPRN_LOG_TIMEOUT},
		{LPRN_RD_BCODE,		LPRN_BCODE_CTL_TIMEOUT},
		{LPRN_NO_BCODE,		LPRN_TEXT_TIMEOUT},
		{LPRN_SNAPSHOTS,	LPRN_SNAPSHOTS_TIMEOUT},
		{LPRN_ERASE_SD,		LPRN_ERASE_SD_TIMEOUT},
	};
	int i;
	timeout = LPRN_INFINITE_TIMEOUT;
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
	rcv_bcode_status,	/* ожидание кода завершения чтения номера бланка */
	rcv_bcode,		/* получение номера бланка */
	rcv_media_status,	/* ожидание кода завершения команды получения типа носителя */
	rcv_media_type,		/* ожидание кода типа носителя */
	rcv_init_status,	/* ожидание кода завершения инициализации ППУ */
	rcv_number,		/* получение заводского номера ППУ */
	rcv_xstatus,		/* ожидание байта статуса ППУ */
	rcv_sd_status,		/* ожидание байта статуса SD-карты */
	rcv_status,		/* ожидание байта завершения команды */
	rcv_bcode_ctl_status,	/* ожидание кода завершения печати с контролем штрих-кода */
	rcv_wr_param_prefix,	/* ожидание S (установка параметра) */
	rcv_wr_param_number,	/* ожидание номера параметра */
	rcv_wr_param_sign,	/* ожидание знака числового значения параметра (+/-) */
	rcv_wr_param_val,	/* получение числового значения параметра */
	rcv_rd_param_prefix,	/* ожидание R (получение значений всех параметров) */
	rcv_rd_param_vals,	/* получение значений всех параметров */
	rcv_time_sync,		/* ожидание кода завершения команды синхронизации времени */ 
	rcv_end,		/* окончание приёма данных от ППУ (в т.ч. по таймауту) */
	rcv_idle,		/* игнорирование данных от ППУ */
};

static int rcv_st = rcv_idle;

/* Счётчик для некоторых состояний конечного автомата приёма */
static off_t rcv_idx;

#if defined __LOG_LPRN__
/* Имя файла для записи обмена с ППУ */
#define LPRN_LOG_FILE	"/tmp/local.txt"

/* 
 * Текущий маркер направления обмена (используется в начале каждой строки
 * в файле обмена)
 */
static bool lprn_log_rcv = true;

/* Индекс текущего элемента в массиве для одного направления */
static off_t lprn_log_idx;

/* Количество элементов в строке файла обмена */
#define LPRN_LOG_STR_ELEMS	16
/*
 * Количество элементов в группе. Группы в составе строки отделяются
 * друг от друга дополнительными пробелами.
 */
#define LPRN_LOG_GROUP_LEN	4

/* Буфер для вывода данных в файл обмена в текстовом виде по окончании строки */
static uint8_t lprn_data_buf[LPRN_LOG_STR_ELEMS];

/* Перед началом работы с ППУ создаём файл обмена */
bool lprn_create_log(void)
{
	bool ret = false;
	int fd = open(LPRN_LOG_FILE, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (fd == -1)
		fprintf(stderr, "Ошибка создания файла обмена с ППУ: %s.\n",
			strerror(errno));
	else{
		close(fd);
		ret = true;
	}
	return ret;
}

/* Вывод в файл обмена текстового представления символов */
static void lprn_log_txt(FILE *f, uint8_t *data, size_t len)
{
	size_t i;
	uint8_t b;
	fprintf(f, "    ");
	for (i = 0; i < len; i++){
		b = data[i];
		fputc(b < 0x20 ? '.' : recode(b), f);
	}
	fprintf(f, "\r\n");
}

/* Запись массива байтов в файл обмена */
static bool lprn_log_data(const uint8_t *data, size_t len)
{
	size_t i;
	FILE *f = fopen(LPRN_LOG_FILE, "a");
	if (f == NULL){
		fprintf(stderr, "Ошибка открытия " LPRN_LOG_FILE
			" для добавления данных: %s.\n", strerror(errno));
		return false;
	}
	for (i = 0; i < len; i++){
		if ((lprn_log_idx % LPRN_LOG_STR_ELEMS) == 0){
			if (lprn_log_idx != 0)
				lprn_log_txt(f, lprn_data_buf, sizeof(lprn_data_buf));
			fprintf(f, "%c %.4x: ", lprn_log_rcv ? 'R' : 'W',
				(uint32_t)lprn_log_idx);
		}else if ((lprn_log_idx % LPRN_LOG_GROUP_LEN) == 0)
			fputc(' ', f);
		fprintf(f, " %.2hhX", data[i]);
		lprn_data_buf[lprn_log_idx++ % sizeof(lprn_data_buf)] = data[i];
	}
	fclose(f);
	return true;
}

/* Запись одного байта в файл обмена */
static bool lprn_log_byte(uint8_t b)
{
	return lprn_log_data(&b, 1);
}

/* Вывод в файл обмена последней строки для текущего направления */
static bool lprn_flush_log_data(void)
{
	bool ret = false;
	size_t i, l;
	FILE *f = fopen(LPRN_LOG_FILE, "a");
	if (f == NULL){
		fprintf(stderr, "Ошибка открытия " LPRN_LOG_FILE
			" для добавления данных: %s.\n", strerror(errno));
		return false;
	}else{
		if (lprn_log_idx > 0){
			l = lprn_log_idx % LPRN_LOG_STR_ELEMS;
			if (l == 0)
				l = LPRN_LOG_STR_ELEMS;
			for (i = l; i < LPRN_LOG_STR_ELEMS; i++){
				if ((i % LPRN_LOG_GROUP_LEN) == 0)
					fputc(' ', f);
				fprintf(f, "   ");
			}
			lprn_log_txt(f, lprn_data_buf, l);
			fprintf(f, "\r\n");
			ret = true;
		}
		fclose(f);
	}
	lprn_log_idx = 0;
	return ret;
}
#endif		/* __LOG_LPRN__ */

/* Установка состояния конечного автомата приёма */
static void lprn_set_rcv_st(int st)
{
	if ((st == rcv_end) || (st == rcv_idle))
		timeout = LPRN_INFINITE_TIMEOUT;
	rcv_st = st;
}

/* Сброс буферов и таймаутов */
static void lprn_reset(void)
{
/*	lprn_status = 0;
	lprn_sd_status = 0;
	memset(lprn_number, 0x30, sizeof(lprn_number));
	lprn_media = LPRN_MEDIA_UNKNOWN;*/
	snd_data_len = sent_len = 0;
	sent_cmd = LPRN_NUL;
	timeout = LPRN_INFINITE_TIMEOUT;
	lprn_set_rcv_st(rcv_idle);
	rcv_idx = 0;
#if defined __LOG_LPRN__
	lprn_flush_log_data();
#endif
}

/* Закрытие устройства для работы с ППУ */
void lprn_close(void)
{
/*	printf("%s: lprn_dev = %d.\n", __func__, lprn_dev);*/
	if (lprn_dev != -1){
		serial_close(lprn_dev);
		lprn_dev = -1;
	}
	lprn_reset();
}

/* Открытие устройства для работы с ППУ */
static bool lprn_open(void)
{
	struct serial_settings ss = {
		.csize		= CS8,
		.parity		= SERIAL_PARITY_NONE,
		.stop_bits	= SERIAL_STOPB_1,
		.control	= SERIAL_FLOW_RTSCTS,
		.baud		= B115200,
	};
	lprn_close();
	lprn_dev = serial_open(LPRN_DEV_NAME, &ss, O_RDWR);
	return lprn_dev != -1;
}

/* Запись в буфер передачи простой команды */
static bool lprn_write_cmd(uint8_t cmd)
{
	if (sizeof(snd_data) >= 3){
		lprn_reset();
		lprn_has_blank_number = false;
		snd_data[0] = LPRN_NUL;
		snd_data[1] = LPRN_DLE;
		snd_data[2] = cmd;
		snd_data_len = 3;
		sent_len = 0;
		sent_cmd = cmd;
		lprn_mark_operation();
		lprn_set_timeout(cmd);
#if defined __LOG_LPRN__
		lprn_log_rcv = false;
#endif
		return true;
	}else
		return false;
}

/* Запись в буфер передачи текста */
static bool lprn_write_text(const uint8_t *txt, size_t len, bool log)
{
	static uint8_t bcode_tmpl[] = {LPRN_DLE, LPRN_RD_BCODE, 0x3b, 0x3b};
	if ((txt == NULL) || (len == 0) || ((len + 1) > sizeof(snd_data)))
		return false;
/*	printf("%s: txt = [%.*s].\n", __func__, len - 1, txt + 1);*/
	lprn_reset();
	lprn_has_blank_number = false;
	snd_data[0] = LPRN_NUL;
	memcpy(snd_data + 1, txt, len);
	snd_data_len = len + 1;
	sent_len = 0;
	lprn_mark_operation();
	if (log)
		sent_cmd = LPRN_LOG;
	else if ((len > sizeof(bcode_tmpl)) && (memcmp(txt, bcode_tmpl,
			sizeof(bcode_tmpl)) == 0))
		sent_cmd = LPRN_RD_BCODE;
	else
		sent_cmd = LPRN_NO_BCODE;
	lprn_set_timeout(sent_cmd);
#if defined __LOG_LPRN__
	lprn_log_rcv = false;
#endif
	return true;
}

/* Запись в буфер передачи текста для печати на контрольной ленте (Ар2 Э) */
static bool lprn_write_text_log1(const uint8_t *data, size_t len)
{
	static uint8_t prefix[] = {LPRN_NUL, LPRN_DLE, LPRN_LOG1, LPRN_STX};
	size_t l = sizeof(prefix) + len + 1;
	bool need_ff;
	if ((data == NULL) || (len == 0))
		return false;
	need_ff = data[len - 1] != LPRN_FORM_FEED;
	if (need_ff)
		l++;
	if (l > sizeof(snd_data))
		return false;
	lprn_reset();
	lprn_has_blank_number = false;
	memcpy(snd_data, prefix, sizeof(prefix));
	memcpy(snd_data + sizeof(prefix), data, len);
	snd_data_len = sizeof(prefix) + len;
	if (need_ff)
		snd_data[snd_data_len++] = LPRN_FORM_FEED;
	snd_data[snd_data_len++] = LPRN_ETX;
	sent_len = 0;
	lprn_mark_operation();
	sent_cmd = LPRN_LOG1;
	lprn_set_timeout(sent_cmd);
#if defined __LOG_LPRN__
	lprn_log_rcv = false;
#endif
	return true;
}

/* Передача данных в ППУ. В случае ошибки возвращает false */
static bool lprn_do_snd(void)
{
	bool ret = true;
	if (lprn_dev == -1)
		ret = false;
	else if (sent_len < snd_data_len){
		ssize_t len = write(lprn_dev, snd_data + sent_len,
			snd_data_len - sent_len);
		if (len < 0){
			if (errno != EWOULDBLOCK){
				fprintf(stderr, "Ошибка записи в %s: %s.\n",
					fd2name(lprn_dev), strerror(errno));
#if defined __LOG_LPRN__
				lprn_log_byte(errno | 0x80);
				lprn_flush_log_data();
#endif
				ret = false;
			}
		}else if (len > 0){
#if defined __LOG_LPRN__
			lprn_log_data(snd_data + sent_len, len);
#endif
			sent_len += len;
		}
	}
	return ret;
}

/* Обработка состояний конечного автомата приёма */
static void on_rcv_start(uint8_t b)
{
	if (b == LPRN_DLE1)
		lprn_set_rcv_st(rcv_cmd);
	else if (b == LPRN_DLE2){
		if (sent_cmd == 'S')
			lprn_set_rcv_st(rcv_wr_param_prefix);
		else if (sent_cmd == 'R')
			lprn_set_rcv_st(rcv_rd_param_prefix);
		else{
			fprintf(stderr, "От ППУ получен символ 0x1d, недопустимый в даном контексте.\n");
			lprn_set_rcv_st(rcv_idle);
		}
	}
}

static void on_rcv_cmd(uint8_t b)
{
	if (b != sent_cmd){
		fprintf(stderr, "ППУ отправлена команда 0x%.2hhx, а в ответ пришла 0x%.2hhx.\n",
			sent_cmd, b);
		lprn_set_rcv_st(rcv_idle);
	}else{
		switch (b){
			case LPRN_BCODE:
				lprn_set_rcv_st(rcv_bcode_status);
				break;
			case LPRN_MEDIA:
				lprn_set_rcv_st(rcv_media_status);
				break;
			case LPRN_INIT:
				lprn_set_rcv_st(rcv_init_status);
				break;
			case LPRN_STATUS:
			case LPRN_SNAPSHOTS:
				lprn_set_rcv_st(rcv_xstatus);
				break;
			case LPRN_LOG:
			case LPRN_LOG1:
			case LPRN_NO_BCODE:
				lprn_set_rcv_st(rcv_status);
				break;
			case LPRN_RD_BCODE:
				lprn_set_rcv_st(rcv_bcode_ctl_status);
				break;
			case LPRN_ERASE_SD:
				lprn_set_rcv_st(rcv_sd_status);
				break;
			default:
				fprintf(stderr, "От ППУ получен неизвестный код команды: 0x%.2hhx.\n", b);
				lprn_set_rcv_st(rcv_idle);
		}
	}
}

static void on_rcv_bcode_status(uint8_t b)
{
	lprn_status = b;
	if (b == 0){
		lprn_set_rcv_st(rcv_bcode);
		rcv_idx = 0;
	}else
		lprn_set_rcv_st(rcv_end);
}

static void on_rcv_bcode(uint8_t b)
{
	if ((b >= 0x30) && (b <= 0x39)){
		lprn_blank_number[rcv_idx++] = b;
		if (rcv_idx >= sizeof(lprn_blank_number)){
			lprn_has_blank_number = true;
			lprn_set_rcv_st(rcv_end);
		}
	}else{
		fprintf(stderr, "Неверный символ штрих-кода: 0x%.2hhx.\n", b);
		lprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_media_status(uint8_t b)
{
	lprn_status = b;
	if (b == 0)
		lprn_set_rcv_st(rcv_media_type);
	else
		lprn_set_rcv_st(rcv_end);
}

static void on_rcv_media_type(uint8_t b)
{
	switch (b){
		case LPRN_MEDIA_BLANK:
		case LPRN_MEDIA_PAPER:
		case LPRN_MEDIA_BOTH:
			lprn_media = b;
			lprn_set_rcv_st(rcv_end);
			break;
		default:
			fprintf(stderr, "Неизвестный тип носителя в ППУ: 0x%.2hhx.\n", b);
			lprn_set_rcv_st(rcv_idle);
			break;
	}
}

static void on_rcv_init_status(uint8_t b)
{
	lprn_status = b;
	if (b == 0){
		rcv_idx = 0;
		lprn_set_rcv_st(rcv_number);
	}else
		lprn_set_rcv_st(rcv_end);
}

static void on_rcv_number(uint8_t b)
{
	if ((b >= 0x20) && (b <= 0x7e)){
		if (rcv_idx < sizeof(lprn_number))
			lprn_number[rcv_idx++] = b;
		if (rcv_idx >= sizeof(lprn_number))
			lprn_set_rcv_st(rcv_end);
	}else{
		fprintf(stderr, "Неверный символ в номере ППУ: 0x%.2hhx.\n", b);
		lprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_status(uint8_t b)
{
	lprn_status = b;
	lprn_set_rcv_st(rcv_end);
}

static void on_rcv_xstatus(uint8_t b)
{
	lprn_status = b;
	lprn_set_rcv_st(rcv_sd_status);
}

static void on_rcv_sd_status(uint8_t b)
{
	lprn_sd_status = b;
	lprn_set_rcv_st(rcv_end);
}

static void on_rcv_bcode_ctl_status(uint8_t b)
{
	lprn_status = b;
	rcv_idx = 0;
	lprn_set_rcv_st(rcv_bcode);
}

static void on_rcv_wr_param_prefix(uint8_t b)
{
	if (b == 'S')
		lprn_set_rcv_st(rcv_wr_param_number);
	else{
		fprintf(stderr, "При установке параметра S%c вместо 'S' "
			"получен 0x%.2hhx.\n", lprn_param_number, b);
		lprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_wr_param_number(uint8_t b)
{
	if (b == lprn_param_number){
		if (b == 0x41)
			lprn_set_rcv_st(rcv_time_sync);
		else{
			lprn_param_value = 0;
			lprn_param_sign = 1;
			rcv_idx = 0;
			lprn_set_rcv_st(rcv_wr_param_sign);
		}
	}else{
		fprintf(stderr, "Устанавливаем параметр S%c, а в ответ пришёл номер %c.\n",
			lprn_param_number, b);
		lprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_wr_param_val(uint8_t b)
{
	if ((b >= 0x30) && (b <= 0x39)){
		lprn_param_value *= 10;
		lprn_param_value += b - 0x30;
		if (++rcv_idx == 3){
			lprn_param_value *= lprn_param_sign;
			lprn_set_rcv_st(rcv_end);
		}
	}else{
		fprintf(stderr, "Неверное значение параметра S%c.\n",
			lprn_param_number);
		lprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_wr_param_sign(uint8_t b)
{
	if (b == '-'){
		lprn_param_sign = -1;
		lprn_set_rcv_st(rcv_wr_param_val);
	}else{
		lprn_set_rcv_st(rcv_wr_param_val);
		on_rcv_wr_param_val(b);
	}
}

static void on_rcv_rd_param_prefix(uint8_t b)
{
	if (b == 'R'){
		rcv_idx = nr_semicolons = 0;
		lprn_set_rcv_st(rcv_rd_param_vals);
	}else{
		fprintf(stderr, "При чтении параметров вместо 'R' получен "
			"0x%.2hhx.\n", b);
		lprn_set_rcv_st(rcv_idle);
	}
}

static void on_rcv_rd_param_vals(uint8_t b)
{
	if (((b >= 0x30) && (b <= 0x39)) || (b == '-') || (b == ';')){
		if (rcv_idx < sizeof(lprn_params))
			lprn_params[rcv_idx++] = b;
		if (b == ';'){
			nr_semicolons++;
			if (nr_semicolons == 10){
				lprn_params_received = true;
				lprn_set_rcv_st(rcv_end);
			}
		}
	}else{
		fprintf(stderr, "В ответ на запрос параметров пришёл "
			"неверный символ: 0x%.2hhx.\n", b);
		lprn_set_rcv_st(rcv_idle);
	}
}

/*
 * Чтение трёхзначного десятичного числа. Возвращает указатель на следующий
 * после последнего прочитанного байт в буфере или NULL в случае ошибки.
 */
static const uint8_t *lprn_read_val(const uint8_t *buf, bool has_sign, int *val)
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
static bool lprn_translate_params(struct term_cfg *cfg)
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
	const uint8_t *pp = lprn_params;
	if ((cfg == NULL) || !lprn_params_received)
		return false;
	for (i = 0; i < ASIZE(map); i++){
		p = (int *)(((uint8_t *)cfg) + map[i].offs);
		pp = lprn_read_val(pp, map[i].has_sign, p);
		if ((pp == NULL) || (*pp != ';'))
			break;
		pp++;
	}
	return i == ASIZE(map);
}

static void on_rcv_time_sync(uint8_t b)
{
	lprn_sd_status = b;
	lprn_set_rcv_st(rcv_end);
}

static bool lprn_do_rcv(void)
{
	bool ret = true;
	uint8_t b;
	ssize_t len;
	len = read(lprn_dev, &b, 1);
	if ((len == -1) && (errno != EWOULDBLOCK)){
		fprintf(stderr, "Ошибка чтения из %s: %s.\n",
			fd2name(lprn_dev), strerror(errno));
#if defined __LOG_LPRN__
		lprn_log_byte(errno | 0x80);
		lprn_flush_log_data();
#endif
		ret = false;
	}else if (len == 1){
/*		printf("%s: b = 0x%.2hhx; rcv_st = %d.\n", __func__,
			b, rcv_st);*/
#if defined __LOG_LPRN__
		lprn_log_byte(b);
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
			case rcv_bcode_status:
				on_rcv_bcode_status(b);
				break;
			case rcv_bcode:
				on_rcv_bcode(b);
				break;
			case rcv_media_status:
				on_rcv_media_status(b);
				break;
			case rcv_media_type:
				on_rcv_media_type(b);
				break;
			case rcv_init_status:
				on_rcv_init_status(b);
				break;
			case rcv_number:
				on_rcv_number(b);
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
			case rcv_bcode_ctl_status:
				on_rcv_bcode_ctl_status(b);
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

/* Обработка подсистемы печати на ППУ */
static bool lprn_process(void)
{
	bool ret = true;
	lprn_timeout = (timeout != LPRN_INFINITE_TIMEOUT) &&
		((u_times() - t0) > timeout);
	if (lprn_timeout){
		lprn_set_rcv_st(rcv_idle);
		ret = false;
#if defined __LOG_LPRN__
		lprn_flush_log_data();
#endif
	}else if (sent_len < snd_data_len){
		if (!lprn_do_snd()){
			lprn_reset();
			ret = false;
		}else if (sent_len >= snd_data_len){
			lprn_set_rcv_st(rcv_start);
#if defined __LOG_LPRN__
			lprn_flush_log_data();
			lprn_log_rcv = true;
#endif
		}
	}else if (!lprn_do_rcv()){
		lprn_reset();
		ret = false;
	}
	return ret;
}

/* Ожидание завершения выполнения команды ППУ */
static int lprn_wait_op(bool show_status)
{
	int ret = LPRN_RET_ERR;
/*	set_term_astate(ast_none);*/
	while ((sent_len < snd_data_len) ||
			((rcv_st != rcv_end) && (rcv_st != rcv_idle))){
		if (lprn_process()){
			if (((get_cmd(false, true) == cmd_reset) &&
					reset_term(false)) || (kt == key_none)){
				lprn_reset();
				ret = LPRN_RET_RST;
			}
		}else
			break;
	}
	if (rcv_st == rcv_end)
		ret = LPRN_RET_OK;
	else if (ret != LPRN_RET_RST){
		if (show_status)
			set_term_astate(ast_nolprn);
	}
	return ret;
}

/* Начало передачи в ППУ команды */
static int lprn_do_cmd(uint8_t cmd)
{
	int ret = LPRN_RET_ERR;
	lprn_status = lprn_sd_status = 0;
	if ((lprn_dev == -1) && !lprn_open())
		set_term_astate(ast_nolprn);
	else{
		if (lprn_write_cmd(cmd))
			ret = lprn_wait_op(true);
		else{
			set_term_astate(ast_nolprn);
			lprn_reset();
		}
	}
	return ret;
}

/* Инициализация ППУ и получение его заводского номера */
int lprn_init(void)
{
	int ret;
	memset(lprn_number, 0x30, sizeof(lprn_number));
	ret = lprn_do_cmd(LPRN_INIT);
	if ((ret != LPRN_RET_OK) || (lprn_status != 0))
		memset(lprn_number, 0x30, sizeof(lprn_number));
	return ret;
}

/* Возвращает true, если заводской номер ППУ состоит из одних нулей */
bool lprn_is_zero_number(void)
{
	int i;
	for (i = 0; i < sizeof(lprn_number); i++){
		if (lprn_number[i] != 0x30)
			break;
	}
	return i == sizeof(lprn_number);
}

/* Получение статуса ППУ */
int lprn_get_status(void)
{
	return lprn_do_cmd(LPRN_STATUS);
}

/* Получение типа носителя в ППУ */
int lprn_get_media_type(void)
{
	return lprn_do_cmd(LPRN_MEDIA);
}

/* Получение номера документа в ППУ */
int lprn_get_blank_number(void)
{
	return lprn_do_cmd(LPRN_BCODE);
}

/* Печать сохранённых изображений */
int lprn_snapshots(void)
{
	return lprn_do_cmd(LPRN_SNAPSHOTS);
}

/* Очистка SD-карты */
int lprn_erase_sd(void)
{
	return lprn_do_cmd(LPRN_ERASE_SD);
}

/* Печать абзаца контрольной ленты */
int lprn_print_log(const uint8_t *data, size_t len)
{
	int ret = LPRN_RET_ERR;
	if ((lprn_dev == -1) && !lprn_open())
		set_term_astate(ast_nolprn);
	else{
		if (lprn_write_text(data, len, true))
			ret = lprn_wait_op(true);
		else{
			set_term_astate(ast_nolprn);
			lprn_reset();
		}
	}
	return ret;
}

/* Печать абзаца контрольной ленты, пришедшего из "Экспресс" (Ар2 Э) */
int lprn_print_log1(const uint8_t *data, size_t len)
{
	int ret = LPRN_RET_ERR;
	if ((lprn_dev == -1) && !lprn_open())
		set_term_astate(ast_nolprn);
	else{
		if (lprn_write_text_log1(data, len))
			ret = lprn_wait_op(true);
		else{
			set_term_astate(ast_nolprn);
			lprn_reset();
		}
	}
	lprn_close();
	return ret;
}

/* Печать текста на БСО */
int lprn_print_ticket(const uint8_t *data, size_t len, bool *sent_to_prn)
{
	int ret = LPRN_RET_ERR;
	*sent_to_prn = false;
/* Ожидание готовности принтера */
	if ((ret = lprn_get_status()) != LPRN_RET_OK)
		;
	else if (lprn_status != 0)
		;
	else if (cfg.has_sd_card && (lprn_sd_status > 0x01))
		;
	else if ((ret = lprn_get_media_type()) != LPRN_RET_OK)
		;
	else if (lprn_status != 0)
		;
	else if ((lprn_media != LPRN_MEDIA_BLANK) &&
			(lprn_media != LPRN_MEDIA_BOTH))
		;
	else{
/* Печать на БСО */
		set_term_astate(ast_none);
		if (lprn_write_text(data, len, false)){
			*sent_to_prn = true;
			ret = lprn_wait_op(true);
			if (ret == LPRN_RET_OK){
				if (lprn_status == 0){
					bool has_number = lprn_has_blank_number;
					if ((ret = lprn_get_status()) == LPRN_RET_OK)
						lprn_has_blank_number = has_number;
				}
			}
		}
	}
	lprn_close();
	return ret;
}

/* Запрос параметров работы ППУ */
int lprn_get_params(struct term_cfg *cfg)
{
	int ret = LPRN_RET_ERR;
	if ((lprn_dev != -1) || lprn_open()){
		lprn_reset();
		lprn_params_received = false;
		snd_data[0] = LPRN_NUL;
		snd_data[1] = LPRN_DLE2;
		snd_data[2] = 'R';
		snd_data_len = 3;
		sent_len = 0;
		sent_cmd = 'R';
		lprn_mark_operation();
		timeout = LPRN_RD_PARAMS_TIMEOUT;
#if defined __LOG_LPRN__
		lprn_log_rcv = false;
#endif
		ret = lprn_wait_op(false);
		lprn_close();
		if ((ret == LPRN_RET_OK) && !lprn_translate_params(cfg))
			ret = LPRN_RET_ERR;
	}
	return ret;
}

/* Запись одного параметра работы ППУ */
static int lprn_set_param(int n, int val)
{
	lprn_reset();
	snd_data[0] = LPRN_NUL;
	snd_data[1] = LPRN_DLE2;
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
	lprn_param_number = n + 0x30;
	lprn_mark_operation();
	timeout = LPRN_WR_PARAM_TIMEOUT;
#if defined __LOG_LPRN__
	lprn_log_rcv = false;
#endif
	return lprn_wait_op(false);
}

/* Запись параметров работы ППУ */
int lprn_set_params(struct term_cfg *cfg)
{
	int ret = LPRN_RET_ERR, i, *p = &cfg->s0;
	if ((lprn_dev != -1) || lprn_open()){
		for (i = 0; i < 10; i++){
			ret = lprn_set_param(i, p[i]);
			if (ret == LPRN_RET_OK)
				p[i] = lprn_param_value;
			else
				break;
		}
	}
	lprn_close();
	return ret;
}

/* Синхронизация времени ППУ с терминалом */
int lprn_sync_time(void)
{
	int ret = LPRN_RET_ERR;
	if ((lprn_dev != -1) || lprn_open()){
		time_t t = time(NULL) + time_delta;
		struct tm *tm = localtime(&t);
		lprn_reset();
		snd_data[0] = LPRN_NUL;
		snd_data[1] = LPRN_DLE2;
		snd_data[2] = 0x53;		/* S */
		snd_data[3] = 0x41;		/* A */
		snprintf((char *)snd_data + 4, sizeof(snd_data) - 4,
			"%.2d%.2d%.2d%.2d%.2d%.2d",
			tm->tm_mday, tm->tm_mon + 1, tm->tm_year % 100,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
		snd_data_len = 16;
		sent_len = 0;
		sent_cmd = 0x53;		/* S */
		lprn_param_number = 0x41;	/* A */
		lprn_sd_status = 0;
		lprn_mark_operation();
		timeout = LPRN_TIME_SYNC_TIMEOUT;
#if defined __LOG_LPRN__
		lprn_log_rcv = false;
#endif
		ret = lprn_wait_op(false);
	}
	lprn_close();
	return ret;
}

/* Получение информации об ошибке ППУ на основании её кода */
struct lprn_error_txt *lprn_get_error_txt(uint8_t code)
{
	static struct {
		uint8_t code;
		int len;
		char *txt;
	} err[] = {
		{
			.code		= 0x01,
			.len		= -1,
			.txt		= "НОМЕР ППУ НЕ ПРОПИСАН В ПАМЯТИ"
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
			return (struct lprn_error_txt *)&err[i].len;
	}
	return NULL;
}

/* Получение информации об ошибке карты памяти на основании её кода */
struct lprn_error_txt *lprn_get_sd_error_txt(uint8_t code)
{
	static struct {
		uint8_t code;
		int len;
		char *txt;
	} err[] = {
		{
			.code		= 0x04,
			.len		= -1,
			.txt		= "ОШИБКА СИНХРОНИЗАЦИИ ВРЕМЕНИ С ППУ"
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
			return (struct lprn_error_txt *)&err[i].len;
	}
	return NULL;
}
