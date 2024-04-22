/* Журналы работы приложения. (c) gsr, 2014-2016, 2024 */

#include <sys/timeb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "sterm.h"
#include "termlog.h"

static int log_lvl = Info;

int get_log_lvl(void)
{
	return log_lvl;
}

void set_log_lvl(int lvl)
{
	log_lvl = lvl;
}

static const char *log_level_str(int lvl)
{
	const char *ret = NULL;
	static struct {
		int lvl;
		const char *txt;
	} map[] = {
		{Error,		"ERR"},
		{Warning,	"WRN"},
		{Info,		"INF"},
		{Debug,		"DBG"},
	};
	for (int i = 0; i < ASIZE(map); i++){
		if (map[i].lvl == lvl){
			ret = map[i].txt;
			break;
		}
	}
	if (ret == NULL){
		static char buf[4];
		snprintf(buf, ASIZE(buf), "%.3d", lvl);
		ret = buf;
	}
	return ret;
}

bool log_internal(int lvl, const char *file, const char *fn, uint32_t line, uint32_t nr_err, const char *fmt, ...)
{
	if ((lvl > log_lvl) || (file == NULL) || (fn == NULL) || (fmt == NULL))
		return false;
	struct timeb tb;
	ftime(&tb);
	struct tm *tm = localtime(&tb.time);
	va_list ap;
	va_start(ap, fmt);
	printf("%s %.2u:%.2u:%.2u.%.3u", log_level_str(lvl),
		tm->tm_hour, tm->tm_min, tm->tm_sec, tb.millitm);
	tb.time += time_delta;
	tm = localtime(&tb.time);
	printf(" [%.2u:%.2u:%.2u]: %s [%s:%u]: ", tm->tm_hour, tm->tm_min, tm->tm_sec,
		fn, file, line);
	vprintf(fmt, ap);
	va_end(ap);
	if (nr_err != UINT32_MAX)
		printf(" %s", strerror(errno));
	else
		putchar('\n');
	return true;
}
