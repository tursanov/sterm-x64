/* Программа работы с файлом ключевой информации терминала. (c) gsr 2004
 * Параметры командной строки:
 * --read-tki=<file> -- чтение файла ключевой информации;
 * --write-tki=<file> -- запись файла ключевой информации;
 * --srv-keys[=<file>] -- настроечные ключи DS1990A;
 * --dbg-keys[=<file>] -- отладочные ключи DS1990A;
 * --repeated -- возможность ввода повторяющихся ключей DS1990A;
 * --number -- заводской номер терминала;
 * --decrypt -- вывод в stdout расшифрованного файла ключевой информации;
 * --all -- вывод содержимого файла ключевой информации;
 * --verify -- проверка файла tki на корректность значений (не реализовано);
 * --help -- вывод справочной информации.
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
#include "ds1990a.h"
#include "genfunc.h"
#include "tki.h"

static char *tki_name;		/* имя файла ключевой информации */
static char *key_file;		/* имя файла ключей DALLAS */
static bool tki_read = true;	/* флаг чтения/записи файла tki */
static bool can_repeat_keys = false;	/* флаг возможности ввода дублирующихся ключей */

/* Об'ект действия */
enum {
	subj_none,
	subj_srv_keys,		/* настроечные ключи DS1990A */
	subj_dbg_keys,		/* отладочные ключи DS1990A */
	subj_number,		/* заводской номер */
	subj_decrypt,		/* расшифровка файла */
	subj_all,		/* вывести содержимое файла tki */
} subj = subj_none;
#define DESCR_WIDTH	20	/* размер поля названия параметра */

/* Вывод справочной информации */
static void usage(void)
{
	static char *help[] = {
		"Программа работы с файлом ключевой информации терминала",
		"(c) gsr 2001 -- 2004.",
		"Использование: mw [options]:",
		"--read-tki=<file> -- чтение файла ключевой информации;",
		"--write-tki=<file> -- запись файла ключевой информации;",
		"--srv-keys[=<file>] -- настроечные ключи DS1990A;",
		"--dbg-keys[=<file>] -- отладочные ключи DS1990A;",
		"--repeated -- возможность ввода повторяющихся ключей DS1990A;",
		"--number -- заводской номер терминала;",
		"--decrypt -- вывод в stdout расшифрованного файла ключевой информации;",
		"--all -- вывод содержимого файла ключевой информации;",
		"--help -- вывод справочной информации.",
	};
	int i;
	for (i = 0; i < ASIZE(help); i++)
		printf("%s\n", help[i]);
}

/* Вывод номера ключа DALLAS */
static void print_dallas_key(struct md5_hash *md5, int ds_type)
{
	struct md5_hash zero_md5 = ZERO_MD5_HASH;
	if (md5 == NULL)
		return;
	printf("%*c:", DESCR_WIDTH, ds_key_char(ds_type));
	if (memcmp(md5, &zero_md5, sizeof(*md5)) == 0)
		printf(" не введен");
	else
		print_md5(md5);
	printf("\n");
}

/* Вывод номеров ключей DALLAS */
static void print_dallas(void)
{
	struct md5_hash srv_keys[NR_SRV_KEYS];
	struct md5_hash dbg_keys[NR_DBG_KEYS];
	int i;
	get_tki_field(&tki, TKI_SRV_KEYS, (uint8_t *)srv_keys);
	for (i = 0; i < NR_SRV_KEYS; i++)
		print_dallas_key(srv_keys + i, key_srv);
	get_tki_field(&tki, TKI_DBG_KEYS, (uint8_t *)dbg_keys);
	for (i = 0; i < NR_DBG_KEYS; i++)
		print_dallas_key(dbg_keys + i, key_dbg);
}

/* Вывод заводского номера */
static void print_term_number(void)
{
	int i, c;
	term_number tn;
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn);
	printf("%*s: ", DESCR_WIDTH, "заводской номер");
	for (i = 0; i < sizeof(tn); i++){
		c = tn[i];
		if (isalnum(c))
			putchar(c);
		else
			break;
	}
	if (i < sizeof(tn))
		printf("[*]\n");
	else
		printf("\n");
}

/* Вывод содержимого структуры tki */
static void print_tki(void)
{
	print_dallas();
	print_term_number();
}

/* Подсчет контрольной суммы ключа DS1990A */
static uint8_t ds_crc(uint8_t *p, int l)
{
#define DOW_B0	0
#define DOW_B1	4
#define DOW_B2	5
	uint8_t b, s = 0;
	uint8_t m0 = 1 << (7-DOW_B0);
	uint8_t m1 = 1 << (7-DOW_B1);
	uint8_t m2 = 1 << (7-DOW_B2);
	int i, j;
	if (p != NULL){
		for (i = 0; i < l; i++){
			b = p[i];
			for (j = 0; j < 8; j++){
				b ^= (s & 0x01);
				s >>= 1;
				if (b & 0x01)
					s ^= (m0 | m1 | m2);
				b >>= 1;
			}
		}
	}
	return s;
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
	int i, j, id[DS_NUMBER_LEN];
	ds_number dsn;
	struct md5_hash md5;
	bool rep;
	FILE *f = NULL;
	if (keys == NULL)
		return false;
	memset(keys, 0, n_keys * sizeof(*keys));
	if (key_file != NULL){
		f = fopen(key_file, "rb");
		if (f == NULL){
			fprintf(stderr, "Ошибка открытия файла ключей %s: %s.\n",
				key_file, strerror(errno));
			return false;
		}
	}else if (!ds_init()){
		fprintf(stderr, "Ошибка открытия драйвера DS1990A.\n");
		return false;
	}
	for (i = 0; i < n_keys; i++){
		printf("ключ #%d: ", i + 1);
		if (key_file == NULL)
			getchar();
		do{
			if (key_file == NULL)
				ds_read(dsn);
			else if (fscanf(f, "%2x %2x %2x %2x %2x %2x %2x %2x\n",
					id, id + 1, id + 2, id + 3, id + 4,
					id + 5, id + 6, id + 7) == 8){
				for (j = 0; j < DS_NUMBER_LEN; j++)
					dsn[j] = (uint8_t)id[j];
				if (ds_crc(dsn, DS_NUMBER_LEN) != 0){
					fprintf(stderr, "Несовпадение "
						"контрольной суммы номера "
						"ключа #%d.\n", i + 1);
					fclose(f);
					return false;
				}
			}else{
				fprintf(stderr, "Ошибка чтения номера ключа #%d из %s.\n",
					i + 1, key_file);
				fclose(f);
				return false;
			}
		} while ((key_file == NULL) && ds_zero_id(dsn));
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
	if (key_file != NULL)
		fclose(f);
	return true;
}

/* Очистка входного потока */
static void flush_stdin(void)
{
/* Переводим клавиатуру в неблокирующий режим */
	int nonblock = true;
	ioctl(STDIN_FILENO, FIONBIO, &nonblock);
/* Читаем оставшиеся символы из буфера */
	while (getchar() != EOF);
/* Возвращаем клавиатуру в блокирующий режим */
	nonblock = 0;
	ioctl(STDIN_FILENO, FIONBIO, &nonblock);
}

/* Ввод пользователем заводского номера терминала */
static bool ask_number(void)
{
	const char *prefix = "F00137001";
	char *p;
	char suffix[6];
	term_number tn;
	bool loop_flag = true;
	while (loop_flag){
		printf("Введите заводской номер терминала: %s", prefix);
		if (fgets(suffix, sizeof(suffix), stdin) == NULL)
			return false;
		flush_stdin();
		if (strlen(suffix) < 5)
			fprintf(stderr, "Слишком мало символов в номере.\n");
		else if (suffix[4] != '\n')
			fprintf(stderr, "Слишком много символов в номере.\n");
		else{
			suffix[4] = 0;
			strtoul(suffix, &p, 10);
			if (*p)
				fprintf(stderr, "Неверный формат номера.\n");
			else{
				memcpy(tn, prefix, 9);
				memcpy(tn + 9, suffix, 4);
				loop_flag = false;
			}
		}
	}
	set_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn);
	return true;
}


/* Вывод в stdout расшифрованного файла ключевой информации */
static void dump_tki(void)
{
	decrypt_data((uint8_t *)&tki, sizeof(tki));
	write(STDOUT_FILENO, &tki, sizeof(tki));
	encrypt_data((uint8_t *)&tki, sizeof(tki));
}

/* Семантическая проверка командной строки */
static bool check_semantic(void)
{
/* Должен быть указан файл ключевой информации и об'ект действия */
	if (tki_name == NULL){
		fprintf(stderr, "Не указан файл ключевой информации (--read-tki или --write-tki).\n");
		return false;
	}else if (subj == subj_none){
		fprintf(stderr, "Не указан об'ект действия.\n");
		return false;
	}else if ((subj == subj_all) && !tki_read){
		fprintf(stderr, "--all имеет смысл только в сочетании с --read-tki.\n");
		return false;
	}
/* Ключи DALLAS */
	if (((subj == subj_srv_keys) || (subj == subj_dbg_keys)) &&
			(key_file != NULL) && tki_read){
		fprintf(stderr, "Файл ключей используется только в сочетании с --write-tki.\n");
		return false;
	}
	if (can_repeat_keys){
		if ((subj == subj_srv_keys) || (subj == subj_dbg_keys)){
			if (tki_read)
				fprintf(stderr, "--repeated не имеет смысла без --write-tki.\n");
		}else
			fprintf(stderr, "--repeated имеет смысл только в сочетании с --xxx-keys.\n");
	}
/* Расшифровка файла */
	if ((subj == subj_decrypt) && !tki_read){
		fprintf(stderr, "--decrypt имеет смысл только в сочетании с --read-tki.\n");
		return false;
	}
	return true;
}

/* Проверка правильности введенных имен файлов */
static bool check_file_names(void)
{
	struct stat st;
	if (key_file != NULL){
		if (stat(key_file, &st) == -1){
			fprintf(stderr, "Неверное имя файла ключей: %s.\n",
				key_file);
			return false;
		}
	}
	return true;
}

/* Разбор командной строки */
static bool parse_cmd_line(int argc, char **argv)
{
	bool set_subj(int _subj)	/* установка об'екта действия */
	{
		if (subj == subj_none){
			subj = _subj;
			return true;
		}else{
			fprintf(stderr, "Указано несколько об'ектов действия.\n");
			return false;
		}
	}
	extern char *optarg;
	extern int optind;
	const struct option longopts[] = {
		{"read-tki",	required_argument,	NULL, 'r'},
		{"write-tki",	required_argument,	NULL, 'w'},
		{"srv-keys",	optional_argument,	NULL, 'v'},
		{"dbg-keys",	optional_argument,	NULL, 'g'},
		{"repeated",	no_argument,		NULL, 's'},
		{"number",	no_argument,		NULL, 'n'},
		{"decrypt",	no_argument,		NULL, 'd'},
		{"all",		no_argument,		NULL, 'a'},
		{"help",	no_argument,		NULL, 'h'},
		{NULL,		0,			NULL, 0},
	};
	char *shortopts = "r:w:vgsndah";
	bool loop_flag = true, ret_val = true;
	bool access_specified = false;
	while (loop_flag && ret_val){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'r':		/* --read-tki */
				if (access_specified){
					fprintf(stderr, "Повторное указание типа доступа.\n");
					ret_val = false;
				}else{
					access_specified = true;
					tki_read = true;
					tki_name = optarg;
				}
				break;
			case 'w':		/* --write-tki */
				if (access_specified){
					fprintf(stderr, "Повторное указание типа доступа.\n");
					ret_val = false;
				}else{
					access_specified = true;
					tki_read = false;
					tki_name = optarg;
				}
				break;
			case 'v':		/* --srv-keys */
				key_file = optarg;
				ret_val = set_subj(subj_srv_keys);
				break;
			case 'g':		/* --dbg-keys */
				key_file = optarg;
				ret_val = set_subj(subj_dbg_keys);
				break;
			case 's':		/* --repeated */
				if (can_repeat_keys)
					fprintf(stderr, "Повторное указание флага повторения ключей.\n");
				can_repeat_keys = true;
				break;
			case 'n':		/* --number */
				ret_val = set_subj(subj_number);
				break;
			case 'd':
				ret_val = set_subj(subj_decrypt);
				break;
			case 'a':		/* --all */
				ret_val = set_subj(subj_all);
				break;
			case 'h':
				usage();
				ret_val = false;
				break;
			case EOF:		/* конец командной строки */
				loop_flag = false;
				if (optind < argc){
					fprintf(stderr, "Лишние данные в командной строке.\n");
					ret_val = false;
				}
				break;
			default:		/* ошибка */
				ret_val = false;
				break;
		}
		loop_flag = loop_flag && ret_val;
	}
	return ret_val && check_semantic() && check_file_names();
}

int main(int argc, char **argv)
{
	struct md5_hash srv_keys[NR_SRV_KEYS];
	struct md5_hash dbg_keys[NR_DBG_KEYS];
	if (argc == 1){
		usage();
		return 1;
	}else if (!parse_cmd_line(argc, argv) || !read_tki(tki_name, true))
		return 1;
	check_tki();
	if (!tki_ok){
		printf("файл ключевой информации поврежден\n");
		return 1;
	}
	if (tki_read){		/* чтение из tki */
		switch (subj){
			case subj_srv_keys:
			case subj_dbg_keys:
				print_dallas();
				break;
			case subj_number:
				print_term_number();
				break;
			case subj_decrypt:
				dump_tki();
				break;
			case subj_all:
				print_tki();
				break;
		}
	}else{			/* запись в tki */
		switch (subj){
			case subj_srv_keys:
				if (write_dallas(srv_keys, NR_SRV_KEYS,
							can_repeat_keys))
					set_tki_field(&tki, TKI_SRV_KEYS,
							(uint8_t *)srv_keys);
				break;
			case subj_dbg_keys:
				if (write_dallas(dbg_keys, NR_DBG_KEYS,
							can_repeat_keys))
					set_tki_field(&tki, TKI_DBG_KEYS,
							(uint8_t *)dbg_keys);
				break;
			case subj_number:
				if (!ask_number()){
					fprintf(stderr, "\nзаводской номер не записан.\n");
					return 1;
				}
				break;
		}
		if (!write_tki(tki_name))
			return 1;
	}
	return 0;
}
