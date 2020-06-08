/* Создание файла списка лицензий ИПТ. (c) gsr 2005, 2019 */

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
#include "mklic.h"
#include "tki.h"

static struct bank_license_info licenses[MAX_BANK_LICENSES];

/* Создание записи для лицензии ИПТ */
static void make_license_info(const uint8_t *number, struct bank_license_info *li)
{
	uint8_t buf[2 * TERM_NUMBER_LEN];
	uint8_t v1[64], v2[64];
	int i, l;
/* Создание хеша заводского номера */
	l = base64_encode(number, TERM_NUMBER_LEN, v1);
	encrypt_data(v1, l);
	get_md5(v1, l, &li->number);
/* Создание хеша лицензии ИПТ */
	memcpy(buf, number, TERM_NUMBER_LEN);
	for (i = 0; i < TERM_NUMBER_LEN; i++)
		buf[sizeof(buf) - i - 1] = ~buf[i];
	l = base64_encode(buf, sizeof(buf), v1);
	l = base64_encode(v1, l, v2);
	get_md5(v2, l, &li->license);
}

/* Создание списка лицензий ИПТ */
static int make_licenses(int fd, struct bank_license_info *licenses)
{
	int n = 0, ret;
	uint8_t number[TERM_NUMBER_LEN];
	while ((ret = read_number(fd, number)) == 1){
		if (n == MAX_BANK_LICENSES){
			fprintf(stderr, "Будет обработано не более %d номеров.\n",
				MAX_BANK_LICENSES);
			break;
		}
		make_license_info(number, licenses + n++);
	}
	if (ret == -1){
		fprintf(stderr, "Ошибка чтения заводского номера #%d.\n", n + 1);
		return -1;
	}else
		return n;
}

/* Вывод списка лицензий в stdout */
static void write_licenses(const struct bank_license_info *licenses, size_t nr_licenses)
{
	write(STDOUT_FILENO, licenses, nr_licenses * sizeof(struct bank_license_info));
}

int main(int argc, char **argv)
{
	int ret = 1;
	if (argc != 2)
		fprintf(stderr, "Укажите имя файла списка заводских номеров.\n");
	else{
		int fd = open_number_file(argv[1]);
		if (fd != -1){
			int nr_licenses = make_licenses(fd, licenses);
			if (nr_licenses > 0){
				write_licenses(licenses, nr_licenses);
				ret = 0;
			}
			close(fd);
		}
	}
	return ret;
}
