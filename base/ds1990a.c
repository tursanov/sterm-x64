/* ����� � ���砬� DS1990A. (c) gsr 2000-2004. */

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

/* ���� ��� ᮧ����� key_t � ������� ftok */
#define FTOK_FILE	"/home/sterm/dsd"

/* ������ ࠧ���塞�� ����� */
#define SHM_SIZE	4096
/* �����䨪��� ᥣ���� ࠧ���塞�� ����� */
static int shmid = -1;
/* �����⥫� �� ᥣ���� ࠧ���塞�� ����� */
static uint8_t *shmptr = SH_NULL;
/* ������ ��� ����㯠 � ࠧ���塞�� ����� */
static int semid = -1;

/* �������� ᥣ���� ࠧ���塞�� ����� */
static bool create_shm(void)
{
	shmid = shmget(ftok(FTOK_FILE, 0), SHM_SIZE, 0);
	if (shmid == -1){
		fprintf(stderr, "�訡�� ᮧ����� ᥣ���� ࠧ���塞�� �����: %s.\n",
			strerror(errno));
		return false;
	}
	shmptr = (uint8_t *)shmat(shmid, (const void *)0, 0);
	if (shmptr == SH_NULL){
		fprintf(stderr, "�訡�� ����祭�� 㪠��⥫� ��� ࠧ���塞�� �����: %s.\n",
			strerror(errno));
		return false;
	}else
		return true;
}

/* �������� ᥬ��� */
static bool create_sem(void)
{
	semid = semget(ftok(FTOK_FILE, 0), 1, 0);
	if (semid == -1){
		fprintf(stderr, "�訡�� ᮧ����� ᥬ���: %s.\n",
			strerror(errno));
		return false;
	}else
		return true;
}

/* ��墠� ᥬ��� */
static bool ds_get_sem(void)
{
	struct sembuf sb[] = {
		{0, 0, 0},		/* ��������� �᢮�������� ᥬ��� */
		{0, 1, 0},		/* ��墠��� ᥬ��� */
	};
	return semid == -1 ? false : semop(semid, sb, ASIZE(sb)) == 0;
}

/* �᢮�������� ᥬ��� */
static bool ds_release_sem(void)
{
	struct sembuf sb[] = {
		{0, -1, 0},
	};
	return semid == -1 ? false : semop(semid, sb, ASIZE(sb)) == 0;
}

/* ��砫� ࠡ��� � ���뢠⥫�� */
bool ds_init(void)
{
	return create_shm() && create_sem();
}

/* �⥭�� ����� ���� DS1990A */
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

/* ��।������ �� ��� ��������� ����� DS1990A */
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

/* �८�ࠧ������ ⨯� ���� � ᨬ���쭮�� ���� */
char ds_key_char(int kt)
{
	switch (kt){
		case key_dbg:
			return '�';
		case key_srv:
			return '�';
		case key_reg:
			return '�';
		default:
			return '?';
	}
}
