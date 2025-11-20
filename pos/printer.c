/* Работа с потоком POS-эмулятора для принтера. (c) A.Popov, gsr 2004 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pos/error.h"
#include "pos/printer.h"
#include "genfunc.h"

uint8_t pos_prn_buf[65536];
size_t pos_prn_data_len = 0;

static bool pos_parse_print(struct pos_data_buf *buf, bool check_only)
{
	if (pos_prn_data_len >= sizeof(pos_prn_buf)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_LOW_MEM, 0);
		return false;
	}
	int n = pos_read_array(buf, pos_prn_buf + pos_prn_data_len,
			sizeof(pos_prn_buf) - pos_prn_data_len);
	if (n <= 0){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}
	size_t l0 = pos_prn_data_len;
	pos_prn_data_len += n;
	uint8_t attr = 0;
	if (!pos_read_byte(buf, &attr) || (attr != 0)){
		pos_set_error(POS_ERROR_CLASS_SYSTEM, POS_ERR_MSG_FMT, 0);
		return false;
	}else{
		if (check_only)
			pos_prn_data_len = l0;
		else{
			if ((l0 >= sizeof(uint32_t)) && (n >= sizeof(uint32_t)) &&
					(*((const uint32_t *)(pos_prn_buf + l0)) == 0)){
				if (n > sizeof(uint32_t))
					memmove(pos_prn_buf + l0, pos_prn_buf + l0 + sizeof(uint32_t),
						n - sizeof(uint32_t));
				pos_prn_data_len -= sizeof(uint32_t);
			}
			pos_set_state(pos_print);
		}
		return true;
	}
}

bool pos_parse_printer_stream(struct pos_data_buf *buf, bool check_only)
{
	static struct {
		uint8_t code;
		bool (*parser)(struct pos_data_buf *, bool);
	} parsers[] = {
		{POS_PRINTER_PRINT,	pos_parse_print},
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
