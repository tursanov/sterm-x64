/* Чтение/запись ключевой информации о терминале. (c) gsr 2004, 2005 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "base64.h"
#include "genfunc.h"
#include "license.h"
#include "licsig.h"
#include "md5.h"
#include "tki.h"

/* Флаги проверки */
/* Файл tki считан и проверен */
bool tki_ok = false;
/* Обнаружен USB-диск для данного терминала */
bool usb_ok = false;
/* Обнаружен dst-файл для этого терминала */
bool iplir_ok = false;
/* Найден правильный файл банковской лицензии */
bool bank_ok = false;

/* Буфер ключевой информации (хранится в зашифрованном виде) */
struct term_key_info tki;
/* Считанный файл привязки USB-диска */
struct md5_hash usb_bind = ZERO_MD5_HASH;
/* Считанный файл привязки ключевых баз */
struct md5_hash iplir_bind = ZERO_MD5_HASH;

/* Вычисленная контрольная сумма ключевой базы VipNet */
struct md5_hash iplir_check_sum = ZERO_MD5_HASH;

/* Считанный файл банковской лицензии */
struct md5_hash bank_license;

/* Шифрование данных */
void encrypt_data(uint8_t *p, int l)
{
	int i;
	if (p == NULL)
		return;
/* Сначала шифруем циклическим xor-ом */
	for (i = 0; i < l; i++)
		p[(i + 1) % l] ^= p[i];
/* После чего меняем местами старшие и младшие полубайты */
	for (i = 0; i < l; i++)
		p[i] = swap_nibbles(p[i]);
}

/* Расшифровка данных */
void decrypt_data(uint8_t *p, int l)
{
	int i;
	if (p == NULL)
		return;
/* Меняем местами старшие и младшие полубайты */
	for (i = 0; i < l; i++)
		p[i] = swap_nibbles(p[i]);
/* Расшифровываем циклический xor */
	for (i = l; i > 0; i--)
		p[i % l] ^= p[i - 1];
}

/* Чтение файла ключевой информации */
bool read_tki(const char *name, bool create)
{
	struct stat st;
	int fd, l;
	if (stat(name, &st) == -1){
		if (create){
			init_tki(&tki);
			return true;
		}else{
			printf("файл ключевой информации не найден\n");
			return false;
		}
	}
	if (st.st_size != sizeof(tki)){
		printf("файл ключевой информации имеет неверный размер\n");
		return false;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		printf("ошибка открытия файла ключевой информации для чтения\n");
		return false;
	}
	memset(&tki, 0, sizeof(tki));
	l = read(fd, &tki, sizeof(tki));
	close(fd);
	if (l != sizeof(tki)){
		printf("ошибка чтения файла ключевой информации\n");
		return false;
	}
	return true;
}

/* Запись файла ключевой информации */
bool write_tki(const char *name)
{
	int fd, l;
	fd = creat(name, 0600);
	if (fd == -1){
		printf("ошибка создания файла ключевой информации\n");
		return false;
	}
	l = write(fd, &tki, sizeof(tki));
	close(fd);
	if (l != sizeof(tki)){
		printf("ошибка записи файла ключевой информации\n");
		return false;
	}
	return true;
}

/* Чтение файла привязки */
bool read_bind_file(const char *name, struct md5_hash *md5)
{
	struct stat st;
	int fd, l;
	if ((name == NULL) || (md5 == NULL))
		return false;
	if (stat(name, &st) == -1)
		return false;
	if (st.st_size != sizeof(*md5))
		return false;
	fd = open(name, O_RDONLY);
	if (fd == -1)
		return false;
	l = read(fd, md5, sizeof(*md5));
	close(fd);
	return l == sizeof(*md5);
}

/* Поверка файла ключевой информации */
void check_tki(void)
{
	struct md5_hash md5, check_sum;
	struct term_key_info __tki = tki;
	tki_ok = false;
	decrypt_data((uint8_t *)&__tki, sizeof(__tki));
	get_md5((uint8_t *)__tki.srv_keys, xsizefrom(__tki, srv_keys), &md5);
	encrypt_data((uint8_t *)&__tki, sizeof(__tki));
	get_tki_field(&__tki, TKI_CHECK_SUM, (uint8_t *)&check_sum);
	tki_ok = cmp_md5(&md5, &check_sum);
}

/* Вычисление контрольной суммы ключевой базы VipNet */
void make_iplir_check_sum(void)
{
	get_md5_file(IPLIR_DST, &iplir_check_sum);
}

/* Проверка файла привязки USB-диска */
void check_usb_bind(void)
{
	term_number tn;
	uint8_t buf[32];
	int l;
	struct md5_hash md5;
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn);
	l = base64_encode((uint8_t *)tn, sizeof(tn), buf);
	encrypt_data(buf, l);
	get_md5(buf, l, &md5);
	usb_ok = cmp_md5(&md5, &usb_bind);
}

/* Проверка файла привязки ключевых баз */
void check_iplir_bind(void)
{
	struct md5_hash md5, buf[2];
	buf[0] = iplir_check_sum;
	buf[1] = usb_bind;
	get_md5((uint8_t *)buf, sizeof(buf), &md5);
	iplir_ok = cmp_md5(&md5, &iplir_bind);
}

/* Проверка лицензии ИПТ */
void check_bank_license(void)
{
	uint8_t buf[2 * TERM_NUMBER_LEN];
	uint8_t v1[64], v2[64];
	struct md5_hash md5;
	int i;
	bank_ok = false;
#if defined __CHECK_LIC_SIGNATURE__
	if (check_lic_signature(BANK_LIC_SIGNATURE_OFFSET, BANK_LIC_SIGNATURE))
		return;		/* лицензия была удалена */
#endif
	get_tki_field(&tki, TKI_NUMBER, buf);
	for (i = 0; i < TERM_NUMBER_LEN; i++)
		buf[sizeof(buf) - i - 1] = ~buf[i];
	i = base64_encode(buf, sizeof(buf), v1);
	i = base64_encode(v1, i, v2);
	get_md5((uint8_t *)v2, i, &md5);
	bank_ok = cmp_md5(&md5, &bank_license);
}

/* Чтение заданного поля из структуры tki */
void get_tki_field(const struct term_key_info *info, int name, uint8_t *val)
{
	decrypt_data((uint8_t *)info, sizeof(*info));
	switch (name){
		case TKI_CHECK_SUM:
			memcpy(val, &info->check_sum, sizeof(info->check_sum));
			break;
		case TKI_SRV_KEYS:
			memcpy(val, info->srv_keys, sizeof(info->srv_keys));
			break;
		case TKI_DBG_KEYS:
			memcpy(val, info->dbg_keys, sizeof(info->dbg_keys));
			break;
		case TKI_NUMBER:
			memcpy(val, info->tn, sizeof(info->tn));
			break;
	}
	encrypt_data((uint8_t *)info, sizeof(*info));
}

/* Установка значения заданного поля структуры tki */
void set_tki_field(struct term_key_info *info, int name, uint8_t *val)
{
	decrypt_data((uint8_t *)info, sizeof(*info));
	switch (name){
		case TKI_CHECK_SUM:
			memcpy(&info->check_sum, val, sizeof(info->check_sum));
			break;
		case TKI_SRV_KEYS:
			memcpy(info->srv_keys, val, sizeof(info->srv_keys));
			break;
		case TKI_DBG_KEYS:
			memcpy(info->dbg_keys, val, sizeof(info->dbg_keys));
			break;
		case TKI_NUMBER:
			memcpy(info->tn, val, sizeof(info->tn));
			break;
	}
	get_md5((uint8_t *)info->srv_keys, xsizefrom(*info, srv_keys),
			&info->check_sum);
	encrypt_data((uint8_t *)info, sizeof(*info));
}

/* Инициализация структуры tki */
void init_tki(struct term_key_info *p)
{
/*
 * Если из-за выравнивания полей структуры между ними возникнут промежутки,
 * то, благодаря memset, они будут заполнены нулями.
 */
	memset(p, 0, sizeof(*p));
	memset(p->srv_keys, 0, sizeof(p->srv_keys));
	memset(p->dbg_keys, 0, sizeof(p->dbg_keys));
	memset(p->tn, 0x30, sizeof(p->tn));
	get_md5((uint8_t *)p->srv_keys, xsizefrom(*p, srv_keys), &p->check_sum);
	encrypt_data((uint8_t *)p, sizeof(*p));
}
