/* Реализация алгоритма подсчета контрольной суммы MD5 (RFC-1321) */

#if !defined MD5_H
#define MD5_H

#include "sysdefs.h"

struct md5_hash {
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
};

static inline uint32_t __rol(uint32_t v, uint32_t n)
{
	return (v << n) | (v >> (32 - n));
}

static inline uint32_t F(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & y) | (~x & z);
}

static inline uint32_t G(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & z) | (y & ~z);
}

static inline uint32_t H(uint32_t x, uint32_t y, uint32_t z)
{
	return x ^ y ^ z;
}

static inline uint32_t I(uint32_t x, uint32_t y, uint32_t z)
{
	return y ^ (x | ~z);
}

static inline uint32_t FF(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
	return b + __rol(a + F(b, c, d) + x + ac, s);
}

static inline uint32_t GG(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
	return b + __rol(a + G(b, c, d) + x + ac, s);
}

static inline uint32_t HH(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
	return b + __rol(a + H(b, c, d) + x + ac, s);
}

static inline uint32_t II(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
	return b + __rol(a + I(b, c, d) + x + ac, s);
}

extern bool get_md5(const uint8_t *p, int l, struct md5_hash *md5);
extern bool get_md5_file(const char *name, struct md5_hash *md5);
extern void print_md5(const struct md5_hash *md5);
extern bool cmp_md5(const struct md5_hash *v1, const struct md5_hash *v2);

#define ZERO_MD5_HASH {.a = 0, .b = 0, .c = 0, .d = 0}

#endif		/* MD5_H */
