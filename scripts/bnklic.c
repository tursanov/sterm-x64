/* Установка на терминал лицензии ИПТ. (c) gsr 2005, 2011, 2019 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "license.h"
#include "licsig.h"

static struct bank_license_info licenses[MAX_BANK_LICENSES];
static int nr_licenses;

/* Чтение файла лицензий */
static int read_license_file(const char *name, struct bank_license_info *licenses)
{
	struct stat st;
	div_t d;
	int fd, ret = -1;
	if (stat(name, &st) == -1)
		fprintf(stderr, "Ошибка получения информации о файле %s: %s.\n",
			name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, "%s не является обычным файлом.\n", name);
	else{
		d = div(st.st_size, sizeof(struct bank_license_info));
		if ((d.quot == 0) || (d.rem != 0))
			fprintf(stderr, "Файл %s имеет неверный размер.\n", name);
		else if (d.quot > MAX_BANK_LICENSES)
			fprintf(stderr, "Число лицензий не может превышать %d.\n",
				MAX_BANK_LICENSES);
		else{
			fd = open(name, O_RDONLY);
			if (fd == -1)
				fprintf(stderr, "Ошибка открытия %s для чтения: %s.\n",
					name, strerror(errno));
			else{
				if (read(fd, licenses, st.st_size) == st.st_size)
					ret = d.quot;
				else
					fprintf(stderr, "Ошибка чтения из %s: %s.\n",
						name, strerror(errno));
				close(fd);
			}
		}
	}
	return ret;
}

/* Чтение хеша заводского номера терминала */
static bool read_term_number(const char *name, struct md5_hash *number)
{
	bool ret = false;
	struct stat st;
	int fd;
	if (stat(name, &st) == -1)
		fprintf(stderr, "Ошибка получения информации о файле %s: %s.\n",
			name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, "%s не является обычным файлом.\n", name);
	else if (st.st_size != sizeof(struct md5_hash))
		fprintf(stderr, "Файл %s имеет неверный размер.\n", name);
	else{
		fd = open(name, O_RDONLY);
		if (fd == -1)
			fprintf(stderr, "Ошибка открытия %s для чтения: %s.\n",
				name, strerror(errno));
		else{
			if (read(fd, number, sizeof(*number)) == sizeof(*number))
				ret = true;
			else
				fprintf(stderr, "Ошибка чтения из %s: %s.\n",
					name, strerror(errno));
			close(fd);
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
	else if (!read_term_number(TERM_NUMBER_FILE, &number))
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
