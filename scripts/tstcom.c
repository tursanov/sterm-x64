/*
 * �஢�ઠ COM-���⮢. (c) gsr 2004.
 * ��� COM-���� ᮥ�������� ����� ᮡ�� ���-������� �������.
 * ��⠭���������� ᫥���騥 ��ࠬ���� ������: 115200 ���, 8n1.
 * ����� ���⠬� � �㯫��᭮� ०��� ��।����� �����.
 * �஢�ઠ ��⠥��� �ᯥ譮�, �᫨ �� ����� ��।��� ��� �訡��.
 * �ᯮ�짮�����: tstcom comN comM.
 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "serial.h"

/* ��७�ᥭ� �� timer.c � timer.h */

#define NR_TIMERS	64	/* ���ᨬ��쭮� �᫮ ⠩��஢ */

/* �� �६���� ���ࢠ�� �������� � ���� ����� ᥪ㭤� */
struct soft_timer {
	uint32_t t0;
	uint32_t timeout;	/* �᫨ 0, � ⠩��� �� �ᯮ������ */
	int signaled;
};

/* �� ⠩��஢ */
static struct soft_timer timers[NR_TIMERS];

/* �������� ������ ⠩��� */
static int alloc_timer(uint32_t timeout, int signaled)
{
	int n;
	if (timeout == 0)
		return -1;
	for (n = 0; n < NR_TIMERS; n++){
		if (timers[n].timeout == 0){
			timers[n].t0 = u_times();
			timers[n].timeout = timeout;
			timers[n].signaled = signaled;
			return n;
		}
	}
	return -1;
}

/* �᢮�������� ⠩��� */
static int release_timer(int timer)
{
	if ((timer >= 0) && (timer < NR_TIMERS) && (timers[timer].timeout > 0)){
		timers[timer].timeout = 0;
		return true;
	}else
		return false;
}

/* �����頥� true, �᫨ ⠩��� ��室���� � ���ﭨ� signalled */
static int check_timer(int timer)
{
	if ((timer >= 0) && (timer < NR_TIMERS) && (timers[timer].timeout > 0))
		return timers[timer].signaled;
	else
		return false;
}

/* ���� ⠩��� */
static int reset_timer(int timer)
{
	if ((timer >= 0) && (timer < NR_TIMERS) && (timers[timer].timeout > 0)){
		timers[timer].t0 = u_times();
		timers[timer].signaled = false;
		return true;
	}else
		return false;
}

/* ��ࠡ�⪠ ⠩��஢ */
static void process_timers(void)
{
	uint32_t t = u_times();
	int i;
	for (i = 0; i < NR_TIMERS; i++){
		if ((timers[i].timeout > 0) && !timers[i].signaled)
			timers[i].signaled = (t - timers[i].t0) >= timers[i].timeout;
	}
}

/* �뢮� �ࠢ�� �� �ᯮ�짮����� �ணࠬ�� */
static void usage(void)
{
	char *s[] = {
		"�ணࠬ�� ��� �஢�ન COM-���⮢. (c) �� ��� \"������\" 2004, 2007.",
		"�ᯮ�짮�����: tstcom comN comM.",
	};
	int i;
	for (i = 0; i < ASIZE(s); i++)
		printf("%s\n", s[i]);
}

/* ���� COM-���� */
static char com1_name[32];
/* ��ன COM-���� */
static char com2_name[32];
/* ��䨪� ����� 䠩�� ���ன�⢠ COM-���� */
#define COM_PREFIX	"/dev/ttyS"
/* ���ᨬ��쭮� ������⢮ COM-���⮢ */
#define NR_COM		4

/* ��ࠬ���� COM-���⮢ */
#define COM_BAUD	B115200
#define COM_CSIZE	CS8

/* ������ ����� ������ ��� ���஢���� */
#define DATA_LEN	65536

/* �������� �ਥ��/��।�� (��� ���� ᥪ㭤�) */
#define SND_TIMEOUT	6000
#define RCV_TIMEOUT	6000

/* ��᫮ ��室�� ��� ������� ��� */
#define N_PASSES	3
#define N_XPASSES	10

/* ���� ��� ��।�� ������ */
static uint8_t out_data[DATA_LEN];
static int sent_len;
static int snd_timer = -1;	/* ⠩��� ��।�� */
/* ���� ��� �ਥ�� ������ */
static uint8_t in_data[DATA_LEN];
static int rcv_len;
static int rcv_timer = -1;	/* ⠩��� �ਥ�� */

static void reset_data(void)
{
	sent_len = rcv_len = 0;
}

/* �������� ��⮢�� ��᫥����⥫쭮�⥩ ������ */

/* ���� "�� �㫨" */
static void all_zeroes(void)
{
	memset(out_data, 0, DATA_LEN);
}

/* ���� "�� �������" */
static void all_ones(void)
{
	memset(out_data, 0xff, DATA_LEN);
}

/* ���� "����騩 ����" */
static void running_zero(void)
{
	int i;
	for (i = 0; i < DATA_LEN; i++)
		out_data[i] = ~(uint8_t)(1 << (i % 8));
}

/* ���� "������ ������" */
static void running_one(void)
{
	int i;
	for (i = 0; i < DATA_LEN; i++)
		out_data[i] = (uint8_t)(1 << (i % 8));
}

/* ���� "����-����" */
static void address_byte(void)
{
	int i;
	for (i = 0; i < DATA_LEN; i++)
		out_data[i] = i & 0xff;
}

/* ���� "��砩�� �᫠". */
static void all_random(void)
{
	int i;
	srand(time(NULL));
	for (i = 0; i < DATA_LEN; i++)
		out_data[i] = (uint8_t)(256.0 * rand() / RAND_MAX);
}

/* ��砫� ��।�� */
static void begin_transmit(void)
{
	reset_data();
	reset_timer(snd_timer);
	reset_timer(rcv_timer);
}

static void do_back(int n)
{
	for (; n > 0; n--)
		printf("\b");
}

/* ��ࠡ�⪠ �࠭���樨 */
static bool process_transaction(int in_fd, int out_fd)
{
	ssize_t l = 0;
	bool flag = false;
	if (sent_len < DATA_LEN){
		if (check_timer(snd_timer)){
			printf("\n\033[1;31m⠩���� ��।��\033[0m\n");
			return false;
		}
		l = write(out_fd, out_data + sent_len, DATA_LEN - sent_len);
		if (l > 0){
			sent_len += l;
			flag = true;
		}else if ((l < 0) && (errno != EWOULDBLOCK)){
			printf("\n\033[1;31m�訡�� ��।��: %m\033[0m\n");
			return false;
		}
	}
	if (rcv_len < DATA_LEN){
		if (check_timer(rcv_timer)){
			printf("\n\033[1;31m⠩���� �ਥ��\033[0m\n");
			return false;
		}
		l = read(in_fd, in_data + rcv_len, DATA_LEN - rcv_len);
		if (l > 0){
			rcv_len += l;
			flag = true;
		}else if ((l < 0) && (errno != EWOULDBLOCK)){
			printf("\n\033[1;31m�訡�� �ਥ��: %m\033[0m\n");
			return false;
		}
	}
	if (flag){
		do_back(20);
		printf("\033[1;37m      %3d%%      %3d%%\033[0m",
			rcv_len * 100 / DATA_LEN, sent_len * 100 / DATA_LEN);
		fflush(stdout);
	}
	return true;
}

/* �뢮� ��ᮢ������� ������ */
static void show_diff(uint8_t *v1, uint8_t *v2, int len)
{
	int i;
	for (i = 0; i < len; i++){
		if (v1[i] != v2[i])
			printf("0x%.4x\t0x%.2hhX <-> 0x%.2hhX\n", i, v1[i], v2[i]);
	}
}

#if 0
/* ������ ���ᨢ� � 䠩� */
static void write_to_file(uint8_t *data, int len, char *name)
{
	int fd = open(name, O_WRONLY | O_CREAT, 0644);
	if (fd != -1){
		write(fd, data, len);
		close(fd);
	}
}
#endif

/* �࠭����� ��� �������� ��᫥����⥫쭮�� ������ */
static int do_transaction(int in_fd, int out_fd, void (*gen_data)(void),
	const char *fname __attribute__((unused)))
{
	gen_data();
	begin_transmit();
	while ((rcv_len != DATA_LEN) || (sent_len != DATA_LEN)){
		process_timers();
		if (!process_transaction(in_fd, out_fd))
			return false;
	}
	if (memcmp(in_data, out_data, DATA_LEN) != 0){
		printf("\n\033[1;31m�訡�� �ࠢ����� ������\033[0m\n");
		show_diff(in_data, out_data, DATA_LEN);
		return false;
	}else
		return true;
}

/* ��।������ ����� COM-����. �����頥� -1 � ��砥 �訡�� */
static int parse_com_name(char *s)
{
	int ret = -1;
	char *p;
	if (strncasecmp(s, "COM", 3) == 0){
		ret = strtoul(s + 3, &p, 10);
		if (*p || (ret > NR_COM))
			ret = -1;
		else
			ret--;
	}
	return ret;
}

/* ������ ��������� ��ப� */
static bool parse_cmd_line(int argc, char **argv)
{
	int n1, n2;
	if (argc != 3){
		usage();
		return false;
	}
	n1 = parse_com_name(argv[1]);
	if (n1 == -1){
		fprintf(stderr, "������ ��ࠬ���: %s\n", argv[1]);
		return false;
	}
	n2 = parse_com_name(argv[2]);
	if (n2 == -1){
		fprintf(stderr, "������ ��ࠬ���: %s\n", argv[2]);
		return false;
	}
/*	if (n1 == n2){
		fprintf(stderr, "㪠��� ࠧ�� COM-�����\n");
		return false;
	}*/
	snprintf(com1_name, sizeof(com1_name), COM_PREFIX "%d", n1);
	snprintf(com2_name, sizeof(com2_name), COM_PREFIX "%d", n2);
	return true;
}

/* �஢�ઠ �ࠢ����� ����� COM-���⮢ (DTR/DSR, RTS/CTS) */
static bool check_serial_lines(int fd1, int fd2)
{
	bool ret = false;
	uint32_t lines1 = serial_get_lines(fd1), lines2 = serial_get_lines(fd2);
/* DTR/DSR */
/* COM1 -> COM2 */
	if (!serial_set_lines(fd1, 0))
		printf("\033[1;31m%s: �訡�� ��� DTR\033[0m\n", com1_name);
	else if (serial_get_lines(fd2) & TIOCM_DSR)
		printf("\033[1;31m%s: DTR �� ���뢠����\033[0m\n", com1_name);
	else if (!serial_set_lines(fd1, TIOCM_DTR))
		printf("\033[1;31m%s: �訡�� ��⠭���� DTR\033[0m\n", com1_name);
	else if (!(serial_get_lines(fd2) & TIOCM_DSR))
		printf("\033[1;31m%s: DTR �� ��⠭����������\033[0m\n", com1_name);
/* COM2 -> COM1 */
	else if (!serial_set_lines(fd2, 0))
		printf("\033[1;31m%s: �訡�� ��� DTR\033[0m\n", com2_name);
	else if (serial_get_lines(fd1) & TIOCM_DSR)
		printf("\033[1;31m%s: DTR �� ���뢠����\033[0m\n", com2_name);
	else if (!serial_set_lines(fd2, TIOCM_DTR))
		printf("\033[1;31m%s: �訡�� ��⠭���� DTR\033[0m\n", com2_name);
	else if (!(serial_get_lines(fd1) & TIOCM_DSR))
		printf("\033[1;31m%s: DTR �� ��⠭����������\033[0m\n", com2_name);
/* RTS/CTS */
/* COM1 -> COM2 */
	else if (!serial_set_lines(fd1, 0))
		printf("\033[1;31m%s: �訡�� ��� RTS\033[0m\n", com1_name);
	else if (serial_get_lines(fd2) & TIOCM_CTS)
		printf("\033[1;31m%s: RTS �� ���뢠����\033[0m\n", com1_name);
	else if (!serial_set_lines(fd1, TIOCM_RTS))
		printf("\033[1;31m%s: �訡�� ��⠭���� RTS\033[0m\n", com1_name);
	else if (!(serial_get_lines(fd2) & TIOCM_CTS))
		printf("\033[1;31m%s: RTS �� ��⠭����������\033[0m\n", com1_name);
/* COM2 -> COM1 */
	else if (!serial_set_lines(fd2, 0))
		printf("\033[1;31m%s: �訡�� ��� RTS\033[0m\n", com2_name);
	else if (serial_get_lines(fd1) & TIOCM_CTS)
		printf("\033[1;31m%s: RTS �� ���뢠����\033[0m\n", com2_name);
	else if (!serial_set_lines(fd2, TIOCM_RTS))
		printf("\033[1;31m%s: �訡�� ��⠭���� RTS\033[0m\n", com2_name);
	else if (!(serial_get_lines(fd1) & TIOCM_CTS))
		printf("\033[1;31m%s: RTS �� ��⠭����������\033[0m\n", com2_name);
/* � ���� ��� DTR ������ ���� ��⨢��, � RTS ��襭 */
	else if (!serial_set_lines(fd1, lines1) ||
			!serial_set_lines(fd2, lines2))
		printf("\033[1;31m�訡�� ����⠭������� ���ﭨ� ����� �ࠢ�����\033[0m\n");
	else
		ret = true;
	return ret;
}

int main(int argc, char **argv)
{
	static struct {
		char *name;
		void (*gen_fn)(void);
		char *fname;
		int nr_passes;
	} tests[] = {
		{"�� �㫨",		all_zeroes,	"zeroes",	N_PASSES},
		{"�� �������",		all_ones,	"ones",		N_PASSES},
		{"����騩 ����",	running_zero,	"rzero",	N_XPASSES},
		{"������ ������",	running_one,	"rone",		N_XPASSES},
		{"����-����",		address_byte,	"addr",		N_XPASSES},
		{"��砩�� �᫠",	all_random,	"rnd",		N_XPASSES},
	};
	struct serial_settings ss = {
		.csize		= COM_CSIZE,
		.parity		= SERIAL_PARITY_NONE,
		.stop_bits	= SERIAL_STOPB_1,
		.control	= SERIAL_FLOW_NONE,
		.baud		= COM_BAUD
	};
	int com1, com2, i, j;
	if (!parse_cmd_line(argc, argv))
		return 1;
	printf("\033[0m\033[H\033[J\033[7;H\033[30;46m"
"                       �஢�ઠ COM-���⮢ ����� ��� ���                       \033[0m\n\n"
		"\033[1;36m"
"                �������� ���    ��室   �ਭ��  ��।���\033[0m\n");
	snd_timer = alloc_timer(SND_TIMEOUT, false);
	rcv_timer = alloc_timer(RCV_TIMEOUT, false);
	if ((snd_timer == -1) || (rcv_timer == -1)){
		fprintf(stderr, "�訡�� �뤥����� ⠩���.\n");
		return 1;
	}
	com1 = serial_open(com1_name, &ss, O_RDWR);
	if (com1 == -1)
		return 1;
	com2 = serial_open(com2_name, &ss, O_RDWR);
	if (com2 == -1){
		serial_close(com1);
		return 1;
	}
	if (!check_serial_lines(com1, com2)){
		fprintf(stderr, "�訡�� �஢�ન �ࠢ����� �����.\n");
		serial_close(com1);
		serial_close(com2);
		return 1;
	}
	ss.control = SERIAL_FLOW_RTSCTS;
	if (!serial_configure(com1, &ss) || !serial_configure(com2, &ss)){
		printf("\n\033[1;31m�訡�� ���䨣��樨 COM-���⮢\033[0m\n");
		return 1;
	}
	for (i = 0; i < ASIZE(tests); i++){
		printf("\033[1;32m%30s%30s\033[1;37m", tests[i].name, "");
		for (j = 0; j < tests[i].nr_passes; j++){
			do_back(30);
			printf("\033[1;37m%10d      %3d%%      %3d%%\033[0m", j + 1, 0, 0);
			fflush(stdout);
			if (!do_transaction(com1, com2, tests[i].gen_fn,
					tests[i].fname) ||
					!do_transaction(com2, com1, tests[i].gen_fn,
					tests[i].fname)){
				serial_close(com1);
				serial_close(com2);
				return 1;
			}
		}
		printf("\n");
	}
	serial_close(com1);
	serial_close(com2);
	release_timer(snd_timer);
	release_timer(rcv_timer);
	printf("\033[0m\n\033[30;46m"
"                            �� ���� ��諨 �ᯥ譮                            \033[0m\n");
/*	for (;;);*/
	return 0;
}
