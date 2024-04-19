#if !defined XBASE64_H
#define XBASE64_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

extern bool xbase64_encode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len, size_t *encoded_len);
extern bool xbase64_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len, size_t *decoded_len);

#if defined __cplusplus
}
#endif

#endif		/* XBASE64_H */
