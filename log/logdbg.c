/* Запись отладочной информации при работе с контрольными лентами. (c) gsr, 2019 */

#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "log/logdbg.h"
#include "paths.h"

/* Запись сообщений в файл журнала */
bool logdbg(const char *fmt, ...)
{
	bool ret = true;
	va_list ap;
	va_start(ap, fmt);
#if defined __LOGDBG__
#define LOG_DBG_FILE	_("logdbg.txt")
	FILE *f = fopen(LOG_DBG_FILE, "a");
	if (f != NULL){
		struct timeval tv;
		gettimeofday(&tv, NULL);
		struct tm *tm = localtime(&tv.tv_sec);
		fprintf(f, "%.2d.%.2d %.2d:%.2d:%.2d.%.3lu  ",
			tm->tm_mday, tm->tm_mon + 1, tm->tm_hour, tm->tm_min, tm->tm_sec,
			tv.tv_usec / 1000);
		vfprintf(f, fmt, ap);
		fflush(f);
		fclose(f);
	}else
		ret = false;
#else
	vfprintf(stderr, fmt, ap);
#endif		/* __LOGDBG__ */
	va_end(ap);
	return ret;
}
