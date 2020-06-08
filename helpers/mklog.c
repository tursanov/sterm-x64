/*
 * Создание контрольной ленты терминала.
 * (c) gsr 2000, 2003, 2004, 2008, 2009, 2018
 * Параметры командной строки:
 * --type=<type> -- тип контрольной ленты (express, local, pos);
 * --log=<file>  -- имя файла контрольной ленты;
 * --size=<size> -- размер области данных файла контрольной ленты.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "log/express.h"
#include "log/local.h"
#include "log/pos.h"

/* Тип контрольной ленты */
enum {
	lt_express,
	lt_local,
	lt_pos,
};

static int log_type = lt_express;

static char *log_file;		/* имя файла контрольной ленты */
static uint32_t log_size;	/* размер области данных файла контрольной ленты */

static void usage(void)
{
	printf(	"Создание контрольной ленты для терминала.\n"
		"(c) АО НПЦ \"Спектр\" 2000, 2003, 2004, 2008, 2009, 2018\n"
		"Использование:\tmklog [options]\n"
		"--type=<type> -- тип контрольной ленты (express, local, pos);\n"
		"--log=<file> -- имя файла контрольной ленты;\n"
		"--size=<size> -- размер области данных файла контрольной ленты;\n"
		"--help -- справка по использованию программы.\n");
}

/* Определение типа контрольной ленты */
static bool get_log_type(const char *type)
{
	bool ret = true;
	if (strcmp(type, "express") == 0)
		log_type = lt_express;
	else if (strcmp(type, "local") == 0)
		log_type = lt_local;
	else if (strcmp(type, "pos") == 0)
		log_type = lt_pos;
	else
		ret = false;
	return ret;
}

/* Разбор командной строки */
static bool parse_cmd_line(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	const struct option longopts[] = {
		{"type",	required_argument,	NULL, 't'},
		{"log",		required_argument,	NULL, 'l'},
		{"size",	required_argument,	NULL, 's'},
		{"help",	no_argument,		NULL, 'h'},
		{NULL,		0,			NULL, 0},
	};
	char *shortopts = "t:l:s:h";
	bool loop_flag = true, type_specified = false;
	char *p;
	while (loop_flag){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 't':		/* --type */
				if (type_specified){
					fprintf(stderr, "Повторное указание типа контрольной ленты.\n");
					return false;
				}else if (!get_log_type(optarg)){
					fprintf(stderr, "Указан неверный тип контрольной ленты: %s.\n",
						optarg);
					return false;
				}else
					type_specified = true;
				break;
			case 'l':		/* --log */
				if (log_file != NULL){
					fprintf(stderr, "Повторное указание имени файла контрольной ленты.\n");
					return false;
				}else
					log_file = optarg;
				break;
			case 's':		/* --size */
				if (log_size != 0){
					fprintf(stderr, "Повторное указание размера контрольной ленты.\n");
					return false;
				}else{
					if (*optarg)
						log_size = strtoul(optarg, &p, 0);
					if (!*optarg || *p){
						fprintf(stderr, "Неверный размер контрольной ленты: %s.\n",
							optarg);
						return false;
					}
				}
				break;
			case 'h':
				usage();
				return false;
				break;
			case EOF:		/* конец командной строки */
				loop_flag = false;
				if (optind < argc){
					fprintf(stderr, "Лишние данные в командной строке.\n");
					return false;
				}
				break;
			default:		/* ошибка */
				return false;
		}
	}
	if (log_file == NULL){
		fprintf(stderr, "Не указано имя файла контрольной ленты.\n");
		return false;
	}else if (log_size == 0){
		fprintf(stderr, "Не указан размер контрольной ленты.\n");
		return false;
	}else
		return true;
}

/* Создание файла контрольной ленты заданного типа */
static bool create_log(void)
{
	bool ret = false;
	struct log_header hdr = {
		.len = log_size,
		.n_recs = 0,
		.head = sizeof(hdr),
		.tail = sizeof(hdr),
		.cur_number = -1U
	};
	int fd;
	switch (log_type){
		case lt_express:
			hdr.tag = XLOG_TAG;
			break;
		case lt_local:
			hdr.tag = LLOG_TAG;
			break;
		case lt_pos:
			hdr.tag = PLOG_TAG;
			break;
		default:
			return false;
	}
	fd = open(log_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd == -1)
		fprintf(stderr, "Ошибка открытия %s для записи: %s.\n",
			log_file, strerror(errno));
	else{
		if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
			fprintf(stderr, "Ошибка записи заголовка контрольной "
					"ленты в %s: %s.\n",
				log_file, strerror(errno));
		else if (!fill_file(fd, log_size))
			fprintf(stderr, "Ошибка записи в файл контрольной "
					"ленты %s: %s\n",
				log_file, strerror(errno));
		else
			ret = true;
		close(fd);
	}
	return ret;
}

int main(int argc, char **argv)
{
	int ret = 1;
	if (argc == 1)
		usage();
	else if (parse_cmd_line(argc, argv) && create_log())
		ret = 0;
	return ret;
}
