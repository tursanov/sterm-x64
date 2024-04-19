#include "xbase64.h"

bool xbase64_encode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len, size_t *encoded_len)
{
	if ((src == NULL) || (src_len == 0) || (dst == NULL) || (dst_len < (((src_len + 2) / 3) * 4)))
		return false;
	*encoded_len = 0;
	uint8_t in[3], out[4];
	for (size_t i = 0, j = 0; i < src_len;){
		int n = 2, k;
		for (k = 0; (k < 3) && (i < src_len); k++, i++)
			in[k] = src[i];
		out[0] = in[0] >> 2;
		out[1] = (in[0] & 0x03) << 4;
		if (k > 1){
			out[1] |= in[1] >> 4;
			out[2] = (in[1] & 0x0f) << 2;
			n++;
			if (k > 2){
				out[2] |= in[2] >> 6;
				out[3] = in[2] & 0x3f;
				n++;
			}
		}
		for (k = 0; k < n; k++)
			dst[j++] = out[k] + 0x20;
		*encoded_len = j;
	};
	return true;
}

bool xbase64_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len, size_t *decoded_len)
{
	if ((src == NULL) || (src_len == 0) || (dst == NULL) || (dst_len == 0))
		return false;
	*decoded_len = 0;
	uint8_t in[4], out[3];
	for (size_t i = 0, j = 0; (i < src_len) && (j < dst_len);){
		int n = 0, k;
		for (k = 0; (k < 4) && (i < src_len); k++, i++)
			in[k] = src[i] - 0x20;
		if (k > 1){
			out[0] = (in[0] << 2) | ((in[1] >> 4) & 0x03);
			n++;
			if (k > 2){
				out[1] = (in[1] << 4) | ((in[2] >> 2) & 0x0f);
				n++;
				if (k > 3){
					out[2] = (in[2] << 6) | (in[3] & 0x3f);
					n++;
				}
			}
		}
		for (int m = 0; (m < n) && (j < dst_len); m++)
			dst[j++] = out[m];
		*decoded_len = j;
	}
	return true;
}
