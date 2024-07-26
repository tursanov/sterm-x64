/* Работа с потоком команд POS-эмулятора. (c) A.Popov, gsr 2004 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/scr.h"
#include "pos/command.h"
#include "pos/error.h"
#include "ds1990a.h"
#include "genfunc.h"
#include "numbers.h"
#include "sterm.h"
#include "kkt/fd/ad.h"

/* Поддержка ЕБТ в ИПТ */
bool ubt_supported = false;

/* Выбранный пункт меню */
static uint8_t pos_menu_item = MTYPE_UNKNOWN;

/* Флаг возможности редактирования суммы и номера квитанции в окне банковского приложения */
static bool pos_can_edit = false;

/* Список номеров документов и их стоимостей */
static char *pos_ords = NULL;

pos_request_param_list_t req_param_list;
pos_response_param_list_t resp_param_list;

/* Удаление списка */
static void pos_request_param_list_free(pos_request_param_list_t *list)
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
	list->count = 0;
	list->params = NULL;
}

/* Удаление списка */
static void pos_response_param_list_free(pos_response_param_list_t *list)
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
	list->count = 0;
	list->params = NULL;
}

/* Определение типа параметра по его имени */
static int get_param_type(char *name)
{
	static struct {
		const char *name;
		int type;
	} map[] = {
		{POS_PARAM_AMOUNT_STR,		POS_PARAM_AMOUNT},
		{POS_PARAM_INVOICE_STR,		POS_PARAM_INVOICE},
		{POS_PARAM_ORDS_STR,		POS_PARAM_ORDS},
		{POS_PARAM_TIME_STR,		POS_PARAM_TIME},
		{POS_PARAM_ID_STR,		POS_PARAM_ID},
		{POS_PARAM_TERMID_STR,		POS_PARAM_TERMID},
		{POS_PARAM_CLERKID_STR,		POS_PARAM_CLERKID},
		{POS_PARAM_CLERKTYPE_STR,	POS_PARAM_CLERKTYPE},
		{POS_PARAM_UBT_STR,		POS_PARAM_UBT},
		{POS_PARAM_VERSION_STR,		POS_PARAM_VERSION},
		{POS_PARAM_MTYPE_STR,		POS_PARAM_MTYPE},
		{POS_PARAM_EDIT_STR,		POS_PARAM_EDIT},
		{POS_PARAM_FMENU_STR,		POS_PARAM_FMENU},
		{POS_PARAM_RES_CODE_STR,	POS_PARAM_RES_CODE},
		{POS_PARAM_RESP_CODE_STR,	POS_PARAM_RESP_CODE},
		{POS_PARAM_ID_POS_STR,		POS_PARAM_ID_POS},
		{POS_PARAM_NMTYPE_STR,		POS_PARAM_NMTYPE},
		{POS_PARAM_PARAMS_STR,		POS_PARAM_PARAMS},
	};
	if (name == NULL)
		return POS_PARAM_UNKNOWN;
	int ret = POS_PARAM_UNKNOWN;
	for (int i = 0; i < ASIZE(map); i++){
		const typeof(*map) *p = map + i;
		if (strcmp(name, p->name) == 0){
			ret = p->type;
			break;
		}
	}
	return ret;
}

static bool pos_parse_finish(struct pos_data_buf *buf __attribute__((unused)),
	bool check_only)
{
	if (!check_only)
		pos_set_state(pos_finish);
	return true;
}

static bool pos_parse_request_parameters(struct pos_data_buf *buf, bool check_only)
{
	if (!pos_read_byte(buf, &req_param_list.count) || (req_param_list.count == 0)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	req_param_list.params = calloc(req_param_list.count, sizeof(pos_request_param_t));
	if (req_param_list.params == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	for (int i = 0; i < req_param_list.count; i++){
		pos_request_param_t *p = req_param_list.params + i;
		char name[33];
		int n = pos_read_array(buf, (uint8_t *)name, 32);
		if (n <= 0){
			pos_request_param_list_free(&req_param_list);
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		recode_str(name, n);
		p->name = strndup(name, n);
		if (p->name == NULL){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			pos_request_param_list_free(&req_param_list);
			return false;
		}
		p->type = get_param_type(name);
		if (p->type == POS_PARAM_UBT)
			ubt_supported = true;
		if (!pos_read_byte(buf, &p->required)){
			pos_request_param_list_free(&req_param_list);
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
	}
	if (check_only)
		pos_request_param_list_free(&req_param_list);
	else
		pos_send_params_resp();
	return true;
}

static bool pos_parse_response_parameters(struct pos_data_buf *buf, bool check_only)
{
	static char s[2049];
	if (!pos_read_byte(buf, &resp_param_list.count) || (resp_param_list.count == 0)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	resp_param_list.params = calloc(resp_param_list.count, sizeof(pos_response_param_t));
	if (resp_param_list.params == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	for (int i = 0; i < resp_param_list.count; i++){
		pos_response_param_t *p = resp_param_list.params + i;
		int n = pos_read_array(buf, (uint8_t *)s, 32);
		if (n <= 0){
			pos_response_param_list_free(&resp_param_list);
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		recode_str(s, n);
		p->name = strndup(s, n);
		if (p->name == NULL){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			pos_response_param_list_free(&resp_param_list);
			return false;
		}
		n = pos_read_array(buf, (uint8_t *)s, 2048);
		if (n == -1){
			pos_response_param_list_free(&resp_param_list);
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
			return false;
		}
		recode_str(s, n);
		p->value = strndup(s, n);
		if (get_param_type(p->name) == POS_PARAM_UBT)
			ubt_supported = p->value[0] != 0;
		if (p->value == NULL){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			pos_response_param_list_free(&resp_param_list);
			return false;
		}
	}
	if (check_only)
		pos_response_param_list_free(&resp_param_list);
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
	uint16_t len;
	if (!pos_read_word(buf, &len) || (len > POS_MAX_BLOCK_DATA_LEN)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	int l = buf->data_index + len;
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
		for (int i = 0; i < ASIZE(parsers); i++){
			typeof(*parsers) *p = parsers + i;
			if (code == p->code){
				if (p->parser(buf, check_only))
					break;
				else
					return false;
			}else if ((i + 1) == ASIZE(parsers)){
				pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
				return false;
			}
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
	bool ret = false;
	if (pos_req_stream_begin(buf, POS_STREAM_COMMAND) &&
			pos_write_byte(buf, POS_COMMAND_INIT) &&
			pos_req_stream_end(buf))
		ret = true;
	else
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
	return ret;
}

/* Запись команды INIT_CHECK */
bool pos_req_save_command_init_check(struct pos_data_buf *buf)
{
	bool ret = false;
	if (pos_req_stream_begin(buf, POS_STREAM_COMMAND) &&
			pos_write_byte(buf, POS_COMMAND_INIT_CHECK) &&
			pos_req_stream_end(buf))
		ret = true;
	else
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
	return ret;
}

/* Запись команды FINISH */
bool pos_req_save_command_finish(struct pos_data_buf *buf)
{
	bool ret = false;
	if (pos_req_stream_begin(buf, POS_STREAM_COMMAND) &&
			pos_write_byte(buf, POS_COMMAND_FINISH) &&
			pos_req_stream_end(buf))
		ret = true;
	else
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
	return ret;
}

static bool termid_empty(void)
{
	bool ret = true;
	for (size_t i = 0; i < sizeof(bd.term_id); i++){
		if (bd.term_id[i] != 0){
			ret = false;
			break;
		}
	}
	return ret;
}

static void normalize_termid(void)
{
	if (_ad &&
		_ad->p1 &&
		_ad->p1->p && _ad->p1->p[0] && _ad->p1->p[1] &&
		_ad->p1->t && _ad->p1->t[0] &&
		termid_empty())
	{
		size_t p_len = strlen(_ad->p1->p + 1);
		size_t t_len = strlen(_ad->p1->t);

		if (p_len + t_len == sizeof(bd.term_id)) {
			memcpy(bd.term_id, _ad->p1->p + 1, p_len);
			memcpy(bd.term_id + p_len, _ad->p1->t, t_len);
			recode_str(bd.term_id, sizeof(bd.term_id));
		}
	}
}

/* Запись в поток команд заданного параметра в ответ на запрос ИПТ */
static bool pos_write_resp_param(struct pos_data_buf *buf, const char *name, int param,
		bool required)
{
	int l = 0;
	static char val[2049];
	static int ind[] = {7, 0, 6, 5, 4, 3, 2, 1};
	if (buf == NULL)
		return false;
	switch (param){
		case POS_PARAM_AMOUNT:
		{
			uint64_t amount = 0;
			for (size_t i = 0; i < bd.nr_docs; i++)
				amount += bd.doc_info[i].amount;
			sprintf(val, "%lu", amount);
			l = strlen(val);
			break;
		}
		case POS_PARAM_INVOICE:
			snprintf(val, sizeof(val), "%.7u", 12345);	/* FIXME */
			l = strlen(val);
			break;
		case POS_PARAM_TIME:
		{
			time_t t = time(NULL) + time_delta;
			struct tm *tm = localtime(&t);
			sprintf(val, "%.4d%.2d%.2d%.2d%.2d%.2d",
				tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
				tm->tm_hour, tm->tm_min, tm->tm_sec);
			l = strlen(val);
			break;
		}
		case POS_PARAM_ID:
			sprintf(val, "%.*u", BNK_REQ_ID_LEN, bd.req_id);
			l = strlen(val);
			break;
		case POS_PARAM_TERMID:
			normalize_termid();
			memcpy(val, bd.term_id, BNK_TERM_ID_LEN);
			l = BNK_TERM_ID_LEN;
			write_hex_byte((uint8_t *)(val + l), cfg.gaddr);
			write_hex_byte((uint8_t *)(val + l + 2), cfg.iaddr);
			l += 4;
			break;
		case POS_PARAM_CLERKID:
			l = 0;
			for (int i = 0; i < ASIZE(ind); i++, l += 2)
				write_hex_byte((uint8_t *)(val + l), dsn[ind[i]]);
			break;
		case POS_PARAM_CLERKTYPE:
			val[0] = recode(ds_key_char(kt));
			val[1] = 0;
			l = 1;
			break;
		case POS_PARAM_ORDS:
			snprintf(val, sizeof(val), "%s", pos_ords);
			l = strlen(val);
			break;
		case POS_PARAM_UBT:
			val[0] = 1;
			l = 1;
			break;
		case POS_PARAM_MTYPE:
			val[0] = pos_menu_item;
			l = 1;
			break;
		case POS_PARAM_EDIT:
			val[0] = pos_can_edit;
			l = 1;
			break;
		case POS_PARAM_FMENU:
			val[0] = 1;
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

/* Запись в поток команд заданного параметра в для запроса ИПТ */
static bool pos_write_req_param(struct pos_data_buf *buf, const char *name, bool required)
{
	if (buf == NULL)
		return false;
	return	pos_write_array(buf, (uint8_t *)name, strlen(name)) &&
		pos_write_byte(buf, required);
}

/* Запись команды RESPONSE_PARAMETERS на основе req_param_list */
bool pos_req_save_command_response_parameters(struct pos_data_buf *buf)
{
	int n = 0;
	if (req_param_list.count == 0)
		return true;
	for (int i = 0; i < req_param_list.count; i++){
		if (req_param_list.params[i].type != POS_PARAM_UNKNOWN)
			n++;
	}
	if (!pos_req_stream_begin(buf, POS_STREAM_COMMAND) ||
			!pos_write_byte(buf, POS_COMMAND_RESPONSE_PARAMETERS) ||
			!pos_write_byte(buf, n)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
	for (int i = 0; i < req_param_list.count; i++){
		pos_request_param_t *p = req_param_list.params + i;
		if (p->type == POS_PARAM_UNKNOWN)
			continue;
		else if (!pos_write_resp_param(buf, p->name, p->type, p->required))
			return false;
	}
	if (pos_req_stream_end(buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
}

/* Запись команды REQUEST_PARAMETERS на основе req_param_list */
bool pos_req_save_command_request_parameters(struct pos_data_buf *buf)
{
	int n = 0;
	if (req_param_list.count == 0)
		return true;
	for (int i = 0; i < req_param_list.count; i++){
		if (req_param_list.params[i].type != POS_PARAM_UNKNOWN)
			n++;
	}
	if (!pos_req_stream_begin(buf, POS_STREAM_COMMAND) ||
			!pos_write_byte(buf, POS_COMMAND_REQUEST_PARAMETERS) ||
			!pos_write_byte(buf, n)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
	for (int i = 0; i < req_param_list.count; i++){
		pos_request_param_t *p = req_param_list.params + i;
		if (p->type == POS_PARAM_UNKNOWN)
			continue;
		if (!pos_write_req_param(buf, p->name, p->required))
			return false;
	}
	if (pos_req_stream_end(buf))
		return true;
	else{
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_SYSTEM, 0);
		return false;
	}
}

bool pos_prepare_request_params(void)
{
	static const struct {
		const char *name;
		int type;
		bool required;
	} params[] = {
		{POS_PARAM_RES_CODE_STR,	POS_PARAM_RES_CODE,	true},
		{POS_PARAM_RESP_CODE_STR,	POS_PARAM_RESP_CODE,	true},
		{POS_PARAM_ID_POS_STR,		POS_PARAM_ID_POS,	true},
		{POS_PARAM_INVOICE_STR,		POS_PARAM_INVOICE,	true},
		{POS_PARAM_NMTYPE_STR,		POS_PARAM_NMTYPE,	true},
		{POS_PARAM_PARAMS_STR,		POS_PARAM_PARAMS,	true},
	};
	pos_request_param_list_free(&req_param_list);
	req_param_list.count = ASIZE(params);
	req_param_list.params = calloc(req_param_list.count, sizeof(pos_request_param_t));
	if (req_param_list.params == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	for (int i = 0; i < ASIZE(params); i++){
		const typeof(*params) *param = params + i;
		pos_request_param_t *p = req_param_list.params + i;
		p->name = strdup(param->name);
		if (p->name == NULL){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			pos_request_param_list_free(&req_param_list);
			return false;
		}
		p->type = param->type;
		p->required = param->required;
	}
	return true;
}
