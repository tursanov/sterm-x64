/*
 * Работа с PPP. (c) gsr 2003.
 */

#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gui/scr.h"
#include "iplir.h"
#include "ppp.h"

/* Дескриптор очереди сообщений для взаимодействия с pppd */
int msqid = -1;
/* Состояние демона ppp */
int ppp_state = ppp_nc;
/* В случае ошибки PPP будет вызван slayer_error */
static bool ppp_verbose;

/* Является ли указанный файл каталогом */
static bool is_dir(char *name)
{
	struct stat buf;
	if (stat(name, &buf) == 0)
		return S_ISDIR(buf.st_mode);
	else
		return false;
}

static bool is_all_digits(char *name)
{
	if (name != NULL){
		for (; *name; name++)
			if (!isdigit(*name))
				return false;
		return true;
	}else
		return false;
}

/*
 * Проверка того, что заданный каталог содержит символьную ссылку 'exe',
 * указывающую на файл 'name'.
 */
static bool check_name(char *dir, char *name)
{
	struct stat buf;
	int l;
	char lnk[128], exe[128];
	snprintf(lnk, sizeof(lnk), "%s/exe", dir);
	if (lstat(lnk, &buf) == -1)
		return false;
	else if (S_ISLNK(buf.st_mode)){
		l = readlink(lnk, exe, sizeof(exe) - 1);
		if (l > 0){
			exe[l] = 0;
			return !strcmp(exe, name);
		}
	}
	return false;
}

/* Определение идентификатора процесса по имени файла */
pid_t get_process_id(char *name)
{
	pid_t pid = -1;
	char s[128];
	struct dirent *p;
	DIR *dir = opendir("/proc");
	if (dir == NULL)
		return -1;
	for (p = readdir(dir); p != NULL; p = readdir(dir)){
		snprintf(s, sizeof(s), "/proc/%s", p->d_name);
		if (is_dir(s) && is_all_digits(p->d_name) && check_name(s, name)){
			pid = atoi(p->d_name);
			break;
		}
	}
	closedir(dir);
	return pid;
}

#define get_pppd_id() get_process_id(PPP_DAEMON)

/* Проверка присутствия в системе процесса с заданным идентификатором */
bool pid_in_sys(pid_t pid)
{
	char lnk[128];
	struct stat buf;
	sprintf(lnk, "/proc/%d/exe", pid);
	if (stat(lnk, &buf) == 0)
		return S_ISLNK(buf.st_mode);
	else
		return false;
}

/* Проверка присутствия в системе процесса с заданным именем */
bool app_in_sys(char *name)
{
	return (name == NULL) ? false : get_process_id(name) != -1;
}

/* Проверка присутствия pppd в системе */
#define pppd_in_sys() app_in_sys(PPP_DAEMON)
#if 0
static bool pppd_in_sys(void)
{
	return system("killall -q -0 pppd") == 0;
}
#endif

/* Посылка pppd сигнала завершения работы */
static void stop_pppd(void)
{
	system("killall -q -TERM pppd");
	system("killall -q -TERM pppd");	/* почему-то нужно 2 сигнала */
}
/* Посылка pppd сигнала SIGKILL */
static void kill_pppd(void)
{
	system("killall -q -KILL pppd");
	system("killall -q -KILL pppd");
}

/* Создание файла /etc/ppp/xxxx-secrets при авторизации PAP/CHAP */
static bool create_secrets_file(char *fname)
{
	FILE *f;
	if (fname == NULL)
		return false;
	f = fopen(fname, "w");
	if (f != NULL){
		fprintf(f, "%s\t*\t\"%s\"\t*\n",
			cfg.ppp_login, cfg.ppp_passwd);
		fclose(f);
		return true;
	}else
		return false;
}

/* Создание файлов xxxx-secrets при необходимости */
static void create_ppp_secrets(void)
{
	unlink(PPP_PAP_FILE);
	unlink(PPP_CHAP_FILE);
	switch (cfg.ppp_auth){
		case ppp_pap:
			create_secrets_file(PPP_PAP_FILE);
			break;
		case ppp_chap:
			create_secrets_file(PPP_CHAP_FILE);
			break;
	}
}

/* Печать в конец строки */
/* FIXME: нет проверки переполнения буфера */
static int xprintf(char *s, char *format, ...)
{
	va_list p;
	if ((s == NULL) || (format == NULL))
		return 0;
	va_start(p, format);
	return vsprintf(s + strlen(s), format, p);
}

/* Запись в файл для chat строки инициализации */
static void write_init_str(FILE *f)
{
	int i, l;
	fprintf(f, "TIMEOUT %d\n", PPP_TIMEOUT_INIT);
	fprintf(f, "SAY '" PPP_STR_INIT "'\n");
	fprintf(f, "'' '\\d");
	l = strlen(cfg.ppp_init);
	for (i = 0; i < l; i++){
		if (cfg.ppp_init[i] == '\'')
			fputc('\\', f);
		fputc(cfg.ppp_init[i], f);
	}
	fputc('\'', f);
}

/* Формирование аргументов для программы chat */
static bool make_chat_script(void)
{
	bool at_init = cfg.ppp_init[0] != 0;
	bool dialup = cfg.ppp_phone[0] != 0;
	FILE *f = fopen(PPP_CHAT_FILE, "w");
	if (f == NULL){
		fprintf(stderr, "ошибка открытия " PPP_CHAT_FILE " для записи: %s\n",
				strerror(errno));
		return false;
	}
	create_ppp_secrets();
	fprintf(f, "ECHO OFF\n");
	if (dialup){
		fprintf(f, "ABORT 'BUSY'\n");
		fprintf(f, "ABORT 'NO CARRIER'\n");
		fprintf(f, "ABORT 'NO DIAL TONE'\n");
		fprintf(f, "ABORT 'VOICE'\n");
		fprintf(f, "ABORT 'NO ANSWER'\n");
		if (at_init){
			write_init_str(f);
			fprintf(f, "\nOK '");
		}else
			fprintf(f, "'' '\\d");
		fprintf(f, cfg.ppp_tone_dial ? "ATDT" : "ATDP");
		fprintf(f, "%s'\n", cfg.ppp_phone);
		fprintf(f, "TIMEOUT %d\n", PPP_TIMEOUT_DIAL);
		fprintf(f, "SAY '" PPP_STR_DIALING "'\n");
		fprintf(f, "CONNECT ''\n");
		if (cfg.ppp_auth == ppp_chat){
			fprintf(f, "TIMEOUT %d\n", PPP_TIMEOUT_LOGIN);
			fprintf(f, "SAY '" PPP_STR_LOGIN "'\n");
			fprintf(f, "ogin: '%s'\n", cfg.ppp_login);
			fprintf(f, "sword: '\\q%s'\n", cfg.ppp_passwd);
			fprintf(f, "SAY '" PPP_STR_IPCP "'\n");
		}else
			fprintf(f, "SAY '" PPP_STR_LOGIN "'\n");
	}else{
		if (at_init){
			write_init_str(f);
			fprintf(f, "\nOK ''\n");
		}
		fprintf(f, "SAY '" PPP_STR_LEASED "'\n");
	}
	fclose(f);
	return true;
}

/* Формирование командной строки для pppd */
char *make_pppd_string(void)
{
	static char s[1024];
	make_chat_script();
	sprintf(s, "pppd /dev/ttyS%d %d %s use-ipc %s "
			"modem defaultroute noauth "
			"noipdefault ipcp-accept-local ipcp-accept-remote ",
			cfg.ppp_tty, cfg.ppp_speed,
			cfg.ppp_crtscts ? "crtscts" : "xonxoff",
			PPP_IPC_FILE);
	if (cfg.ppp_auth != ppp_chat)
		xprintf(s, "user '%s' ", cfg.ppp_login);
	xprintf(s, "debug connect 'chat -vm " PPP_IPC_FILE
			" -f " PPP_CHAT_FILE "'");
	printf("pppd string: [%s]\n", s);
	return s;
}

/* Работа с очередью сообщений */
/* Инициализация очереди */
bool init_ppp_ipc(void)
{
	key_t key = ftok(PPP_IPC_FILE, PPP_IPC_ID);
	if (key == -1)
		return false;
	msqid = msgget(key, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
/* Если очередь сообщений уже существует, удаляем ее и пытаемся создать новую */
	if ((msqid == -1) && (errno == EEXIST)){
		msqid = msgget(key, 0);
		if ((msqid == -1) || (msgctl(msqid, IPC_RMID, NULL) == -1))
			return false;
		msqid = msgget(key, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	}
	return msqid != -1;
}

/* Освобождение очереди */
void release_ppp_ipc(void)
{
	if (msqid != -1){
		msgctl(msqid, IPC_RMID, NULL);
		msqid = -1;
	}
//	stop_pppd();	/* для Инфотекс */
}

/* Определение ошибки сеансового уровня в зависимости от ppp_state */
static int serr_from_ppp_state(void)
{
	switch (ppp_state){
		case ppp_starting:
			return PPPERR_STARTING;
		case ppp_init:
			return PPPERR_INIT;
		case ppp_dialing:
			return PPPERR_DIALING;
		case ppp_login:
			return PPPERR_LOGIN;
		case ppp_ipcp:
			return PPPERR_IPCP;
		default:
			return 0;
	}
}

static bool can_draw_hints(void)
{
	return	cfg.has_hint &&
		!menu_active &&
		!optn_active &&
		!calc_active &&
		!xlog_active &&
		!plog_active &&
		!llog_active &&
		!xchg_active &&
		!help_active &&
		!ssaver_active &&
		!ping_active &&
		!pos_active;
}

static void on_ppp_up_down(void)
{
	if (can_draw_hints())
		show_hints();
}

/* Начало очередной операции PPP */
static time_t ppp_t0;

/* Опрос очереди сообщений IPC. */
static void process_ppp_ipc(void)
{
	char buf[PPP_MSG_MAX_SIZE + sizeof(long)];
	struct msgbuf *mb = (struct msgbuf *)buf;
	int l;
	time_t t;
	bool err;
	if ((msqid == -1) || !pppd_in_sys()){
		if (ppp_verbose)
			slayer_error(serr_from_ppp_state());
		ppp_state = ppp_nc;
		return;
	}
	l = msgrcv(msqid, mb, PPP_MSG_MAX_SIZE, 0, IPC_NOWAIT);
	t = time(NULL);
	if (l <= 0){
/*
 * При проверке таймаутов мы используем '>' вместо '>=', чтобы позволить
 * pppd и chat самим обрабатывать таймауты.
 */
		switch (ppp_state){
/*			case ppp_starting:
				err =  (t - ppp_t0) > PPP_TIMEOUT_START;
				break;*/
			case ppp_init:
				err =  (t - ppp_t0) > PPP_TIMEOUT_INIT + 1;
				break;
			case ppp_dialing:
				err = (t - ppp_t0) > PPP_TIMEOUT_DIAL + 1;
				break;
			case ppp_login:
				err = (t - ppp_t0) > PPP_TIMEOUT_LOGIN + 1;
				break;
			case ppp_ipcp:
				err = (t - ppp_t0) > PPP_TIMEOUT_IPCP + 1;
				break;
			default:
				err = false;
		}
		if (err){
			if (ppp_verbose)
				slayer_error(serr_from_ppp_state());
			ppp_hangup();
/*			stop_pppd();
			ppp_state = ppp_nc;
			usleep(100000);*/
		}
		return;
	}
	for (l--; l > 0; l--){
		if (isspace(mb->mtext[l - 1]))
			mb->mtext[l - 1] = 0;
		else
			break;
	}
	if (mb->mtype == PPP_MSG_TYPE){
		printf("сообщение от chat: %s\n", mb->mtext);
		if (strcmp(mb->mtext, PPP_STR_INIT) == 0){
			ppp_t0 = t;
			ppp_state = ppp_init;
			set_term_state(st_ppp_init);
		}else if (strcmp(mb->mtext, PPP_STR_DIALING) == 0){
			ppp_t0 = t;
			ppp_state = ppp_dialing;
			set_term_state(st_ppp_dialing);
		}else if (strcmp(mb->mtext, PPP_STR_LOGIN) == 0){
			ppp_t0 = t;
			ppp_state = ppp_login;
			set_term_state(st_ppp_login);
		}else if ((strcmp(mb->mtext, PPP_STR_IPCP) == 0) ||
				(strcmp(mb->mtext, PPP_STR_LEASED) == 0)){
			ppp_t0 = t;
			ppp_state = ppp_ipcp;
			set_term_state(st_ppp_ipcp);
		}else
			printf("Неизвестное сообщение от pppd: [%s]\n",	mb->mtext);
	}else if (mb->mtype == IPCP_MSG_TYPE){
		printf("сообщение от pppd: %s\n", mb->mtext);
		if (strcmp(mb->mtext, PPP_IP_UP_SCRIPT) == 0){
			ppp_state = ppp_ready;
			on_ppp_up_down();
		}else if (strcmp(mb->mtext, PPP_IP_DOWN_SCRIPT) == 0){
			ppp_state = ppp_nc;
			on_ppp_up_down();
		}else
			printf("Неизвестное сообщение от pppd: [%s]\n",	mb->mtext);
	}
}

/* Запуск pppd */
bool start_pppd(void)
{
	return system(make_pppd_string()) == 0;
}

/* Модальная установка сеанса PPP */
bool ppp_connect(void)
{
	int cmd;
	if (ppp_state == ppp_ready)
		return true;
	else if (ppp_state != ppp_nc){
		if (!ppp_hangup())
			return false;
	}
	if (start_pppd()){
		ppp_state = ppp_starting;
		set_term_state(st_ppp_startup);
	}else{
		ppp_state = ppp_nc;
		slayer_error(PPPERR_STARTING);
		return false;
	}
	ppp_verbose = true;
	while (1){
		process_ppp_ipc();
		if ((ppp_state == ppp_ready) || (ppp_state == ppp_nc))
			break;
		cmd = get_cmd(false, true);
		if (cmd == cmd_reset){
			if (reset_term(false)){
				kill_pppd();
				ppp_state = ppp_nc;
				return ppp_verbose = false;
			}
		}else if (cmd == cmd_ppp_hangup){
			hangup_ppp();
			return ppp_verbose = false;
		}
	}
	ppp_verbose = false;
	guess_term_state();
	if (ppp_state == ppp_ready){
//		remove(PPP_CHAT_FILE);
		set_ppp_cfg();
	}
	return ppp_state == ppp_ready;
}

/* Модальный разрыв сенса PPP */
bool ppp_hangup(void)
{
	time_t t0 = time(NULL);
	if (ppp_state == ppp_nc)
		return true;
	stop_pppd();
	ppp_state = ppp_stopping;
	set_term_state(st_ppp_cleanup);
	ppp_verbose = false;
	while (1){
		process_ppp_ipc();
		if (ppp_state == ppp_nc)
			break;
		else if (get_cmd(false, true) == cmd_reset){
			if (reset_term(false)){
				kill_pppd();
				return false;
			}
		}else if ((time(NULL) - t0) >= PPP_TIMEOUT_STOP){
			kill_pppd();
			break;
		}
	}
	ppp_state = ppp_nc;
	guess_term_state();
	return true;
}

/* Обработка PPP после установки соединения */
void ppp_process(void)
{
	if (ppp_state == ppp_ready)
		process_ppp_ipc();
}
