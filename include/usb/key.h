/* Работа с ключевым USB-диском. (c) gsr, Alex Popov 2004 */

#if !defined USB_KEY_H
#define USB_KEY_H

#include "md5.h"

/* Сигнатура заголовка USB-ключа */
#define USB_KEY_MAGIC		0x53464b55	/* UKFS */

/* Заголовок USB-ключа */
struct key_header {
	int magic;		/* сигнатура */
	int data_size;		/* размер данных, включая заголовок */
	int n_chunks;		/* число блоков данных */
} __attribute__((__packed__));

/* Заголовок блока данных */
struct key_data_header {
	int len;		/* длина данных вместе с заголовком */
	struct md5_hash crc;	/* контрольная сумма MD5 данных без заголовка */
} __attribute__((__packed__));

/* Чтение модуля безопасности и создание необходимых файлов */
extern bool read_usbkey(void);

/* Чтение файла привязки из модуля безопасности */
extern bool read_usbkey_bind(void);

/* Сигнатура дополнительных данных модуля безопасности */
#define USB_AUX_MAGIC		0x5855413e	/* >AUX */

/* Дополнительная информация модуля безопасности */
struct key_aux_data {
	int magic;		/* сигнатура */
	struct md5_hash sig;	/* уникальная сигнатура */
	uint8_t gaddr;		/* групповой адрес терминала */
	uint8_t iaddr;		/* индивидуальный адрес терминала */
	uint32_t host_ip;	/* ip-адрес хост-ЭВМ */
	uint32_t bank_ip;	/* ip-адрес процессингового центра */
} __attribute__((__packed__));

/* Чтение дополнительной информации из модуля безопасности */
extern bool read_usbkey_aux(struct key_aux_data *aux_data);

#endif		/* USB_KEY_H */
