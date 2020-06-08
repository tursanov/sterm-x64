/* Преобразование данных по алгоритму base64. (c) gsr 2004 */

#include <stdio.h>
#include "base64.h"

/* Алфавит base64 */
static char tbl_base64[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

static int int_from_base64(int c)
{
	if (c == '=')
		return 0;
	for (size_t i = 0; i < sizeof(tbl_base64); i++){
		if (c == tbl_base64[i])
			return i;
	}
	return -1;
}

/* Определение длины закодированного сообщения */
int base64_get_encoded_len(int l)
{
	return ((l - 1) / 3 + 1) * 4;
}	

/* Кодирование base64 */
int base64_encode(const uint8_t *src, int l, uint8_t *dst)
{
	int i, j, k, m;
	uint8_t b[3];
	if (l <= 0)
		return 0;
	for (i = m = 0; i <= (l - 1) / 3; i++, m += 4){
		for (j = 0; j < 3; j++){
			k = 3 * i + j;
			b[j] = (k < l) ? src[k] : 0;
		}
		dst[m] = tbl_base64[b[0] >> 2];
		dst[m + 1] = tbl_base64[((b[0] & 3) << 4) | (b[1] >> 4)];
		if ((3 * i + 1) < l)
			dst[m + 2] = tbl_base64[((b[1] & 0x0f) << 2) | (b[2] >> 6)];
		else
			dst[m + 2] = '=';
		if ((3 * i + 2) < l)
			dst[m + 3] = tbl_base64[b[2] & 0x3f];
		else
			dst[m + 3] = '=';
	}
	return m;
}

/* Раскодирование base64 */
int base64_decode(const uint8_t *src, int l, uint8_t *dst)
{
	int i, j, k, b[4];
	if ((src == NULL) || (dst == NULL) || (l % 4))
		return -1;
	for (i = j = 0; i < l; i += 4){
		for (k = 0; k < 4; k++){
			b[k] = int_from_base64(src[i + k]);
			if (b[k] == -1)
				return -1;
		}
		dst[j++] = (b[0] << 2) | (b[1] >> 4);
		if (src[i + 2] == '=')
			break;
		dst[j++] = (b[1] << 4) | (b[2] >> 2);
		if (src[i + 3] == '=')
			break;
		dst[j++] = (b[2] << 6) | b[3];
	}
	return j;
}

#if 0
int main()
{
	char src[] = "Привет, мир";
	char dst[32];
	int l = base64_encode(src, strlen(src), dst);
	printf("pass 1: %.*s\n", l, dst);
	l = base64_decode(dst, l, dst);
	printf("pass 2: %.*s\n", l, dst);
	return 0;
}
#endif
