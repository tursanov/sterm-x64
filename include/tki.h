/* Ключевая информация терминала. (c) gsr 2001, 2004 */

#if !defined TKI_H
#define TKI_H

#include "ds1990a.h"
#include "gd.h"
#include "md5.h"

/* Имя файла привязки USB-диска */
#define USB_BIND_FILE		"/sdata/disk.dat"
/* Имя файла ключевого дистрибутива VipNet */
#define IPLIR_DST		"/sdata/iplir.dst"
/* Имя файла привязки ключевого дистрибутива VipNet */
#define IPLIR_BIND_FILE		"/sdata/iplir.dat"
/* Имя файла пароля ключевых баз VipNet */
#define IPLIR_PSW_DATA		"/sdata/iplirpsw.dat"

/* Информация о терминале (не изменяется при обновлении терминала */
struct term_key_info{
	struct md5_hash check_sum;	/* контрольная сумма остальных данных */
	struct md5_hash srv_keys[NR_SRV_KEYS];	/* настроечные ключи */
	struct md5_hash dbg_keys[NR_DBG_KEYS];	/* отладочные ключи */
	term_number tn;				/* заводской номер терминала */
};

/* Идентификаторы полей в tki */
enum {
	TKI_CHECK_SUM,
	TKI_SRV_KEYS,
	TKI_DBG_KEYS,
	TKI_NUMBER,
};

/* Буфер ключевой информации (хранится в зашифрованном виде) */
extern struct term_key_info tki;
/* Считанный файл привязки USB-диска */
extern struct md5_hash usb_bind;
/* Считанный файл привязки ключевых баз */
extern struct md5_hash iplir_bind;
/* Вычисленная контрольная сумма ключевой базы VipNet */
extern struct md5_hash iplir_check_sum;
/* Считанный файл банковской лицензии */
extern struct md5_hash bank_license;

/* Флаги проверки */
extern bool tki_ok;	/* файл .tki считан и проверен */
extern bool usb_ok;	/* обнаружен USB-диск для данного терминала */
extern bool iplir_ok;	/* обнаружен dst-файл для этого терминала */
extern bool bank_ok;	/* найден правильный файл банковской лицензии */

/* Перестановка старшего и младшего полубайтов */
static inline uint8_t swap_nibbles(uint8_t b)
{
	return (b << 4) | (b >> 4);
}

extern void encrypt_data(uint8_t *p, int l);
extern void decrypt_data(uint8_t *p, int l);

extern bool read_tki(const char *name, bool create);
extern bool write_tki(const char *name);
extern bool read_bind_file(const char *name, struct md5_hash *md5);
extern void check_tki(void);
extern void make_iplir_check_sum(void);
extern void check_usb_bind(void);
extern void check_iplir_bind(void);
extern void check_bank_license(void);
extern void get_tki_field(const struct term_key_info *info, int name, uint8_t *val);
extern void set_tki_field(struct term_key_info *info, int name, uint8_t *val);
extern void init_tki(struct term_key_info *p);

#endif		/* TKI_H */
