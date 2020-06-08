/* Общие функции для создания файла списка лицензий ИПТ/ППУ. (c) gsr 2011, 2019 */

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
#include "license.h"
#include "tki.h"

/* Является ли символ пробелом */
static bool is_space(uint8_t ch)
{
	return (ch == ' ') || (ch == '\n') || (ch == '\r');
}

/* Является ли символ префиксом заводского номера терминала */
static bool is_number_prefix(uint8_t ch)
{
	bool ret = false;
	static const char *prefixes = "ADEF";
	int i;
	for (i = 0; prefixes[i]; i++){
		if (ch == prefixes[i]){
			ret = true;
			break;
		}
	}
	return ret;
}

/*
 * Чтение очередного заводского номера из файла. Возвращает число
 * считанных номеров (0/1) или -1 в случае ошибки.
 */
int read_number(int fd, uint8_t *number)
{
	enum {
		st_space,
		st_infix,
		st_number,
		st_end,
		st_stop,
		st_err,
	};
	static char *infix = "00137001";
	int st = st_space, n = 0, i = 0;
	uint8_t b;
	while ((st != st_stop) && (st != st_err)){
		if (read(fd, &b, 1) != 1){
			switch (st){
				case st_space:
					return 0;
				case st_end:
					return 1;
				default:
					return -1;
			}
		}
		switch (st){
			case st_space:
				if (is_number_prefix(b)){
					number[i++] = b;
					st = st_infix;
				}else if (!is_space(b))
					st = st_err;
				break;
			case st_infix:
				if (b == infix[n++]){
					number[i++] = b;
					if (!infix[n]){
						n = 4;
						st = st_number;
					}
				}else
					st = st_err;
				break;
			case st_number:
				if (isdigit(b)){
					number[i++] = b;
					if (--n == 0)
						st = st_end;
				}else
					st = st_err;
				break;
			case st_end:
				st = is_space(b) ? st_stop : st_err;
		}
	}
	return st == st_stop ? 1 : -1;
}

/* Открытие файла списка номеров */
int open_number_file(const char *name)
{
	int ret = -1;
	struct stat st;
	if (stat(name, &st) == -1)
		fprintf(stderr, "Ошибка получения информации о %s: %s.\n",
			name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, "%s не является обычным файлом.\n", name);
	else{
		ret = open(name, O_RDONLY);
		if (ret == -1)
			fprintf(stderr, "Ошибка открытия %s для чтения: %s.\n",
				name, strerror(errno));
	}
	return ret;
}
