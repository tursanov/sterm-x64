/* Работа с ключами DS1990A. (c) gsr 2000-2004. */

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/user.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "sys/ioctls.h"
#include "ds1990a.h"

#define SH_NULL		(void *)(-1)

/* Файл для создания key_t с помощью ftok */
#define FTOK_FILE	"/home/sterm/dsd"

/* Размер разделяемой памяти */
#define SHM_SIZE	4096
/* Идентификатор сегмента разделяемой памяти */
static int shmid = -1;
/* Указатель на сегмент разделяемой памяти */
static uint8_t *shmptr = SH_NULL;
/* Семафор для доступа к разделяемой памяти */
static int semid = -1;

/* Создание сегмента разделяемой памяти */
static bool create_shm(void)
{
	shmid = shmget(ftok(FTOK_FILE, 0), SHM_SIZE, 0);
	if (shmid == -1){
		fprintf(stderr, "Ошибка создания сегмента разделяемой памяти: %s.\n",
			strerror(errno));
		return false;
	}
	shmptr = (uint8_t *)shmat(shmid, (const void *)0, 0);
	if (shmptr == SH_NULL){
		fprintf(stderr, "Ошибка получения указателя для разделяемой памяти: %s.\n",
			strerror(errno));
		return false;
	}else
		return true;
}

/* Создание семафора */
static bool create_sem(void)
{
	semid = semget(ftok(FTOK_FILE, 0), 1, 0);
	if (semid == -1){
		fprintf(stderr, "Ошибка создания семафора: %s.\n",
			strerror(errno));
		return false;
	}else
		return true;
}

/* Захват семафора */
static bool ds_get_sem(void)
{
	struct sembuf sb[] = {
		{0, 0, 0},		/* дождаться освобождения семафора */
		{0, 1, 0},		/* захватить семафор */
	};
	return semid == -1 ? false : semop(semid, sb, ASIZE(sb)) == 0;
}

/* Освобождение семафора */
static bool ds_release_sem(void)
{
	struct sembuf sb[] = {
		{0, -1, 0},
	};
	return semid == -1 ? false : semop(semid, sb, ASIZE(sb)) == 0;
}

/* Начало работы со считывателем */
bool ds_init(void)
{
	return create_shm() && create_sem();
}

/* Чтение номера ключа DS1990A */
bool ds_read(ds_number dsn)
{
	int i;
	bool flag = false;
	if (ds_get_sem()){
		memcpy(dsn, shmptr, DS_NUMBER_LEN);
		ds_release_sem();
		for (i = 0; (i < DS_NUMBER_LEN) && !dsn[i]; i++){}
		flag = i < DS_NUMBER_LEN;
	}else
		memset(dsn, 0, DS_NUMBER_LEN);
	return flag;
}

/* Определение хеша для заданного номера DS1990A */
bool ds_hash(ds_number dsn, struct md5_hash *md5)
{
	ds_number tmp;
	int i, j;
	if (md5 == NULL)
		return false;
	memcpy(tmp, dsn, DS_NUMBER_LEN);
	for (i = 0; i < 3; i++){
		for (j = 0; j < DS_NUMBER_LEN; j++)
			tmp[(j + 1) % DS_NUMBER_LEN] ^= ~tmp[j];
	}
	return get_md5(tmp, DS_NUMBER_LEN, md5);
}

/* Преобразование типа ключа к символьному виду */
char ds_key_char(int kt)
{
	switch (kt){
		case key_dbg:
			return 'О';
		case key_srv:
			return 'П';
		case key_reg:
			return 'Р';
		default:
			return '?';
	}
}
