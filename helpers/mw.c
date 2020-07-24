/* �ணࠬ�� ࠡ��� � 䠩��� ���祢�� ���ଠ樨 �ନ����. (c) gsr 2004
 * ��ࠬ���� ��������� ��ப�:
 * --read-tki=<file> -- �⥭�� 䠩�� ���祢�� ���ଠ樨;
 * --write-tki=<file> -- ������ 䠩�� ���祢�� ���ଠ樨;
 * --srv-keys[=<file>] -- ����஥�� ���� DS1990A;
 * --dbg-keys[=<file>] -- �⫠���� ���� DS1990A;
 * --repeated -- ����������� ����� ����������� ���祩 DS1990A;
 * --number -- �����᪮� ����� �ନ����;
 * --decrypt -- �뢮� � stdout ����஢������ 䠩�� ���祢�� ���ଠ樨;
 * --all -- �뢮� ᮤ�ন���� 䠩�� ���祢�� ���ଠ樨;
 * --verify -- �஢�ઠ 䠩�� tki �� ���४⭮��� ���祭�� (�� ॠ��������);
 * --help -- �뢮� �ࠢ�筮� ���ଠ樨.
 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ds1990a.h"
#include "genfunc.h"
#include "tki.h"

static char *tki_name;		/* ��� 䠩�� ���祢�� ���ଠ樨 */
static char *key_file;		/* ��� 䠩�� ���祩 DALLAS */
static bool tki_read = true;	/* 䫠� �⥭��/����� 䠩�� tki */
static bool can_repeat_keys = false;	/* 䫠� ���������� ����� �㡫�������� ���祩 */

/* ��'��� ����⢨� */
enum {
	subj_none,
	subj_srv_keys,		/* ����஥�� ���� DS1990A */
	subj_dbg_keys,		/* �⫠���� ���� DS1990A */
	subj_number,		/* �����᪮� ����� */
	subj_decrypt,		/* ����஢�� 䠩�� */
	subj_all,		/* �뢥�� ᮤ�ন��� 䠩�� tki */
} subj = subj_none;
#define DESCR_WIDTH	20	/* ࠧ��� ���� �������� ��ࠬ��� */

/* �뢮� �ࠢ�筮� ���ଠ樨 */
static void usage(void)
{
	static char *help[] = {
		"�ணࠬ�� ࠡ��� � 䠩��� ���祢�� ���ଠ樨 �ନ����",
		"(c) gsr 2001 -- 2004.",
		"�ᯮ�짮�����: mw [options]:",
		"--read-tki=<file> -- �⥭�� 䠩�� ���祢�� ���ଠ樨;",
		"--write-tki=<file> -- ������ 䠩�� ���祢�� ���ଠ樨;",
		"--srv-keys[=<file>] -- ����஥�� ���� DS1990A;",
		"--dbg-keys[=<file>] -- �⫠���� ���� DS1990A;",
		"--repeated -- ����������� ����� ����������� ���祩 DS1990A;",
		"--number -- �����᪮� ����� �ନ����;",
		"--decrypt -- �뢮� � stdout ����஢������ 䠩�� ���祢�� ���ଠ樨;",
		"--all -- �뢮� ᮤ�ন���� 䠩�� ���祢�� ���ଠ樨;",
		"--help -- �뢮� �ࠢ�筮� ���ଠ樨.",
	};
	int i;
	for (i = 0; i < ASIZE(help); i++)
		printf("%s\n", help[i]);
}

/* �뢮� ����� ���� DALLAS */
static void print_dallas_key(struct md5_hash *md5, int ds_type)
{
	struct md5_hash zero_md5 = ZERO_MD5_HASH;
	if (md5 == NULL)
		return;
	printf("%*c:", DESCR_WIDTH, ds_key_char(ds_type));
	if (memcmp(md5, &zero_md5, sizeof(*md5)) == 0)
		printf(" �� ������");
	else
		print_md5(md5);
	printf("\n");
}

/* �뢮� ����஢ ���祩 DALLAS */
static void print_dallas(void)
{
	struct md5_hash srv_keys[NR_SRV_KEYS];
	struct md5_hash dbg_keys[NR_DBG_KEYS];
	int i;
	get_tki_field(&tki, TKI_SRV_KEYS, (uint8_t *)srv_keys);
	for (i = 0; i < NR_SRV_KEYS; i++)
		print_dallas_key(srv_keys + i, key_srv);
	get_tki_field(&tki, TKI_DBG_KEYS, (uint8_t *)dbg_keys);
	for (i = 0; i < NR_DBG_KEYS; i++)
		print_dallas_key(dbg_keys + i, key_dbg);
}

/* �뢮� �����᪮�� ����� */
static void print_term_number(void)
{
	int i, c;
	term_number tn;
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn);
	printf("%*s: ", DESCR_WIDTH, "�����᪮� �����");
	for (i = 0; i < sizeof(tn); i++){
		c = tn[i];
		if (isalnum(c))
			putchar(c);
		else
			break;
	}
	if (i < sizeof(tn))
		printf("[*]\n");
	else
		printf("\n");
}

/* �뢮� ᮤ�ন���� �������� tki */
static void print_tki(void)
{
	print_dallas();
	print_term_number();
}

/* ������ ����஫쭮� �㬬� ���� DS1990A */
static uint8_t ds_crc(uint8_t *p, int l)
{
#define DOW_B0	0
#define DOW_B1	4
#define DOW_B2	5
	uint8_t b, s = 0;
	uint8_t m0 = 1 << (7-DOW_B0);
	uint8_t m1 = 1 << (7-DOW_B1);
	uint8_t m2 = 1 << (7-DOW_B2);
	int i, j;
	if (p != NULL){
		for (i = 0; i < l; i++){
			b = p[i];
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

/* �஢�ઠ �㫥���� ����� DS1990A */
static bool ds_zero_id(ds_number dsn)
{
	int i;
	for (i = 0; (i < DS_NUMBER_LEN) && (dsn[i] == 0); i++){}
	return i == DS_NUMBER_LEN;
}

/* ������ ����஢ ���祩 */
static bool write_dallas(struct md5_hash *keys, int n_keys, bool can_repeat)
{
	int i, j, id[DS_NUMBER_LEN];
	ds_number dsn;
	struct md5_hash md5;
	bool rep;
	FILE *f = NULL;
	if (keys == NULL)
		return false;
	memset(keys, 0, n_keys * sizeof(*keys));
	if (key_file != NULL){
		f = fopen(key_file, "rb");
		if (f == NULL){
			fprintf(stderr, "�訡�� ������ 䠩�� ���祩 %s: %s.\n",
				key_file, strerror(errno));
			return false;
		}
	}else if (!ds_init()){
		fprintf(stderr, "�訡�� ������ �ࠩ��� DS1990A.\n");
		return false;
	}
	for (i = 0; i < n_keys; i++){
		printf("���� #%d: ", i + 1);
		if (key_file == NULL)
			getchar();
		do{
			if (key_file == NULL)
				ds_read(dsn);
			else if (fscanf(f, "%2x %2x %2x %2x %2x %2x %2x %2x\n",
					id, id + 1, id + 2, id + 3, id + 4,
					id + 5, id + 6, id + 7) == 8){
				for (j = 0; j < DS_NUMBER_LEN; j++)
					dsn[j] = (uint8_t)id[j];
				if (ds_crc(dsn, DS_NUMBER_LEN) != 0){
					fprintf(stderr, "��ᮢ������� "
						"����஫쭮� �㬬� ����� "
						"���� #%d.\n", i + 1);
					fclose(f);
					return false;
				}
			}else{
				fprintf(stderr, "�訡�� �⥭�� ����� ���� #%d �� %s.\n",
					i + 1, key_file);
				fclose(f);
				return false;
			}
		} while ((key_file == NULL) && ds_zero_id(dsn));
		ds_hash(dsn, &md5);
		rep = false;
		if (!can_repeat){	/* �஢�ઠ ����������� ���祩 */
			for (j = 0; !rep && (j < i); j++){
				if (cmp_md5(&md5, keys + j))
					rep = true;
			}
		}
		if (rep){
			i--;
			printf("�����\n");
		}else{
			memcpy(keys + i, &md5, sizeof(md5));
			printf("ok\n");
		}
	}
	if (key_file != NULL)
		fclose(f);
	return true;
}

/* ���⪠ �室���� ��⮪� */
static void flush_stdin(void)
{
/* ��ॢ���� ���������� � ����������騩 ०�� */
	int nonblock = true;
	ioctl(STDIN_FILENO, FIONBIO, &nonblock);
/* ��⠥� ��⠢訥�� ᨬ���� �� ���� */
	while (getchar() != EOF);
/* �����頥� ���������� � ��������騩 ०�� */
	nonblock = 0;
	ioctl(STDIN_FILENO, FIONBIO, &nonblock);
}

/* ���� ���짮��⥫�� �����᪮�� ����� �ନ���� */
static bool ask_number(void)
{
	const char *prefix = "F00137001";
	char *p;
	char suffix[6];
	term_number tn;
	bool loop_flag = true;
	while (loop_flag){
		printf("������ �����᪮� ����� �ନ����: %s", prefix);
		if (fgets(suffix, sizeof(suffix), stdin) == NULL)
			return false;
		flush_stdin();
		if (strlen(suffix) < 5)
			fprintf(stderr, "���誮� ���� ᨬ����� � �����.\n");
		else if (suffix[4] != '\n')
			fprintf(stderr, "���誮� ����� ᨬ����� � �����.\n");
		else{
			suffix[4] = 0;
			strtoul(suffix, &p, 10);
			if (*p)
				fprintf(stderr, "������ �ଠ� �����.\n");
			else{
				memcpy(tn, prefix, 9);
				memcpy(tn + 9, suffix, 4);
				loop_flag = false;
			}
		}
	}
	set_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn);
	return true;
}


/* �뢮� � stdout ����஢������ 䠩�� ���祢�� ���ଠ樨 */
static void dump_tki(void)
{
	decrypt_data((uint8_t *)&tki, sizeof(tki));
	write(STDOUT_FILENO, &tki, sizeof(tki));
	encrypt_data((uint8_t *)&tki, sizeof(tki));
}

/* �������᪠� �஢�ઠ ��������� ��ப� */
static bool check_semantic(void)
{
/* ������ ���� 㪠��� 䠩� ���祢�� ���ଠ樨 � ��'��� ����⢨� */
	if (tki_name == NULL){
		fprintf(stderr, "�� 㪠��� 䠩� ���祢�� ���ଠ樨 (--read-tki ��� --write-tki).\n");
		return false;
	}else if (subj == subj_none){
		fprintf(stderr, "�� 㪠��� ��'��� ����⢨�.\n");
		return false;
	}else if ((subj == subj_all) && !tki_read){
		fprintf(stderr, "--all ����� ��� ⮫쪮 � ��⠭�� � --read-tki.\n");
		return false;
	}
/* ���� DALLAS */
	if (((subj == subj_srv_keys) || (subj == subj_dbg_keys)) &&
			(key_file != NULL) && tki_read){
		fprintf(stderr, "���� ���祩 �ᯮ������ ⮫쪮 � ��⠭�� � --write-tki.\n");
		return false;
	}
	if (can_repeat_keys){
		if ((subj == subj_srv_keys) || (subj == subj_dbg_keys)){
			if (tki_read)
				fprintf(stderr, "--repeated �� ����� ��᫠ ��� --write-tki.\n");
		}else
			fprintf(stderr, "--repeated ����� ��� ⮫쪮 � ��⠭�� � --xxx-keys.\n");
	}
/* �����஢�� 䠩�� */
	if ((subj == subj_decrypt) && !tki_read){
		fprintf(stderr, "--decrypt ����� ��� ⮫쪮 � ��⠭�� � --read-tki.\n");
		return false;
	}
	return true;
}

/* �஢�ઠ �ࠢ��쭮�� ��������� ���� 䠩��� */
static bool check_file_names(void)
{
	struct stat st;
	if (key_file != NULL){
		if (stat(key_file, &st) == -1){
			fprintf(stderr, "����୮� ��� 䠩�� ���祩: %s.\n",
				key_file);
			return false;
		}
	}
	return true;
}

/* ������ ��������� ��ப� */
static bool parse_cmd_line(int argc, char **argv)
{
	bool set_subj(int _subj)	/* ��⠭���� ��'��� ����⢨� */
	{
		if (subj == subj_none){
			subj = _subj;
			return true;
		}else{
			fprintf(stderr, "������� ��᪮�쪮 ��'��⮢ ����⢨�.\n");
			return false;
		}
	}
	extern char *optarg;
	extern int optind;
	const struct option longopts[] = {
		{"read-tki",	required_argument,	NULL, 'r'},
		{"write-tki",	required_argument,	NULL, 'w'},
		{"srv-keys",	optional_argument,	NULL, 'v'},
		{"dbg-keys",	optional_argument,	NULL, 'g'},
		{"repeated",	no_argument,		NULL, 's'},
		{"number",	no_argument,		NULL, 'n'},
		{"decrypt",	no_argument,		NULL, 'd'},
		{"all",		no_argument,		NULL, 'a'},
		{"help",	no_argument,		NULL, 'h'},
		{NULL,		0,			NULL, 0},
	};
	char *shortopts = "r:w:vgsndah";
	bool loop_flag = true, ret_val = true;
	bool access_specified = false;
	while (loop_flag && ret_val){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'r':		/* --read-tki */
				if (access_specified){
					fprintf(stderr, "����୮� 㪠����� ⨯� ����㯠.\n");
					ret_val = false;
				}else{
					access_specified = true;
					tki_read = true;
					tki_name = optarg;
				}
				break;
			case 'w':		/* --write-tki */
				if (access_specified){
					fprintf(stderr, "����୮� 㪠����� ⨯� ����㯠.\n");
					ret_val = false;
				}else{
					access_specified = true;
					tki_read = false;
					tki_name = optarg;
				}
				break;
			case 'v':		/* --srv-keys */
				key_file = optarg;
				ret_val = set_subj(subj_srv_keys);
				break;
			case 'g':		/* --dbg-keys */
				key_file = optarg;
				ret_val = set_subj(subj_dbg_keys);
				break;
			case 's':		/* --repeated */
				if (can_repeat_keys)
					fprintf(stderr, "����୮� 㪠����� 䫠�� ����७�� ���祩.\n");
				can_repeat_keys = true;
				break;
			case 'n':		/* --number */
				ret_val = set_subj(subj_number);
				break;
			case 'd':
				ret_val = set_subj(subj_decrypt);
				break;
			case 'a':		/* --all */
				ret_val = set_subj(subj_all);
				break;
			case 'h':
				usage();
				ret_val = false;
				break;
			case EOF:		/* ����� ��������� ��ப� */
				loop_flag = false;
				if (optind < argc){
					fprintf(stderr, "��譨� ����� � ��������� ��ப�.\n");
					ret_val = false;
				}
				break;
			default:		/* �訡�� */
				ret_val = false;
				break;
		}
		loop_flag = loop_flag && ret_val;
	}
	return ret_val && check_semantic() && check_file_names();
}

int main(int argc, char **argv)
{
	struct md5_hash srv_keys[NR_SRV_KEYS];
	struct md5_hash dbg_keys[NR_DBG_KEYS];
	if (argc == 1){
		usage();
		return 1;
	}else if (!parse_cmd_line(argc, argv) || !read_tki(tki_name, true))
		return 1;
	check_tki();
	if (!tki_ok){
		printf("䠩� ���祢�� ���ଠ樨 ���०���\n");
		return 1;
	}
	if (tki_read){		/* �⥭�� �� tki */
		switch (subj){
			case subj_srv_keys:
			case subj_dbg_keys:
				print_dallas();
				break;
			case subj_number:
				print_term_number();
				break;
			case subj_decrypt:
				dump_tki();
				break;
			case subj_all:
				print_tki();
				break;
		}
	}else{			/* ������ � tki */
		switch (subj){
			case subj_srv_keys:
				if (write_dallas(srv_keys, NR_SRV_KEYS,
							can_repeat_keys))
					set_tki_field(&tki, TKI_SRV_KEYS,
							(uint8_t *)srv_keys);
				break;
			case subj_dbg_keys:
				if (write_dallas(dbg_keys, NR_DBG_KEYS,
							can_repeat_keys))
					set_tki_field(&tki, TKI_DBG_KEYS,
							(uint8_t *)dbg_keys);
				break;
			case subj_number:
				if (!ask_number()){
					fprintf(stderr, "\n�����᪮� ����� �� ����ᠭ.\n");
					return 1;
				}
				break;
		}
		if (!write_tki(tki_name))
			return 1;
	}
	return 0;
}
