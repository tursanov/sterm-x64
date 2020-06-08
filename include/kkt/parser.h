/* Разбор ответов ККТ. (c) gsr 2018-2019 */

#if !defined KKT_PARSER_H
#define KKT_PARSER_H

#include "kkt/kkt.h"

extern uint8_t tx_prefix;
extern uint8_t tx_cmd;

extern uint8_t kkt_status;

extern void begin_rx(void);

struct doc_info_arg {
	struct kkt_doc_info di;
	struct kkt_var_data err_info;
};

struct last_doc_info_arg {
	struct kkt_last_doc_info ldi;
	struct kkt_var_data err_info;
};

struct last_printed_info_arg {
	struct kkt_last_printed_info lpi;
	struct kkt_var_data err_info;
};

struct find_doc_info_arg {
	struct kkt_fs_find_doc_info fdi;
	struct kkt_var_data data;
};

struct read_doc_tlv_arg {
	uint16_t tag;
	struct kkt_var_data data;
};

typedef bool (*parser_t)();

extern parser_t get_parser(uint8_t prefix, uint8_t cmd);

#endif		/* KKT_PARSER_H */
