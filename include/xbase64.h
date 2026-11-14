#if !defined XBASE64_H
#define XBASE64_H

#include "sysdefs.h"

__BEGIN_DECLS

extern bool xbase64_encode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len, size_t *encoded_len);
extern bool xbase64_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len, size_t *decoded_len);

__END_DECLS

#endif		/* XBASE64_H */
