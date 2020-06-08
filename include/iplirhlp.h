/*
 * Утилита для записи обновленных ключевых дистрибутивов в модуль
 * безопасности. (c) gsr 2007.
 */

#if !defined IPLIRHLP_H
#define IPLIRHLP_H

#include "iplir.h"

/* Имя файла с идентификатором процесса iplirhlp */
#define PID_FILE_NAME		"/var/run/terminal.pid"

/* Сигнал, который получает программа при необходимости обновления */
#define SIGUPD			42

/* Каталог, в котором находится обновленный ключевой дистрибутив */
#define NEW_DST_DIR_NAME	"/tmp/vipnet/"

#endif		/* IPLIRHLP_H */
