/* ��⠭���� �� �ନ��� ��業��� ���. (c) gsr 2005, 2011, 2019-2020 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "base64.h"
#include "license.h"
#include "licsig.h"
#include "tki.h"

static struct bank_license_info licenses[MAX_BANK_LICENSES];
static int nr_licenses;

/* �⥭�� 䠩�� ��業��� */
static int read_license_file(const char *path, struct bank_license_info *licenses)
{
	struct stat st;
	div_t d;
	int fd, ret = -1;
	if (stat(path, &st) == -1)
		fprintf(stderr, "�訡�� ����祭�� ���ଠ樨 � 䠩�� %s: %m.\n", path);
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, "%s �� ���� ����� 䠩���.\n", path);
	else{
		d = div(st.st_size, sizeof(struct bank_license_info));
		if ((d.quot == 0) || (d.rem != 0))
			fprintf(stderr, "���� %s ����� ������ ࠧ���.\n", path);
		else if (d.quot > MAX_BANK_LICENSES)
			fprintf(stderr, "��᫮ ��業��� �� ����� �ॢ���� %d.\n",
				MAX_BANK_LICENSES);
		else{
			fd = open(path, O_RDONLY);
			if (fd == -1)
				fprintf(stderr, "�訡�� ������ %s ��� �⥭��: %m.\n", path);
			else{
				if (read(fd, licenses, st.st_size) == st.st_size)
					ret = d.quot;
				else
					fprintf(stderr, "�訡�� �⥭�� �� %s: %m.\n", path);
				close(fd);
			}
		}
	}
	return ret;
}

/* �⥭�� �� �����᪮�� ����� �ନ���� */
static bool read_term_number(const char *path, struct md5_hash *number)
{
	bool ret = false;
	struct stat st;
	if (stat(path, &st) == -1)
		fprintf(stderr, "�訡�� ����祭�� ���ଠ樨 � 䠩�� %s: %m.\n", path);
	else if (!S_ISREG(st.st_mode))
		fprintf(stderr, "%s �� ���� ����� 䠩���.\n", path);
	else{
		FILE *f = fopen(path, "r");
		if (f == NULL)
			fprintf(stderr, "�訡�� ������ %s ��� �⥭��: %m.\n", path);
		else{
			char nr[TERM_NUMBER_LEN + 2];	/* \n + 0 */
			if (fgets(nr, sizeof(nr), f) == NULL)
				fprintf(stderr, "�訡�� �⥭�� �� %s: %m.\n", path);
			else{
				size_t len = strlen(nr);
				ret = (len == (TERM_NUMBER_LEN + 1)) &&
					(strncmp(nr, "F00137001", 9) == 0) &&
					((nr[len - 1] == '\n') || (nr[len - 1] == '\r'));
				if (ret){
					for (int i = 9; i < 13; i++){
						if (!isdigit(nr[i])){
							ret = false;
							break;
						}
					}
				}
				if (ret){
					uint8_t buf[64];
					ssize_t l = base64_encode((const uint8_t *)nr,
						TERM_NUMBER_LEN, buf);
					encrypt_data(buf, l);
					get_md5(buf, l, number);
				}
			}
			if (fclose(f) == EOF)
				fprintf(stderr, "�訡�� ������� %s: %m.\n", path);
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
	else if (!read_term_number("/sdata/term-number.txt", &number))
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
