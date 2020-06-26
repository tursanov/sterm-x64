/* Преобразование данных по алгоритму base64. (c) gsr 2004, 2020 */

#if !defined BASE64_H
#define BASE64_H

#include "sysdefs.h"

extern size_t base64_get_encoded_len(size_t l);
extern ssize_t base64_encode(const uint8_t *src, size_t l, uint8_t *dst);
extern ssize_t base64_decode(const uint8_t *src, size_t l, uint8_t *dst);

#endif		/* BASE64_H */
