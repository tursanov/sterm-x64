/* Основной модуль для работы с POS-эмулятором. (c) A.Popov, gsr 2004 */

#include <sys/timeb.h>
#include <sys/times.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/scr.h"
#include "log/pos.h"
#include "pos/command.h"
#include "pos/error.h"
#include "pos/pos.h"
#include "pos/printer.h"
#include "pos/screen.h"
#include "pos/serial.h"
#include "pos/tcp.h"
#include "prn/express.h"
#include "sterm.h"

/* Флаги потоков, присутствующих в ответе */
uint32_t stream_flags;

/* Буфер сообщения для работы с POS-эмулятором */
static struct pos_data_buf pos_buf;

/* Отладочная печать */
#ifdef __POS_DEBUG__
bool pos_dump(struct pos_data_buf *buf)
{
	struct timeb tp;
	struct tm *tm;
	int i;
	uint8_t *p;
	if (buf == NULL)
		return false;
	ftime(&tp);
	tm = localtime(&tp.time);
	printf("%.2d:%.2d:%.2d.%.3hu\n", tm->tm_hour, tm->tm_min, tm->tm_sec,
			tp.millitm);
	for (i = 0, p = buf->un.data; i < buf->data_len; i++, p++){
		if (((i & 0x07) == 0) && i)
			printf("%c", (i & 0x0f) ? ' ' : '\n');
		printf("%.2hx ", (uint16_t)*p);
	}
	printf("\n");
	return true;
}
#endif

/* Чтение данных из буфера */
bool pos_read(struct pos_data_buf *buf, uint8_t *data, int len)
{
	if ((buf == NULL) || (data == NULL) || (len <= 0) ||
			((buf->data_index + len) > buf->data_len))
		return false;
	memcpy(data, buf->un.data + buf->data_index, len);
	buf->data_index += len;
	return true;
}

/* Чтение байта из буфера */
bool pos_read_byte(struct pos_data_buf *buf, uint8_t *v)
{
	if ((buf == NULL) || (v == NULL) ||
			((buf->data_index + 1) > buf->data_len))
		return false;
	*v = buf->un.data[buf->data_index++];
	return true;
}

/* Чтение слова из буфера */
bool pos_read_word(struct pos_data_buf *buf, uint16_t *v)
{
	if ((buf == NULL) || (v == NULL) ||
			((buf->data_index + sizeof(*v)) > buf->data_len))
		return false;
	*v = ntohs(*(uint16_t *)(buf->un.data + buf->data_index));
	buf->data_index += sizeof(uint16_t);
	return true;
}

/* Чтение двойного слова из буфера */
bool pos_read_dword(struct pos_data_buf *buf, uint32_t *v)
{
	if ((buf == NULL) || (v == NULL) ||
			((buf->data_index + sizeof(*v)) > buf->data_len))
		return false;
	*v = ntohl(*(uint32_t *)(buf->un.data + buf->data_index));
	buf->data_index += sizeof(uint32_t);
	return true;
}

/*
 * Чтение массива байтов из буфера. Возвращает число считанных байт
 * без учета поля длины или -1 в случае ошибки.
 */
int pos_read_array(struct pos_data_buf *buf, uint8_t *data, int max_len)
{
	uint16_t len;
	if ((buf == NULL) || (data == NULL) || !pos_read_word(buf, &len) ||
			(len > max_len) ||
			((buf->data_index + len) > buf->data_len))
		return -1;
	if (len > 0){
		memcpy(data, buf->un.data + buf->data_index, len);
		buf->data_index += len;
	}
	return len;
}

/* Запись данных в буфер */
bool pos_write(struct pos_data_buf *buf, uint8_t *data, int len)
{
	if ((buf == NULL) || (data == NULL) || (len <= 0) ||
			((buf->data_index + len) > sizeof(buf->un.data)))
		return false;
	memcpy(buf->un.data + buf->data_index, data, len);
	buf->data_index += len;
	return true;
}

/* Запись байта в буфер */
bool pos_write_byte(struct pos_data_buf *buf, uint8_t v)
{
	if ((buf == NULL) || ((buf->data_index + 1) > sizeof(buf->un.data)))
		return false;
	buf->un.data[buf->data_index++] = v;
	return true;
}

/* Запись слова в буфер */
bool pos_write_word(struct pos_data_buf *buf, uint16_t v)
{
	if ((buf == NULL) ||
			((buf->data_index + sizeof(v)) > sizeof(buf->un.data)))
		return false;
	*(uint16_t *)(buf->un.data + buf->data_index) = htons(v);
	buf->data_index += sizeof(v);
	return true;
}

/* Запись двойного слова в буфер */
bool pos_write_dword(struct pos_data_buf *buf, uint32_t v)
{
	if ((buf == NULL) ||
			((buf->data_index + sizeof(v)) > sizeof(buf->un.data)))
		return false;
	*(uint32_t *)(buf->un.data + buf->data_index) = htonl(v);
	buf->data_index += sizeof(v);
	return true;
}

/* Запись массива байтов в запрос */ 
bool pos_write_array(struct pos_data_buf *buf, uint8_t *data, int len)
{
	if ((buf == NULL) || (data == NULL) || (len < 0) ||
			((buf->data_index + len + sizeof(uint16_t)) > sizeof(buf->un.data)))
		return false;
	if (!pos_write_word(buf, len))
		return false;
	if (len > 0){
		memcpy(buf->un.data + buf->data_index, data, len);
		buf->data_index += len;
	}
	return true;
}

/* Проверка и установка бита. Возвращает true, если бит уже установлен */
#define bts(b,mask) ((b & mask) || (b |= mask, false))

/* Проверка синтаксиса сообщения от POS-эмулятора */
static bool pos_check_resp(struct pos_data_buf *buf)
{
	static struct {
		int stream;
		int mask;
		bool (*parser)(struct pos_data_buf *, bool);
	} parsers[] = {
		{POS_STREAM_SCREEN,	POS_HAS_STREAM_SCREEN,
			pos_parse_screen_stream},
		{POS_STREAM_PRINTER,	POS_HAS_STREAM_PRINTER,
			pos_parse_printer_stream},
		{POS_STREAM_TCP,	POS_HAS_STREAM_TCP,
			pos_parse_tcp_stream},
		{POS_STREAM_ERROR,	POS_HAS_STREAM_ERROR,
			pos_parse_error_stream},
		{POS_STREAM_COMMAND,	POS_HAS_STREAM_COMMAND,
			pos_parse_command_stream},
	};
	uint8_t b;
	uint16_t w;
	uint32_t dw;
	int i, j, n_blocks;
	stream_flags = 0;
	if (buf == NULL)
		return false;
	buf->data_index = 0;
/* Проверка версии протокола */
	if (!pos_read_dword(buf, &dw) || (dw != POS_CURRENT_VERSION)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
/* Проверка длины сообщения */
	if (!pos_read_dword(buf, &dw) || (dw > POS_MAX_MESSAGE_DATA_LEN)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
/* Проверка числа блоков */
	if (!pos_read_byte(buf, &b) || (b > POS_MAX_BLOCKS)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	n_blocks = b;
/* Поблочная проверка */
	for (i = 0; i < n_blocks; i++){
/* Проверка потока, для которого предназначен блок */
		if (!pos_read_word(buf, &w)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
/* Для каждого потока вызываем соответствующий обработчик */
		for (j = 0; j < ASIZE(parsers); j++){
			if (w == parsers[j].stream){
/* Поток сообщений об ошибках должен быть единственным в сообщении */
				if ((w == POS_STREAM_ERROR) && stream_flags){
					pos_set_error(POS_ERROR_CLASS_SYSTEM,
							POS_ERR_MSG_FMT, 0);
					return false;
				}else if ((stream_flags & POS_HAS_STREAM_ERROR) ||
						bts(stream_flags, parsers[j].mask)){
					pos_set_error(POS_ERROR_CLASS_SYSTEM,
							POS_ERR_MSG_FMT, 0);
					return false;
				}else if (!parsers[j].parser(buf, true))
					return false;
				break;
			}
		}
/* Если для потока не удалось найти обработчик, это считается ошибкой */
		if (j == ASIZE(parsers)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
	}
/* После проверки в сообщении не должно оставаться лишних данных */
	if (buf->data_index == buf->data_len)
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
}

/* Разбор сообщения от РОS-эмулятора */
static bool pos_parse_resp(struct pos_data_buf *buf)
{
	static struct {
		int stream;
		bool (*parser)(struct pos_data_buf *, bool);
	} parsers[] = {
		{POS_STREAM_SCREEN,	pos_parse_screen_stream},
		{POS_STREAM_PRINTER,	pos_parse_printer_stream},
		{POS_STREAM_TCP,	pos_parse_tcp_stream},
		{POS_STREAM_ERROR,	pos_parse_error_stream},
		{POS_STREAM_COMMAND,	pos_parse_command_stream},
	};
	int i, j;
	uint16_t stream;
	pos_error_clear();
	if ((buf == NULL) || !pos_check_resp(buf))
		return false;
	buf->data_index = sizeof(struct pos_message_header);
	for (i = 0; i < buf->un.hdr.msg.nr_blocks; i++){
		if (!pos_read_word(buf, &stream)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		for (j = 0; j < ASIZE(parsers); j++){
			if (stream == parsers[j].stream){
				if (!parsers[j].parser(buf, false))
					return false;
				else
					break;
			}
		}
		if (j == ASIZE(parsers)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
	}
	return true;
}

/* Начало формирования буфера */
bool pos_req_begin(struct pos_data_buf *buf)
{
	if (buf == NULL)
		return false;
	buf->data_index = 0;
	return	pos_write_dword(buf, POS_CURRENT_VERSION) &&
		pos_write_dword(buf, 0) &&
		pos_write_byte(buf, 0);
}

/* Конец формирования буфера */
bool pos_req_end(struct pos_data_buf *buf)
{
	if (buf == NULL)
		return false;
/* Запись длины буфера */
	buf->un.hdr.length = htonl(buf->data_index - 8);
	buf->data_len = buf->data_index;
	return true;
}

/* Начало формирования потока */
bool pos_req_stream_begin(struct pos_data_buf *buf, uint16_t stream)
{
	if (buf == NULL)
		return false;
	else if (buf->un.hdr.msg.nr_blocks >= POS_MAX_BLOCKS)
		return false;
	buf->block_start = buf->data_index;
	return pos_write_word(buf, stream) && pos_write_word(buf, 0);
}

/* Конец формирования потока */
bool pos_req_stream_end(struct pos_data_buf *buf)
{
	struct pos_message_block *blk_hdr;
	if (buf == NULL)
		return false;
	blk_hdr = (struct pos_message_block *)(buf->un.data + buf->block_start);
	blk_hdr->length = htons(buf->data_index - buf->block_start - sizeof(*blk_hdr));
	buf->data_len = buf->data_index;
	buf->un.hdr.msg.nr_blocks++;
	return true;
}

static int pos_state = pos_new;
/* Используется для периодического опроса POS-эмулятора */
uint32_t pos_t0 = 0;
/* На пустой запрос пришел ответ в течение заданного таймаута */
bool poll_ok = true;

static bool pos_send_empty(void)
{
	pos_req_begin(&pos_buf);
	pos_req_end(&pos_buf);
	if (pos_serial_send_msg(&pos_buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}	
}

static bool pos_send_init_check(void)
{
	pos_req_begin(&pos_buf);
	pos_req_save_command_init_check(&pos_buf);
	pos_req_end(&pos_buf);
	if (pos_serial_send_msg(&pos_buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}	
}

static bool pos_send_init(void)
{
	pos_req_begin(&pos_buf);
	pos_req_save_command_init(&pos_buf);
	pos_req_end(&pos_buf);
	if (pos_serial_send_msg(&pos_buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}	
}

static bool pos_send_kbd(void)
{
	if (!pos_screen_initialized()){
		pos_set_error(POS_ERROR_CLASS_SCREEN, POS_ERR_SCR_NOT_READY, 0);
		return false;
	}
	pos_req_begin(&pos_buf);
	pos_req_save_keyboard_stream(&pos_buf);
	pos_req_end(&pos_buf);
	if (pos_serial_send_msg(&pos_buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}	
}

bool pos_send_params(void)
{
	static struct pos_data_buf buf;
/*	int i;*/
	if (req_param_list.count == 0)
		return true;
/*	for (i = 0; i < req_param_list.count; i++){
		if (req_param_list.params[i].required)
			break;
	}
	if (i == req_param_list.count)
		return true;*/
	pos_req_begin(&buf);
	pos_req_save_command_response_parameters(&buf);
	pos_req_end(&buf);
	if (pos_serial_send_msg(&buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}	
}

static bool pos_send_finish(void)
{
	pos_req_begin(&pos_buf);
	pos_req_save_command_finish(&pos_buf);
	pos_req_end(&pos_buf);
	if (pos_serial_send_msg(&pos_buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}	
}

static bool pos_send_tcp(void)
{
	if ((pos_count_tcp_events() == 0) || !pos_serial_is_free() || !poll_ok)
		return true;
	pos_req_begin(&pos_buf);
	pos_req_save_tcp(&pos_buf);
	pos_req_end(&pos_buf);
	if (pos_serial_send_msg(&pos_buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
}

static bool pos_send_error(void)
{
	pos_req_begin(&pos_buf);
	pos_req_save_error_stream(&pos_buf, &pos_error);
	pos_req_end(&pos_buf);
	if (pos_serial_send_msg(&pos_buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
}

int pos_get_state(void)
{
	return pos_state;
}

void pos_print_state(int st)
{
	static struct {
		int st;
		char *name;
	} states[] = {
		{pos_new,		"pos_new"},
		{pos_init_check,	"pos_init_check"},
		{pos_idle,		"pos_idle"},
		{pos_init,		"pos_init"},
		{pos_ready,		"pos_ready"},
		{pos_enter,		"pos_enter"},
		{pos_print,		"pos_print"},
		{pos_printing,		"pos_printing"},
		{pos_finish,		"pos_finish"},
		{pos_break,		"pos_break"},
		{pos_err,		"pos_err"},
		{pos_err_out,		"pos_err_out"},
		{pos_wait,		"pos_wait"},
		{pos_ewait,		"pos_ewait"},
	};
	int i;
	for (i = 0; i < ASIZE(states); i++){
		if (st == states[i].st){
			printf(states[i].name);
			break;
		}
	}
	if (i == ASIZE(states))
		printf("???");
}

void pos_set_state(int st)
{
#if defined __POS_DEBUG__
	struct timeb tp;
	struct tm *tm;
	ftime(&tp);
	tm = localtime(&tp.time);
	printf("%.2d:%.2d:%.2d.%.3hu ", tm->tm_hour, tm->tm_min, tm->tm_sec,
			tp.millitm);
	pos_print_state(pos_state);
	printf("->");
	pos_print_state(st);
	printf("\n");
#endif
	pos_state = st;
}

static bool pos_open(void)
{
	if (pos_serial_open()){
		pos_stop_transactions();
		return true;
	}else
		return false;
}

static void pos_close(bool close_com)
{
	if (pos_screen_initialized())
		pos_screen_destroy();
	pos_stop_transactions();
	if (close_com)
		pos_serial_close();
}

/* Обработка различных состояний конечного автомата POS-эмулятора */
static void on_pos_new(uint32_t t __attribute__((unused)))
{
	pos_close(true);
	pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_NOT_INIT, 0);
	if (pos_open() && pos_send_init_check())
		pos_set_state(pos_init_check);
}

static void on_pos_init_check(uint32_t t)
{
	uint32_t dt = t - pos_t0;
	if (pos_serial_peek_msg()){
		pos_serial_get_msg(&pos_buf);
		if (pos_parse_resp(&pos_buf)){
			pos_close(true);
			pos_set_state(pos_idle);
		}else
			pos_set_state(pos_new);
	}else if (dt > MAX_POS_TIMEOUT){
		pos_close(true);
		pos_set_state(pos_new);
	}
}

static void on_pos_idle(uint32_t t __attribute__((unused)))
{
}

static void on_pos_init(uint32_t t __attribute__((unused)))
{
	if (pos_open() && pos_send_init()){
		pos_write_scr(&pos_buf, "ИДЕТ СОЕДИНЕНИЕ С POS-ЭМУЛЯТОРОМ",
				GREEN, BLACK);
		pos_parse_resp(&pos_buf);
		pos_set_state(pos_ready);
	}else
		pos_set_error(POS_ERROR_CLASS_SYSTEM,
			POS_ERR_SYSTEM, 0);
}

static void on_pos_ready(uint32_t t)
{
	uint32_t dt = t - pos_t0;
	pos_screen_draw();
	if (!pos_tcp_process() || !pos_send_tcp())
		return;
	else if (pos_serial_peek_msg()){
		pos_serial_get_msg(&pos_buf);
		pos_parse_resp(&pos_buf);
	}else if (dt > POS_TIMEOUT){
		if (!poll_ok){
			if (dt > MAX_POS_TIMEOUT)
				pos_set_error(POS_ERROR_CLASS_SYSTEM,
					POS_ERR_TIMEOUT, 0);
		}else if ((pos_get_state() == pos_ready) && pos_serial_is_free())
			pos_send_empty();
	}
}

static void on_pos_enter(uint32_t t)
{
	if (!pos_screen_has_menu()){
		err_beep();
		pos_set_state(pos_ready);
	}else if (poll_ok){
		if (pos_send_kbd())
			pos_set_state(pos_ready);
	}else if ((t - pos_t0) > MAX_POS_TIMEOUT)
		pos_set_error(POS_ERROR_CLASS_SYSTEM,
			POS_ERR_TIMEOUT, 0);
}

static void on_pos_print(uint32_t t __attribute__((unused)))
{
	if (pos_prn_data_len > 0){
		plog_write_rec(hplog, pos_prn_buf, pos_prn_data_len, PLRT_NORMAL);
		pos_set_state(pos_printing);
		if (xprn_print((char *)pos_prn_buf, pos_prn_data_len)){
			pos_prn_data_len = 0;
			if (pos_get_state() == pos_printing){
				pos_set_state(pos_ready);
/* NB: мы не можем использовать здесь t, т.к. xprn_print -- блокирующая функция */
				pos_t0 = u_times();
			}
		}
	}else
		pos_set_state(pos_ready);
}

static void on_pos_printing(uint32_t t)
{
	uint32_t dt = t - pos_t0;
	if (dt > POS_TIMEOUT){
		if (!poll_ok){
			if (dt > MAX_POS_TIMEOUT)
				pos_set_error(POS_ERROR_CLASS_SYSTEM,
					POS_ERR_TIMEOUT, 0);
		}else if ((pos_get_state() == pos_printing) && pos_serial_is_free())
			pos_send_empty();
	}
}

static void on_pos_finish(uint32_t t __attribute__((unused)))
{
	pos_set_state(pos_new);
}

static void on_pos_break(uint32_t t __attribute__((unused)))
{
	pos_send_finish();
	pos_close(false);
	pos_set_state(pos_wait);
}

static void on_pos_err(uint32_t t __attribute__((unused)))
{
	char *s = pos_err_xdesc;	/* pos_parse_resp сбрасывает pos_err_xdesc */
	err_beep();
	plog_write_rec(hplog, (uint8_t *)pos_err_desc, strlen(pos_err_desc),
		PLRT_ERROR);
	pos_save_err_msg(&pos_buf, pos_err_desc);
	pos_parse_resp(&pos_buf);
	pos_set_state(pos_ewait);
	pos_t0 = u_times();	/* err_beep -- блокирующая функция */
	pos_err_xdesc = s;
}

static void on_pos_err_out(uint32_t t)
{
	pos_send_error();
	on_pos_err(t);
}

static void on_pos_wait(uint32_t t)
{
	if ((t - pos_t0) > POS_TIMEOUT){
		pos_serial_close();
		pos_set_state(pos_new);
	}
}

static void on_pos_ewait(uint32_t t)
{
	if ((t - pos_t0) > 300)
		pos_set_state(pos_break);
	else
		pos_screen_draw();
}

void pos_process(void)
{
	static struct {
		int state;
		void (*handler)(uint32_t);
	} handlers[] = {
		{pos_new,		on_pos_new},
		{pos_init_check,	on_pos_init_check},
		{pos_idle,		on_pos_idle},
		{pos_init,		on_pos_init},
		{pos_ready,		on_pos_ready},
		{pos_enter,		on_pos_enter},
		{pos_print,		on_pos_print},
		{pos_printing,		on_pos_printing},
		{pos_finish,		on_pos_finish},
		{pos_break,		on_pos_break},
		{pos_err,		on_pos_err},
		{pos_err_out,		on_pos_err_out},
		{pos_wait,		on_pos_wait},
		{pos_ewait,		on_pos_ewait},
	};
	int i;
	if (!cfg.bank_system)
		return;
	pos_serial_receive();
	pos_serial_transmit();
	for (i = 0; i < ASIZE(handlers); i++){
		if (pos_state == handlers[i].state){
			handlers[i].handler(u_times());
			break;
		}
	}
}

bool pos_create(void)
{
	pos_init_transactions();
	return true;
}

void pos_release(void)
{
	pos_close(true);
	pos_release_transactions();
	pos_set_state(pos_new);
}
