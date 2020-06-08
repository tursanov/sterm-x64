/*
 * Создание файлов привязки USB-диска и VipNet. (c) gsr 2004, 2005.
 * Параметры запуска:
 * --term-number=<number>	-- заводской номер терминала;
 * --usb-bind=<name>		-- имя файла привязки USB-диска;
 * --iplir=<name>		-- имя файла ключевого дистрибутива VipNet;
 * --iplir-bind=<name>		-- имя файла привязки ключевого дистрибутива;
 * --iplir-psw=<password>	-- пароль ключевого дистрибутива VipNet;
 * --iplir-psw-file=<name>	-- имя файла пароля ключевого дистрибутива;
 * --bank-license=<name>	-- имя файла банковской лицензии.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "base64.h"
#include "md5.h"
#include "tki.h"

/*
 * Размер файла ключевой информации о терминале.
 * FIXME: при изменении структуры term_key_info необходимо изменить этот размер.
 */
#define TKI_SIZE	80
/* Заводской номер терминала */
static char *number;
/* Имя файла привязки USB-диска */
static char *usb_bind_name;
/* Имя файла ключевого дистрибутива VipNet */
static char *iplir_name;
/* Имя файла привязки ключевого дистрибутива */
static char *iplir_bind_name;
/* Пароль ключевого дистрибутива VipNet */
static char *iplir_psw;
/* Имя файла пароля ключевого дистрибутива VipNet */
static char *iplir_psw_name;
/* Имя файла банковской лицензии */
static char *bank_license_name;

/* Вывод справки по использованию программы */
static void show_usage(void)
{
	static char *help[] = {
		"Программа создания файлов привязки терминала. (c) gsr 2004",
		"Использование: mkbind <options>",
		"--term-number=<number>\t-- заводской номер терминала;",
		"--usb-bind=<name>\t-- имя файла привязки USB-диска;",
		"--iplir=<name>	\t-- имя файла ключевого дистрибутива VipNet;",
		"--iplir-bind=<name>\t-- имя файла привязки ключевого дистрибутива;",
		"--iplir-psw=<password>\t-- пароль ключевого дистрибутива VipNet;",
		"--iplir-psw-file=<name>\t-- имя файла пароля ключевого дистрибутива;",
		"--bank-license=<name>\t-- имя файла банковской лицензии.",
	};
	int i;
	for (i = 0; i < ASIZE(help); i++)
		printf("%s\n", help[i]);
}

/* Чтение файла привязки USB-диска */
static bool read_usb_bind(char *name, struct md5_hash *md5)
{
	struct stat st;
	int l, fd;
	if (stat(name, &st) == -1){
		printf("файл привязки USB-диска %s не найден\n", name);
		return false;
	}
	if (st.st_size != sizeof(*md5)){
		printf("файл привязки USB-диска имеет неверный размер\n");
		return false;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		printf("ошибка открытия %s для чтения\n", name);
		return false;
	}
	l = read(fd, md5, sizeof(*md5));
	close(fd);
	if (l != sizeof(*md5)){
		printf("ошибка чтения из %s\n", name);
		return false;
	}else
		return true;
}

/* Запись контрольной суммы для заданного массива */
static bool write_md5(char *name, uint8_t *buf, int l)
{
	struct md5_hash md5 = ZERO_MD5_HASH;
	int fd;
	if ((name == NULL) || (buf == NULL))
		return false;
	fd = creat(name, 0600);
	if (fd == -1){
		printf("ошибка создания %s\n", name);
		return false;
	}
	get_md5(buf, l, &md5);
	if (write(fd, &md5, sizeof(md5)) != sizeof(md5)){
		printf("ошибка записи в %s\n", name);
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

/* Создание файла привязки USB-диска */
static bool create_usb_bind(char *number, char *bind_name)
{
	uint8_t buf[32];
	int l;
	if ((number == NULL) || (bind_name == NULL))
		return false;
	l = strlen(number);
	if (l != 13){
		printf("неверный размер заводского номера терминала\n");
		return false;
	}
	l = base64_encode((uint8_t *)number, l, buf);
	encrypt_data(buf, l);
	return write_md5(bind_name, buf, l);
}

/* Создание файла привязки ключевых баз */
static bool create_iplir_bind(char *usb_bind_name,
		char *iplir_name, char *bind_name)
{
	struct md5_hash buf[2];
	if (!get_md5_file(iplir_name, buf)){
		printf("ошибка определения контрольной суммы %s\n", iplir_name);
		return false;
	}
	return	read_usb_bind(usb_bind_name, buf + 1) &&
		write_md5(bind_name, (uint8_t *)buf, sizeof(buf));
}

/* Запись файла пароля ключевого дистрибутива VipNet */
static bool write_iplir_psw(char *psw, char *psw_file)
{
	static uint8_t psw_buf[128], _psw_buf[128];
	int l, ll, fd;
	if ((psw == NULL) || (psw_file == NULL))
		return false;
	l = strlen(psw);
	if (base64_get_encoded_len(base64_get_encoded_len(l)) > sizeof(psw_buf)){
		printf("слишком длинный пароль\n");
		return false;
	}
	l = base64_encode((uint8_t *)psw, l, _psw_buf);
	l = base64_encode(_psw_buf, l, psw_buf);
	fd = creat(psw_file, 0600);
	if (fd == -1){
		printf("ошибка создания файла %s\n", psw_file);
		return false;
	}
	ll = write(fd, psw_buf, l);
	close(fd);
	if (ll != l){
		printf("ошибка записи в %s\n", psw_file);
		return false;
	}else
		return true;
}

/* Создание файла банковской лицензии */
static bool create_bank_license(char *number, char *license_name)
{
	uint8_t buf[2 * TERM_NUMBER_LEN];
	uint8_t v1[64], v2[64];
	int i;
	memcpy(buf, number, TERM_NUMBER_LEN);
	for (i = 0; i < TERM_NUMBER_LEN; i++)
		buf[sizeof(buf) - i - 1] = ~buf[i];
	i = base64_encode(buf, sizeof(buf), v1);
	i = base64_encode(v1, i, v2);
	return write_md5(license_name, v2, i);
}

/* Разбор командной строки */
bool parse_cmd_line(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	const struct option longopts[] = {
		{"term-number",		required_argument,	NULL, 'n'},
		{"usb-bind",		required_argument,	NULL, 'u'},
		{"iplir",		required_argument,	NULL, 'd'},
		{"iplir-bind",		required_argument,	NULL, 'i'},
		{"iplir-psw",		required_argument,	NULL, 'p'},
		{"iplir-psw-file",	required_argument,	NULL, 'f'},
		{"bank-license",	required_argument,	NULL, 'b'},
		{NULL,			0,			NULL, 0},
	};
	char *shortopts = "n:u:d:i:p:f:b:";
	bool loop_flag = true, err = false;
	while (loop_flag && !err){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'n':
				if (number == NULL)
					number = optarg;
				else{
					printf("повторное указание заводского номера терминала\n");
					err = true;
				}
				break;
			case 'u':
				if (usb_bind_name == NULL)
					usb_bind_name = optarg;
				else{
					printf("повторное указание файла привязки USB-диска\n");
					err = true;
				}
				break;
			case 'd':
				if (iplir_name == NULL)
					iplir_name = optarg;
				else{
					printf("повторное указание файла ключевого дистрибутива\n");
					err = true;
				}
				break;
			case 'i':
				if (iplir_bind_name == NULL)
					iplir_bind_name = optarg;
				else{
					printf("повторное указание файла привязки ключевого дистрибутива\n");
					err = true;
				}
				break;
			case 'p':
				if (iplir_psw == NULL)
					iplir_psw = optarg;
				else{
					printf("повторное указание пароля ключевого дистрибутива\n");
					err = true;
				}
				break;
			case 'f':
				if (iplir_psw_name == NULL)
					iplir_psw_name = optarg;
				else{
					printf("повторное укзание файла пароля ключевого дистрибутива\n");
					err = true;
				}
				break;
			case 'b':
				if (bank_license_name == NULL)
					bank_license_name = optarg;
				else{
					printf("повторное указание имени файла банковской лицензии\n");
					err = true;
				}
				break;
			case EOF:		/* конец командной строки */
				loop_flag = false;
				if (optind < argc){
					printf("лишне данные в командной строке: %d/%d\n",
						optind, argc);
					err = true;
				}
				break;
			default:		/* ошибка */
				err = true;
				break;
		}
	}
	if (err)
		return false;
	if ((number == NULL) && (usb_bind_name == NULL) &&
			(iplir_name == NULL) && (iplir_bind_name == NULL) &&
			(iplir_psw == NULL) && (iplir_psw_name == NULL)){
		show_usage();
		return false;
	}
	if (iplir_name == NULL){
		if (iplir_bind_name != NULL){
			printf("не указан файл ключевого дистрибутива VipNet (--iplir)\n");
			return false;
		}
	}else if (iplir_bind_name == NULL){
		printf("не указан файл привязки ключевого дистрибутива VipNet (--iplir-bind)\n");
		return false;
	}else if (usb_bind_name == NULL){
		printf("необходимо указать файл привязки USB-диска (--usb-bind)\n");
		return false;
	}
	if ((iplir_psw == NULL) ^ (iplir_psw_name == NULL)){
		printf("используйте --iplir-psw и --iplir-psw-file одновременно\n");
		return false;
	}
	if ((bank_license_name != NULL) && (number == NULL)){
		printf("используйте --bank-license и --term-number одновременно\n");
		return false;
	}
	return true;
}

/* Выполнение действий, указанных в командной строке */
static bool do_actions(void)
{
	if ((usb_bind_name != NULL) && (number != NULL)){
		if (!create_usb_bind(number, usb_bind_name))
			return false;
	}
	if (iplir_bind_name != NULL){
		if (!create_iplir_bind(usb_bind_name, iplir_name, iplir_bind_name))
			return false;
	}
	if (iplir_psw != NULL){		/* iplir_psw_name проверяется выше */
		if (!write_iplir_psw(iplir_psw, iplir_psw_name))
			return false;
	}
	if (bank_license_name != NULL){
		if (!create_bank_license(number, bank_license_name))
			return false;
	}
	return true;
}

int main(int argc, char **argv)
{
	if (!parse_cmd_line(argc, argv))
		return 1;
	return do_actions() ? 0 : 1;
}
