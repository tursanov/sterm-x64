/*  ΅®β  α ―®β®ª®¬ ―¥ΰ¥¤ η¨ ®θ¨΅®ª POS-ν¬γ«οβ®ΰ . (c) A.Popov, gsr 2004 */

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pos/error.h"
#include "genfunc.h"

pos_error_t pos_error;
/* ―¨α ­¨¥ ®θ¨΅ª¨ */
char pos_err_desc[256];
/* ®¤ ®θ¨΅ª¨ Ά α¨αβ¥¬¥ "ªα―ΰ¥αα" */
char *pos_err_xdesc = NULL;

bool pos_parse_error_stream(struct pos_data_buf *buf, bool check_only)
{
	uint8_t class;
	uint16_t len, code;
	static char s[1025];
	int l, n;
/* „«¨­  α®®΅ι¥­¨ο */
	if (!pos_read_word(buf, &len) || (len > POS_MAX_BLOCK_DATA_LEN)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	l = buf->data_index + len;
	if (l > buf->data_len){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
/* « αα ¨ ª®¤ ®θ¨΅ª¨ */
	if (!pos_read_byte(buf, &class) || !pos_read_word(buf, &code)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
/* ‘®®΅ι¥­¨¥ ®΅ ®θ¨΅ª¥ */
	n = pos_read_array(buf, (uint8_t *)s, 128);
	if (n == -1){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	s[n] = 0;
	recode_str(s, n);
	if (!check_only)
		strcpy(pos_error.message, s);
/*  ΰ ¬¥βΰλ (¥α«¨ ¥αβμ) */
	n = pos_read_array(buf, (uint8_t *)s, 1024);
	if (n == -1){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	if (buf->data_index != l){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}else if (!check_only){
		pos_error.class = class;
		pos_error.code = code;
		if (n > 0){
			memcpy(pos_error.parameters, s, n);
			pos_error.param_len = n;
		}else{
			pos_error.parameters[0] = 0;
			pos_error.param_len = 0;
		}
		pos_set_error(-1, -1, 0);
	}
	return true;
}

/* ‡ ―¨αμ ®θ¨΅ª¨ */
bool pos_req_save_error_stream(struct pos_data_buf *buf, pos_error_t *err)
{
	static char s[sizeof(err->message) + 1];
	int l;
	if (!pos_req_stream_begin(buf, POS_STREAM_ERROR))
		return false;
	strcpy(s, err->message);
	s[sizeof(err->message)] = 0;
	l = strlen(s);
	recode_str(s, l);
	if (!pos_write_byte(buf, err->class) ||
			!pos_write_word(buf, err->code) ||
			!pos_write_array(buf, (uint8_t *)s, l) ||
			!pos_write_array(buf, (uint8_t *)err->parameters,
				err->param_len))
		return false;
	return pos_req_stream_end(buf);
}

/* “αβ ­®Άª  ®θ¨΅ª¨ */
enum {
	err_param_none,
	err_param_byte,
	err_param_str,
};

struct {
	uint8_t class;
	struct err_code {
		uint16_t code;
		int param_type;
		char *desc;
		char *xdesc;
	} *codes;
} err[] = {
/* ‘¨αβ¥¬  */
	{
		.class	= POS_ERROR_CLASS_SYSTEM,
		.codes	= (struct err_code[]){
			{	
				POS_ERR_LOW_MEM,
				err_param_none,
				"…•‚€’€ €’ „‹ ‚›‹… …€–",
				":04"
			},
			{
				POS_ERR_MSG_FMT,
				err_param_none,
				"€ ”€’€: ’›… €…’› … ƒ“’ ›’ €‡€›",
				":01"
			},
			{
				POS_ERR_SYSTEM,
				err_param_none,
				"”€’€‹€ € ‘‘’…›",
				":02"
			},
			{
				POS_ERR_TIMEOUT,
				err_param_none,
				"’€‰€“’ ‘€ POS-“‹’€",
				":03"
			},
			{
				POS_ERR_CRYPTO,
				err_param_none,
				"€ ’ƒ€”",
				":05"
			},
			{
				POS_ERR_NOT_INIT,
				err_param_none,
				"POS-“‹’ … –€‹‡‚€",
				":06"
			},
			{0, 0, NULL, NULL},
		}
	}, 
/* ªΰ ­ */
	{
		.class	= POS_ERROR_CLASS_SCREEN,
		.codes	= (struct err_code[]){
			{
				POS_ERR_SCR_NOT_READY,
				err_param_none,
				"“‘’‰‘’‚ ‚›‚„€ …„‘’“",
				":11"
			},
			{
				POS_ERR_DEPICT,
				err_param_str,
				"…‚‡† ’€‡’",
				":12"
			},
			{0, 0, NULL, NULL},
		}
	},
/* « Ά¨ βγΰ  */
	{
		.class	= POS_ERROR_CLASS_KEYBOARD,
		.codes	= (struct err_code[]){
			{
				POS_ERR_KEY_MSG_FMT,
				err_param_none,
				"€ ”€’€: …‚‡† €‡€’ ‘™……",
				":23"
			},
			{
				POS_ERR_KEY_DBL_OBJ,
				err_param_str,
				"„“‹‚€…  '…’€",
				":24"
			},
			{
				POS_ERR_UNKNOWN_OBJ,
				err_param_str,
				"……„…‹…€ ………€",
				":25"
			},
			{0, 0, NULL, NULL},
		}
	},
/* ΰ¨­β¥ΰ */
	{
		.class	= POS_ERROR_CLASS_PRINTER,
		.codes	= (struct err_code[]){
			{
				POS_ERR_PRN,
				err_param_none,
				"€ ’…€",
				":38"
			},
			{0, 0, NULL, NULL},
		}
	},
/* TCP/IP */
	{
		.class	= POS_ERROR_CLASS_TCP,
		.codes	= (struct err_code[]){
			{
				POS_ERR_TCP_INIT,
				err_param_none,
				"€ –€‹‡€– TCP/IP „€‰‚…‚",
				":42"
			},
			{
				POS_ERR_HOST_NOT_CNCT,
				err_param_byte,
				"… “‘’€‚‹… ‘…„…… ‘ ‘…‚…",
				":44"
			},
			{
				POS_ERR_CONNECT,
				err_param_byte,
				"…‚‡† “‘’€‚’ ‘…„…… ‘ ‘…‚…",
				":45"
			},
			{
				POS_ERR_DATA_TX,
				err_param_byte,
				"€ ……„€— „€›• ‘…‚…“",
				":46"
			},
			{
				POS_ERR_TIMEOUT_RX,
				err_param_byte,
				"’€‰€“’ …€ „€›• ‘ ‘…‚…€",
				":47"
			},
			{
				POS_ERR_TIMEOUT_TX,
				err_param_byte,
				"’€‰€“’ ……„€— „€›• € ‘…‚…",
				":48"
			},
			{0, 0, NULL, NULL},
		}
	},
/* ®¬ ­¤λ */
	{
		.class	= POS_ERROR_CLASS_COMMANDS,
		.codes	= (struct err_code[]){
			{
				POS_ERR_FMT,
				err_param_none,
				"€ ”€’€ €„›",
				":91"
			},
			{
				POS_ERR_CMD,
				err_param_none,
				"€„€ … †…’ ›’ €’€€",
				":91"
			},
			{0, 0, NULL, NULL},
		}
	},
};

void pos_set_error(int class, int code, intptr_t param)
{
	static char s[sizeof(pos_error.parameters)];
	int i, j;
	bool in = (class == -1) && (code == -1);
	if (in){
		class = pos_error.class;
		code = pos_error.code;
	}else{
		pos_error.class = class;
		pos_error.code = code;
		pos_error.param_len = 0;
	}
	if ((class == 0) && (code == 0)){
		pos_err_desc[0] = 0;
		pos_err_xdesc = NULL;
		return;
	}
	switch (pos_get_state()){
		case pos_init:
		case pos_ready:
		case pos_enter:
		case pos_print:
		case pos_printing:
			pos_set_state(in ? pos_err : pos_err_out);
			break;
		case pos_new:
			break;
		default:
			pos_set_state(pos_break);
			break;
	}
	for (i = 0; i < ASIZE(err); i++){
		if (err[i].class == class){
			for (j = 0; err[i].codes[j].xdesc != NULL; j++){
				if (err[i].codes[j].code == code){
					if (!in)
						snprintf(pos_error.message,
							sizeof(pos_error.message),
							"%s", err[i].codes[j].desc);
					pos_err_xdesc = err[i].codes[j].xdesc;
					if (err[i].codes[j].param_type == err_param_byte){
						if (in){
							if (pos_error.param_len == 1)
								param = pos_error.parameters[0];
							else
								param = 0;
						}else{
							pos_error.parameters[0] = param;
							pos_error.param_len = 1;
						}
						snprintf(pos_err_desc, sizeof(pos_err_desc),
							"%s %td",
							err[i].codes[j].desc, param & 0xff);
					}else if (err[i].codes[j].param_type == err_param_str){
						if (in){
							if (pos_error.param_len > 0){
								memcpy(s, pos_error.parameters, pos_error.param_len);
								recode_str(s, pos_error.param_len);
								snprintf(pos_err_desc, sizeof(pos_err_desc),
									"%s '%.*s'",
									err[i].codes[j].desc,
									pos_error.param_len, s);
							}else
								snprintf(pos_err_desc, sizeof(pos_err_desc),
									"%s", err[i].codes[j].desc);
						}else if ((char *)param != NULL){
							uint16_t l = strlen((char *)param);
							if (l > sizeof(pos_error.parameters))
								l = sizeof(pos_error.parameters);
							if (l > 0){
								memcpy(pos_error.parameters,
									(char *)param, l);
								recode_str(pos_error.parameters, l);
								snprintf(pos_err_desc, sizeof(pos_err_desc),
									"%s '%s'",
									err[i].codes[j].desc, (char *)param);
							}else
								snprintf(pos_err_desc, sizeof(pos_err_desc),
									"%s", err[i].codes[j].desc);
							pos_error.param_len = l;
						}else{
							snprintf(pos_err_desc, sizeof(pos_err_desc),
								"%s", err[i].codes[j].desc);
							pos_error.param_len = 0;
						}
					}else{	/* ­¥β ― ΰ ¬¥βΰ®Ά */
						if (!in)
							pos_error.param_len = 0;
						snprintf(pos_err_desc, sizeof(pos_err_desc),
							"%s", err[i].codes[j].desc);
					}
					break;
				}
			}
			break;
		}
	}
}

/* ‘΅ΰ®α ®θ¨΅ª¨ */
void pos_error_clear(void)
{
	pos_set_error(0, 0, 0);
}
