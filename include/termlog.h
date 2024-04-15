/* Журналы работы приложения. (c) gsr, 2014-2016, 2024 */

#pragma once

#include "sysdefs.h"

enum {
	Error = 0,
	Warning,
	Info,
	Debug,
};

extern bool log_internal(int lvl, const char *file, const char *fn, uint32_t line, uint32_t nr_err, const char *fmt, ...);
#define log_generic(lvl, fmt, nr_err, ...) log_internal(lvl, __FILE__, __FUNCTION__, __LINE__, nr_err, fmt, __VA_ARGS__)
#define log_err(fmt, ...)		log_generic(Error, fmt, UINT32_MAX, __VA_ARGS__)
#define log_win_err(fmt, ...)		log_generic(Error, fmt, errno, __VA_ARGS__)
#define log_warn(fmt, ...)		log_generic(Warning, fmt, UINT32_MAX, __VA_ARGS__)
#define log_info(fmt, ...)		log_generic(Info, fmt, UINT32_MAX, __VA_ARGS__)
#define log(fmt, ...)			log_info(fmt, __VA_ARGS__)
#define log_dbg(fmt, ...)		log_generic(Debug, fmt, UINT32_MAX, __VA_ARGS__)

/* Некоторые предопределённые сообщения для занесения в журнал */
#define LOG_NULL_PTR		"Получен нулевой указатель."
#define LOG_INVALID_ARG		"Неверные аргументы."
#define LOG_NO_XPRN		"БПУ отсутствует."
#define LOG_XPRN_BUSY		"БПУ занято."
#define LOG_NO_FR		"ККТ отсутствует."
#define LOG_FR_BUSY		"ККТ занята."
#define LOG_NO_BNK_BIN		"Банковская корзина отсутствует."
