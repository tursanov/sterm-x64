/*
 * ��������஢�� �ନ���� ��᫥ 㤠����� � ���� ��業��� ���/���. (c) gsr 2011
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "unlock.h"

/* �⥭�� �� �����᪮�� ����� �ନ���� */
bool read_term_number(const char *name, struct md5_hash *number)
{
	bool ret = false;
	struct stat st;
	if (stat(name, &st) == -1)
		fprintf(stderr, tRED "�訡�� ����祭�� ���ଠ樨 � 䠩�� %s: %s\n"
			ANSI_DEFAULT, name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, tRED "䠩� %s �� ���� ����� 䠩���\n"
			ANSI_DEFAULT, name);
	else if (st.st_size != sizeof(struct md5_hash))
		fprintf(stderr, tRED "䠩� %s ����� ������ ࠧ���\n"
			ANSI_DEFAULT, name);
	else{
		int fd = open(name, O_RDONLY);
		if (fd == -1){
			fprintf(stderr, tRED "�訡�� ������ %s ��� �⥭��: %s\n"
				ANSI_DEFAULT, name, strerror(errno));
			return false;
		}else{
			if (read(fd, number, sizeof(*number)) == sizeof(*number))
				ret = true;
			else
				fprintf(stderr, tRED "�訡�� �⥭�� �� %s: %s\n"
					ANSI_DEFAULT, name, strerror(errno));
			close(fd);
		}
	}
	return ret;
}

/* ���� ��������� �����᪮�� ����� � ᯨ᪥ */
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

/* �뢮� �� �࠭ ᮮ�饭�� �� �ᯥ譮� ࠧ�����஢�� �ନ���� */
void print_unlock_msg(const char *name)
{
	printf(CLR_SCR);
	printf(ANSI_PREFIX "10;H" tWHT bBlu
"                                                                               \n"
"  �������������������������������������������������������������������������ͻ  \n"
"  �" tGRN
   "                        ��ନ��� ࠧ�����஢��.                          "
tWHT "�  \n"
"  �" tCYA
   "        ������ �� ����� ��⠭����� �� ��� �ନ��� ��業��� %s        "
tWHT "�  \n"
"  �" tCYA
   "                            ����� ��ࠧ��.                             "
tWHT "�  \n"
"  �������������������������������������������������������������������������ͼ  \n"
"                                                                               \n"
		ANSI_DEFAULT, name);
}
