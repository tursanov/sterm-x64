/*
 * Разблокировка терминала после удаления с него лицензим ИПТ/ППУ. (c) gsr 2011
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "unlock.h"

/* Чтение хеша заводского номера терминала */
bool read_term_number(const char *name, struct md5_hash *number)
{
	bool ret = false;
	struct stat st;
	if (stat(name, &st) == -1)
		fprintf(stderr, tRED "ошибка получения информации о файле %s: %s\n"
			ANSI_DEFAULT, name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, tRED "файл %s не является обычным файлом\n"
			ANSI_DEFAULT, name);
	else if (st.st_size != sizeof(struct md5_hash))
		fprintf(stderr, tRED "файл %s имеет неверный размер\n"
			ANSI_DEFAULT, name);
	else{
		int fd = open(name, O_RDONLY);
		if (fd == -1){
			fprintf(stderr, tRED "ошибка открытия %s для чтения: %s\n"
				ANSI_DEFAULT, name, strerror(errno));
			return false;
		}else{
			if (read(fd, number, sizeof(*number)) == sizeof(*number))
				ret = true;
			else
				fprintf(stderr, tRED "ошибка чтения из %s: %s\n"
					ANSI_DEFAULT, name, strerror(errno));
			close(fd);
		}
	}
	return ret;
}

/* Поиск заданного заводского номера в списке */
bool check_term_number(const struct md5_hash *number,
	const struct fuzzy_md5 *known_numbers, int n)
{
	bool ret = false;
	int i;
	for (i = 0; i < n; i++){
		const struct fuzzy_md5 *p = known_numbers + i;
		if ((p->a == number->a) && (p->b == number->b) &&
				(p->c == number->c) && (p->d == number->d)){
			ret = true;
			break;
		}
	}
	return ret;
}

/* Вывод на экран сообщения об успешной разблокировке терминала */
void print_unlock_msg(const char *name)
{
	printf(CLR_SCR);
	printf(ANSI_PREFIX "10;H" tWHT bBlu
"                                                                               \n"
"  ╔═════════════════════════════════════════════════════════════════════════╗  \n"
"  ║" tGRN
   "                        Терминал разблокирован.                          "
tWHT "║  \n"
"  ║" tCYA
   "        Теперь вы можете установить на этот терминал лицензию %s        "
tWHT "║  \n"
"  ║" tCYA
   "                            обычным образом.                             "
tWHT "║  \n"
"  ╚═════════════════════════════════════════════════════════════════════════╝  \n"
"                                                                               \n"
		ANSI_DEFAULT, name);
}
