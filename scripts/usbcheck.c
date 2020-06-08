/*
 * �஢�ઠ ����� ������᭮�� �� �⥭��/������. (c) gsr & Alex 2004, 2013.
 * �ᯮ�짮�����: usbcheck [--number=n] [--passes=m] [--reads=k]
 * --number	����� ���ன�⢠ USB (0..15). �� 㬮�砭�� 0;
 * --passes	�᫮ ��室�� (1..100). �� 㬮�砭�� 10.
 * --reads	�᫮ 横��� �⥭�� ��᫥ ����� (1..100). �� 㬮�砭�� 5;
 * --help	�뢮� �ࠢ��.
 */

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "colortty.h"
#include "usb/core.h"

/* ����� ����� ������᭮�� */
#define USBKEY_NUMBER	0
static int usbkey_number = USBKEY_NUMBER;

/* ��᫮ ��室�� */
#define NR_PASSES	10
static int nr_passes	= NR_PASSES;

/* ��᫮ 横��� �⥭�� ��᫥ ����� */
#define NR_READS	5
static int nr_reads	= NR_READS;

/* �뢮� �ࠢ�� �� �ᯮ�짮����� �ணࠬ�� */
static void usage(void)
{
	static char *strs[] = {
		"�஢�ઠ ����� ������᭮��. (c) �� ��� \"������\" 2004, 2013.",
		"�ᯮ�짮�����: usbcheck [��ࠬ����]",
		"��ࠬ���� (� ᪮���� 㪠���� ���祭�� �� 㬮�砭��)",
		"--number=n\t\t����� ����� ������᭮�� (0..15) [0];",
		"--passes=n\t\t�᫮ ��室�� (1..100) [10];",
		"--reads=n\t\t�᫮ 横��� �⥭�� ��᫥ ����� (1..100) [5];",
		"--help\t\t\t�뢮� �ࠢ��.",
	};
	int i;
	for (i = 0; i < ASIZE(strs); i++)
		printf("%s\n", strs[i]);
}

/* ������ ��������� ��ப� */
static bool parse_cmd_line(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	const struct option longopts[] = {
		{"number",	required_argument,	NULL,	'n'},
		{"passes",	required_argument,	NULL,	'p'},
		{"reads",	required_argument,	NULL,	'r'},
		{"help",	no_argument,		NULL,	'h'},
		{NULL,		0,			NULL,	0},
	};
	char *shortopts = "n:p:r:h";
	bool loop_flag = true;
	char *p;
#define OPT_NUMBER	1
#define OPT_PASSES	2
#define OPT_READS	4
	int opts = 0;
	while (loop_flag){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'n':
				if ((opts & OPT_NUMBER) == 0){
					if (*optarg)
						usbkey_number = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (usbkey_number > NR_USB_KEYS)){
						printf("������ ����� ����� ������᭮��: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_NUMBER;
				}else
					printf("����୮� 㪠����� ����� ������᭮�� ����������\n");
				break;
			case 'p':
				if ((opts & OPT_PASSES) == 0){
					if (*optarg)
						nr_passes = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (nr_passes > 100)){
						printf("������ �ଠ� �᫠ ��室��: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_PASSES;
				}else
					printf("����୮� 㪠����� �᫠ ��室�� ����������\n");
				break;
			case 'r':
				if ((opts & OPT_READS) == 0){
					if (*optarg)
						nr_reads = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (nr_reads > 100)){
						printf("������ �ଠ� �᫠ 横��� �⥭��: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_READS;
				}else
					printf("����୮� 㪠����� �᫠ ��室�� ����������\n");
				break;
			case 'h':
				return false;
			case EOF:		/* ����� ��������� ��ப� */
				loop_flag = false;
				if (optind < argc){
					printf("��譨� ����� � ��������� ��ப�\n");
					return false;
				}
				break;
			default:		/* �訡�� */
				return false;
		}
	}
	return true;
}

/* ���������� ���� ��ﬨ */
void make_all_zeroes(uint8_t *data, size_t l)
{
	for (; l > 0; l--)
		*data++ = 0;
}

/* ���������� ���� �����栬� */
void make_all_ones(uint8_t *data, size_t l)
{
	for (; l > 0; l--)
		*data++ = 1;
}

/* ���������� ���� ����騬 ��� */
void make_running_zero(uint8_t *data, size_t l)
{
	off_t i;
	uint8_t b = ~1;
	for (i = 0; i < l; i++){
		if ((i % 8) == 0)
			b = 1;
		else
			b <<= 1;
		data[i] = ~b;
	}
}

/* ���������� ���� ����饩 �����楩 */
void make_running_one(uint8_t *data, size_t l)
{
	int i;
	uint8_t b = 1;
	for (i = 0; i < l; i++){
		if ((i % 8) == 0)
			b = 1;
		else
			b <<= 1;
		data[i] = b;
	}
}

/* ���������� ���� ����-���⮬ */
void make_address_byte(uint8_t *data, size_t l)
{
	uint8_t b = 0;
	for (; l > 0; l--)
		*data++ = b++;
}

/* ���������� ���� ��砩�묨 �᫠�� */
void make_random_data(uint8_t *data, size_t l)
{
	for (; l > 0; l--)
		*data++ = rand() & 0xff;
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

static int flush_getchar(void)
{
	flush_stdin();
	return getchar();
}

static bool do_test(int n)
{
	static struct {
		void (*fn)(uint8_t *, size_t);
		const char *name;
	} gen_fn[] = {
		{make_all_zeroes,	"0000"},
		{make_all_ones,		"1111"},
		{make_running_zero,	"RUN0"},
		{make_running_one,	"RUN1"},
		{make_address_byte,	"ADDR"},
		{make_random_data,	"RAND"}
	};
	static uint8_t tx[USB_KEY_SIZE];
	static uint8_t rx[USB_KEY_SIZE];
	int i, j;
	bool fault = false;
	printf("***���: %.3d***\n", n + 1);
	for (i = 0; !fault && (i < ASIZE(gen_fn)); i++){
		printf("\t%s:\t���...", gen_fn[i].name);
		fflush(stdout);
		gen_fn[i].fn(tx, sizeof(tx));
		if (usbkey_write(usbkey_number, tx)){
			printf("\b\b\b   " tGRN "OK" ANSI_DEFAULT "\n");
			printf("\t\t���..." tGRN);
			fflush(stdout);
			for (j = 0; !fault && (j < nr_reads); j++){
				if (usbkey_read(usbkey_number, rx, 0, USB_KEY_BLOCKS) &&
						(memcmp(rx, tx, USB_KEY_SIZE) == 0)){
					putchar('+');
					fflush(stdout);
				}else{
					for (; j > 0; j--)
						putchar('\b');
					printf("\b\b\b   " tRED "***�� ���***" ANSI_DEFAULT "\n");
					fault = true;
				}
			}
			if (!fault){
				for (j = 0; j < nr_reads; j++)
					putchar('\b');
				printf("\b\b\b   " tGRN "OK" ANSI_DEFAULT "\n");
			}
		}else{
			printf("\b\b\b   " tRED "***�� ���***" ANSI_DEFAULT "\n");
			fault = true;
		}
	}
	return !fault;
}

int main(int argc, char **argv)
{
	int ret = 1;
	if (!parse_cmd_line(argc, argv))
		usage();
	else{
		bool fault = false;;
		srand(time(NULL));
		while (!fault){
			printf(tWHT "�������� ������ ������������ � ������ � ������� ENTER: "
					ANSI_DEFAULT);
			fflush(stdout);
			flush_getchar();
			if (usbkey_open(usbkey_number)){
				int i;
				for (i = 0; !fault && (i < nr_passes); i++){
					if (!do_test(i))
						fault = true;
				}
			}else{
				puts(tRED "������ ������� � ������ ������������." ANSI_DEFAULT);
				fault = true;
			}
		}
		usbkey_close(usbkey_number);
		if (fault)
			puts(tRED "***��� �������� ������ ������������ ��������� ������***"
				ANSI_DEFAULT "\n");
		else
			ret = 0;
	}
	return ret;
}
