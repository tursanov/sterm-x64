/*
 * �ணࠬ�� �⥭�� ������ �� ����� ������᭮��. (c) gsr 2006.
 * �ᯮ�짮�����: usbread [--first-block=n] [--blocks=m] [--verbose] [--help].
 * --first-block	-- ����� ��ࢮ�� ����� ��� �⥭��;
 * --blocks		-- ࠧ��� ������ � ������;
 * --verbose		-- �뢮� �������⥫쭮� ���ଠ樨;
 * --help		-- �뢮� �ࠢ��.
 * ������ ����� -- 256 ����. ��⠭�� ����� �뢮����� � stdout.
 */

#include <sys/times.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usb/core.h"

/* ����� ��ࢮ�� ����� ��� �⥭�� */
static int first_block;
/* ������ ������ � ������ */
static int nr_blocks = USB_KEY_SIZE / USB_BLOCK_LEN;
/* ���� �뢮�� �������⥫쭮� ���ଠ樨 */
static bool verbose;

/* ���� ��� �⥭�� */
static uint8_t in_data[USB_KEY_SIZE];

static void usage(void)
{
	static char *strs[] = {
		"�ணࠬ�� �⥭�� ������ �� ����� ������᭮��. (c) �� ��� \"������\" 2006.",
		"�ᯮ�짮�����: usbread [--first-block=n] [--blocks=m] [--verbose] [--help].",
		"\t--first-block\t\t-- ����� ��ࢮ�� ����� ��� �⥭��;",
		"\t--blocks\t\t-- ࠧ��� ������ � ������;",
		"\t--verbose\t\t-- �뢮� �������⥫쭮� ���ଠ樨;",
		"\t--help\t\t\t-- �뢮� �ࠢ��.",
		"������ ����� -- 256 ����. ��⠭�� ����� �뢮����� � stdout.",
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
		{"first-block",	required_argument,	NULL,	'f'},
		{"blocks",	required_argument,	NULL,	'b'},
		{"verbose",	no_argument,		NULL,	'v'},
		{"help",	no_argument,		NULL,	'h'},
		{NULL,		0,			NULL,	0},
	};
	char *shortopts = "f:b:vh";
	bool loop_flag = true;
	char *p;
#define OPT_FIRST_BLOCK		1
#define OPT_BLOCKS		2
	int opts = 0;
	while (loop_flag){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'f':
				if ((opts & OPT_FIRST_BLOCK) == 0){
					if (*optarg)
						first_block = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (first_block >= USB_KEY_BLOCKS)){
						fprintf(stderr, "������ ����� ��ࢮ�� ����� ��� �⥭��: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_FIRST_BLOCK;
				}else
					fprintf(stderr, "����୮� 㪠����� ����� ��ࢮ�� ����� ����������\n");
				break;
			case 'b':
				if ((opts & OPT_BLOCKS) == 0){
					if (*optarg)
						nr_blocks = strtoul(optarg, &p, 0);
					if (!*optarg || *p || (nr_blocks > USB_KEY_BLOCKS)){
						fprintf(stderr, "����୮� ������⢮ ������ ��� �⥭��: %s\n",
							optarg);
						return false;
					}
					opts |= OPT_BLOCKS;
				}else
					fprintf(stderr, "����୮� 㪠����� ������⢠ ������ ����������\n");
				break;
			case 'v':
				verbose = true;
				break;
			case 'h':
				return false;
			case EOF:		/* ����� ��������� ��ப� */
				loop_flag = false;
				if (optind < argc){
					fprintf(stderr, "��譨� ����� � ��������� ��ப�\n");
					return false;
				}
				break;
			default:		/* �訡�� */
				return false;
		}
	}
	if ((first_block + nr_blocks) > USB_KEY_BLOCKS){
		fprintf(stderr, "�������⨬�� ������⢮ ������ ��� �⥭��: %d\n",
			nr_blocks);
		return false;
	}else
		return true;
}

static bool do_read(void)
{
	uint32_t t0 = u_times();
	if (verbose)
		fprintf(stderr, "���뢠�� ����� ������᭮��...");
	if (!usbkey_open(0)){
		if (verbose)
			fprintf(stderr, "\b\b\b: �訡��\n");
		else
			fprintf(stderr, "�訡�� ������ ����� ������᭮��\n");
		return false;
	}else if (verbose)
		fprintf(stderr, "\b\b\b: ok\n");
	if (verbose)
		fprintf(stderr, "�⥭�� ������ �� ����� ������᭮��...");
	if (!usbkey_read(0, in_data, first_block, nr_blocks)){
		if (verbose)
			fprintf(stderr, "\b\b\b: �訡��\n");
		else
			fprintf(stderr, "�訡�� ������ ����� ������᭮��\n");
		usbkey_close(0);
		return false;
	}else if (verbose)
		fprintf(stderr, "\b\b\b: ok\n");
	write(STDOUT_FILENO, in_data, nr_blocks * USB_BLOCK_LEN);
	usbkey_close(0);
	if (verbose)
		fprintf(stderr, "�⥭�� ���﫮 %u �ᥪ\n", (u_times() - t0) * 10);
	return true;
}

int main(int argc, char **argv)
{
	if (!parse_cmd_line(argc, argv)){
		usage();
		return 1;
	}else
		return do_read() ? 0 : 1;
}
