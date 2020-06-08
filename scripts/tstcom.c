/*
 * Проверка COM-портов. (c) gsr 2004.
 * Два COM-порта соединяются между собой нуль-модемным кабелем.
 * Устанавливаются следующие параметры обмена: 115200 бод, 8n1.
 * Между портами в дуплексном режиме передаются данные.
 * Проверка считается успешной, если все данные переданы без ошибок.
 * Использование: tstcom comN comM.
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

/* Перенесено из timer.c и timer.h */

#define NR_TIMERS	64	/* максимальное число таймеров */

/* Все временные интервалы задаются в сотых долях секунды */
struct soft_timer {
	uint32_t t0;
	uint32_t timeout;	/* если 0, то таймер не используется */
	int signaled;
};

/* Пул таймеров */
static struct soft_timer timers[NR_TIMERS];

/* Создание нового таймера */
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

/* Освобождение таймера */
static int release_timer(int timer)
{
	if ((timer >= 0) && (timer < NR_TIMERS) && (timers[timer].timeout > 0)){
		timers[timer].timeout = 0;
		return true;
	}else
		return false;
}

/* Возвращает true, если таймер находится в состоянии signalled */
static int check_timer(int timer)
{
	if ((timer >= 0) && (timer < NR_TIMERS) && (timers[timer].timeout > 0))
		return timers[timer].signaled;
	else
		return false;
}

/* Сброс таймера */
static int reset_timer(int timer)
{
	if ((timer >= 0) && (timer < NR_TIMERS) && (timers[timer].timeout > 0)){
		timers[timer].t0 = u_times();
		timers[timer].signaled = false;
		return true;
	}else
		return false;
}

/* Обработка таймеров */
static void process_timers(void)
{
	uint32_t t = u_times();
	int i;
	for (i = 0; i < NR_TIMERS; i++){
		if ((timers[i].timeout > 0) && !timers[i].signaled)
			timers[i].signaled = (t - timers[i].t0) >= timers[i].timeout;
	}
}

/* Вывод справки об использовании программы */
static void usage(void)
{
	char *s[] = {
		"Программа для проверки COM-портов. (c) АО НПЦ \"Спектр\" 2004, 2007.",
		"Использование: tstcom comN comM.",
	};
	int i;
	for (i = 0; i < ASIZE(s); i++)
		printf("%s\n", s[i]);
}

/* Первый COM-порт */
static char com1_name[32];
/* Второй COM-порт */
static char com2_name[32];
/* Префикс имени файла устройства COM-порта */
#define COM_PREFIX	"/dev/ttyS"
/* Максимальное количество COM-портов */
#define NR_COM		4

/* Параметры COM-портов */
#define COM_BAUD	B19200
#define COM_CSIZE	CS8

/* Размер блока данных для тестирования */
#define DATA_LEN	65536

/* Таймауты приема/передачи (сотые доли секунды) */
#define SND_TIMEOUT	6000
#define RCV_TIMEOUT	6000

/* Число проходов для каждого теста */
#define N_PASSES	3
#define N_XPASSES	10

/* Буфер для передачи данных */
static uint8_t out_data[DATA_LEN];
static int sent_len;
static int snd_timer = -1;	/* таймер передачи */
/* Буфер для приема данных */
static uint8_t in_data[DATA_LEN];
static int rcv_len;
static int rcv_timer = -1;	/* таймер приема */

static void reset_data(void)
{
	sent_len = rcv_len = 0;
}

/* Создание тестовых последовательностей данных */

/* Тест "все нули" */
static void all_zeroes(void)
{
	memset(out_data, 0, DATA_LEN);
}

/* Тест "все единицы" */
static void all_ones(void)
{
	memset(out_data, 0xff, DATA_LEN);
}

/* Тест "бегущий ноль" */
static void running_zero(void)
{
	int i;
	for (i = 0; i < DATA_LEN; i++)
		out_data[i] = ~(uint8_t)(1 << (i % 8));
}

/* Тест "бегущая единица" */
static void running_one(void)
{
	int i;
	for (i = 0; i < DATA_LEN; i++)
		out_data[i] = (uint8_t)(1 << (i % 8));
}

/* Тест "адрес-байт" */
static void address_byte(void)
{
	int i;
	for (i = 0; i < DATA_LEN; i++)
		out_data[i] = i & 0xff;
}

/* Тест "случайные числа". */
static void all_random(void)
{
	int i;
	srand(time(NULL));
	for (i = 0; i < DATA_LEN; i++)
		out_data[i] = (uint8_t)(256.0 * rand() / RAND_MAX);
}

/* Начало передачи */
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

/* Обработка транзакции */
static int process_transaction(int in_fd, int out_fd)
{
	int l, flag = false;
	if (sent_len < DATA_LEN){
		if (check_timer(snd_timer)){
			printf("\n\033[1;31mтаймаут передачи\033[0m\n");
			return false;
		}
		l = write(out_fd, out_data + sent_len, DATA_LEN - sent_len);
		if (l > 0){
			sent_len += l;
			flag = true;
		}else if ((l < 0) && (errno != EAGAIN)){
			printf("\n\033[1;31mошибка передачи: %s\033[0m\n",
					strerror(errno));
			return false;
		}
	}
	if (rcv_len < DATA_LEN){
		if (check_timer(rcv_timer)){
			printf("\n\033[1;31mтаймаут приема\033[0m\n");
			return false;
		}
		l = read(in_fd, in_data + rcv_len, DATA_LEN - rcv_len);
		if (l > 0){
			rcv_len += l;
			flag = true;
		}else if ((l < 0) && (errno != EAGAIN)){
			printf("\n\033[1;31mошибка приема: %s\033[0m\n",
					strerror(errno));
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

/* Вывод несовпадающих данных */
static void show_diff(uint8_t *v1, uint8_t *v2, int len)
{
	int i;
	for (i = 0; i < len; i++){
		if (v1[i] != v2[i])
			printf("0x%.4x\t0x%.2hhX <-> 0x%.2hhX\n", i, v1[i], v2[i]);
	}
}

#if 0
/* Запись массива в файл */
static void write_to_file(uint8_t *data, int len, char *name)
{
	int fd = open(name, O_WRONLY | O_CREAT, 0644);
	if (fd != -1){
		write(fd, data, len);
		close(fd);
	}
}
#endif

/* Транзакция для заданной последовательности данных */
static int do_transaction(int in_fd, int out_fd, void (*gen_data)(void),
		char *fname)
{
#define USB_PREFIX	"/mnt/usb/"
	char name[64];
	snprintf(name, sizeof(name), USB_PREFIX "%s1.bin", fname);
	gen_data();
/*	write_to_file(out_data, sizeof(out_data), name);*/
	begin_transmit();
	while ((rcv_len != DATA_LEN) || (sent_len != DATA_LEN)){
		process_timers();
		if (!process_transaction(in_fd, out_fd))
			return false;
	}
	snprintf(name, sizeof(name), USB_PREFIX "%s2.bin", fname);
/*	write_to_file(in_data, sizeof(in_data), name);*/
	if (memcmp(in_data, out_data, DATA_LEN) != 0){
		printf("\n\033[1;31mошибка сравнения данных\033[0m\n");
		show_diff(in_data, out_data, DATA_LEN);
		return false;
	}else
		return true;
}

/* Определение номера COM-порта. Возвращает -1 в случае ошибки */
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

/* Разбор командной строки */
static bool parse_cmd_line(int argc, char **argv)
{
	int n1, n2;
	if (argc != 3){
		usage();
		return false;
	}
	n1 = parse_com_name(argv[1]);
	if (n1 == -1){
		fprintf(stderr, "неверный параметр: %s\n", argv[1]);
		return false;
	}
	n2 = parse_com_name(argv[2]);
	if (n2 == -1){
		fprintf(stderr, "неверный параметр: %s\n", argv[2]);
		return false;
	}
/*	if (n1 == n2){
		fprintf(stderr, "укажите разные COM-порты\n");
		return false;
	}*/
	snprintf(com1_name, sizeof(com1_name), COM_PREFIX "%d", n1);
	snprintf(com2_name, sizeof(com2_name), COM_PREFIX "%d", n2);
	return true;
}

/* Проверка управляющих линий COM-портов (DTR/DSR, RTS/CTS) */
static bool check_serial_lines(int fd1, int fd2)
{
	bool ret = false;
	uint32_t lines1 = serial_get_lines(fd1), lines2 = serial_get_lines(fd2);
/* DTR/DSR */
/* COM1 -> COM2 */
	if (!serial_set_lines(fd1, 0))
		printf("\033[1;31m%s: ошибка сброса DTR\033[0m\n", com1_name);
	else if (serial_get_lines(fd2) & TIOCM_DSR)
		printf("\033[1;31m%s: DTR не сбрасывается\033[0m\n", com1_name);
	else if (!serial_set_lines(fd1, TIOCM_DTR))
		printf("\033[1;31m%s: ошибка установки DTR\033[0m\n", com1_name);
	else if (!(serial_get_lines(fd2) & TIOCM_DSR))
		printf("\033[1;31m%s: DTR не устанавливается\033[0m\n", com1_name);
/* COM2 -> COM1 */
	else if (!serial_set_lines(fd2, 0))
		printf("\033[1;31m%s: ошибка сброса DTR\033[0m\n", com2_name);
	else if (serial_get_lines(fd1) & TIOCM_DSR)
		printf("\033[1;31m%s: DTR не сбрасывается\033[0m\n", com2_name);
	else if (!serial_set_lines(fd2, TIOCM_DTR))
		printf("\033[1;31m%s: ошибка установки DTR\033[0m\n", com2_name);
	else if (!(serial_get_lines(fd1) & TIOCM_DSR))
		printf("\033[1;31m%s: DTR не устанавливается\033[0m\n", com2_name);
/* RTS/CTS */
/* COM1 -> COM2 */
	else if (!serial_set_lines(fd1, 0))
		printf("\033[1;31m%s: ошибка сброса RTS\033[0m\n", com1_name);
	else if (serial_get_lines(fd2) & TIOCM_CTS)
		printf("\033[1;31m%s: RTS не сбрасывается\033[0m\n", com1_name);
	else if (!serial_set_lines(fd1, TIOCM_RTS))
		printf("\033[1;31m%s: ошибка установки RTS\033[0m\n", com1_name);
	else if (!(serial_get_lines(fd2) & TIOCM_CTS))
		printf("\033[1;31m%s: RTS не устанавливается\033[0m\n", com1_name);
/* COM2 -> COM1 */
	else if (!serial_set_lines(fd2, 0))
		printf("\033[1;31m%s: ошибка сброса RTS\033[0m\n", com2_name);
	else if (serial_get_lines(fd1) & TIOCM_CTS)
		printf("\033[1;31m%s: RTS не сбрасывается\033[0m\n", com2_name);
	else if (!serial_set_lines(fd2, TIOCM_RTS))
		printf("\033[1;31m%s: ошибка установки RTS\033[0m\n", com2_name);
	else if (!(serial_get_lines(fd1) & TIOCM_CTS))
		printf("\033[1;31m%s: RTS не устанавливается\033[0m\n", com2_name);
/* В конце теста DTR должен быть активен, а RTS сброшен */
	else if (!serial_set_lines(fd1, lines1) ||
			!serial_set_lines(fd2, lines2))
		printf("\033[1;31mошибка восстановления состояния линий управления\033[0m\n");
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
		{"все нули",		all_zeroes,	"zeroes",	N_PASSES},
		{"все единицы",		all_ones,	"ones",		N_PASSES},
		{"бегущий ноль",	running_zero,	"rzero",	N_XPASSES},
		{"бегущая единица",	running_one,	"rone",		N_XPASSES},
		{"адрес-байт",		address_byte,	"addr",		N_XPASSES},
		{"случайные числа",	all_random,	"rnd",		N_XPASSES},
	};
	struct serial_settings ss = {
		.csize		= COM_CSIZE,
		.parity		= SERIAL_PARITY_NONE,
		.stop_bits	= SERIAL_STOPB_1,
		.control	= SERIAL_FLOW_RTSCTS,
		.baud		= COM_BAUD
	};
	int com1, com2, i, j;
	if (!parse_cmd_line(argc, argv))
		return 1;
	printf("\033[0m\033[H\033[J\033[7;H\033[30;46m"
"                       Проверка COM-портов модуля ЭВМ ТКП                       \033[0m\n\n"
		"\033[1;36m"
"                Название теста    Проход   Принято  Передано\033[0m\n");
	snd_timer = alloc_timer(SND_TIMEOUT, false);
	rcv_timer = alloc_timer(RCV_TIMEOUT, false);
	if ((snd_timer == -1) || (rcv_timer == -1)){
		fprintf(stderr, "Ошибка выделения таймера.\n");
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
		fprintf(stderr, "Ошибка проверки управляющих линий.\n");
		serial_close(com1);
		serial_close(com2);
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
"                            Все тесты прошли успешно                            \033[0m\n");
/*	for (;;);*/
	return 0;
}
