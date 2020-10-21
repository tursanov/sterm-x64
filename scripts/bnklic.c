/* Установка на терминал лицензии ИПТ. (c) gsr 2005, 2011, 2019-2020 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "base64.h"
#include "license.h"
#include "licsig.h"
#include "tki.h"

static struct bank_license_info licenses[MAX_BANK_LICENSES];
static int nr_licenses;

/* Чтение файла лицензий */
static int read_license_file(const char *path, struct bank_license_info *licenses)
{
	struct stat st;
	div_t d;
	int fd, ret = -1;
	if (stat(path, &st) == -1)
		fprintf(stderr, "Ошибка получения информации о файле %s: %m.\n", path);
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, "%s не является обычным файлом.\n", path);
	else{
		d = div(st.st_size, sizeof(struct bank_license_info));
		if ((d.quot == 0) || (d.rem != 0))
			fprintf(stderr, "Файл %s имеет неверный размер.\n", path);
		else if (d.quot > MAX_BANK_LICENSES)
			fprintf(stderr, "Число лицензий не может превышать %d.\n",
				MAX_BANK_LICENSES);
		else{
			fd = open(path, O_RDONLY);
			if (fd == -1)
				fprintf(stderr, "Ошибка открытия %s для чтения: %m.\n", path);
			else{
				if (read(fd, licenses, st.st_size) == st.st_size)
					ret = d.quot;
				else
					fprintf(stderr, "Ошибка чтения из %s: %m.\n", path);
				close(fd);
			}
		}
	}
	return ret;
}

/* Чтение хеша заводского номера терминала */
static bool read_term_number(const char *path, struct md5_hash *number)
{
	bool ret = false;
	struct stat st;
	if (stat(path, &st) == -1)
		fprintf(stderr, "Ошибка получения информации о файле %s: %m.\n", path);
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, "%s не является обычным файлом.\n", path);
	else{
		FILE *f = fopen(path, "r");
		if (f == NULL)
			fprintf(stderr, "Ошибка открытия %s для чтения: %m.\n", path);
		else{
			char nr[TERM_NUMBER_LEN + 2];	/* \n + 0 */
			if (fgets(nr, sizeof(nr), f) == NULL)
				fprintf(stderr, "Ошибка чтения из %s: %m.\n", path);
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
				if (ret){
					uint8_t buf[64];
					ssize_t l = base64_encode((const uint8_t *)nr,
						TERM_NUMBER_LEN, buf);
					encrypt_data(buf, l);
					get_md5(buf, l, number);
				}
			}
			if (fclose(f) == EOF)
				fprintf(stderr, "Ошибка закрытия %s: %m.\n", path);
		}
	}
	return ret;
}

/*
 * Поиск лицензии ИПТ по хешу заводского номера. Если хеш заводского
 * номера повторяется в списке более одного раза, значит лицензия для этого
 * терминала была удалена и мы должны записать в MBR признак удаления
 * лицензии.
 */
static const struct md5_hash *find_license(const struct bank_license_info *licenses,
		int nr_licenses, const struct md5_hash *number)
{
	const struct md5_hash *ret = NULL;
	int i, n, m;
	for (i = n = m = 0; i < nr_licenses; i++){
		if (cmp_md5(&licenses[i].number, number)){
			n++;
			m = i;
		}
	}
	if (n == 1)
		ret = &licenses[m].license;
	else if (n > 1)
		write_lic_signature(BANK_LIC_SIGNATURE_OFFSET, BANK_LIC_SIGNATURE);
	return ret;
}

/* Вывод лицензии ИПТ в stdout */
static void write_license(const struct md5_hash *license)
{
	write(STDOUT_FILENO, license, sizeof(*license));
}

int main(int argc, char **argv)
{
	int ret = 1;
	struct md5_hash number;
	const struct md5_hash *license;
	if (argc != 2)
		fprintf(stderr, "Укажите имя файла списка лицензий для работы с ИПТ.\n");
	else if (!read_term_number("/sdata/term-number.txt", &number))
		fprintf(stderr, "Не найден модуль безопасности.\n");
	else if (check_lic_signature(BANK_LIC_SIGNATURE_OFFSET, BANK_LIC_SIGNATURE))
		fprintf(stderr, "Установка лицензии для работы с ИПТ на данный "
			"терминал невозможна;\nобратитесь, пожалуйста, в ЗАО НПЦ \"Спектр\".\n");
	else{
		nr_licenses = read_license_file(argv[1], licenses);
		if (nr_licenses > 0){
			license = find_license(licenses, nr_licenses, &number);
			if (license == NULL)
				fprintf(stderr, "Не найдена лицензия ИПТ для этого терминала.\n");
			else{
				write_license(license);
				ret = 0;
			}
		}
	}
	return ret;
}
