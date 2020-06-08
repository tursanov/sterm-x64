/* ��⠭���� �� �ନ��� ��業��� ���. (c) gsr 2005, 2011, 2019 */

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

/* �⥭�� 䠩�� ��業��� */
static int read_license_file(const char *name, struct bank_license_info *licenses)
{
	struct stat st;
	div_t d;
	int fd, ret = -1;
	if (stat(name, &st) == -1)
		fprintf(stderr, "�訡�� ����祭�� ���ଠ樨 � 䠩�� %s: %s.\n",
			name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, "%s �� ���� ����� 䠩���.\n", name);
	else{
		d = div(st.st_size, sizeof(struct bank_license_info));
		if ((d.quot == 0) || (d.rem != 0))
			fprintf(stderr, "���� %s ����� ������ ࠧ���.\n", name);
		else if (d.quot > MAX_BANK_LICENSES)
			fprintf(stderr, "��᫮ ��業��� �� ����� �ॢ���� %d.\n",
				MAX_BANK_LICENSES);
		else{
			fd = open(name, O_RDONLY);
			if (fd == -1)
				fprintf(stderr, "�訡�� ������ %s ��� �⥭��: %s.\n",
					name, strerror(errno));
			else{
				if (read(fd, licenses, st.st_size) == st.st_size)
					ret = d.quot;
				else
					fprintf(stderr, "�訡�� �⥭�� �� %s: %s.\n",
						name, strerror(errno));
				close(fd);
			}
		}
	}
	return ret;
}

/* �⥭�� �� �����᪮�� ����� �ନ���� */
static bool read_term_number(const char *name, struct md5_hash *number)
{
	bool ret = false;
	struct stat st;
	int fd;
	if (stat(name, &st) == -1)
		fprintf(stderr, "�訡�� ����祭�� ���ଠ樨 � 䠩�� %s: %s.\n",
			name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, "%s �� ���� ����� 䠩���.\n", name);
	else if (st.st_size != sizeof(struct md5_hash))
		fprintf(stderr, "���� %s ����� ������ ࠧ���.\n", name);
	else{
		fd = open(name, O_RDONLY);
		if (fd == -1)
			fprintf(stderr, "�訡�� ������ %s ��� �⥭��: %s.\n",
				name, strerror(errno));
		else{
			if (read(fd, number, sizeof(*number)) == sizeof(*number))
				ret = true;
			else
				fprintf(stderr, "�訡�� �⥭�� �� %s: %s.\n",
					name, strerror(errno));
			close(fd);
		}
	}
	return ret;
}

/*
 * ���� ��業��� ��� �� ��� �����᪮�� �����. �᫨ �� �����᪮��
 * ����� ��������� � ᯨ᪥ ����� ������ ࠧ�, ����� ��業��� ��� �⮣�
 * �ନ���� �뫠 㤠���� � �� ������ ������� � MBR �ਧ��� 㤠�����
 * ��業���.
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

/* �뢮� ��業��� ��� � stdout */
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
		fprintf(stderr, "������ ��� 䠩�� ᯨ᪠ ��業��� ��� ࠡ��� � ���.\n");
	else if (!read_term_number(TERM_NUMBER_FILE, &number))
		fprintf(stderr, "�� ������ ����� ������᭮��.\n");
	else if (check_lic_signature(BANK_LIC_SIGNATURE_OFFSET, BANK_LIC_SIGNATURE))
		fprintf(stderr, "��⠭���� ��業��� ��� ࠡ��� � ��� �� ����� "
			"�ନ��� ����������;\n�������, ��������, � ��� ��� \"������\".\n");
	else{
		nr_licenses = read_license_file(argv[1], licenses);
		if (nr_licenses > 0){
			license = find_license(licenses, nr_licenses, &number);
			if (license == NULL)
				fprintf(stderr, "�� ������� ��業��� ��� ��� �⮣� �ନ����.\n");
			else{
				write_license(license);
				ret = 0;
			}
		}
	}
	return ret;
}
