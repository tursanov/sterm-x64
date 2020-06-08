/*
 * �������� ��業��� ���/��� � �ନ���� � �ନ஢���� ���� 㤠�����.
 * �� 㤠����� � MBR �����뢠���� ᯥ樠��� 䫠�, � ���짮��⥫�
 * �뤠���� 32-���筮� ��⭠����筮� �᫮. ��᫮ �ନ�����
 * �� �᭮�� ⥪�饩 ����, ⠡���� ��砩��� �ᥫ (rndtbl.c) �
 * ��� �����᪮�� ����� �ନ����.
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

/* �������� 䠩�� ��業��� */
bool rm_lic_file(const char *name, const char *lic_name)
{
	bool ret = false;
	struct stat st;
	if (stat(name, &st) == -1){
		if (errno == ENOENT)
			ret = true;
		else
			fprintf(stderr, tRED
				"�訡�� ����祭�� ���ଠ樨 � ��業��� %s: %s.\n"
				ANSI_DEFAULT, lic_name, strerror(errno));
	}else if (!S_ISREG(st.st_mode))
		fprintf(stderr, tRED "������ �ଠ� ��業��� %s.\n"
			ANSI_DEFAULT, lic_name);
	else if (unlink(name) == -1)
		fprintf(stderr, tRED "�訡�� 㤠����� ��業��� %s: %s.\n"
			ANSI_DEFAULT, lic_name, strerror(errno));
	else
		ret = true;
	return ret;
}

/* �⥭�� �� �����᪮�� ����� �ନ���� */
static bool read_term_number(char *name, struct md5_hash *number)
{
	bool ret = false;
	struct stat st;
	if (stat(name, &st) == -1)
		fprintf(stderr, tRED "�訡�� ����祭�� ���ଠ樨 � 䠩�� %s: %s.\n"
			ANSI_DEFAULT, name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, tRED "%s �� ���� ����� 䠩���.\n"
			ANSI_DEFAULT, name);
	else if (st.st_size != sizeof(struct md5_hash))
		fprintf(stderr, tRED "���� %s ����� ������ ࠧ���.\n"
			ANSI_DEFAULT, name);
	else{
		int fd = open(name, O_RDONLY);
		if (fd == -1)
			fprintf(stderr, tRED "�訡�� ������ %s ��� �⥭��: %s.\n"
				ANSI_DEFAULT, name, strerror(errno));
		else{
			if (read(fd, number, sizeof(*number)) == sizeof(*number))
				ret = true;
			else
				fprintf(stderr, tRED "�訡�� �⥭�� �� %s: %s.\n"
					ANSI_DEFAULT, name, strerror(errno));
			close(fd);
		}
	}
	return ret;
}

/* ��ନ஢���� ���� 㤠����� ��業��� ��� ��������� �����᪮�� ����� �ନ���� */
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

/* ��ନ஢���� ���� 㤠����� ��業��� */
bool mk_del_hash(uint8_t *buf, const struct rnd_rec *rnd_tbl, int n)
{
	bool ret = false;
	struct md5_hash number;
	if (read_term_number(USB_BIND_FILE, &number))
		ret = mk_del_hash_for_tnumber(buf, &number, rnd_tbl, n);
	return ret;
}

/* �뢮� ���� 㤠����� ��業��� �� �࠭ */
void print_del_hash(const uint8_t *del_hash, const char *lic_name)
{
	int i;
	printf(CLR_SCR);
	printf(ANSI_PREFIX "12;H" tCYA
"                   ��業��� %s 㤠���� � ��襣� �ନ����.\n"
"   ��� ���⢥ত���� 㤠����� ᮮ���, ��������, �����, 㪠����� ����,\n"
"                             � ��� ��� \"������\".\n", lic_name);
	printf(ANSI_PREFIX "16;20H" tYLW);
	for (i = 0; i < 16; i++){
		if ((i > 0) && (i < 15) && ((i % 2) == 0))
			putchar('-');
		printf("%.2hhX", del_hash[i]);
	}
	printf(ANSI_DEFAULT "\n");
}

/* ���⪠ �室���� ��⮪� */
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

/* ���⪠ �室���� ��⮪� � �⥭�� �� ���� ᨬ���� */
static int flush_getchar(void)
{
	flush_stdin();
	return getchar();
}

/* ����� ���⢥ত���� 㤠����� ��業��� */
bool ask_rmlic_confirmation(const char *lic_name)
{
	int c;
	while (true){
		printf(CLR_SCR);
		printf(	ANSI_PREFIX "10;H" tYLW
"      �� ����⢨⥫쭮 ��� 㤠���� ��業��� %s � ������� �ନ���� ?\n"
			ANSI_PREFIX "11;30H" tGRN "1" tYLW " -- ��;\n"
			ANSI_PREFIX "12;30H" tGRN "2" tYLW " -- ���\n"
			ANSI_PREFIX "13;30H" tWHT "��� �롮�: " ANSI_DEFAULT,
			lic_name);
		c = flush_getchar();
		switch (c){
			case '1':
				return true;
			case '2':
				printf(CLR_SCR ANSI_PREFIX "12;23H");
				printf(tRED "��業��� %s �� �㤥� 㤠����\n"
					ANSI_DEFAULT, lic_name);
				return false;
		}
	}
}
