/* Программа работы с файлом ключевой информации терминала. (c) gsr 2004, 2008, 2020.
 * Параметры командной строки:
 * --tki=<file>		-- файл ключевой информации для записи;
 * --number		-- чтение заводского номера терминала из модуля
 *			   безопасности и запись его в файл ключевой информации;
 * --dallas-keys	-- запись в файл ключевой информации номеров жетонов
 *			   установки параметров;
 * --repeated		-- возможность ввода повторяющихся ключей DS1990A.
 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "base64.h"
#include "ds1990a.h"
#include "genfunc.h"
#include "md5.h"
#include "tki.h"

static char *tki_name;		/* имя файла ключевой информации */
static bool can_repeat_keys;	/* флаг возможности ввода дублирующихся ключей */

/* Об'ект действия */
enum {
	subj_none,
	subj_number,		/* заводской номер */
	subj_keys,		/* настроечные ключи DS1990A */
} subj = subj_none;

/* Вывод справочной информации */
static void usage(void)
{
	printf(
"Программа работы с файлом ключевой информации терминала.\n"
"(c) АО НПЦ \"Спектр\" 2001 -- 2020.\n"
"Использование: mwl [options]:\n"
"  --tki=<file>\t\t-- файл ключевой информации для записи;\n"
"  --number\t\t-- чтение заводского номера терминала из модуля\n"
"\t\t\t   безопасности и запись его в файл ключевой информации;\n"
"  --dallas-keys\t-- запись в файл ключевой информации номеров жетонов\n"
"\t\t\t   установки параметров;\n"
"  --repeated\t\t-- возможность ввода повторяющихся ключей DS1990A.\n"
	);
}

/* Проверка нулевого номера DS1990A */
static bool ds_zero_id(ds_number dsn)
{
	int i;
	for (i = 0; (i < DS_NUMBER_LEN) && (dsn[i] == 0); i++){}
	return i == DS_NUMBER_LEN;
}

/* Запись номеров ключей */
static bool write_dallas(struct md5_hash *keys, int n_keys, bool can_repeat)
{
	int i, j;
	ds_number dsn;
	struct md5_hash md5;
	bool rep;
	if (keys == NULL)
		return false;
	memset(keys, 0, n_keys * sizeof(*keys));
	if (!ds_init()){
		fprintf(stderr, "Ошибка открытия драйвера DALLAS.\n");
		return false;
	}
	for (i = 0; i < n_keys; i++){
		printf("ключ #%d: ", i + 1);
		getchar();
		do
			ds_read(dsn);
		while (ds_zero_id(dsn));
		ds_hash(dsn, &md5);
		rep = false;
		if (!can_repeat){	/* проверка повторяющихся ключей */
			for (j = 0; !rep && (j < i); j++){
				if (cmp_md5(&md5, keys + j))
					rep = true;
			}
		}
		if (rep){
			i--;
			printf("повтор\n");
		}else{
			memcpy(keys + i, &md5, sizeof(md5));
			printf("ok\n");
		}
	}
	return true;
}

/*
 * Чтение заводского номера терминала из модуля безопасности и запись его
 * в структуру tki.
 */
static bool read_term_number(void)
{
	if (system("mount-usb.sh") != 0)
		return false;
	bool ret = false;
	FILE *f = fopen(TERM_NR_FILE_USB, "r");
	if (f == NULL)
		fprintf(stderr, "Ошибка открытия " TERM_NR_FILE_USB " для чтения: %m.\n");
	else{
		char nr[TERM_NUMBER_LEN + 2];	/* \n + 0 */
		if (fgets(nr, sizeof(nr), f) == NULL)
			fprintf(stderr, "Ошибка чтения из " TERM_NR_FILE_USB ": %m.\n");
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
				ret = set_tki_field(&tki, TKI_NUMBER, (uint8_t *)nr);
			else
				fprintf(stderr, "Заводской номер терминала имеет неверный формат "
					"(не F00137001xxxx).\n");
		}
		if (fclose(f) == EOF)
			fprintf(stderr, "Ошибка закрытия " TERM_NR_FILE_USB ": %m.\n");
	}
	system("umount /mnt/usb");
	return ret;
}

/* Семантическая проверка командной строки */
static bool check_semantic(void)
{
	bool ret = false;
	if (tki_name == NULL)
		fprintf(stderr, "Не указан файл ключевой информации (--tki).\n");
	else if (subj == subj_none)
		fprintf(stderr, "Не указан объект действия (--number или --dallas-keys).\n");
	else if ((can_repeat_keys) && (subj != subj_keys))
		fprintf(stderr, "--repeated не имеет смысла без --dallas-keys.\n");
	else
		ret = true;
	return ret;
}

/* Разбор командной строки */
static bool parse_cmd_line(int argc, char **argv)
{
	bool set_subj(int _subj)	/* установка объекта действия */
	{
		if (subj == subj_none){
			subj = _subj;
			return true;
		}else{
			fprintf(stderr, "Повторное указание объекта действия.\n");
			return false;
		}
	}
	extern char *optarg;
	extern int optind;
	const struct option longopts[] = {
		{"tki",		required_argument,	NULL, 't'},
		{"number",	no_argument,		NULL, 'n'},
		{"dallas-keys",	no_argument,		NULL, 'k'},
		{"repeated",	no_argument,		NULL, 'r'},
		{NULL,		0,			NULL, 0},
	};
	char *shortopts = "t:nkr";
	bool loop_flag = true, ret_val = true;
	while (loop_flag && ret_val){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 't':		/* --tki */
				if (tki_name != NULL){
					fprintf(stderr, "Повторное указание файла ключевой информации.\n");
					ret_val = false;
				}else
					tki_name = optarg;
				break;
			case 'n':		/* --number */
				ret_val = set_subj(subj_number);
				break;
			case 'k':		/* --dallas-keys */
				ret_val = set_subj(subj_keys);
				break;
			case 'r':		/* --repeated */
				if (can_repeat_keys)
					fprintf(stderr, "Повторное указание флага повторения ключей.\n");
				can_repeat_keys = true;
				break;
			case EOF:		/* конец командной строки */
				loop_flag = false;
				if (optind < argc){
					fprintf(stderr, "Лишние данные в командной строке\n");
					ret_val = false;
				}
				break;
			default:		/* ошибка */
				ret_val = false;
				break;
		}
		loop_flag = loop_flag && ret_val;
	}
	return ret_val && check_semantic();
}

int main(int argc, char **argv)
{
	int ret = 1;
	struct md5_hash keys[NR_SRV_KEYS];
	if (argc == 1)
		usage();
	else if (parse_cmd_line(argc, argv) && read_tki(tki_name, true)){
		check_tki();
		if (!tki_ok)
			fprintf(stderr, "Файл ключевой информации поврежден.\n");
		else{
			switch (subj){
				case subj_number:
					if (read_term_number())
						ret = 0;
					break;
				case subj_keys:
					if (write_dallas(keys, NR_SRV_KEYS,
							can_repeat_keys)){
						set_tki_field(&tki, TKI_SRV_KEYS,
							(uint8_t *)keys);
						ret = 0;
					}
					break;
			}
			if ((ret == 0) && !write_tki(tki_name))
				ret = 1;
		}
	}
	return ret;
}
