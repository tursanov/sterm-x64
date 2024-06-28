/* Чтение/запись десятичных и шестнадцатеричных чисел. (c) gsr 2004, 2024 */

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
bool number_error = false;

/* Чтение шестнадцатеричного числа с заданным количеством цифр (не более 8) */
uint32_t read_hex(uint8_t *p, int n)
{
	uint8_t b;
	uint32_t v = 0;
	number_error = true;
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
	number_error = false;
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
	number_error = false;
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

/* Чтение десятичного числа без знака переменной длины */
bool read_var_uint(const uint8_t *data, size_t *len, size_t max_len, uint32_t *val)
{
	number_error = true;
	if ((data != NULL) && (len != NULL) && (max_len > 0) && (val != NULL)){
		*len = 0;
		*val = 0;
		for (; *len < max_len; (*len)++){
			uint8_t b = data[*len];
			if ((b >= 0x30) && (b <= 0x39))
				b -= 0x30;
			else
				break;
			*val *= 10;
			*val += b;
		}
		if (*len > 0)
			number_error = false;
	}

	return !number_error;
}

/* Чтение десятичного числа без знака заданной длины */
bool read_uint(const uint8_t *data, size_t len, uint32_t *val)
{
	size_t rd_len = 0;
	return read_var_uint(data, &rd_len, len, val) && (rd_len == len);
}

/* Запись десятичного числа без знака */
bool write_uint(uint8_t *data, uint32_t val, size_t len)
{
	number_error = true;
	if ((data != NULL) && (len > 0)){
		for (size_t i = len; i > 0; i--){
			data[i - 1] = 0x30;
			if (val > 0){
				data[i - 1] += val % 10;
				val /= 10;
			}
		}
		number_error = false;
	}
	return !number_error;
}

/* Чтение десятичного числа со знаком переменной длины */
bool read_var_int(const uint8_t *data, size_t *len, size_t max_len, int32_t *val)
{
	bool ret = false;
	number_error = true;
	if ((data != NULL) && (len != NULL) && (max_len > 0) && (val != NULL)){
		*len = 0;
		*val = 0;
		int sign = 1;
		if (data[0] == '-')
			sign = -1;
		uint32_t tmp = 0;
		if (sign > 0)
			ret = read_var_uint(data, len, max_len, &tmp);
		else{
			ret = read_var_uint(data + 1, len, max_len - 1, &tmp);
			if (ret)
				(*len)++;
		}
		if (ret)
			*val = sign * tmp;
	}
	return !number_error;
}

/* Чтение десятичного числа со знаком заданной длины */
bool read_int(const uint8_t *data, size_t len, int32_t *val)
{
	size_t rd_len = 0;
	if (read_var_int(data, &rd_len, len, val)){
		if (rd_len != len)
			number_error = true;
	}
	return !number_error;
}

/* Запись десятичного числа со знаком */
bool write_int(uint8_t *data, int32_t val, size_t len)
{
	number_error = true;
	if ((data != NULL) && (len > 0)){
		if (val < 0){
			*data++ = '-';
			val *= -1;
		}
		write_uint(data, val, len);
	}
	return !number_error;
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
