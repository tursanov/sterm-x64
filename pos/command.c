/* Работа с потоком команд POS-эмулятора. (c) A.Popov, gsr 2004 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/scr.h"
#include "pos/command.h"
#include "pos/error.h"
#include "ds1990a.h"
#include "genfunc.h"
#include "hex.h"
#include "sterm.h"

pos_request_param_list_t req_param_list;
pos_response_param_list_t resp_param_list;

/* Удаление списка */
static void pos_request_param_list_free(pos_request_param_list_t *list,
		bool free_list)
{
	int i;
	if (list == NULL)
		return;
	if (list->count && (list->params != NULL)){
		for (i = 0; i < list->count; i++){
			if (list->params[i].name != NULL)
				free(list->params[i].name);
		}
		free(list->params);
	}
	if (free_list)
		free(list);
	else{
		list->count = 0;
		list->params = NULL;
	}
}

/* Удаление списка */
static void pos_response_param_list_free(pos_response_param_list_t *list,
		bool free_list)
{
	int i; 
	if (list == NULL)
		return;
	if (list->count && (list->params != NULL)){
		for (i = 0; i < list->count; i++){
			if (list->params[i].name)
				free(list->params[i].name);
			if (list->params[i].value)
				free(list->params[i].value);
		}
		free(list->params);
	}
	if (free_list)
		free(list);
	else{
		list->count = 0;
		list->params = NULL;
	}
}

/* Определение типа параметра по его имени */
static int get_param_type(char *name)
{
	static struct {
		char *name;
		int type;
	} params[] = {
		{POS_PARAM_AMOUNT_STR,	POS_PARAM_AMOUNT},
		{POS_PARAM_TIME_STR,	POS_PARAM_TIME},
		{POS_PARAM_ID_STR,	POS_PARAM_ID},
		{POS_PARAM_TERMID_STR,	POS_PARAM_TERMID},
		{POS_PARAM_CLERKID_STR,	POS_PARAM_CLERKID},
		{POS_PARAM_CLERKTYPE_STR,POS_PARAM_CLERKTYPE},
	};
	int i;
	if (name == NULL)
		return POS_PARAM_UNKNOWN;
	for (i = 0; i < ASIZE(params); i++){
		if (strcmp(name, params[i].name) == 0)
			return params[i].type;
	}
	return POS_PARAM_UNKNOWN;
}

static bool pos_parse_finish(struct pos_data_buf *buf __attribute__((unused)), bool check_only)
{
	if (!check_only)
		pos_set_state(pos_finish);
	return true;
}

static bool pos_parse_request_parameters(struct pos_data_buf *buf,
		bool check_only)
{
	int i, n;
	char name[33];
	if (!pos_read_byte(buf, &req_param_list.count) ||
			(req_param_list.count == 0)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	req_param_list.params = calloc(req_param_list.count,
			sizeof(pos_request_param_t));
	if (req_param_list.params == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	for (i = 0; i < req_param_list.count; i++){
		n = pos_read_array(buf, (uint8_t *)name, 32);
		if (n <= 0){
			pos_request_param_list_free(&req_param_list, false);
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		name[n] = 0;
		recode_str(name, n);
		req_param_list.params[i].name = strdup(name);
		if (req_param_list.params[i].name == NULL){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			pos_request_param_list_free(&req_param_list, false);
			return false;
		}
		req_param_list.params[i].type = get_param_type(name);
		if (!pos_read_byte(buf, &req_param_list.params[i].required)){
			pos_request_param_list_free(&req_param_list, false);
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
	}
	if (check_only)
		pos_request_param_list_free(&req_param_list, false);
	else
		pos_send_params();
	return true;
}

static bool pos_parse_response_parameters(struct pos_data_buf *buf,
		bool check_only)
{
	int i, n;
	static char s[2049];
	if (!pos_read_byte(buf, &resp_param_list.count) ||
			(resp_param_list.count == 0)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	resp_param_list.params = calloc(resp_param_list.count,
			sizeof(pos_response_param_t));
	if (resp_param_list.params == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	for (i = 0; i < resp_param_list.count; i++){
		n = pos_read_array(buf, (uint8_t *)s, 32);
		if (n <= 0){
			pos_response_param_list_free(&resp_param_list, false);
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		s[n] = 0;
		recode_str(s, n);
		resp_param_list.params[i].name = strdup(s);
		if (resp_param_list.params[i].name == NULL){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			pos_response_param_list_free(&resp_param_list, false);
			return false;
		}
		n = pos_read_array(buf, (uint8_t *)s, 2048);
		if (n == -1){
			pos_response_param_list_free(&resp_param_list, false);
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		s[n] = 0;
		recode_str(s, n);
		resp_param_list.params[i].value = strdup(s);
		if (resp_param_list.params[i].value == NULL){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			pos_response_param_list_free(&resp_param_list, false);
			return false;
		}
	}
	if (check_only)
		pos_response_param_list_free(&resp_param_list, false);
	return true;
}

static bool pos_parse_init_required(struct pos_data_buf *buf __attribute__((unused)),
	bool check_only)
{
	if (!check_only && (_term_aux_state == ast_none))
		set_term_astate(ast_pos_need_init);
	return true;
}

bool pos_parse_command_stream(struct pos_data_buf *buf, bool check_only)
{
	static struct {
		uint8_t code;
		bool (*parser)(struct pos_data_buf *, bool);
	} parsers[] = {
		{POS_COMMAND_FINISH,			pos_parse_finish},
		{POS_COMMAND_REQUEST_PARAMETERS,	pos_parse_request_parameters},
		{POS_COMMAND_RESPONSE_PARAMETERS,	pos_parse_response_parameters},
		{POS_COMMAND_INIT_REQUIRED,		pos_parse_init_required},
	};
	int i, l;
	uint16_t len;
	if (!pos_read_word(buf, &len) || (len > POS_MAX_BLOCK_DATA_LEN)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	l = buf->data_index + len;
	if (l > buf->data_len){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	while (buf->data_index < l){
		uint8_t code;
		if (!pos_read_byte(buf, &code)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		for (i = 0; i < ASIZE(parsers); i++){
			if (code == parsers[i].code){
				if (!parsers[i].parser(buf, check_only))
					return false;
				break;
			}
		}
		if (i == ASIZE(parsers)){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
	}
	if (buf->data_index == l)
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
}

/* Запись команды INIT */
bool pos_req_save_command_init(struct pos_data_buf *buf)
{
	if (pos_req_stream_begin(buf, POS_STREAM_COMMAND) &&
			pos_write_byte(buf, POS_COMMAND_INIT) &&
			pos_req_stream_end(buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
}

/* Запись команды INIT_CHECK */
bool pos_req_save_command_init_check(struct pos_data_buf *buf)
{
	if (pos_req_stream_begin(buf, POS_STREAM_COMMAND) &&
			pos_write_byte(buf, POS_COMMAND_INIT_CHECK) &&
			pos_req_stream_end(buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
}

/* Запись команды FINISH */
bool pos_req_save_command_finish(struct pos_data_buf *buf)
{
	if (pos_req_stream_begin(buf, POS_STREAM_COMMAND) &&
			pos_write_byte(buf, POS_COMMAND_FINISH) &&
			pos_req_stream_end(buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
}

/* Запись в поток команд заданного параметра */
static bool pos_write_param(struct pos_data_buf *buf, char *name, int param,
		bool required)
{
	static char val[2049];
	static int ind[] = {7, 0, 6, 5, 4, 3, 2, 1};
	int i, l = 0;
	if (buf == NULL)
		return false;
	switch (param){
		case POS_PARAM_AMOUNT:
			sprintf(val, "%u", bi_pos.amount1 * 100 +
					bi_pos.amount2 * 10);
			l = strlen(val);
			break;
		case POS_PARAM_TIME:{
			time_t t = time(NULL) + time_delta;
			struct tm *tm = localtime(&t);
			sprintf(val, "%.4d%.2d%.2d%.2d%.2d%.2d",
				tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
				tm->tm_hour, tm->tm_min, tm->tm_sec);
			l = strlen(val);
			break;
		}
		case POS_PARAM_ID:
			sprintf(val, "%.7u", bi_pos.id);
			l = strlen(val);
			break;
		case POS_PARAM_TERMID:
			memcpy(val, bi_pos.termid, sizeof(bi_pos.termid));
			l = sizeof(bi_pos.termid);
			write_hex_byte((uint8_t *)(val + l), cfg.gaddr);
			write_hex_byte((uint8_t *)(val + l + 2), cfg.iaddr);
			l += 4;
			break;
		case POS_PARAM_CLERKID:
			for (i = l = 0; i < ASIZE(ind); i++, l += 2)
				write_hex_byte((uint8_t *)(val + l), dsn[ind[i]]);
			break;
		case POS_PARAM_CLERKTYPE:
			val[0] = recode(ds_key_char(kt));
			val[1] = 0;
			l = 1;
			break;
		default:
			if (required){
				pos_set_error(POS_ERROR_CLASS_KEYBOARD,
					POS_ERR_UNKNOWN_OBJ, (intptr_t)name);
				return false;
			}else
				return true;
	}
	if (pos_write_array(buf, (uint8_t *)name, strlen(name)) &&
				pos_write_array(buf, (uint8_t *)val, l))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
}

/* Запись команды RESPONSE_PARAMETERS на основе req_param_list */
bool pos_req_save_command_response_parameters(struct pos_data_buf *buf)
{
	int i, n;
	if (req_param_list.count == 0)
		return true;
	for (i = n = 0; i < req_param_list.count; i++){
		if (req_param_list.params[i].type != POS_PARAM_UNKNOWN)
			n++;
	}
/*	if (n == 0)
		return true;*/
	if (!pos_req_stream_begin(buf, POS_STREAM_COMMAND) ||
			!pos_write_byte(buf, POS_COMMAND_RESPONSE_PARAMETERS) ||
			!pos_write_byte(buf, n)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
	for (i = 0; i < req_param_list.count; i++){
		if (!pos_write_param(buf, req_param_list.params[i].name,
				req_param_list.params[i].type,
				req_param_list.params[i].required))
			return false;
	}
	if (pos_req_stream_end(buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
}
