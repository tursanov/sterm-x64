#ifndef USB_CORE_H
#define USB_CORE_H

#include "sysdefs.h"

/* Максимальное число модулей безопасности */
#define NR_USB_KEYS		16

/* Размер данных USB-ключа */
#define USB_KEY_SIZE		65536
/* Размер блока при работе с USB-ключом */
#define USB_BLOCK_LEN		256
/* Размер данных в блоках */
#define USB_KEY_BLOCKS		(USB_KEY_SIZE / USB_BLOCK_LEN)

/* Открытие модуля безопасности с заданным номером */
extern bool usbkey_open(int n);
/* Закрытие модуля безопасности с заданным номером */
extern bool usbkey_close(int n);
/*
 * Чтение данных с USB-ключа. Размер и смещение данных указываются в
 * USB_BLOCK_LEN-байтных блоках.
 */
extern bool usbkey_read(int n, uint8_t *data, int start, int count);
/* Запись данных в модуль безопасности с заданным номером */
extern bool usbkey_write(int n, uint8_t *data);

#endif /* USB_CORE_H */

