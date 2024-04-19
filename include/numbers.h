/* Чтение/запись десятичных и шестнадцатеричных чисел. (c) gsr 2004, 2024 */

#if !defined HEX_H
#define HEX_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

extern bool number_error;

extern uint32_t read_hex(uint8_t *p, int n);
extern uint8_t read_hex_byte(uint8_t *p);
extern uint16_t read_hex_word(uint8_t *p);
extern uint32_t read_hex_dword(uint8_t *p);
extern void write_hex(uint8_t *p, uint32_t v, int n);
extern void write_hex_byte(uint8_t *p, uint8_t b);
extern void write_hex_word(uint8_t *p, uint16_t w);
extern void write_hex_dword(uint8_t *p, uint32_t dw);

extern bool read_var_uint(const uint8_t *data, size_t *len, size_t max_len, uint32_t *val);
extern bool read_uint(const uint8_t *data, size_t len, uint32_t *val);
extern bool write_uint(uint8_t *data, uint32_t val, size_t len);
extern bool read_var_int(const uint8_t *data, size_t *len, size_t max_len, int32_t *val);
extern bool read_int(const uint8_t *data, size_t len, int32_t *val);
extern bool write_int(uint8_t *data, int32_t val, size_t len);

#if defined __cplusplus
}
#endif

#endif		/* HEX_H */
