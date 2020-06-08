/* Преобразование данных по алгоритму base64. (c) gsr 2004 */

#if !defined BASE64_H
#define BASE64_H

#include "sysdefs.h"

extern int base64_get_encoded_len(int l);
extern int base64_encode(const uint8_t *src, int l, uint8_t *dst);
extern int base64_decode(const uint8_t *src, int l, uint8_t *dst);

#endif		/* BASE64_H */
