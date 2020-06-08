/* Работа с VipNet Client (Инфотекс). (c) gsr 2003-2020 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "base64.h"
#include "cfg.h"
#include "iplir.h"
#include "tki.h"

bool iplir_disabled = false;		/* работа с VipNet невозможна */
bool has_iplir = false;			/* ключевой дистрибутив распакован нормально */

/* Запуск iplir */
bool start_iplir(void)
{
/*	if (has_iplir && (system(IPLIR_CTL " start") == 0) &&
			(system(MFTP_CTL " start") == 0)){
		sleep(1);
		return true;
	}else
		return false;*/
	return true;
}

/* Остановка iplir */
bool stop_iplir(void)
{
/*	if (has_iplir && (system(IPLIR_CTL " stop") == 0) &&
			(system(MFTP_CTL " stop") == 0)){
		sleep(1);
		return true;
	}else
		return false;*/
	return true;
}

/* Проверка наличия модуля iplir */
bool is_iplir_loaded(void)
{
/*	return	is_module_loaded(IPLIR_MOD_NAME) &&
		is_module_loaded(IPLIR_MOD1_NAME);*/
	return true;
}
