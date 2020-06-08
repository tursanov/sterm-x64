/* Работа с потоком POS-эмулятора для принтера. (c) A.Popov, gsr 2004 */

#if !defined POS_PRINTER_H
#define POS_PRINTER_H

#include "pos/pos.h"

#define POS_PRINTER_PRINT	0x05

extern uint8_t pos_prn_buf[2048];
extern int  pos_prn_data_len;

extern bool pos_parse_printer_stream(struct pos_data_buf *buf, bool check_only);

#endif		/* POS_PRINTER_H */
