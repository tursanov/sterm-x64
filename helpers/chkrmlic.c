/* Проверка кода удаления лицензии ИПТ. (c) gsr 2006, 2011, 2019 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "base64.h"
#include "md5.h"
#include "rndtbl.h"
#include "tki.h"

/* Длина заводского номера терминала */
#define TERM_NUMBER_LEN		13
/* Длина кода подтверждения */
#define LIC_CODE_LEN		32
/* Длина группы цифр в коде подтверждения */
#define LIC_CODE_GR_LEN		4

/* Вывод справки по использованию программы */
static void usage(void)
{
	static char *help[] = {
		"Программа проверки кодов удаления лицензий ИПТ/ППУ.",
		"(c) ЗАО НПЦ \"Спектр\" 2006, 2011, 2019.",
		"Использование: chkrmlic <file>",
		"В файле <file> указываются коды удаления лицензий ИПТ/ППУ",
		"в формате <номер терминала> <код удаления>",
	};
	int i;
	for (i = 0; i < ASIZE(help); i++)
		printf("%s\n", help[i]);
}

/* Номер текущей обрабатываемой строки */
static int line_number;
/* Номер в строке текущего обрабатываемого символа */
static int col_number;
/* Флаг конца файла */
static bool eof;
/* Размер горизонтальной табуляции */
#define TAB_SIZE	8

/* Открытие файла с кодами подтверждения */
static FILE *open_data(const char *name)
{
	FILE *f = fopen(name, "r");
	if (f == NULL)
		fprintf(stderr, "Ошибка открытия %s для чтения: %s\n",
			name, strerror(errno));
	return f;
}

static bool is_nl(int c)
{
	return (c == '\r') || (c == '\n');
}

static bool is_space(int c)
{
	return (c == ' ') || (c == '\t');
}

static bool is_digit(int c)
{
	return (c >= '0') && (c <= '9');
}

static bool is_xdigit(int c)
{
	return is_digit(c) || ((c >= 'A') && (c <= 'F')) ||
		((c >= 'a') && (c <= 'f'));
}

static bool is_number_char(int c)
{
	return is_digit(c) ||
		(c == 'A') || (c == 'D') || (c == 'E') || (c == 'F');
}

static bool is_code_char(int c)
{
	return is_xdigit(c) || (c == '-');
}

/* Увеличение счетчиков строк и символов */
static void adjust_data_pos(int c)
{
	switch (c){
		case '\t':
			col_number = ((col_number + TAB_SIZE) / TAB_SIZE) * TAB_SIZE;
			break;
		case '\n':
			line_number++;
			col_number = 0;
			break;
		case '\r':
			col_number = 0;
			break;
		default:
			col_number++;
			break;
	}
}

/* Чтение строки из файла данных */
static bool read_line(FILE *f, char *number, char *code)
{
	enum {
		st_start,
		st_number,
		st_space1,
		st_code,
		st_space2,
		st_stop,
		st_err,
	};
	int i = 0, c, st = st_start;
	bool delim = false;
	while ((st != st_stop) && (st != st_err)){
		errno = 0;
		c = fgetc(f);
		if (c == EOF){
			eof = true;
			if (errno == 0){
				if ((st == st_code) && (st == st_space2))
					st = st_stop;
				else
					st = st_err;
			}else{
				fprintf(stderr, "Ошибка чтения файла данных: %s\n",
					strerror(errno));
				st = st_err;
			}
		}else{
			switch (st){
				case st_start:
					if (is_number_char(c)){
						i = 0;
						st = st_number;	/* NB: fall through */
					}else{
						if (!is_space(c) && !is_nl(c))
							st = st_err;
						break;
					}
				case st_number:
					if (is_number_char(c)){
						if (i >= TERM_NUMBER_LEN)
							st = st_err;
						else
							number[i++] = c;
					}else if (is_space(c)){
						if (i != TERM_NUMBER_LEN)
							st = st_err;
						else
							st = st_space1;
					}else
						st = st_err;
					break;
				case st_space1:
					if (is_xdigit(c)){
						i = 0;
						st = st_code;	/* NB: fall through */
					}else{
						if (!is_space(c))
							st = st_err;
						break;
					}
				case st_code:
					if (is_code_char(c)){
						if (i >= LIC_CODE_LEN)
							st = st_err;
						else if (c == '-'){
							if (delim)
								st = st_err;
							else{
								delim = true;
								if ((i % LIC_CODE_GR_LEN) != 0)
									st = st_err;
							}
						}else{
							if ((i > 0) && ((i % LIC_CODE_GR_LEN) == 0) &&
									!delim)
								st = st_err;
							else
								code[i++] = c;
							delim = false;
						}
					}else if (is_nl(c))
						st = (i == LIC_CODE_LEN) ? st_stop : st_err;
					else if (is_space(c))
						st = (i == LIC_CODE_LEN) ? st_space2 : st_err;
					else
						st = st_err;
					break;
				case st_space2:
					if (is_nl(c))
						st = st_stop;
					else if (!is_space(c))
						st = st_err;
					break;
			}
			if (st == st_err)
				fprintf(stderr, "Строка %d:%d: ошибка в файле данных\n",
					line_number + 1, col_number + 1);
			else
				adjust_data_pos(c);
		}
	}
	return st == st_stop;
}

/* Проверка формата заводского номера терминала */
static bool check_number(const char *number)
{
	static char *prefix = "ADEF";
	static char *infix = "00137001";
	int i, j;
	for (i = 0; i < TERM_NUMBER_LEN; i++){
		if (i == 0){
			for (j = 0; prefix[j]; j++){
				if (number[i] == prefix[j])
					break;
			}
			if (!prefix[j])
				return false;
		}else if (i < 9){
			if (number[i] != infix[i - 1])
				return false;
		}else if (!is_digit(number[i]))
			return false;
	}
	return true;
}

/* Преобразование символьного кода в двоичный */
static void translate_code(const char *code, uint8_t *buf)
{
	int i, j, c, v;
	for (i = j = 0; i < 32; i++){
		c = toupper(code[i]);
		v = (c > '9') ? c - 'A' + 10 : c - '0';
		if (i & 1)
			buf[j] <<= 4;
		else
			buf[j] = 0;
		buf[j] |= v;
		j += i & 1;
	}
}

/* Проверка соответствия кода номеру */
static bool check_code(const char *number, const char *code, time_t *t,
	const struct rnd_rec *rnd_tbl, size_t n)
{
	uint8_t buf[16];
	uint8_t tmp[32];
	struct md5_hash md5;
	int i;
	uint32_t a, b, c;
	i = base64_encode((uint8_t *)number, TERM_NUMBER_LEN, tmp);
	encrypt_data(tmp, i);
	get_md5(tmp, i, &md5);
	memcpy(tmp, &md5, sizeof(md5));
	translate_code(code, buf);
	for (i = sizeof(md5); i > 0; i--)
		buf[i % sizeof(md5)] ^= buf[i - 1];
	for (i = 0; i < sizeof(md5); i++)
		buf[i] ^= tmp[i];
	a = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[5];
	b = (buf[6] << 24) | (buf[7] << 16) | (buf[9] << 8) | buf[10];
	c = (buf[11] << 24) | (buf[13] << 16) | (buf[14] << 8) | buf[15];
	*t = (buf[0] << 24) | (buf[4] << 16) | (buf[8] << 8) | buf[12];
	for (i = 0; i < n; i++){
		if ((a == rnd_tbl[i].a) && (b == rnd_tbl[i].b) &&
				(c == rnd_tbl[i].c))
			break;
	}
	return i < NR_RND_REC;
}

/* Вывод сведений о коде удаления лицензии ИПТ */
static void print_rmlic(const char *lic_name, const char *number, time_t t,
	bool ok)
{
	struct tm *tm = localtime(&t);
	if (ok)
		printf("\t%s:\t%.*s: %.2d.%.2d.%.4d %.2d:%.2d:%.2d\n",
			lic_name, TERM_NUMBER_LEN, number,
			tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	else
		printf("\t%s:\t%.*s: неверный код\n",
			lic_name, TERM_NUMBER_LEN, number);
}

int main(int argc, char **argv)
{
	int ret = 1;
	if (argc != 2)
		usage();
	else{
		FILE *f = open_data(argv[1]);
		if (f != NULL){
			char number[TERM_NUMBER_LEN], code[32];
			while (!eof){
				if (read_line(f, number, code)){
					if (check_number(number)){
						time_t t;
						if (check_code(number, code, &t,
								rnd_tbl_bnk, ASIZE(rnd_tbl_bnk)))
							print_rmlic("ИПТ", number, t, true);
						else if (check_code(number, code, &t,
								rnd_tbl_lprn, ASIZE(rnd_tbl_lprn)))
							print_rmlic("ППУ", number, t, true);
						else
							print_rmlic("???", number, t, false);
					}else
						fprintf(stderr, "Строка %d: неверный формат заводского номера\n",
							line_number + 1);
				}
			}
			fclose(f);
			ret = 0;
		}
	}
	return ret;
}
