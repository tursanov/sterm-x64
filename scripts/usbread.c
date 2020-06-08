/*
 * Программа чтения данных из модуля безопасности. (c) gsr 2006.
 * Использование: usbread [--first-block=n] [--blocks=m] [--verbose] [--help].
 * --first-block	-- номер первого блока для чтения;
 * --blocks		-- размер данных в блоках;
 * --verbose		-- вывод дополнительной информации;
 * --help		-- вывод справки.
 * Размер блока -- 256 байт. Считанные данные выводятся в stdout.
 */

#include <sys/times.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usb/core.h"

/* Номер первого блока для чтения */
static int first_block;
/* Размер данных в блоках */
static int nr_blocks = USB_KEY_SIZE / USB_BLOCK_LEN;
/* Флаг вывода дополнительной информации */
static bool verbose;

/* Буфер для чтения */
static uint8_t in_data[USB_KEY_SIZE];

static void usage(void)
{
	static char *strs[] = {
		"Программа чтения данных из модуля безопасности. (c) АО НПЦ \"Спектр\" 2006.",
		"Использование: usbread [--first-block=n] [--blocks=m] [--verbose] [--help].",
		"\t--first-block\t\t-- номер первого блока для чтения;",
		"\t--blocks\t\t-- размер данных в блоках;",
		"\t--verbose\t\t-- вывод дополнительной информации;",
		"\t--help\t\t\t-- вывод справки.",
		"Размер блока -- 256 байт. Считанные данные выводятся в stdout.",
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
		{"first-block",	required_argument,	NULL,	'f'},
		{"blocks",	required_argument,	NULL,	'b'},
		{"verbose",	no_argument,		NULL,	'v'},
		{"help",	no_argument,		NULL,	'h'},
		{NULL,		0,			NULL,	0},
	};
	char *shortopts = "f:b:vh";
	bool loop_flag = true;
	char *p;
#define OPT_FIRST_BLOCK		1
#define OPT_BLOCKS		2
	int opts = 0;
	while (loop_flag){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'f':
				if ((opts & OPT_FIRST_BLOCK) == 0){
					if (*optarg)
						first_block = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (first_block >= USB_KEY_BLOCKS)){
						fprintf(stderr, "неверный номер первого блока для чтения: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_FIRST_BLOCK;
				}else
					fprintf(stderr, "повторное указание номера первого блока игнорируется\n");
				break;
			case 'b':
				if ((opts & OPT_BLOCKS) == 0){
					if (*optarg)
						nr_blocks = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (nr_blocks > USB_KEY_BLOCKS)){
						fprintf(stderr, "неверное количество блоков для чтения: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_BLOCKS;
				}else
					fprintf(stderr, "повторное указание количества блоков игнорируется\n");
				break;
			case 'v':
				verbose = true;
				break;
			case 'h':
				return false;
			case EOF:		/* конец командной строки */
				loop_flag = false;
				if (optind < argc){
					fprintf(stderr, "лишние данные в командной строке\n");
					return false;
				}
				break;
			default:		/* ошибка */
				return false;
		}
	}
	if ((first_block + nr_blocks) > USB_KEY_BLOCKS){
		fprintf(stderr, "недопустимое количество блоков для чтения: %d\n",
			nr_blocks);
		return false;
	}else
		return true;
}

static bool do_read(void)
{
	uint32_t t0 = u_times();
	if (verbose)
		fprintf(stderr, "открываем модуль безопасности...");
	if (!usbkey_open(0)){
		if (verbose)
			fprintf(stderr, "\b\b\b: ошибка\n");
		else
			fprintf(stderr, "ошибка открытия модуля безопасности\n");
		return false;
	}else if (verbose)
		fprintf(stderr, "\b\b\b: ok\n");
	if (verbose)
		fprintf(stderr, "чтение данных из модуля безопасности...");
	if (!usbkey_read(0, in_data, first_block, nr_blocks)){
		if (verbose)
			fprintf(stderr, "\b\b\b: ошибка\n");
		else
			fprintf(stderr, "ошибка открытия модуля безопасности\n");
		usbkey_close(0);
		return false;
	}else if (verbose)
		fprintf(stderr, "\b\b\b: ok\n");
	write(STDOUT_FILENO, in_data, nr_blocks * USB_BLOCK_LEN);
	usbkey_close(0);
	if (verbose)
		fprintf(stderr, "чтение заняло %u мсек\n", (u_times() - t0) * 10);
	return true;
}

int main(int argc, char **argv)
{
	if (!parse_cmd_line(argc, argv)){
		usage();
		return 1;
	}else
		return do_read() ? 0 : 1;
}
