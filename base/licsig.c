/* Работа с признаками удаленных лицензий ИПТ/ППУ. (c) gsr 2011, 2018 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "licsig.h"

/* Буфер для работы с MBR */
static uint8_t mbr[SECTOR_SIZE];
/* MBR прочитан */
static bool mbr_read = false;

/* Определение имени файла устройства, смонтированного в заданную точку */
#define PROC_MOUNTS		"/proc/mounts"
static const char *get_partition(const char *mnt)
{
	const char *ret = NULL;
	FILE *f = fopen(PROC_MOUNTS, "r");
	if (f != NULL){
		static char str[1024];
		while(fgets(str, sizeof(str), f) != NULL){
			static char dev[32], mpt[256];
			if (sscanf(str, "%s %s", dev, mpt) == 2){
				if (strcmp(mpt, mnt) == 0){
					ret = dev;
					break;
				}
			}
		}
		fclose(f);
	}
	return ret;
}

/* Определение имени файла устройства, на котором находится заданный раздел */
static const char *get_partition_dev(const char *part)
{
	const char *ret = NULL;
	static char name[1024];
	int i, l = strlen(part);
	if (l < sizeof(name)){
		strcpy(name, part);
		for (i = l; i > 0; i--){
			if (!isdigit(name[i - 1]))
				break;
		}
		name[i] = 0;
		ret = name;
	}
	return ret;
}

/* Открытие устройства для чтения/записи признаков удаления лицензий ИПТ/ППУ */
static int open_term_dev(void)
{
	int ret = -1;
	const char *dev = get_partition("/home");
	if (dev != NULL){
		dev = get_partition_dev(dev);
		if (dev != NULL)
			ret = open(dev, O_RDWR);
	}
	return ret;
}

/* Чтение MBR носителя терминала */
static bool read_mbr(void)
{
	bool ret = false;
	int dev = open_term_dev();
	if (dev != -1){
		ret =	(lseek(dev, 0, SEEK_SET) == 0) &&
			(read(dev, mbr, sizeof(mbr)) == sizeof(mbr));
		mbr_read = ret;
		close(dev);
	}
	return ret;
}

/* Чтение MBR при необходимости */
static bool read_mbr_if_need(void)
{
	bool ret = true;
	if (!mbr_read)
		ret = read_mbr();
	return ret;
}

/* Проверка присутствия признака удаления лицензии */
bool check_lic_signature(off_t offs, uint16_t sig)
{
	if (read_mbr_if_need())
		return *(uint16_t *)(mbr + offs) == sig;
	else
		return true;
}

/* Запись MBR носителя терминала */
static bool write_mbr(void)
{
	bool ret = false;
	int dev = open_term_dev();
	if (dev != -1){
		ret =	(lseek(dev, 0, SEEK_SET) == 0) &&
			(write(dev, mbr, sizeof(mbr)) == sizeof(mbr));
		close(dev);
	}
	return ret;
}

/* Запись признака удаления лицензии */
bool write_lic_signature(off_t offs, uint16_t sig)
{
	bool ret = false;
	if (read_mbr_if_need()){
		*(uint16_t *)(mbr + offs) = sig;
		ret = write_mbr();
	}
	return ret;
}

/* Очистка признака удаления лицензии */
bool clear_lic_signature(off_t offs, uint16_t sig)
{
	bool ret = false;
	if (read_mbr_if_need()){
		uint16_t *p = (uint16_t *)(mbr + offs);
		if (*p == sig)
			*p = 0;
		ret = write_mbr();
	}
	return ret;
}
