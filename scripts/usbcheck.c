/*
 * Проверка модуля безопасности на чтение/запись. (c) gsr & Alex 2004, 2013.
 * Использование: usbcheck [--number=n] [--passes=m] [--reads=k]
 * --number	номер устройства USB (0..15). По умолчанию 0;
 * --passes	число проходов (1..100). По умолчанию 10.
 * --reads	число циклов чтения после записи (1..100). По умолчанию 5;
 * --help	вывод справки.
 */

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "colortty.h"
#include "usb/core.h"

/* Номер модуля безопасности */
#define USBKEY_NUMBER	0
static int usbkey_number = USBKEY_NUMBER;

/* Число проходов */
#define NR_PASSES	10
static int nr_passes	= NR_PASSES;

/* Число циклов чтения после записи */
#define NR_READS	5
static int nr_reads	= NR_READS;

/* Вывод справки об использовании программы */
static void usage(void)
{
	static char *strs[] = {
		"Проверка модуля безопасности. (c) АО НПЦ \"Спектр\" 2004, 2013.",
		"Использование: usbcheck [параметры]",
		"Параметры (в скобках указаны значения по умолчанию)",
		"--number=n\t\tномер модуля безопасности (0..15) [0];",
		"--passes=n\t\tчисло проходов (1..100) [10];",
		"--reads=n\t\tчисло циклов чтения после записи (1..100) [5];",
		"--help\t\t\tвывод справки.",
	};
	int i;
	for (i = 0; i < ASIZE(strs); i++)
		printf("%s\n", strs[i]);
}

/* Разбор командной строки */
static bool parse_cmd_line(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	const struct option longopts[] = {
		{"number",	required_argument,	NULL,	'n'},
		{"passes",	required_argument,	NULL,	'p'},
		{"reads",	required_argument,	NULL,	'r'},
		{"help",	no_argument,		NULL,	'h'},
		{NULL,		0,			NULL,	0},
	};
	char *shortopts = "n:p:r:h";
	bool loop_flag = true;
	char *p;
#define OPT_NUMBER	1
#define OPT_PASSES	2
#define OPT_READS	4
	int opts = 0;
	while (loop_flag){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'n':
				if ((opts & OPT_NUMBER) == 0){
					if (*optarg)
						usbkey_number = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (usbkey_number > NR_USB_KEYS)){
						printf("неверный номер модуля безопасности: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_NUMBER;
				}else
					printf("повторное указание номера безопасности игнорируется\n");
				break;
			case 'p':
				if ((opts & OPT_PASSES) == 0){
					if (*optarg)
						nr_passes = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (nr_passes > 100)){
						printf("неверный формат числа проходов: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_PASSES;
				}else
					printf("повторное указание числа проходов игнорируется\n");
				break;
			case 'r':
				if ((opts & OPT_READS) == 0){
					if (*optarg)
						nr_reads = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (nr_reads > 100)){
						printf("неверный формат числа циклов чтения: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_READS;
				}else
					printf("повторное указание числа проходов игнорируется\n");
				break;
			case 'h':
				return false;
			case EOF:		/* конец командной строки */
				loop_flag = false;
				if (optind < argc){
					printf("лишние данные в командной строке\n");
					return false;
				}
				break;
			default:		/* ошибка */
				return false;
		}
	}
	return true;
}

/* Заполнение буфера нулями */
void make_all_zeroes(uint8_t *data, size_t l)
{
	for (; l > 0; l--)
		*data++ = 0;
}

/* Заполнение буфера единицами */
void make_all_ones(uint8_t *data, size_t l)
{
	for (; l > 0; l--)
		*data++ = 1;
}

/* Заполнение буфера бегущим нулём */
void make_running_zero(uint8_t *data, size_t l)
{
	off_t i;
	uint8_t b = ~1;
	for (i = 0; i < l; i++){
		if ((i % 8) == 0)
			b = 1;
		else
			b <<= 1;
		data[i] = ~b;
	}
}

/* Заполнение буфера бегущей единицей */
void make_running_one(uint8_t *data, size_t l)
{
	int i;
	uint8_t b = 1;
	for (i = 0; i < l; i++){
		if ((i % 8) == 0)
			b = 1;
		else
			b <<= 1;
		data[i] = b;
	}
}

/* Заполнение буфера адрес-байтом */
void make_address_byte(uint8_t *data, size_t l)
{
	uint8_t b = 0;
	for (; l > 0; l--)
		*data++ = b++;
}

/* Заполнение буфера случайными числами */
void make_random_data(uint8_t *data, size_t l)
{
	for (; l > 0; l--)
		*data++ = rand() & 0xff;
}

/* Очистка входного потока */
static void flush_stdin(void)
{
	int flags = fcntl(STDIN_FILENO, F_GETFL);
	if (flags == -1)
		return;
	if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1)
		return;
	while (getchar() != EOF);
	fcntl(STDIN_FILENO, F_SETFL, flags);
}

static int flush_getchar(void)
{
	flush_stdin();
	return getchar();
}

static bool do_test(int n)
{
	static struct {
		void (*fn)(uint8_t *, size_t);
		const char *name;
	} gen_fn[] = {
		{make_all_zeroes,	"0000"},
		{make_all_ones,		"1111"},
		{make_running_zero,	"RUN0"},
		{make_running_one,	"RUN1"},
		{make_address_byte,	"ADDR"},
		{make_random_data,	"RAND"}
	};
	static uint8_t tx[USB_KEY_SIZE];
	static uint8_t rx[USB_KEY_SIZE];
	int i, j;
	bool fault = false;
	printf("***ШАГ: %.3d***\n", n + 1);
	for (i = 0; !fault && (i < ASIZE(gen_fn)); i++){
		printf("\t%s:\tЗАП...", gen_fn[i].name);
		fflush(stdout);
		gen_fn[i].fn(tx, sizeof(tx));
		if (usbkey_write(usbkey_number, tx)){
			printf("\b\b\b   " tGRN "OK" ANSI_DEFAULT "\n");
			printf("\t\tЧТН..." tGRN);
			fflush(stdout);
			for (j = 0; !fault && (j < nr_reads); j++){
				if (usbkey_read(usbkey_number, rx, 0, USB_KEY_BLOCKS) &&
						(memcmp(rx, tx, USB_KEY_SIZE) == 0)){
					putchar('+');
					fflush(stdout);
				}else{
					for (; j > 0; j--)
						putchar('\b');
					printf("\b\b\b   " tRED "***ОШ ЧТН***" ANSI_DEFAULT "\n");
					fault = true;
				}
			}
			if (!fault){
				for (j = 0; j < nr_reads; j++)
					putchar('\b');
				printf("\b\b\b   " tGRN "OK" ANSI_DEFAULT "\n");
			}
		}else{
			printf("\b\b\b   " tRED "***ОШ ЗАП***" ANSI_DEFAULT "\n");
			fault = true;
		}
	}
	return !fault;
}

int main(int argc, char **argv)
{
	int ret = 1;
	if (!parse_cmd_line(argc, argv))
		usage();
	else{
		bool fault = false;;
		srand(time(NULL));
		while (!fault){
			printf(tWHT "ВСТАВЬТЕ МОДУЛЬ БЕЗОПАСНОСТИ В РАЗЪЕМ И НАЖМИТЕ ENTER: "
					ANSI_DEFAULT);
			fflush(stdout);
			flush_getchar();
			if (usbkey_open(usbkey_number)){
				int i;
				for (i = 0; !fault && (i < nr_passes); i++){
					if (!do_test(i))
						fault = true;
				}
			}else{
				puts(tRED "ОШИБКА ДОСТУПА К МОДУЛЮ БЕЗОПАСНОСТИ." ANSI_DEFAULT);
				fault = true;
			}
		}
		usbkey_close(usbkey_number);
		if (fault)
			puts(tRED "***ПРИ ПРОВЕРКЕ МОДУЛЯ БЕЗОПАСНОСТИ ПРОИЗОШЛА ОШИБКА***"
				ANSI_DEFAULT "\n");
		else
			ret = 0;
	}
	return ret;
}
