/* Чтение/запись шестнадцатеричных чисел. (c) gsr 2004 */

#if !defined HEX_H
#define HEX_H

#include "sysdefs.h"

extern bool hex_error;

extern uint32_t read_hex(uint8_t *p, int n);
extern uint8_t read_hex_byte(uint8_t *p);
extern uint16_t read_hex_word(uint8_t *p);
extern uint32_t read_hex_dword(uint8_t *p);
extern void write_hex(uint8_t *p, uint32_t v, int n);
extern void write_hex_byte(uint8_t *p, uint8_t b);
extern void write_hex_word(uint8_t *p, uint16_t w);
extern void write_hex_dword(uint8_t *p, uint32_t dw);

#endif		/* HEX_H */
