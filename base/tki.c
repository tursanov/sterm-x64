/* Чтение/запись ключевой информации о терминале. (c) gsr 2004, 2005, 2020 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
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
#include "paths.h"
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

/* Инициализация структуры tki */
static void init_tki(struct term_key_info *p)
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

/* Чтение файла ключевой информации */
bool read_tki(const char *path, bool create)
{
	bool ret = false;
	struct stat st;
	if (stat(path, &st) == -1){
		if (create){
			init_tki(&tki);
			ret = true;
		}else
			fprintf(stderr, "Файл ключевой информации не найден.\n");
	}else{
		if (st.st_size != sizeof(tki))
			fprintf(stderr, "Файл ключевой информации имеет неверный размер.\n");
		else{
			int fd = open(path, O_RDONLY);
			if (fd == -1)
				fprintf(stderr, "Ошибка открытия файла ключевой информации "
					"для чтения: %m.\n");
			else{
				memset(&tki, 0, sizeof(tki));
				ssize_t l = read(fd, &tki, sizeof(tki));
				if (l == sizeof(tki))
					ret = true;
				else if (l == -1)
					fprintf(stderr, "Ошибка чтения файла ключевой "
						"информации: %m.\n");
				else
					fprintf(stderr, "Из файла ключевой информации "
						"прочитано %zd байт вместо %zd.\n", l, sizeof(tki));
				close(fd);
			}
		}
	}
	return ret;
}

/* Запись файла ключевой информации */
bool write_tki(const char *path)
{
	bool ret = false;
	int fd = creat(path, 0600);
	if (fd == -1)
		fprintf(stderr, "Ошибка создания файла ключевой информации: %m.\n");
	else{
		ssize_t l = write(fd, &tki, sizeof(tki));
		if (l == sizeof(tki))
			ret = true;
		else if (l == -1)
			fprintf(stderr, "Ошибка записи файла ключевой информации: %m.\n");
		else
			fprintf(stderr, "В файл ключевой информации записано "
				"%zd байт вместо %zd.\n", l, sizeof(tki));
		close(fd);
	}
	return ret;
}


/* Чтение заданного поля из структуры tki */
bool get_tki_field(const struct term_key_info *info, int name, uint8_t *val)
{
	bool ret = true;
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
		default:
			ret = false;
	}
	encrypt_data((uint8_t *)info, sizeof(*info));
	return ret;
}

/* Установка значения заданного поля структуры tki */
bool set_tki_field(struct term_key_info *info, int name, const uint8_t *val)
{
	bool ret = true;
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
		default:
			ret = false;
	}
	get_md5((uint8_t *)info->srv_keys, xsizefrom(*info, srv_keys),
			&info->check_sum);
	encrypt_data((uint8_t *)info, sizeof(*info));
	return ret;
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

/* Чтение файла привязки */
static bool read_bind_file(const char *path, struct md5_hash *md5)
{
	bool ret = false;
	if ((path == NULL) || (md5 == NULL))
		return ret;
	struct stat st;
	if (stat(path, &st) == -1)
		fprintf(stderr, "Ошибка получения информации о файле %s: %m.\n", path);
	else if (st.st_size != sizeof(*md5))
		fprintf(stderr, "Файл %s имеет неверный размер.\n", path);
	else{
		int fd = open(path, O_RDONLY);
		if (fd == -1)
			fprintf(stderr, "Ошибка открытия файла %s: %m.\n", path);
		else{
			ssize_t l = read(fd, md5, sizeof(*md5));
			if (l == sizeof(*md5))
				ret = true;
			else if (l == -1)
				fprintf(stderr, "Ошибка чтения из файла %s: %m.\n", path);
			else
				fprintf(stderr, "Из файла %s считано %zd байт вместо %zd.\n",
					path, l, sizeof(*md5));
			close(fd);
		}
	}
	return ret;
}

/* Чтение заводского номера с модуля безопасности */
static bool read_term_nr(term_number tn)
{
	bool ret = false;
	FILE *f = fopen(TERM_NR_FILE, "r");
	if (f == NULL)
		fprintf(stderr, "Ошибка открытия " TERM_NR_FILE " для чтения: %m.\n");
	else{
		char nr[TERM_NUMBER_LEN + 2];	/* \n + 0 */
		if (fgets(nr, sizeof(nr), f) == NULL)
			fprintf(stderr, "Ошибка чтения из " TERM_NR_FILE ": %m.\n");
		else{
			size_t len = strlen(nr);
			ret = (len == (TERM_NUMBER_LEN + 1)) &&
				(strncmp(nr, "F00137001", 9) == 0) &&
				((nr[len - 1] == '\n') || (nr[len - 1] == '\r'));
			if (ret){
				for (int i = 9; i < 13; i++){
					if (!isdigit(nr[i])){
						ret = false;
						break;
					}
				}
			}
			if (ret)
				memcpy(tn, nr, TERM_NUMBER_LEN);
			else
				fprintf(stderr, "Неверный формат заводского номера терминала.\n");
		}
		if (fclose(f) == EOF)
			fprintf(stderr, "Ошибка закрытия " TERM_NR_FILE ": %m.\n");
	}
	return ret;
}

/* Проверка файла привязки USB-диска */
void check_usb_bind(void)
{
	usb_ok = false;
	term_number tn1, tn2;
	if (read_term_nr(tn1) && get_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn2))
		usb_ok = memcmp(tn1, tn2, TERM_NUMBER_LEN) == 0;
}

/* Проверка файла привязки ключевых баз */
void check_iplir_bind(void)
{
	iplir_ok = false;
	struct md5_hash iplir_bind;
	if (read_bind_file(IPLIR_BIND_FILE, &iplir_bind)){
		struct md5_hash buf[2];
		get_md5_file(IPLIR_DST, buf);
		term_number tn;
		if (get_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn)){
			get_md5((uint8_t *)tn, sizeof(tn), buf + 1);
			struct md5_hash md5;
			get_md5((uint8_t *)buf, sizeof(buf), &md5);
			iplir_ok = cmp_md5(&md5, &iplir_bind);
		}
	}
}

/* Проверка лицензии ИПТ */
void check_bank_license(void)
{
	bank_ok = false;
#if defined __CHECK_LIC_SIGNATURE__
	if (check_lic_signature(BANK_LIC_SIGNATURE_OFFSET, BANK_LIC_SIGNATURE))
		return;		/* лицензия была удалена */
#endif
	struct md5_hash bnk_lic;
	if (read_bind_file(BANK_LICENSE, &bnk_lic)){
		uint8_t buf[2 * TERM_NUMBER_LEN];
		get_tki_field(&tki, TKI_NUMBER, buf);
		for (int i = 0; i < TERM_NUMBER_LEN; i++)
			buf[sizeof(buf) - i - 1] = ~buf[i];
		uint8_t v1[64], v2[64];
		size_t len = base64_encode(buf, sizeof(buf), v1);
		len = base64_encode(v1, len, v2);
		struct md5_hash md5;
		get_md5((uint8_t *)v2, len, &md5);
		bank_ok = cmp_md5(&md5, &bnk_lic);
	}
}
