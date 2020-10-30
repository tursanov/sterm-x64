/* Сравнение двух файлов или каталогов. (c) gsr 2008, 2020 */

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sysdefs.h"

/* Сравнение двух файлов */
static bool cmp_files(const char *path1, const char *path2)
{
	struct stat st1, st2;
	if (stat(path1, &st1) == -1){
		fprintf(stderr, "Ошибка получения информации о %s: %m.\n", path1);
		return false;
	}else if (!S_ISREG(st1.st_mode)){
		fprintf(stderr, "%s не является обычным файлом.\n", path1);
		return false;
	}else if (stat(path2, &st2) == -1){
		fprintf(stderr, "Ошибка получения информации о %s: %m.\n", path2);
		return false;
	}else if (!S_ISREG(st2.st_mode)){
		fprintf(stderr, "%s не является обычным файлом.\n", path2);
		return false;
	}else if (st1.st_size != st2.st_size){
		fprintf(stderr, "%s и %s имеют разные размеры.\n", path1, path2);
		return false;
	}
	int fd1 = open(path1, O_RDONLY);
	if (fd1 == -1){
		fprintf(stderr, "Ошибка открытия %s для чтения: %m.\n", path1);
		return false;
	}
	int fd2 = open(path2, O_RDONLY);
	if (fd2 == -1){
		fprintf(stderr, "Ошибка открытия %s для чтения: %m.\n", path2);
		close(fd1);
		return false;
	}
	bool ret = false;
	uint32_t offs = 0;
	while (true){
#define CMP_BUF_SIZE	4096
		static uint8_t data1[CMP_BUF_SIZE];
		static uint8_t data2[CMP_BUF_SIZE];
		ssize_t len1 = read(fd1, data1, sizeof(data1));
		if (len1 < 0){
			fprintf(stderr, "Ошибка чтения из %s: %m.\n", path1);
			break;
		}
		ssize_t len2 = read(fd2, data2, sizeof(data2));
		if (len2 < 0){
			fprintf(stderr, "Ошибка чтения из %s: %m.\n", path2);
			break;
		}else if (len1 != len2){
			fprintf(stderr, "Несовпадение размеров %s и %s при чтении.\n",
				path1, path2);
			break;
		}else if (len1 == 0){
			ret = true;
			break;
		}
		for (ssize_t i = 0; len1 > 0; i++, offs++, len1--){
			if (data1[i] != data2[i]){
				fprintf(stderr, "Несовпадение %s и %s по смещению 0x%.8x.\n",
					path1, path2, offs);
				break;
			}
		}
		if (len1 > 0)
			break;
	}
	if (close(fd1) == -1)
		fprintf(stderr, "Ошибка закрытия %s после чтения: %m.\n", path1);
	if (close(fd2) == -1)
		fprintf(stderr, "Ошибка закрытия %s после чтения: %m.\n", path2);
	return ret;
}

/* Максимальная глубина вложенности каталогов */
#define MAX_DEPTH		100

static int max_depth = 0;

/* Сравнение двух каталогов */
static bool cmp_dirs(const char *dir1, const char *dir2, int depth)
{
	bool ret = false;
	if (depth >= MAX_DEPTH){
		fprintf(stderr, "Превышение максимальной глубины вложенности сравниваемых каталогов.\n");
		return false;
	}else if (depth > max_depth)
		max_depth = depth;
/* Сканируем первый каталог */
	DIR *dir = opendir(dir1);
	if (dir == NULL){
		fprintf(stderr, "Ошибка открытия каталога %s: %m.\n", dir1);
		return false;
	}
	char *path1 = malloc(PATH_MAX);
	if (path1 == NULL){
		fprintf(stderr, "Ошибка выделения памяти: %m.\n");
		closedir(dir);
		return false;
	}
	char *path2 = malloc(PATH_MAX);
	if (path2 == NULL){
		fprintf(stderr, "Ошибка выделения памяти: %m.\n");
		free(path1);
		closedir(dir);
		return false;
	}
	while (true){
		struct dirent *de = readdir(dir);
		if (de == NULL){
			ret = (errno == 0);
			if (!ret)
				fprintf(stderr, "Ошибка чтения каталога %s: %m.\n", dir1);
			break;
		}else if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0))
			continue;
		snprintf(path1, PATH_MAX, "%s/%s", dir1, de->d_name);
		snprintf(path2, PATH_MAX, "%s/%s", dir2, de->d_name);
		struct stat st;
		if (stat(path1, &st) == -1){
			fprintf(stderr, "#1: Ошибка получения данных о %s: %m.\n", path1);
			break;
		}else if (S_ISREG(st.st_mode)){
			if (!cmp_files(path1, path2))
				break;
		}else if (S_ISDIR(st.st_mode)){
			if (!cmp_dirs(path1, path2, depth + 1))
				break;
		}
	}
	closedir(dir);
	if (!ret){
		free(path1);
		free(path2);
		return false;
	}
/* Сканируем второй каталог */
	ret = false;
	dir = opendir(dir2);
	if (dir == NULL){
		fprintf(stderr, "Ошибка открытия каталога %s: %m.\n", dir2);
		free(path1);
		free(path2);
		return false;
	}
	while (true){
		struct dirent *de = readdir(dir);
		if (de == NULL){
			ret = (errno == 0);
			if (!ret)
				fprintf(stderr, "Ошибка чтения каталога %s: %m.\n", dir1);
			break;
		}else if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0))
			continue;
		snprintf(path1, PATH_MAX, "%s/%s", dir1, de->d_name);
		snprintf(path2, PATH_MAX, "%s/%s", dir2, de->d_name);
		struct stat st1, st2;
		if (stat(path1, &st1) == -1){
			fprintf(stderr, "#2: Ошибка получения данных о %s: %m.\n", path1);
			break;
		}else if (stat(path2, &st2) == -1){
			fprintf(stderr, "#3: Ошибка получения данных о %s: %m.\n", path2);
			break;
		}else if (st1.st_mode != st2.st_mode){
			fprintf(stderr, "%s и %s имеют различный тип.\n", path1, path2);
			break;
		}
	}
	free(path1);
	free(path2);
	closedir(dir);
	return ret;
}

/* Вывод справки об использовании программы */
static void usage(void)
{
	printf("Программа сравнения двух каталогов.\n");
	printf("Использование: dircmp <dir1> <dir2>.\n");
}

int main(int argc, char **argv)
{
	int ret = 1;
	struct stat st;
	if (argc != 3)
		usage();
	else if (stat(argv[1], &st) == -1)
		fprintf(stderr, "Ошибка получения сведений о %s: %m.\n", argv[1]);
	else if (S_ISREG(st.st_mode)){
		if (cmp_files(argv[1], argv[2]))
			ret = 0;
	}else if (cmp_dirs(argv[1], argv[2], 0))
		ret = 0;
	if (ret == 0)
		printf("max depth: %d.\n", max_depth);
	return ret;
}
