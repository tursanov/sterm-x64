/* Работа с потоком POS-эмулятора для принтера. (c) A.Popov, gsr 2004 */

#if !defined POS_PRINTER_H
#define POS_PRINTER_H

#if defined __cplusplus
extern "C" {
#endif

#include "kkt/cmd.h"
#include "pos/pos.h"
#include "cfg.h"

#define POS_PRINTER_PRINT	0x05

extern uint8_t pos_prn_buf[65536];
extern size_t pos_prn_data_len;

static inline bool is_pos_prn_special(void)
{
	const uint32_t *p = (const uint32_t *)pos_prn_buf;
	return cfg.tickets_on_kkt && (pos_prn_data_len >= sizeof(uint32_t)) && (*p == 0);
}

static inline bool is_pos_prn_stream_complete(void)
{
	bool ret = false;
	if (pos_prn_data_len > 0){
		uint8_t b = pos_prn_buf[pos_prn_data_len];
		ret = (b == KKT_FF) || (b == KKT_END_BLOCK);
	}
	return ret;
}

extern bool pos_parse_printer_stream(struct pos_data_buf *buf, bool check_only);

#if defined __cplusplus
}
#endif

#endif		/* POS_PRINTER_H */
