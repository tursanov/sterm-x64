/*
 * Удаление лицензий ИПТ/ППУ с терминала и формирование кода удаления.
 * При удалении в MBR записывается специальный флаг, а пользователю
 * выдается 32-значное шестнадцатеричное число. Число формируется
 * на основе текущей даты, таблицы случайных чисел (rndtbl.c) и
 * хэша заводского номера терминала.
 * (c) gsr 2006, 2011, 2019.
 */

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "colortty.h"
#include "license.h"
#include "md5.h"
#include "rndtbl.h"
#include "tki.h"

/* Удаление файла лицензии */
bool rm_lic_file(const char *name, const char *lic_name)
{
	bool ret = false;
	struct stat st;
	if (stat(name, &st) == -1){
		if (errno == ENOENT)
			ret = true;
		else
			fprintf(stderr, tRED
				"Ошибка получения информации о лицензии %s: %s.\n"
				ANSI_DEFAULT, lic_name, strerror(errno));
	}else if (!S_ISREG(st.st_mode))
		fprintf(stderr, tRED "Неверный формат лицензии %s.\n"
			ANSI_DEFAULT, lic_name);
	else if (unlink(name) == -1)
		fprintf(stderr, tRED "Ошибка удаления лицензии %s: %s.\n"
			ANSI_DEFAULT, lic_name, strerror(errno));
	else
		ret = true;
	return ret;
}

/* Чтение хеша заводского номера терминала */
static bool read_term_number(char *name, struct md5_hash *number)
{
	bool ret = false;
	struct stat st;
	if (stat(name, &st) == -1)
		fprintf(stderr, tRED "Ошибка получения информации о файле %s: %s.\n"
			ANSI_DEFAULT, name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, tRED "%s не является обычным файлом.\n"
			ANSI_DEFAULT, name);
	else if (st.st_size != sizeof(struct md5_hash))
		fprintf(stderr, tRED "Файл %s имеет неверный размер.\n"
			ANSI_DEFAULT, name);
	else{
		int fd = open(name, O_RDONLY);
		if (fd == -1)
			fprintf(stderr, tRED "Ошибка открытия %s для чтения: %s.\n"
				ANSI_DEFAULT, name, strerror(errno));
		else{
			if (read(fd, number, sizeof(*number)) == sizeof(*number))
				ret = true;
			else
				fprintf(stderr, tRED "Ошибка чтения из %s: %s.\n"
					ANSI_DEFAULT, name, strerror(errno));
			close(fd);
		}
	}
	return ret;
}

/* Формирование кода удаления лицензии для заданного заводского номера терминала */
bool mk_del_hash_for_tnumber(uint8_t *buf, const struct md5_hash *number,
	const struct rnd_rec *rnd_tbl, int n)
{
	const uint8_t *p = (const uint8_t *)number;
	uint32_t t;
	int i, index;
	t = (uint32_t)time(NULL);
	index = u_times() % n;
	buf[0] = t >> 24;
	buf[1] = rnd_tbl[index].a >> 24;
	buf[2] = (rnd_tbl[index].a >> 16) & 0xff;
	buf[3] = (rnd_tbl[index].a >> 8) & 0xff;
	buf[4] = (t >> 16) & 0xff;
	buf[5] = rnd_tbl[index].a & 0xff;
	buf[6] = rnd_tbl[index].b >> 24;
	buf[7] = (rnd_tbl[index].b >> 16) & 0xff;
	buf[8] = (t >> 8) & 0xff;
	buf[9] = (rnd_tbl[index].b >> 8) & 0xff;
	buf[10] = rnd_tbl[index].b & 0xff;
	buf[11] = rnd_tbl[index].c >> 24;
	buf[12] = t & 0xff;
	buf[13] = (rnd_tbl[index].c >> 16) & 0xff;
	buf[14] = (rnd_tbl[index].c >> 8) & 0xff;
	buf[15] = rnd_tbl[index].c & 0xff;
	for (i = 0; i < 16; i++)
		buf[i] ^= p[i];
	for (i = 0; i < 16; i++)
		buf[(i + 1) % 16] ^= buf[i];
	return true;
}

/* Формирование кода удаления лицензии */
bool mk_del_hash(uint8_t *buf, const struct rnd_rec *rnd_tbl, int n)
{
	bool ret = false;
	struct md5_hash number;
	if (read_term_number(USB_BIND_FILE, &number))
		ret = mk_del_hash_for_tnumber(buf, &number, rnd_tbl, n);
	return ret;
}

/* Вывод кода удаления лицензии на экран */
void print_del_hash(const uint8_t *del_hash, const char *lic_name)
{
	int i;
	printf(CLR_SCR);
	printf(ANSI_PREFIX "12;H" tCYA
"                   Лицензия %s удалена с вашего терминала.\n"
"   Для подтверждения удаления сообщите, пожалуйста, номер, указанный ниже,\n"
"                             в ЗАО НПЦ \"Спектр\".\n", lic_name);
	printf(ANSI_PREFIX "16;20H" tYLW);
	for (i = 0; i < 16; i++){
		if ((i > 0) && (i < 15) && ((i % 2) == 0))
			putchar('-');
		printf("%.2hhX", del_hash[i]);
	}
	printf(ANSI_DEFAULT "\n");
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

/* Очистка входного потока и чтение из него символа */
static int flush_getchar(void)
{
	flush_stdin();
	return getchar();
}

/* Запрос подтверждения удаления лицензии */
bool ask_rmlic_confirmation(const char *lic_name)
{
	int c;
	while (true){
		printf(CLR_SCR);
		printf(	ANSI_PREFIX "10;H" tYLW
"      Вы действительно хотите удалить лицензию %s с данного терминала ?\n"
			ANSI_PREFIX "11;30H" tGRN "1" tYLW " -- да;\n"
			ANSI_PREFIX "12;30H" tGRN "2" tYLW " -- нет\n"
			ANSI_PREFIX "13;30H" tWHT "Ваш выбор: " ANSI_DEFAULT,
			lic_name);
		c = flush_getchar();
		switch (c){
			case '1':
				return true;
			case '2':
				printf(CLR_SCR ANSI_PREFIX "12;23H");
				printf(tRED "Лицензия %s НЕ будет удалена\n"
					ANSI_DEFAULT, lic_name);
				return false;
		}
	}
}
