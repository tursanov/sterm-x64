/* Работа с потоком команд POS-эмулятора. (c) A.Popov, gsr 2004 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gui/scr.h"
#include "kkt/fd/ad.h"
#include "kkt/kkt.h"
#include "pos/command.h"
#include "pos/error.h"
#include "ds1990a.h"
#include "genfunc.h"
#include "numbers.h"
#include "sterm.h"
#include "termlog.h"

/* Имеются незавершённые операции ИПТ */
bool pos_incomplete_op = false;

/* Поддержка ЕБТ в ИПТ */
bool ubt_supported = false;

/* Параметры запроса ИПТ */
static struct pos_query_params pos_query_params;

static void clr_pos_query_params(void)
{
	pos_query_params.amount = 0;
	pos_query_params.order_id = 0;
	pos_query_params.can_edit = false;
	pos_query_params.time = 0;
	pos_query_params.invoice = 0;
	if (pos_query_params.ords != NULL){
		free((void *)pos_query_params.ords);
		pos_query_params.ords = NULL;
	}
	if (pos_query_params.type != NULL){
		free((void *)pos_query_params.type);
		pos_query_params.type = NULL;
	}
	if (pos_query_params.subtype != NULL){
		free((void *)pos_query_params.subtype);
		pos_query_params.subtype = NULL;
	}
	if (pos_query_params.famio != NULL){
		free((void *)pos_query_params.famio);
		pos_query_params.famio = NULL;
	}
	if (pos_query_params.rfnd_info != NULL){
		free((void *)pos_query_params.rfnd_info);
		pos_query_params.rfnd_info = NULL;
	}
	pos_query_params.mtype = MTYPE_UNKNOWN;
}

static void set_pos_query_params(const struct pos_query_params *params)
{
	clr_pos_query_params();
	pos_query_params.amount = params->amount;
	pos_query_params.order_id = params->order_id;
	pos_query_params.can_edit = params->can_edit;
	pos_query_params.time = params->time;
	pos_query_params.invoice = params->invoice;
	if (params->ords != NULL)
		pos_query_params.ords = strdup(params->ords);
	if (params->type != NULL)
		pos_query_params.type = strdup(params->type);
	if (params->subtype != NULL)
		pos_query_params.subtype = strdup(params->subtype);
	if (params->famio != NULL)
		pos_query_params.famio = strdup(params->famio);
	if (params->rfnd_info != NULL)
		pos_query_params.rfnd_info = strdup(params->rfnd_info);
	pos_query_params.mtype = params->mtype;
}

/* Информация об ИПТ */
struct pos_info pos_info;
bool pos_info_req_sent = false;

static inline bool pos_info_empty(void)
{
	return	(pos_info.version == NULL) &&
		(pos_info.op_types == NULL) &&
		(pos_info.model == NULL) &&
		(pos_info.serial_nr == NULL) &&
		(pos_info.os_version == NULL) &&
		(pos_info.tms_id == NULL) &&
		(pos_info.servers == POS_DEF_SERVERS);
}

void pos_clr_info(void)
{
	if (pos_info.version != NULL){
		free((void *)pos_info.version);
		pos_info.version = NULL;
	}
	if (pos_info.op_types != NULL){
		free((void *)pos_info.op_types);
		pos_info.op_types = NULL;
	}
	if (pos_info.model != NULL){
		free((void *)pos_info.model);
		pos_info.model = NULL;
	}
	if (pos_info.serial_nr != NULL){
		free((void *)pos_info.serial_nr);
		pos_info.serial_nr = NULL;
	}
	if (pos_info.os_version != NULL){
		free((void *)pos_info.os_version);
		pos_info.os_version = NULL;
	}
	if (pos_info.tms_id != NULL){
		free((void *)pos_info.tms_id);
		pos_info.tms_id = NULL;
	}
	pos_info.servers = POS_DEF_SERVERS;
	pos_info_req_sent = false;
}

/* Получена команда FINISHMENU */
bool fmenu = false;

pos_request_param_list_t req_param_list;
pos_response_param_list_t resp_param_list;

/* Удаление списка */
static void pos_request_param_list_free(pos_request_param_list_t *list, bool free_list)
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
		{POS_PARAM_TYPES_STR,		POS_PARAM_TYPES},
		{POS_PARAM_MODEL_STR,		POS_PARAM_MODEL},
		{POS_PARAM_SERIALNO_STR,	POS_PARAM_SERIALNO},
		{POS_PARAM_OSVERSION_STR,	POS_PARAM_OSVERSION},
		{POS_PARAM_TMS_ID_STR,		POS_PARAM_TMS_ID},
		{POS_PARAM_SERVERS_STR,		POS_PARAM_SERVERS},
		{POS_PARAM_MTYPE_STR,		POS_PARAM_MTYPE},
		{POS_PARAM_EDIT_STR,		POS_PARAM_EDIT},
		{POS_PARAM_FMENU_STR,		POS_PARAM_FMENU},
		{POS_PARAM_RES_CODE_STR,	POS_PARAM_RES_CODE},
		{POS_PARAM_RESP_CODE_STR,	POS_PARAM_RESP_CODE},
		{POS_PARAM_ID_POS_STR,		POS_PARAM_ID_POS},
		{POS_PARAM_NMTYPE_STR,		POS_PARAM_NMTYPE},
		{POS_PARAM_NR_PARAMS_STR,	POS_PARAM_NR_PARAMS},
		{POS_PARAM_PARAMS_STR,		POS_PARAM_PARAMS},
		{POS_PARAM_TYPE_STR,		POS_PARAM_TYPE},
		{POS_PARAM_SUBTYPE_STR,		POS_PARAM_SUBTYPE},
		{POS_PARAM_FAMIO_STR,		POS_PARAM_FAMIO},
		{POS_PARAM_RFNDINFO_STR,	POS_PARAM_RFNDINFO},
		{POS_PARAM_FRAGMENTATION_STR,	POS_PARAM_FRAGMENTATION},
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

static bool pos_parse_finish(struct pos_data_buf *buf __attribute__((unused)), bool check_only)
{
	if (!check_only)
		pos_set_state(pos_finish);
	return true;
}

static bool pos_parse_request_parameters(struct pos_data_buf *buf, bool check_only)
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
		pos_send_params_resp();
	return true;
}

static struct pos_response pos_resp;

static void clr_pos_resp(struct pos_response *pos_resp)
{
	pos_resp->res_code = 0;
	if (pos_resp->resp_code != NULL){
		free((void *)pos_resp->resp_code);
		pos_resp->resp_code = NULL;
	}
	if (pos_resp->id_pos != NULL){
		free((void *)pos_resp->id_pos);
		pos_resp->id_pos = NULL;
	}
	pos_resp->invoice = 0;
	pos_resp->next_mtype = MTYPE_UNKNOWN;
	pos_resp->nr_params = 0;
	for (int i = 0; i < ASIZE(pos_resp->params); i++){
		struct pos_param *p = pos_resp->params + i;
		if (p->name != NULL){
			free((void *)p->name);
			p->name = NULL;
		}
		if (p->val != NULL){
			free((void *)p->val);
			p->val = NULL;
		}
	}
}

static void make_pos_resp(struct pos_response *pos_resp)
{
	clr_pos_resp(pos_resp);
	for (int i = 0; i < resp_param_list.count; i++){
		pos_response_param_t *p = resp_param_list.params + i;
		switch (get_param_type(p->name)){
			case POS_PARAM_RES_CODE:
				pos_resp->res_code = p->value[0];
				break;
			case POS_PARAM_RESP_CODE:
				pos_resp->resp_code = strdup(p->value);
				break;
			case POS_PARAM_ID_POS:
				pos_resp->id_pos = strdup(p->value);
				break;
			case POS_PARAM_INVOICE: {
				char *ep = NULL;
				uint32_t v = strtoul(p->value, &ep, 10);
				if (*ep == 0)
					pos_resp->invoice = v;
				}
				break;
			case POS_PARAM_NMTYPE:
				pos_resp->next_mtype = p->value[0];
				break;
		}
	}
	log_info("\n\tres_code = 0x%.hhx;"
			"\n\tresp_code = %s;"
			"\n\tid_pos = %s;"
			"\n\tinvoice = %u;"
			"\n\tnext_mtype = 0x%.2hhx;"
			"\n\tnr_params = %hhd.",
		pos_resp->res_code, pos_resp->resp_code, pos_resp->id_pos, pos_resp->invoice,
		pos_resp->next_mtype, pos_resp->nr_params);
}

static uint32_t get_srv_list(const char *txt)
{
	uint32_t ret = 0;
	char *delim = NULL;
	for (const char *s = txt; *s != 0; s = delim + 1){
		int n = 0;
		if ((sscanf(s, "%d", &n) == 1) && (n > 0) && (n < 9))
			ret |= 1 << (n - 1);
		delim = strchr(s, ',');
		if (delim == NULL)
			break;
	}
	if (ret == 0)
		ret = POS_DEF_SERVERS;
	return ret;
}

static void make_pos_info(void)
{
	int n = 0;
	for (int i = 0; i < resp_param_list.count; i++){
		pos_response_param_t *p = resp_param_list.params + i;
		switch (get_param_type(p->name)){
			case POS_PARAM_VERSION:
				if (pos_info.version != NULL)
					free((void *)pos_info.version);
				pos_info.version = strdup(p->value);
				n++;
				break;
			case POS_PARAM_TYPES:
				if (pos_info.op_types != NULL)
					free((void *)pos_info.op_types);
				pos_info.op_types = strdup(p->value);
				n++;
				break;
			case POS_PARAM_MODEL:
				if (pos_info.model != NULL)
					free((void *)pos_info.model);
				pos_info.model = strdup(p->value);
				n++;
				break;
			case POS_PARAM_SERIALNO:
				if (pos_info.serial_nr != NULL)
					free((void *)pos_info.serial_nr);
				pos_info.serial_nr = strdup(p->value);
				n++;
				break;
			case POS_PARAM_OSVERSION:
				if (pos_info.os_version != NULL)
					free((void *)pos_info.os_version);
				pos_info.os_version = strdup(p->value);
				n++;
				break;
			case POS_PARAM_TMS_ID:
				if (pos_info.tms_id != NULL)
					free((void *)pos_info.tms_id);
				pos_info.tms_id = strdup(p->value);
				n++;
				break;
			case POS_PARAM_SERVERS:
				pos_info.servers = get_srv_list(p->value);
				n++;
				break;
		}
	}
	if (n > 0)
		pos_reinit();
}

static bool pos_parse_response_parameters(struct pos_data_buf *buf, bool check_only)
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
	else if (fmenu){
		fmenu = false;
		make_pos_resp(&pos_resp);
		pos_send_empty();
	}else if (pos_info_empty())
		make_pos_info();
	return true;
}

static bool pos_parse_init_required(struct pos_data_buf *buf __attribute__((unused)),
	bool check_only)
{
	if (!check_only && (_term_aux_state == ast_none))
		pos_incomplete_op = true;
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
		if (bd.term_id[i] != '0'){
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
	static char val[2049];
	static int ind[] = {7, 0, 6, 5, 4, 3, 2, 1};
	int l = 0;
	if (buf == NULL)
		return false;
	switch (param){
		case POS_PARAM_TERMID:
			normalize_termid();
			memcpy(val, bd.term_id, BANK_TERM_ID_LEN);
			l = BANK_TERM_ID_LEN;
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
		case POS_PARAM_AMOUNT:
			sprintf(val, "%lu", pos_query_params.amount);
			l = strlen(val);
			break;
		case POS_PARAM_ID:
			sprintf(val, "%.*u", BANK_REQ_ID_LEN_NEW, pos_query_params.order_id);
			l = strlen(val);
			break;
		case POS_PARAM_EDIT:
			val[0] = pos_query_params.can_edit ? 1 : 0;
			l = 1;
			break;
		case POS_PARAM_TIME:
		{
			struct tm *tm = localtime(&pos_query_params.time);
			sprintf(val, "%.4d%.2d%.2d%.2d%.2d%.2d",
				tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
				tm->tm_hour, tm->tm_min, tm->tm_sec);
			l = strlen(val);
			break;
		}
		case POS_PARAM_INVOICE:
			snprintf(val, sizeof(val), "%.7u", pos_query_params.invoice);
			l = strlen(val);
			break;
		case POS_PARAM_ORDS:
			snprintf(val, sizeof(val), "%s", pos_query_params.ords);
			l = strlen(val);
			break;
		case POS_PARAM_TYPE:
			snprintf(val, sizeof(val), "%s", pos_query_params.type);
			l = strlen(val);
			break;
		case POS_PARAM_SUBTYPE:
			snprintf(val, sizeof(val), "%s", pos_query_params.subtype);
			l = strlen(val);
			break;
		case POS_PARAM_FAMIO:
			snprintf(val, sizeof(val), "%s", pos_query_params.famio);
			l = strlen(val);
			break;
		case POS_PARAM_RFNDINFO:
			snprintf(val, sizeof(val), "%s", pos_query_params.rfnd_info);
			l = strlen(val);
			break;
		case POS_PARAM_UBT:
			val[0] = 1;
			l = 1;
			break;
		case POS_PARAM_MTYPE:
			val[0] = pos_query_params.mtype;
			l = 1;
			break;
		case POS_PARAM_FMENU:
			val[0] = 1;
			l = 1;
			fmenu = true;
			break;
		case POS_PARAM_FRAGMENTATION:
			val[0] = kkt_has_param("SUPPORT_FRAGMENTATION") ? 1 : 0;
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

/* Запись в поток команд заданного параметра для запроса ИПТ */
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
		else if ((p->type == POS_PARAM_MTYPE) && !pos_info_req_sent)
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

struct param_info {
	const char *name;
	int type;
	bool required;
};

static bool pos_prepare_request(const struct param_info *params, size_t nr_params)
{
	pos_request_param_list_free(&req_param_list, false);
	req_param_list.count = nr_params;
	req_param_list.params = calloc(req_param_list.count, sizeof(pos_request_param_t));
	if (req_param_list.params == NULL){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	for (int i = 0; i < nr_params; i++){
		const struct param_info *param = params + i;
		pos_request_param_t *p = req_param_list.params + i;
		p->name = strdup(param->name);
		if (p->name == NULL){
			pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
			pos_request_param_list_free(&req_param_list, false);
			return false;
		}
		p->type = param->type;
		p->required = param->required;
	}
	return true;
}

/* Подготовка списка параметров для запроса у ИПТ (FINISHMENU) */
bool pos_prepare_request_params(void)
{
	static const struct param_info params[] = {
		{POS_PARAM_MTYPE_STR,		POS_PARAM_MTYPE,	false},
		{POS_PARAM_RES_CODE_STR,	POS_PARAM_RES_CODE,	false},
		{POS_PARAM_RESP_CODE_STR,	POS_PARAM_RESP_CODE,	false},
		{POS_PARAM_ID_POS_STR,		POS_PARAM_ID_POS,	false},
		{POS_PARAM_INVOICE_STR,		POS_PARAM_INVOICE,	false},
		{POS_PARAM_NMTYPE_STR,		POS_PARAM_NMTYPE,	false},
		{POS_PARAM_NR_PARAMS_STR,	POS_PARAM_NR_PARAMS,	false},
		{POS_PARAM_PARAMS_STR,		POS_PARAM_PARAMS,	false},
	};
	return pos_prepare_request(params, ASIZE(params));
}

struct pos_response *pos_query(const struct pos_query_params *params)
{
	if (params == NULL)
		return NULL;
	set_pos_query_params(params);
	clr_pos_resp(&pos_resp);
	show_pos();
	if ((pos_state == pos_new) || (pos_state == pos_idle))
		return NULL;
	while (pos_state != pos_new){
		if (get_cmd(false, true) == cmd_reset){
			if (reset_term(false))
				return NULL;
		}
	}
	return &pos_resp;
}

/* Подготовка списка параметров для запроса информации об ИПТ */
bool pos_prepare_request_info(void)
{
	static const struct param_info params[] = {
		{POS_PARAM_VERSION_STR,		POS_PARAM_VERSION,	true},
		{POS_PARAM_TYPES_STR,		POS_PARAM_TYPES,	false},
		{POS_PARAM_MODEL_STR,		POS_PARAM_MODEL,	false},
		{POS_PARAM_SERIALNO_STR,	POS_PARAM_SERIALNO,	false},
		{POS_PARAM_OSVERSION_STR,	POS_PARAM_OSVERSION,	false},
		{POS_PARAM_TMS_ID_STR,		POS_PARAM_TMS_ID,	false},
		{POS_PARAM_SERVERS_STR,		POS_PARAM_SERVERS,	false},
	};
	return pos_prepare_request(params, ASIZE(params));
}
