#pragma once

#include "sysdefs.h"

extern bool xbase64Encode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len, size_t *encoded_len);
extern bool xbase64Decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len, size_t *decoded_len);
