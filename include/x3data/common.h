/* Общие определения для всех файлов. (c) gsr, 2024 */

#if !defined X3DATA_COMMON_H
#define X3DATA_COMMON_H

#include "sysdefs.h"

#if defined __cplusplus
extern "C" {
#endif

typedef void (*x3_sync_callback_t)(bool done, const char *message);

/* Заголовок изображения в БПУ/ККТ */
struct pic_header {
	uint8_t hdr_len;	/* длина заголовка */
	uint8_t w;		/* ширина изображения в мм */
	uint8_t h;		/* высота изображения в мм */
	uint8_t id;		/* идентификатор "Экспресс" изображения */
	uint8_t name[SPRN_MAX_PIC_NAME_LEN];
	uint32_t data_len;	/* длина данных */
} __attribute__((__packed__));

/* Вычисление контрольной суммы CRC32 x^30 + x^27 + x^18 + x^3 + x^1 блока данных */
extern uint32_t pic_crc32(const uint8_t *data, size_t len);

#if defined __cplusplus
}
#endif

#endif		/* X3DATA_COMMON_H */
