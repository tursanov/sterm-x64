/*
 * Проверка одновременной работы динамика, управляемого через COM-порт,
 * и считывателя DS1990A. (c) gsr 2010.
 */

#include <linux/kd.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "colortty.h"
#include "serial.h"

/* Вывод на экран сообщения об ошибке */
__attribute__((format (printf, 1, 2))) static void err_msg(const char *fmt, ...)
{
	va_list p;
	va_start(p, fmt);
	fprintf(stderr, "\n" tRED);
	vfprintf(stderr, fmt, p);
	fprintf(stderr, ANSI_DEFAULT);
	va_end(p);
}

/* Устройство для работы с клавиатурой */
#define KBD_DEV		"/dev/tty0"
/* Дескриптор устройства для работы с клавиатурой */
static int kbd = -1;

/* Исходный режим работы клавиатуры */
static int orig_kbmode = -1;
/* Исходные флаги клавиатуры */
static int orig_flags = -1;
/* Исходные настройки клавиатуры */
static struct termios _orig_tio;
/* Указатель на исходные настройки клавиатуры. Усанавливается после вызова tcgetattr */
static struct termios *orig_tio = NULL;

/* Восстановление исходных настроек клавиатуры */
static bool restore_kbd(int fd)
{
	bool ret = true;
	if (orig_kbmode != -1){
		if (ioctl(fd, KDSKBMODE, orig_kbmode) == -1){
			err_msg("%s: Ошибка ioctl(KDSKBMODE) для %s: %s.\n",
				__func__, fd2name(fd), strerror(errno));
			ret = false;
		}
		orig_kbmode = -1;
	}
	if (orig_flags != -1){
		if (fcntl(fd, F_SETFL, orig_flags) == -1){
			err_msg("%s: Ошибка fcntl(F_SETFL) для %s: %s.\n",
				__func__, fd2name(fd), strerror(errno));
			ret = false;
		}
		orig_flags = -1;
	}
	if (orig_tio != NULL){
		if (tcsetattr(fd, TCSAFLUSH, orig_tio) == -1){
			err_msg("%s: Ошибка tcsetattr для %s: %s.\n",
				__func__, fd2name(fd), strerror(errno));
			ret = false;
		}
		orig_tio = NULL;
	}
	return ret;
}

/*
 * Перевод клавиатуры в неблокирующий режим. Возвращает дескриптор устройства
 * для работы с клавиатурой или -1 в случае ошибки.
 */
static int init_kbd(void)
{
	int ret = -1;
	int fd = open(KBD_DEV, O_RDONLY);
	if (fd == -1)
		err_msg("%s: Ошибка открытия " KBD_DEV ": %s.\n", __func__,
			strerror(errno));
	else{
		orig_flags = fcntl(fd, F_GETFL);
		if (ioctl(fd, KDGKBMODE, &orig_kbmode) == -1)
			err_msg("%s: Ошибка ioctl(KDGKBMODE) для %s: %s.\n",
				__func__, fd2name(fd), strerror(errno));
		else if (tcgetattr(fd, &_orig_tio) == -1)
			err_msg("%s: Ошибка tcgetattr для %s: %s.\n",
				__func__, fd2name(fd), strerror(errno));
		else if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
			err_msg("%s: Ошибка fcntl(F_SETFL) для %s: %s.\n",
				__func__, fd2name(fd), strerror(errno));
		else{
			struct termios tio = _orig_tio;
			orig_tio = &_orig_tio;
			tio.c_iflag = 0;
			tio.c_lflag &= ~(ICANON | ECHO | ISIG);
			tio.c_cc[VTIME] = 1;
			if (tcsetattr(fd, TCSAFLUSH, &tio) == -1)
				err_msg("%s: Ошибка tcsetattr для %s: %s.\n",
					__func__, fd2name(fd), strerror(errno));
			else if (ioctl(fd, KDSKBMODE, K_MEDIUMRAW) == -1)
				err_msg("%s: Ошибка ioctl(KDSKBMODE) для %s: %s.\n",
					__func__, fd2name(fd), strerror(errno));
			else
				ret = fd;
		}
		if (ret == -1){
			restore_kbd(fd);
			close(fd);
		}
	}
	return ret;
}

/*
 * Чтение символа, введённого с клавиатуры. Возвращает 0, если входной буфер
 * пуст и 0xff в случае ошибки.
 */
static uint8_t read_kbd(int fd)
{
	uint8_t key = 0;
	ssize_t ret = read(fd, &key, 1);
	if (ret == -1){
		if (errno == EWOULDBLOCK)
			key = 0;
		else{
			err_msg("%s: Ошибка чтения из %s: %s.\n",
				__func__, fd2name(fd), strerror(errno));
			key = 0xff;
		}
	}else if (ret != 1){
		err_msg("%s: Вместо 1 байта из %s прочитано %zd.\n",
			__func__, fd2name(fd), ret);
		key = 0xff;
	}
	return key;
}

/* Очистка входного буфера клавиатуры */
static void flush_kbd(int fd)
{
	uint8_t key;
	do
		key = read_kbd(fd);
	while ((key != 0) && (key != 0xff));
}

/* Устройство для управления динамиком через COM-порт (см. include/ds1990a.h) */
#define SPEAKER_DEV		"/dev/speaker"
/* Дескриптор устройства для работы с динамиком */
static int spkr = -1;

/* Установка линии DTR COM-порта */
static bool set_com_dtr(int fd, bool val)
{
	bool ret;
	uint32_t lines = serial_get_lines(fd);
	if (val)
		lines &= ~TIOCM_DTR;
	else
		lines |= TIOCM_DTR;
	ret = serial_set_lines(fd, lines);
	if (!ret){
#if defined PREFIX
#undef PREFIX
#endif
#define PREFIX	"%s: Ошибка установки DTR %s"
		if (errno == 0)
			err_msg(PREFIX ".\n", __func__, fd2name(fd));
		else
			err_msg(PREFIX ": %s.\n", __func__, fd2name(fd),
				strerror(errno));
#undef PREFIX
	}
	return ret;
}

/*
 * Счётчик включений динамика (увеличивается в com_sound_on и уменьшается
 * в com_sound_off.
 */
static int com_sound_counter;

/* Включение динамика посредством DTR COM-порта */
static bool com_sound_on(int fd)
{
	bool ret = set_com_dtr(fd, false);
	if (ret)
		com_sound_counter++;
	return ret;
}

/* Выключение динамика посредством DTR COM-порта */
static bool com_sound_off(int fd)
{
	bool ret = set_com_dtr(fd, true);
	if (ret)
		com_sound_counter--;
	return ret;
}

/* Выключение динамика так, чтобы com_sound_counter стал равен нулю */
static bool com_sound_off_balanced(int fd)
{
	bool ret = true;
	while (com_sound_counter > 0){
		if (!com_sound_off(fd)){
			ret = false;
			break;
		}
	}
	return ret;
}

/* Открытие устройства управления динамиком */
static int open_sound_com(void)
{
	int ret = open(SPEAKER_DEV, O_RDONLY);
	if (ret == -1)
		err_msg("%s: Ошибка открытия " SPEAKER_DEV ": %s.\n", __func__,
			strerror(errno));	
	return ret;
}

/* Закрытие устройства управления динамиком */
static bool close_sound_com(int fd)
{
	bool ret = com_sound_off_balanced(fd);
	if (close(fd) == -1){
		err_msg("%s: Ошибка закрытия " SPEAKER_DEV ": %s.\n", __func__,
			strerror(errno));
		ret = false;
	}
	return ret;
}

/* Длина номера DS1990A */
#define DS_NUMBER_LEN		8

/* Текущий прочитанный номер жетона DS1990A */
static uint8_t ds_number[DS_NUMBER_LEN] = {[0 ... sizeof(ds_number) - 1] = 0};

/* IPC для работы с утилитой чтения DS1990A */

#define SH_NULL		(void *)(-1)

/* Файл для создания key_t с помощью ftok */
#define FTOK_FILE	"/home/sterm/dsd"

/* Размер разделяемой памяти */
#define SHM_SIZE	4096
/* Идентификатор сегмента разделяемой памяти */
static int shmid = -1;
/* Указатель на сегмент разделяемой памяти */
static uint8_t *shmptr = SH_NULL;
/* Семафор для доступа к разделяемой памяти */
static int semid = -1;

/* Создание сегмента разделяемой памяти */
static bool create_shm(void)
{
	bool ret = false;
	shmid = shmget(ftok(FTOK_FILE, 0), SHM_SIZE, 0);
	if (shmid == -1)
		err_msg("%s: Ошибка создания сегмента разделяемой памяти: %s.\n",
			__func__, strerror(errno));
	else{
		shmptr = (uint8_t *)shmat(shmid, (const void *)0, 0);
		if (shmptr == SH_NULL)
			err_msg("%s: Ошибка получения указателя для "
				"разделяемой памяти: %s.\n", __func__,
				strerror(errno));
		else
			ret = true;
	}
	return ret;
}

/* Создание семафора */
static bool create_sem(void)
{
	bool ret = false;
	semid = semget(ftok(FTOK_FILE, 0), 1, 0);
	if (semid == -1)
		err_msg("%s: Ошибка создания семафора: %s.\n", __func__,
			strerror(errno));
	else
		ret = true;
	return ret;
}

/* Захват семафора */
static bool ds_get_sem(void)
{
	bool ret = true;
	struct sembuf sb[] = {
		{0, 0, 0},		/* дождаться освобождения семафора */
		{0, 1, 0},		/* захватить семафор */
	};
	if (semop(semid, sb, ASIZE(sb)) == -1){
		err_msg("%s: Ошибка semop: %s.\n", __func__, strerror(errno));
		ret = false;
	}
	return ret;
}

/* Освобождение семафора */
static bool ds_release_sem(void)
{
	bool ret = true;
	struct sembuf sb[] = {
		{0, -1, 0},
	};
	if (semop(semid, sb, ASIZE(sb)) == -1){
		err_msg("%s: Ошибка semop: %s.\n", __func__, strerror(errno));
		ret = false;
	}
	return ret;
}

/* Начало работы со считывателем */
bool ds_init(void)
{
	return create_shm() && create_sem();
}

/* Чтение номера ключа DS1990A */
static bool ds_read(uint8_t dsn[DS_NUMBER_LEN])
{
	int i;
	bool ret = false;
	if (ds_get_sem()){
		memcpy(dsn, shmptr, DS_NUMBER_LEN);
		ds_release_sem();
		for (i = 0; (i < DS_NUMBER_LEN) && !ret; i++){
			if (dsn[i] != 0)
				ret = true;
		}
	}else
		memset(dsn, 0, DS_NUMBER_LEN);
	return ret;
}

/* Вывод на экран заставки с описанием программы */
static void show_banner(void)
{
	printf(CLR_SCR tYLW "Проверка совместной работы динамика и считывателя "
		"DS1990A." ANSI_DEFAULT "\n"
		"(Для выхода нажмите Esc).\n\n\n");
}

/* Вывод на экран сообщения об окончании работы программы */
static void show_final_banner(void)
{
	printf(ANSI_DEFAULT "\n\n\n" tYLW "Работа программы завершена."
		ANSI_DEFAULT "\n");
}

/* Выдача звукового сигнала. Длительность сигнала задаётся в мс */
static bool sound(int delay)
{
	bool ret = com_sound_on(spkr);
	if (ret)
		usleep(delay * 1000);
	if (!com_sound_off(spkr))
		ret = false;
	return ret;
}

/* Пауза между звуковыми сигналами. Длительность паузы задаётся в мс */
static bool nosound(int delay)
{
	bool ret = true, first = true;
	uint8_t dsn[DS_NUMBER_LEN];
	uint32_t t0 = u_times();
	while (ret){
		if (!ds_read(dsn)){
			err_msg("%s: Ошибка чтения номера жетона.\n", __func__);
			ret = false;
		}else if (!first && (memcmp(dsn, ds_number, sizeof(dsn)) != 0)){
			err_msg("%s: Считанный номер отличается от считанного ранее.\n",
				__func__);
			ret = false;
		}else{
			uint8_t key = read_kbd(kbd);
			if (key == 0x01)
				ret = false;
			else{
				memcpy(ds_number, dsn, sizeof(dsn));
				usleep(1000);
				first = false;
				if (((u_times() - t0) * 10) > delay)
					break;
			}
		}
	}
	return ret;
}

/* Инициализация необходимых устройств */
static bool init(void)
{
	bool ret = false;
	kbd = init_kbd();
	if (kbd != -1){
		spkr = open_sound_com();
		if (spkr != -1)
			ret = ds_init();
	}
	if (ret){
		flush_kbd(kbd);
		show_banner();
	}
	return ret;
}

/* Завершение работы программы */
static void release(void)
{
	if (kbd != -1){
		restore_kbd(kbd);
		kbd = -1;
	}
	if (spkr != -1){
		close_sound_com(spkr);
		spkr = -1;
	}
	show_final_banner();
}

/* "Мелодия", которую будет играть программа */
#define BASE_INTERVAL		100
static struct {
	int sound_delay;	/* длительность звука (мс) */
	int nosound_delay;	/* длительность паузы (мс) */
} melody[] = {
	{1 * BASE_INTERVAL,	3 * BASE_INTERVAL},
	{2 * BASE_INTERVAL,	3 * BASE_INTERVAL},
	{4 * BASE_INTERVAL,	3 * BASE_INTERVAL},
	{8 * BASE_INTERVAL,	3 * BASE_INTERVAL},
	{4 * BASE_INTERVAL,	3 * BASE_INTERVAL},
	{2 * BASE_INTERVAL,	3 * BASE_INTERVAL},
};

/* Циклическое исполнение "мелодии" */
static void do_buzzer(void)
{
	bool flag = true;
	while (flag){
		int i;
		for (i = 0; flag && (i < ASIZE(melody)); i++)
			flag =	sound(melody[i].sound_delay) &
				nosound(melody[i].nosound_delay);
	}
}

int main()
{
	if (init())
		do_buzzer();
	release();
	return 0;
}
