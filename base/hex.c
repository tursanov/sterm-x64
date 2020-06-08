/* Чтение/запись шестнадцатеричных чисел. (c) gsr 2004 */

#include "sysdefs.h"

/*
 * В записи шестнадцатеричных чисел используются только заглавные латинские
 * буквы (KOI-7), поэтому мы не можем использовать стандартную функцию
 * isxdigit из ctype.h.
 */
static bool is_xdigit(uint8_t b)
{
	return ((b >= '0') && (b <= '9')) ||
		((b >= 'A') && (b <= 'F'));
}

/* Флаг ошибки чтения */
bool hex_error = false;

/* Чтение шестнадцатеричного числа с заданным количеством цифр (не более 8) */
uint32_t read_hex(uint8_t *p, int n)
{
	uint8_t b;
	uint32_t v = 0;
	hex_error = true;
	if (n > 8)
		n = 8;
	for (; n > 0; n--){
		b = *p++;
		if (is_xdigit(b)){
			b -= (b < 'A') ? 0x30 : 0x37;
			v |= ((uint32_t)(b & 0x0f) << ((n - 1) * 4));
		}else
			return 0;
	}
	hex_error = false;
	return v;
}

uint8_t read_hex_byte(uint8_t *p)
{
	return read_hex(p, 2);
}

uint16_t read_hex_word(uint8_t *p)
{
	return (uint16_t)read_hex(p, 4);
}

uint32_t read_hex_dword(uint8_t *p)
{
	return read_hex(p, 8);
}

/* Запись шестнадцатеричного числа с заданным количеством цифр (не более 8) */
void write_hex(uint8_t *p, uint32_t v, int n)
{
	uint8_t b;
	for (; n > 0; n--){
		b = (v >> ((n - 1) * 4)) & 0x0f;
		b += (b < 10) ? 0x30 : 0x37;
		*p++ = b;
	}
	hex_error = false;
}

void write_hex_byte(uint8_t *p, uint8_t b)
{
	write_hex(p, b, 2);
}

void write_hex_word(uint8_t *p, uint16_t w)
{
	write_hex(p, w, 4);
}

void write_hex_dword(uint8_t *p, uint32_t dw)
{
	write_hex(p, dw, 8);
}

#if 0
/* Проверка */
int main()
{
	uint8_t a[14];
	uint8_t b;
	uint16_t w;
	uint32_t dw, i;
	for (i = 0; i < 256; i++){
		b = i;
		w = (i << 8) | i;
		dw = (i << 24) | (i << 16) | (i << 8) | i;
		write_hex_byte(a, b);
		write_hex_word(a + 2, w);
		write_hex_dword(a + 6, dw);
		if (read_hex_byte(a) != b){
			printf("read_hex_byte failed with b = %.2hX\n", b);
			break;
		}
		if (read_hex_word(a + 2) != w){
			printf("read_hex_byte failed with w = %.4hX\n", w);
			break;
		}
		if (read_hex_dword(a + 6) != dw){
			printf("read_hex_byte failed with dw = %.8lX\n", dw);
			break;
		}
	}
	if (i == 256){
		printf("all tests succeded\n");
		return 0;
	}else
		return 1;
}
#endif
