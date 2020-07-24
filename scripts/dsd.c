/*
 * �⥭�� ���祩 DS1990A. (c) gsr 2004-2018.
 * �ணࠬ�� ��ਮ���᪨ �⠥� ���� DS1990A �१ COM-����
 * � ��࠭�� ��⠭��� ���祭�� � ࠧ���塞�� ᥣ���� �����.
 * ��� ����㯠 � ����� �ᯮ������ ᥬ���.
 * �� ���� �ணࠬ�� ��⮬���᪨ ��室�� ���� COM-����, � ���஬�
 * ������祭 ���� DS1990A � ࠡ�⠥� � ��� (���� �.�.�䠭��쥢�).
 * 17.03.05: �᫨ �� ���� �ணࠬ�� �� �����㦥� ���� DALLAS,
 * ������祭�� � COM-�����, �ணࠬ�� ��⠥��� ࠡ���� � /dev/dallas.
 * 01.10.08: ��� �ࠢ��쭮� ࠡ��� � ���뢠⥫ﬨ, ������砥�묨 �� USB,
 * ����� 6-��⭮�� ०��� ࠡ��� � COM-���⮬ ������ 7-����, �.�. �� ��
 * ��������� USB-COM �����ন���� 6-���� ०��.
 * 02.08.10: ��������� �����প� ��������, �ࠢ�塞��� ᨣ����� DTR COM-����,
 * � ���஬� ������祭 ���뢠⥫� DS1990A.
 * 11.11.10: �� ����⨨ ��᫥����⥫쭮�� ���� DTR ���뢠���� ⮫쪮 ���
 * �८�ࠧ���⥫�� USB-COM, �.�. ��� ॠ���� COM-���⮢ DTR �ᯮ������
 * � �奬� ���뢠⥫� DS1990A. �� �⮬ �।����������, �� ������� ����� ����
 * ������祭 ⮫쪮 � ����㠫쭮�� COM-�����.
 * 01.10.18: ⥯��� �ணࠬ�� ���� ��ॡ�ࠥ� 8 ttySx � 8 ttyUSBx �� �����㦥���
 * ���뢠⥫�.
 */

#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/times.h>
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
#include "colortty.h"
#include "ds1990a.h"
#include "serial.h"

/* ��ਮ��筮��� �⥭�� ���� */
#define SCAN_INTERVAL	10

/* ���ᨬ��쭮� �᫮ ��᫥����⥫��� �訡�� �⥭�� DS1990A */
#define DS_MAX_FAULTS		3

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

/* ���� ���ன�⢠ ��� �⥭�� DS1990A */
static char ds_dev_name[32];
/* ��᫮ ������ � ����㠫��� COM-���⮢ ��� ᪠��஢���� */
#define NR_PORTS		16
/* ��᫮ COM-���⮢ ��� ᪠��஠��� */
#define NR_COMS			8

/* ���ன�⢮ ��� �⥭�� DS1990A */
static int ds_dev = -1;

static bool loop_flag = true;

/* �������� ᥣ���� ࠧ���塞�� ����� */
static void release_shm(void)
{
	struct shmid_ds buf;
	if (shmid != -1){
		if (shmptr != SH_NULL){
			shmdt(shmptr);
			shmptr = SH_NULL;
		}
		shmctl(shmid, IPC_RMID, &buf);
		shmid = -1;
	}
}

/* �������� ᥣ���� ࠧ���塞�� ����� */
static bool create_shm(void)
{
	shmid = shmget(ftok(FTOK_FILE, 0), SHM_SIZE, IPC_CREAT | IPC_EXCL);
	if (shmid == -1){
		fprintf(stderr, "�訡�� ᮧ����� ᥣ���� ࠧ���塞�� �����: %s.\n",
			strerror(errno));
		return false;
	}
	shmptr = (uint8_t *)shmat(shmid, (const void *)0, 0);
	if (shmptr == SH_NULL){
		fprintf(stderr, "�訡�� ����祭�� 㪠��⥫� �� ࠧ���塞�� ������: %s.\n",
			strerror(errno));
		release_shm();
		return false;
	}else
		return true;
}

/* �������� ᥬ��� */
static void free_sem(void)
{
	if (semid != -1){
		semctl(semid, 0, IPC_RMID, 0);
		semid = -1;
	}
}

/* �������� ᥬ��� */
static bool create_sem(void)
{
	semid = semget(ftok(FTOK_FILE, 0), 1, IPC_CREAT | IPC_EXCL);
	if (semid == -1){
		fprintf(stderr, "�訡�� ᮧ����� ᥬ���: %s.\n",
			strerror(errno));
		return false;
	}else
		return true;
}

/* ��墠� ᥬ��� */
static bool get_sem(void)
{
	struct sembuf sb[] = {
		{0, 0, 0},		/* ��������� �᢮�������� ᥬ��� */
		{0, 1, 0},		/* ��墠��� ᥬ��� */
	};
	return semop(semid, sb, ASIZE(sb)) == 0;
}

/* �᢮�������� ᥬ��� */
static bool release_sem(void)
{
	struct sembuf sb[] = {
		{0, -1, 0},
	};
	return semop(semid, sb, ASIZE(sb)) == 0;
}

/* ������ � ᥣ���� ࠧ���塞�� ����� �����䨪��� DS1990A */
static bool write_ds_id(uint8_t *id)
{
	if (get_sem()){
		memcpy(shmptr, id, 8);
		release_sem();
		return true;
	}else
		return false;
}

/* ���樠������ ������� IPC */
static bool init_ipc(void)
{
	if (create_shm()){
		if (create_sem())
			return true;
		else
			release_shm();
	}
	return false;
}

/* �᢮�������� ������� IPC */
static void release_ipc(void)
{
	release_shm();
	free_sem();
}

/* ��⠭���� ᪮��� COM-���� � ����� ᨬ���� */
static bool set_com(int baud, int csize)
{
	if (ds_dev != -1){
		struct serial_settings ss = {
			.csize		= csize,
			.parity		= SERIAL_PARITY_NONE,
			.stop_bits	= SERIAL_STOPB_1,
			.control	= SERIAL_FLOW_NONE,
			.baud		= baud
		};
		return serial_configure(ds_dev, &ss);
	}else
		return false;
}

/* ��।������ ࠧ���� �� �६��� */
static uint32_t time_diff(uint32_t t0)
{
	return u_times() - t0;
}

/* �஢�ઠ ������⢨� ���� DS1990A */
static bool is_ds_present(void)
{
	uint32_t t0 = u_times();
	uint8_t b = 0xf0;
	if (!set_com(B9600, CS8))
		return false;
/* ������ ���� */
	while (loop_flag && (time_diff(t0) < SCAN_INTERVAL)){
		if (write(ds_dev, &b, 1) == 1)
			break;
		else if (errno != EWOULDBLOCK){
			printf("errno = %d\n", errno);
			return false;
		}
	}
/* �⥭�� ���� */
	while (loop_flag && (time_diff(t0) < SCAN_INTERVAL)){
		if (read(ds_dev, &b, 1) == 1)
			return b != 0xf0;
		else if (errno != EWOULDBLOCK)
			break;
	}
	return false;
}

/* �⥭�� ���⮢ ������ DS1990A */
static bool read_ds_bytes(uint8_t *b, uint8_t wb)
{
	uint32_t t0 = u_times();
	uint8_t data[72] = {[8 ... sizeof(data) - 1] = 0x7f};
	int i, j, ret;
	for (i = 0; i < 8; i++){
		data[i] = (wb & 1) ? 0x7f : 0;
		wb >>= 1;
	}
	if (!set_com(B115200, CS7) ||
			(write(ds_dev, data, sizeof(data)) != sizeof(data)))
		return false;
	i = 0;
	while (loop_flag && (time_diff(t0) < SCAN_INTERVAL) && (i < sizeof(data))){
		ret = read(ds_dev, data + i, sizeof(data) - i);
		if (ret > 0)
			i += ret;
		else if (errno != EWOULDBLOCK)
			return false;
	}
	if (i == sizeof(data)){
		for (i = 0; i < 8; i++){
			for (j = 0; j < 8; j++){
				b[i] >>= 1;
				if (data[(i + 1) * 8 + j] == 0x7f)
					b[i] |= 0x80;
			}
		}
 		return true;
	}else
		return false;
}

/* �⥭�� ����� DS1990A � ������� COM-���� */
static bool read_ds_number_com(uint8_t *buf)
{
	tcflush(ds_dev, TCIOFLUSH);
	return is_ds_present() && read_ds_bytes(buf, 0x33 /* sic */);
}

/* �⥭�� ����� ���� � ������� /dev/dallas */
static bool read_ds_number_ioctl(uint8_t *buf)
{
	memset(buf, 0, 8);
	return ioctl(ds_dev, DALLAS_IO_READ, buf) == 0;
}

static bool (*read_ds_number)(uint8_t *) = read_ds_number_com;

/* ������ ����஫쭮� �㬬� DALLAS */
static uint8_t ds_crc(uint8_t *buf)
{
#define DOW_B0	0
#define DOW_B1	4
#define DOW_B2	5
	uint8_t b, s = 0;
	uint8_t m0 = 1 << (7-DOW_B0);
	uint8_t m1 = 1 << (7-DOW_B1);
	uint8_t m2 = 1 << (7-DOW_B2);
	int i, j;
	if (buf != NULL){
		for (i = 0; i < 8; i++){
			b = buf[i];
			for (j = 0; j < 8; j++){
				b ^= (s & 0x01);
				s >>= 1;
				if (b & 0x01)
					s ^= (m0 | m1 | m2);
				b >>= 1;
			}
		}
	}
	return s;
}

/*
 * �᫨ � ��⠭��� ����� DALLAS �� ����� ࠢ�� 0, � CRC ᮢ�����,
 * �� �� ������ १����.
 */
static bool ds_zero_id(uint8_t *buf)
{
	int i;
	for (i = 0; i < 8; i++)
		if (buf[i] != 0)
			return false;
	return true;
}

#if 0
/* ����� ����� ��⮭� (��� �⫠����� 楫��) */
static void print_ds_number(uint8_t *number)
{
	static int map[] = {7, 0, 6, 5, 4, 3, 2, 1};
	int i;
	for (i = 0; i < 8; i++){
		printf("%.2hhx", number[map[i]]);
		if (i < 7)
			putchar(' ');
	}
	putchar('\n');
}
#endif

/* ���� ��ࢮ�� COM-����, � ���஬� ������祭 ���� DS1990A */
static bool scan_ports(uint8_t *number)
{
	int i;
	uint32_t t0, dt;
	bool found = false;
	unlink(SPEAKER_DEV);
	printf(tYLW "���� DS1990A: " ANSI_DEFAULT);
	for (i = 0; (i < NR_PORTS) && !found; i++){
		bool usb = i >= NR_COMS, fault = false;
		if (usb)
			snprintf(ds_dev_name, sizeof(ds_dev_name), "/dev/ttyUSB%d",
				i - NR_COMS);
		else
			snprintf(ds_dev_name, sizeof(ds_dev_name), "/dev/ttyS%d", i);
		printf(tGRN "%s" ANSI_DEFAULT, ds_dev_name);
		fflush(stdout);
		t0 = u_times();
		ds_dev = open(ds_dev_name, O_RDWR);
		if (ds_dev > 0){
			if (usb)
				serial_ch_lines(ds_dev, TIOCM_DTR, false);
			if (fcntl(ds_dev, F_SETFL, O_NONBLOCK) == 0){
				if (read_ds_number(number) &&
						!ds_zero_id(number) &&
						(ds_crc(number) == 0)){
					if (usb)
						symlink(ds_dev_name, SPEAKER_DEV);
					found = true;
				}
			}else
				fault = true;
			if (!found)
				close(ds_dev);
		}else
			fault = true;
		if (!found){
			printf("\b\b\b\b\b\b\b\b\b\b");
			if (usb)
				printf("\b\b");
			dt = SCAN_INTERVAL - (u_times() - t0);
			if (dt < SCAN_INTERVAL)
				usleep(dt * 10000);
		}
		if (fault){
			if (usb)
				break;
			else
				i = NR_COMS - 1;
		}
	}
	if (found)
		putchar('\n');
	return found;
}

/* ���樠������ */
static bool init_dsd(void)
{
	return init_ipc();
}

/* �᢮�������� ����ᮢ */
static void release_dsd(void)
{
	release_ipc();
	if (ds_dev != -1){
		close(ds_dev);
		ds_dev = -1;
	}
}

static void sigterm_handler(int n __attribute__((unused)))
{
	loop_flag = false;
}

int main()
{
	uint32_t t0, dt;
	static uint8_t null_id[8] = {[0 ... 7] = 0};
	int n_faults = 0;
	uint8_t number[8];
	if (!scan_ports(number)){
		ds_dev = open(DALLAS_DEV_NAME, O_RDONLY);
		if (ds_dev == -1){
			printf(tRED "�� ������   " ANSI_DEFAULT "\n");
			return 1;
		}else{
			printf(tGRN "������ BSC " ANSI_DEFAULT "\n");
			read_ds_number = read_ds_number_ioctl;
		}
	}
	if (init_dsd()){
		daemon(0, 0);
		signal(SIGTERM, sigterm_handler);
		write_ds_id(number);
		while (loop_flag){
			t0 = u_times();
			if (read_ds_number(number) && !ds_zero_id(number) &&
					(ds_crc(number) == 0)){
				n_faults = 0;
				write_ds_id(number);
			}else if (++n_faults > DS_MAX_FAULTS){
				n_faults = 0;
				write_ds_id(null_id);
			}
			dt = SCAN_INTERVAL - (u_times() - t0);
			if (dt < SCAN_INTERVAL)
				usleep(dt * 10000);
		}
		release_dsd();
		return 0;
	}else
		return 1;
}
