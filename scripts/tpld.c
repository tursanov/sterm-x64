/*
 * Загрузка данных в термопринтер через последовательный интерфейс.
 * (c) gsr 2005.
 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/times.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "cfg.h"
#include "colortty.h"
#include "hex.h"
#include "serial.h"

/* Максимальный размер передаваемого блока без учета контрольной суммы */
#define MAX_BLOCK_SIZE		4096	/* 4K */
/* Максимальный размер передаваемого файла */
#define MAX_FILE_SIZE		32768	/* 32K */

/* NB: все таймауты задаются в сотых долях секунды */
/* Таймаут передачи команды */
#define CMD_SEND_TIMEOUT	100
/* Таймаут передачи блока данных */
#define DATA_SEND_TIMEOUT	3000
/* Таймаут ожидания ответа от принтера */
/*#define WRESP_TIMEOUT		300*/
#define WRESP_TIMEOUT		3000
/* Таймаут приостановки передачи */
#define EOT_TIMEOUT		1000

/* Максимальное число повторов команды или данных */
#define MAX_RETRIES		7

/* Устройство для работы с COM-портом */
static int com_dev = -1;

/* Файл данных для передачи в термопринтер */
static int data_fd = -1;
/* Если этот флаг установлен, данные записываются в интерфейс принтера, а не во FLASH */
static bool dta;
/* COM-порт, к которому подключен термопринтер (--serial-port) */
static int serial_port = -1;
/* Очередной подготовленный блок для передачи */
static uint8_t data[(MAX_BLOCK_SIZE + 2) * 2 + 2];	/* STX data CRC ETX */
/* Длина текущего блока данных */
static int data_len;
/* Номер текущего блока данных */
static int nr_block;

/* Эта заглушка нужна для cfg.c. Настоящая функция находится в sterm.c */
void flush_home(void)
{
}

/* Вывод на экран */
#define ACTION_LEN		30
#define DESC_COL		31
#define LINE_COL(n)		ANSI_PREFIX _s(n) "G"
#define LINE_HOME		LINE_COL(1)

static void print_action(char *s)
{
	printf(LINE_COL(ACTION_LEN) CLR_SOL LINE_HOME tWHT "%s" ANSI_DEFAULT, s);
	fflush(stdout);
}

static void print_desc(char *s)
{
	printf(LINE_COL(DESC_COL) tCYA "%s" ANSI_DEFAULT CLR_EOL, s);
	fflush(stdout);
}

static void print_line(char *action, char *desc)
{
	print_action(action);
	print_desc(desc);
}

static void print_success(void)
{
	printf(LINE_COL(DESC_COL) tGRN "ok" ANSI_DEFAULT CLR_EOL "\n");
	fflush(stdout);
}

static void print_completion(void)
{
	printf(tGRN "передача данных завершена\n" ANSI_DEFAULT);
}

static void print_error(char *s, bool show_errno)
{
	printf(LINE_COL(DESC_COL) tRED "%s", s);
	if (show_errno)
		printf(" (%d)", errno);
	printf(ANSI_DEFAULT CLR_EOL "\n");
	fflush(stdout);
}

static void print_wresp(void)
{
	print_desc("ожидание ответа...");
}

static void print_nak(void)
{
	print_error("запрос повторной передачи", false);
}

static void print_nak_over(void)
{
	print_error("слишком много повторов", false);
}

static void print_block_begin(void)
{
	char buf[80];
	snprintf(buf, sizeof(buf), "передача блока #%d", nr_block + 1);
	print_line(buf, "передача...");
}

static void print_unexpected(uint32_t b)
{
	char buf[80];
	snprintf(buf, sizeof(buf), "неожиданный ответ: %.2x", b);
	print_error(buf, false);
}

/* Закрытие COM-порта */
static void close_com(void)
{
	if (com_dev != -1){
		serial_close(com_dev);
		com_dev = -1;
	}
}

/* Открытие COM-порта с заданным номером (нумерация с 0) */
static bool open_com(int n)
{
	struct serial_settings ss = {
		.csize		= CS8,
		.parity		= SERIAL_PARITY_NONE,
		.stop_bits	= SERIAL_STOPB_1,
		.control	= SERIAL_FLOW_NONE,
		.baud		= B19200
	};
	char com_name[16];
	close_com();
	snprintf(com_name, sizeof(com_name), "/dev/ttyS%d", n);
	com_dev = serial_open(com_name, &ss, O_RDWR);
	return com_dev != -1;
}

/*
 * Передача данных по COM-порту с заданным таймаутом.
 * Если timeout == -1, проверка таймаута не производится.
 * Возвращаемые значения:
 *  -1 -- ошибка передачи;
 * >=0 -- данные были переданы; если код возврата меньше размера данных,
 *	значит при передаче произошел таймаут.
 */
static int send_com(uint8_t *data, int len, int timeout)
{
	uint32_t t0 = u_times();
	int i, ret, lsr;
	if ((com_dev == -1) || (data == NULL) || (len <= 0))
		return -1;
	for (i = 0; i < len; i += ret){
		ret = write(com_dev, data + i, len - i);
		if ((ret == -1) && (errno != EAGAIN)){
			print_error("ошибка передачи", true);
			return -1;
		}else if ((timeout != -1) && ((u_times() - t0) > timeout)){
			print_error("таймаут передачи", false);
			break;
		}else if (ret == -1)
			ret = 0;
	}
/*
 * В случае передачи всех данных в буфер COM-порта необходимо дождаться
 * окончания передачи в канал.
 */
	if (serial_port < 2){
		do {
			if (ioctl(com_dev, TIOCSERGETLSR, &lsr) == -1){
				print_error("ошибка получения статуса COM-порта", true);
				return -1;
			}else if ((timeout != -1) && ((u_times() - t0) > timeout)){
				print_error("таймаут передачи", false);
				break;
			}
		} while (!(lsr & TIOCSER_TEMT));
	}
	return i;
}

/* Команды загрузчика термопринтера (см. tp/include/loader.h) */
#define LD_SEL		0x01		/* выборка */
#define LD_STX		0x02		/* начало блока текста */
#define LD_ETX		0x03		/* конец блока текста */
#define LD_ACK		0x04		/* подтверждение блока текста */
#define LD_NAK		0x05		/* неподтверждение блока текста */
#define LD_ENQ		0x06		/* непонятное сообщение */
#define LD_EOT		0x07		/* временное прекращение передачи */
#define LD_DTA		0x08		/* данные для передачи в интерфейс принтера */
#define LD_RST		0x7f		/* переход в начальное состояние */

/* Передача различных команд загрузчику */
static int write_cmd(uint8_t cmd)
{
	return send_com(&cmd, 1, CMD_SEND_TIMEOUT);
}

#define write_sel()	write_cmd(LD_SEL)
#define write_enq()	write_cmd(LD_ENQ)
#define write_rst()	write_cmd(LD_RST)

/* Вычисление контрольной суммы по полиному x^0 + x^13 + x^15 */
static uint16_t crc(uint8_t *p, int l)
{
	uint16_t div = 1 | (1 << 13) | (1 << 15);
	uint32_t s = 0;
	int i;
	if (p != NULL){
		for (i = 0; i < l; i++){
			s <<= 8;
			s |= p[i];
			s %= div;
		}
	}
	return (uint16_t)s;
}

/* Закрытие файла данных */
static void close_data(void)
{
	if (data_fd != -1){
		close(data_fd);
		data_fd = -1;
	}
}

/* Проверка и открытие файла данных */
static bool open_data(char *name)
{
	struct stat st;
	close_data();
	data_len = 0;
	if (name == NULL)
		return false;
	if (stat(name, &st) == -1){
		fprintf(stderr, "ошибка получения данных о файле %s: %s\n",
				name, strerror(errno));
		return false;
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, "%s не является стандартным файлом\n", name);
		return false;
	}else if (!dta && (st.st_size > MAX_FILE_SIZE)){
		fprintf(stderr, "размер файла ПО не должен превышать "
				_s(MAX_FILE_SIZE) " байт\n");
		return false;
	}
	data_fd = open(name, O_RDONLY);
	if (data_fd == -1){
		fprintf(stderr, "ошибка открытия %s для чтения: %s\n",
				name, strerror(errno));
		return false;
	}else
		return true;
}

/* Чтение очередного блока файла */
static int read_next_block(void)
{
	static uint8_t block[MAX_BLOCK_SIZE];
	int block_len, i;
	if (data_fd == -1)
		return -1;
	block_len = read(data_fd, block, sizeof(block));
	if (block_len == -1){
		print_error("ошибка чтения данных из файла", true);
		return -1;
	}else if (block_len == 0)
		return 0;
	data[0] = dta ? LD_DTA : LD_STX;
	for (i = 0; i < block_len; i++)
		write_hex_byte(data + 2 * i + 1, block[i]);
	data_len = 2 * block_len + 1;
	write_hex_word(data + data_len, crc(block, block_len));
	data[data_len + 4] = LD_ETX;
	data_len += 5;
	nr_block++;
	return block_len;
}

/*
 * Чтение очередного байта из COM-порта. Если нет данных для чтения,
 * функция возвращает 0. Если при чтении произошла ошибка возвращается -1.
 * При нормальном чтении функция возвращает 1.
 */
int read_com(uint8_t *b)
{
	int ret;
	if ((com_dev == -1) || (b == NULL))
		return -1;
	ret = read(com_dev, b, 1);
	if (ret == 1)
		return 1;
	else if ((ret == -1) && (errno == EAGAIN))
		return 0;
	else{
		print_error("ошибка приема", true);
		return -1;
	}
}

/* Состояния конечного автомата загрузчика */
enum {
	st_start,		/* начальное состояние */
	st_sel,			/* послана выборка, ожидаем ACK */
	st_txt,			/* отправлен блок текста, ожидается подтверждение */
	st_eot,			/* временная приостановка передачи */
	st_end,			/* конечное состояние */
	st_err,			/* ошибка */
};

static int st = st_start;

/* Время начала операции */
static uint32_t t0;
/* Число повторений операции */
static int nr_retries;

/*
 * Функции обработки состояний конечного автомата загрузчика.
 * Если b == -1UL, данных по COM-порту не получено.
 */
static void on_st_start(uint32_t b __attribute__((unused)))
{
	int ret;
	nr_block = -1;
	lseek(data_fd, 0, SEEK_SET);
	ret = read_next_block();
	if (ret == 0){
		st = st_end;
		return;
	}else{
		st = st_err;
		if (ret == -1)
			return;
	}
	print_line("установка соединения", "запрос связи...");
	if ((write_rst() != 1) || (write_sel() != 1))
		return;
	print_wresp();
	t0 = u_times();
	nr_retries = 0;
	st = st_sel;
}

static void on_st_sel(uint32_t b)
{
	if (b != -1U){
		switch (b){
			case LD_ENQ:
				print_desc("повторный запрос связи...");
				if (write_sel() != 1)
					st = st_err;
				else
					print_wresp();
				break;
			case LD_ACK:
				print_success();
				print_block_begin();
				if (send_com(data, data_len, DATA_SEND_TIMEOUT) == data_len){
					print_wresp();
					t0 = u_times();
					nr_retries = 0;
					st = st_txt;
				}else
					st = st_err;
				break;
			default:
				print_unexpected(b);
				st = st_err;
		}
	}else if ((u_times() - t0) > WRESP_TIMEOUT){
		print_error("таймаут ожидания ответа на выборку", false);
		st = st_start;
	}
}

static void on_st_txt(uint32_t b)
{
	int ret;
	if (b != -1U){
		switch (b){
			case LD_NAK:
				if (++nr_retries >= MAX_RETRIES){
					print_nak_over();
					st = st_err;
				}else{
					print_nak();
					print_block_begin();
					if (send_com(data, data_len, DATA_SEND_TIMEOUT) == data_len){
						print_wresp();
						t0 = u_times();
					}else
						st = st_err;
				}
				break;
			case LD_EOT:
				print_desc("обработка принтером...");
				t0 = u_times();
				st = st_eot;
				break;
			case LD_ACK:
				ret = read_next_block();
				if (ret == -1)
					st = st_err;
				else if (ret == 0){
					print_success();
					print_completion();
					st = st_end;
				}else
					on_st_sel(b);
				break;
			default:
				print_unexpected(b);
				st = st_err;
		}
	}else if ((u_times() - t0) > WRESP_TIMEOUT){
		print_error("таймаут ожидания ответа на блок текста", false);
		st = st_start;
	}
}

static void on_st_eot(uint32_t b)
{
	if (b != -1U){
		switch (b){
			case LD_ACK:
				on_st_txt(b);
				break;
			case LD_NAK:
				if (++nr_retries >= MAX_RETRIES){
					print_nak_over();
					st = st_err;
				}else{
					print_nak();
					print_block_begin();
					if (send_com(data, data_len, DATA_SEND_TIMEOUT) == data_len){
						print_wresp();
						t0 = u_times();
					}else
						st = st_err;
				}
				break;
			default:
				print_unexpected(b);
				st = st_err;
		}
	}else if ((u_times() - t0) > EOT_TIMEOUT){
		print_error("таймаут обработки данных принтером", false);
		st = st_start;
	}
}

static void process_fsa(uint32_t b)
{
	if (b == LD_RST){
		print_error("соединение разорвано", false);
		nr_retries = 0;
		t0 = u_times();
		lseek(data_fd, 0, SEEK_SET);
		st = st_start;
		return;
	}
	switch (st){
		case st_start:
			on_st_start(b);
			break;
		case st_sel:
			on_st_sel(b);
			break;
		case st_txt:
			on_st_txt(b);
			break;
		case st_eot:
			on_st_eot(b);
			break;
	}
}

/* Возвращаемое значение программы */
static int ret_val = 1;

/* Возвращает false, если надо завершить программу */
static bool do_loader(void)
{
	uint32_t b = 0;
	int ret;
	if (st == st_end){
		ret_val = 0;
		return false;
	}else if (st == st_err)
		return false;
	ret = read_com((uint8_t *)&b);
	if (ret == -1)
		return false;
	else if (ret == 0)
		b = -1U;
	process_fsa(b);
	return true;
}

/* Разбор командной строки */
static bool parse_cmd_line(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	char *shortopts = "ds:", *p;
	const struct option longopts[] = {
		{"dta",		no_argument,		NULL,	'd'},
		{"serial-port",	required_argument,	NULL,	's'},
		{NULL,		0,			NULL,	0},
	};
	bool loop_flag = true, ret_flag = true;
	while (loop_flag){
		switch (getopt_long_only(argc, argv, shortopts, longopts, NULL)){
			case 'd':
				dta = true;
				break;
			case 's':
				serial_port = strtoul(optarg, &p, 0);
				if (*p != 0){
					fprintf(stderr, "неверный формат аргумента --serial-port\n");
					serial_port = -1;
				}
				break;
			case ':':
			case '?':
				ret_flag = false;
			case EOF:
				loop_flag = false;
				break;
		}
	}
	if (ret_flag){
		if (optind == argc){
			fprintf(stderr, "не указан файл данных\n");
			ret_flag = false;
		}else if (optind < (argc - 1)){
			fprintf(stderr, "нельзя указывать несколько файлов данных\n");
			ret_flag = false;
		}else
			ret_flag = open_data(argv[optind]);
	}
	return ret_flag;
}

/* Определение COM-порта, к которому подключен термопринтер */
static bool find_tprn(void)
{
	if (serial_port == -1){
		if (!read_cfg()){
			fprintf(stderr, "ошибка чтения конфигурации терминала\n");
			return false;
		}else if (!cfg.has_aprn){
			fprintf(stderr, "не удалось найти COM-порт, к которому подключен термопринтер\n");
			return false;
		}else
			serial_port = cfg.aprn_tty;
	}
	return open_com(serial_port);
}

int main(int argc, char **argv)
{
	if (parse_cmd_line(argc, argv) && find_tprn())
		while (do_loader());
	close_data();
	close_com();
	return ret_val;
}
