/*
 * �������� 䠩��� �ਢ離� USB-��᪠ � VipNet. (c) gsr 2004, 2005.
 * ��ࠬ���� ����᪠:
 * --term-number=<number>	-- �����᪮� ����� �ନ����;
 * --usb-bind=<name>		-- ��� 䠩�� �ਢ離� USB-��᪠;
 * --iplir=<name>		-- ��� 䠩�� ���祢��� ����ਡ�⨢� VipNet;
 * --iplir-bind=<name>		-- ��� 䠩�� �ਢ離� ���祢��� ����ਡ�⨢�;
 * --iplir-psw=<password>	-- ��஫� ���祢��� ����ਡ�⨢� VipNet;
 * --iplir-psw-file=<name>	-- ��� 䠩�� ��஫� ���祢��� ����ਡ�⨢�;
 * --bank-license=<name>	-- ��� 䠩�� ������᪮� ��業���.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "base64.h"
#include "md5.h"
#include "tki.h"

/*
 * ������ 䠩�� ���祢�� ���ଠ樨 � �ନ����.
 * FIXME: �� ��������� �������� term_key_info ����室��� �������� ��� ࠧ���.
 */
#define TKI_SIZE	80
/* �����᪮� ����� �ନ���� */
static char *number;
/* ��� 䠩�� �ਢ離� USB-��᪠ */
static char *usb_bind_name;
/* ��� 䠩�� ���祢��� ����ਡ�⨢� VipNet */
static char *iplir_name;
/* ��� 䠩�� �ਢ離� ���祢��� ����ਡ�⨢� */
static char *iplir_bind_name;
/* ��஫� ���祢��� ����ਡ�⨢� VipNet */
static char *iplir_psw;
/* ��� 䠩�� ��஫� ���祢��� ����ਡ�⨢� VipNet */
static char *iplir_psw_name;
/* ��� 䠩�� ������᪮� ��業��� */
static char *bank_license_name;

/* �뢮� �ࠢ�� �� �ᯮ�짮����� �ணࠬ�� */
static void show_usage(void)
{
	static char *help[] = {
		"�ணࠬ�� ᮧ����� 䠩��� �ਢ離� �ନ����. (c) gsr 2004",
		"�ᯮ�짮�����: mkbind <options>",
		"--term-number=<number>\t-- �����᪮� ����� �ନ����;",
		"--usb-bind=<name>\t-- ��� 䠩�� �ਢ離� USB-��᪠;",
		"--iplir=<name>	\t-- ��� 䠩�� ���祢��� ����ਡ�⨢� VipNet;",
		"--iplir-bind=<name>\t-- ��� 䠩�� �ਢ離� ���祢��� ����ਡ�⨢�;",
		"--iplir-psw=<password>\t-- ��஫� ���祢��� ����ਡ�⨢� VipNet;",
		"--iplir-psw-file=<name>\t-- ��� 䠩�� ��஫� ���祢��� ����ਡ�⨢�;",
		"--bank-license=<name>\t-- ��� 䠩�� ������᪮� ��業���.",
	};
	int i;
	for (i = 0; i < ASIZE(help); i++)
		printf("%s\n", help[i]);
}

/* �⥭�� 䠩�� �ਢ離� USB-��᪠ */
static bool read_usb_bind(char *name, struct md5_hash *md5)
{
	struct stat st;
	int l, fd;
	if (stat(name, &st) == -1){
		printf("䠩� �ਢ離� USB-��᪠ %s �� ������\n", name);
		return false;
	}
	if (st.st_size != sizeof(*md5)){
		printf("䠩� �ਢ離� USB-��᪠ ����� ������ ࠧ���\n");
		return false;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		printf("�訡�� ������ %s ��� �⥭��\n", name);
		return false;
	}
	l = read(fd, md5, sizeof(*md5));
	close(fd);
	if (l != sizeof(*md5)){
		printf("�訡�� �⥭�� �� %s\n", name);
		return false;
	}else
		return true;
}

/* ������ ����஫쭮� �㬬� ��� ��������� ���ᨢ� */
static bool write_md5(char *name, uint8_t *buf, int l)
{
	struct md5_hash md5 = ZERO_MD5_HASH;
	int fd;
	if ((name == NULL) || (buf == NULL))
		return false;
	fd = creat(name, 0600);
	if (fd == -1){
		printf("�訡�� ᮧ����� %s\n", name);
		return false;
	}
	get_md5(buf, l, &md5);
	if (write(fd, &md5, sizeof(md5)) != sizeof(md5)){
		printf("�訡�� ����� � %s\n", name);
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

/* �������� 䠩�� �ਢ離� USB-��᪠ */
static bool create_usb_bind(char *number, char *bind_name)
{
	uint8_t buf[32];
	int l;
	if ((number == NULL) || (bind_name == NULL))
		return false;
	l = strlen(number);
	if (l != 13){
		printf("������ ࠧ��� �����᪮�� ����� �ନ����\n");
		return false;
	}
	l = base64_encode((uint8_t *)number, l, buf);
	encrypt_data(buf, l);
	return write_md5(bind_name, buf, l);
}

/* �������� 䠩�� �ਢ離� ���祢�� ��� */
static bool create_iplir_bind(char *usb_bind_name,
		char *iplir_name, char *bind_name)
{
	struct md5_hash buf[2];
	if (!get_md5_file(iplir_name, buf)){
		printf("�訡�� ��।������ ����஫쭮� �㬬� %s\n", iplir_name);
		return false;
	}
	return	read_usb_bind(usb_bind_name, buf + 1) &&
		write_md5(bind_name, (uint8_t *)buf, sizeof(buf));
}

/* ������ 䠩�� ��஫� ���祢��� ����ਡ�⨢� VipNet */
static bool write_iplir_psw(char *psw, char *psw_file)
{
	static uint8_t psw_buf[128], _psw_buf[128];
	int l, ll, fd;
	if ((psw == NULL) || (psw_file == NULL))
		return false;
	l = strlen(psw);
	if (base64_get_encoded_len(base64_get_encoded_len(l)) > sizeof(psw_buf)){
		printf("᫨誮� ������ ��஫�\n");
		return false;
	}
	l = base64_encode((uint8_t *)psw, l, _psw_buf);
	l = base64_encode(_psw_buf, l, psw_buf);
	fd = creat(psw_file, 0600);
	if (fd == -1){
		printf("�訡�� ᮧ����� 䠩�� %s\n", psw_file);
		return false;
	}
	ll = write(fd, psw_buf, l);
	close(fd);
	if (ll != l){
		printf("�訡�� ����� � %s\n", psw_file);
		return false;
	}else
		return true;
}

/* �������� 䠩�� ������᪮� ��業��� */
static bool create_bank_license(char *number, char *license_name)
{
	uint8_t buf[2 * TERM_NUMBER_LEN];
	uint8_t v1[64], v2[64];
	int i;
	memcpy(buf, number, TERM_NUMBER_LEN);
	for (i = 0; i < TERM_NUMBER_LEN; i++)
		buf[sizeof(buf) - i - 1] = ~buf[i];
	i = base64_encode(buf, sizeof(buf), v1);
	i = base64_encode(v1, i, v2);
	return write_md5(license_name, v2, i);
}

/* ������ ��������� ��ப� */
bool parse_cmd_line(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	const struct option longopts[] = {
		{"term-number",		required_argument,	NULL, 'n'},
		{"usb-bind",		required_argument,	NULL, 'u'},
		{"iplir",		required_argument,	NULL, 'd'},
		{"iplir-bind",		required_argument,	NULL, 'i'},
		{"iplir-psw",		required_argument,	NULL, 'p'},
		{"iplir-psw-file",	required_argument,	NULL, 'f'},
		{"bank-license",	required_argument,	NULL, 'b'},
		{NULL,			0,			NULL, 0},
	};
	char *shortopts = "n:u:d:i:p:f:b:";
	bool loop_flag = true, err = false;
	while (loop_flag && !err){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'n':
				if (number == NULL)
					number = optarg;
				else{
					printf("����୮� 㪠����� �����᪮�� ����� �ନ����\n");
					err = true;
				}
				break;
			case 'u':
				if (usb_bind_name == NULL)
					usb_bind_name = optarg;
				else{
					printf("����୮� 㪠����� 䠩�� �ਢ離� USB-��᪠\n");
					err = true;
				}
				break;
			case 'd':
				if (iplir_name == NULL)
					iplir_name = optarg;
				else{
					printf("����୮� 㪠����� 䠩�� ���祢��� ����ਡ�⨢�\n");
					err = true;
				}
				break;
			case 'i':
				if (iplir_bind_name == NULL)
					iplir_bind_name = optarg;
				else{
					printf("����୮� 㪠����� 䠩�� �ਢ離� ���祢��� ����ਡ�⨢�\n");
					err = true;
				}
				break;
			case 'p':
				if (iplir_psw == NULL)
					iplir_psw = optarg;
				else{
					printf("����୮� 㪠����� ��஫� ���祢��� ����ਡ�⨢�\n");
					err = true;
				}
				break;
			case 'f':
				if (iplir_psw_name == NULL)
					iplir_psw_name = optarg;
				else{
					printf("����୮� 㪧���� 䠩�� ��஫� ���祢��� ����ਡ�⨢�\n");
					err = true;
				}
				break;
			case 'b':
				if (bank_license_name == NULL)
					bank_license_name = optarg;
				else{
					printf("����୮� 㪠����� ����� 䠩�� ������᪮� ��業���\n");
					err = true;
				}
				break;
			case EOF:		/* ����� ��������� ��ப� */
				loop_flag = false;
				if (optind < argc){
					printf("��譥 ����� � ��������� ��ப�: %d/%d\n",
						optind, argc);
					err = true;
				}
				break;
			default:		/* �訡�� */
				err = true;
				break;
		}
	}
	if (err)
		return false;
	if ((number == NULL) && (usb_bind_name == NULL) &&
			(iplir_name == NULL) && (iplir_bind_name == NULL) &&
			(iplir_psw == NULL) && (iplir_psw_name == NULL)){
		show_usage();
		return false;
	}
	if (iplir_name == NULL){
		if (iplir_bind_name != NULL){
			printf("�� 㪠��� 䠩� ���祢��� ����ਡ�⨢� VipNet (--iplir)\n");
			return false;
		}
	}else if (iplir_bind_name == NULL){
		printf("�� 㪠��� 䠩� �ਢ離� ���祢��� ����ਡ�⨢� VipNet (--iplir-bind)\n");
		return false;
	}else if (usb_bind_name == NULL){
		printf("����室��� 㪠���� 䠩� �ਢ離� USB-��᪠ (--usb-bind)\n");
		return false;
	}
	if ((iplir_psw == NULL) ^ (iplir_psw_name == NULL)){
		printf("�ᯮ���� --iplir-psw � --iplir-psw-file �����६����\n");
		return false;
	}
	if ((bank_license_name != NULL) && (number == NULL)){
		printf("�ᯮ���� --bank-license � --term-number �����६����\n");
		return false;
	}
	return true;
}

/* �믮������ ����⢨�, 㪠������ � ��������� ��ப� */
static bool do_actions(void)
{
	if ((usb_bind_name != NULL) && (number != NULL)){
		if (!create_usb_bind(number, usb_bind_name))
			return false;
	}
	if (iplir_bind_name != NULL){
		if (!create_iplir_bind(usb_bind_name, iplir_name, iplir_bind_name))
			return false;
	}
	if (iplir_psw != NULL){		/* iplir_psw_name �஢������ ��� */
		if (!write_iplir_psw(iplir_psw, iplir_psw_name))
			return false;
	}
	if (bank_license_name != NULL){
		if (!create_bank_license(number, bank_license_name))
			return false;
	}
	return true;
}

int main(int argc, char **argv)
{
	if (!parse_cmd_line(argc, argv))
		return 1;
	return do_actions() ? 0 : 1;
}
