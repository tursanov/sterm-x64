/* Журналы работы приложения. (c) gsr, 2014-2016, 2024, 2026 */

#include <sys/time.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "x3data/common.h"
#include "paths.h"
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

static const char *cur_pattern = NULL;
static regex_t reg;

static int log_selector(const struct dirent *entry)
{
	return regexec(&reg, entry->d_name, 0, NULL, 0) == REG_NOERROR;
}

static ssize_t del_excess_logs(const char *folder, const char *prefix, const char *ext,
	size_t max_files)
{
	if ((folder == NULL) || (prefix == NULL) || (ext == NULL))
		return -1;
	char pattern[256];
	snprintf(pattern, sizeof(pattern), "^%s-[0-9]{4}-[0-9]{2}-[0-9]{2}\\.%s$",
		prefix, ext);
	if ((cur_pattern == NULL) || (strcmp(cur_pattern, pattern) != 0)){
		if (cur_pattern != NULL){
			free((void *)cur_pattern);
			cur_pattern = NULL;
		}
		if (regcomp(&reg, pattern, REG_EXTENDED | REG_NOSUB) != REG_NOERROR)
			return -1;
		cur_pattern = strdup(pattern);
	}
	struct dirent **names;
	int n = scandir(folder, &names, log_selector, alphasort);
	if (n == -1)
		return -1;
	ssize_t ret = 0;
	if (n > max_files){
		char path[PATH_MAX];
		for (int i = 0; (i + max_files) < n; i++){
			snprintf(path, sizeof(path), "%s/%s", folder, names[i]->d_name);
			if (unlink(path) == 0)
				ret++;
		}
	}
	return ret;
}

static time_t last_write_date = 0;
#define SECONDS_IN_DAY	(24 * 3600)
static const char *log_name_prefix = "log";

static inline bool create_log_folder_if_need(void)
{
	return create_folder_if_need(LOG_FOLDER);
}

bool log_internal(int lvl, const char *file, const char *fn, uint32_t line, uint32_t nr_err, const char *fmt, ...)
{
	if ((lvl > log_lvl) || (file == NULL) || (fn == NULL) || (fmt == NULL))
		return false;
	else if (!create_log_folder_if_need())
		return false;
	static char path[PATH_MAX];
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm *tm = localtime(&tv.tv_sec);
	snprintf(path, sizeof(path), LOG_FOLDER "/%s-%.4u-%.2u-%.2u.txt",
		log_name_prefix, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	FILE *f = fopen(path, "a");
	if (f == NULL)
		return false;
	va_list ap;
	va_start(ap, fmt);
	fprintf(f, "%s %.2u:%.2u:%.2u.%.3ld", log_level_str(lvl),
		tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec / 1000);
	tv.tv_sec += time_delta;
	tm = localtime(&tv.tv_sec);
	fprintf(f, " [%.2u:%.2u:%.2u]: %s [%s:%u]: ", tm->tm_hour, tm->tm_min, tm->tm_sec,
		fn, file, line);
	vfprintf(f, fmt, ap);
	va_end(ap);
	if (nr_err != UINT32_MAX)
		fprintf(f, " %s", strerror(errno));
	else
		fputc('\n', f);
	fflush(f);
	fclose(f);
	if (tv.tv_sec > (last_write_date + SECONDS_IN_DAY)){
		del_excess_logs(LOG_FOLDER, log_name_prefix, "txt", MAX_LOG_FILES);
		last_write_date = (tv.tv_sec / SECONDS_IN_DAY) * SECONDS_IN_DAY;
	}
	return true;
}

bool log_data(const char *prefix, const char *title, const uint8_t *data, size_t len)
{
	if ((prefix == NULL) || (data == NULL) || (len == 0))
		return false;
	else if (!create_log_folder_if_need())
		return false;
#define LINE_LEN	16
#define HALF_LINE_LEN	(LINE_LEN / 2)
	static char path[PATH_MAX];
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm *tm = localtime(&tv.tv_sec);
	snprintf(path, sizeof(path), LOG_FOLDER "/%s-%.4u-%.2u-%.2u.txt",
		prefix, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	FILE *f = fopen(path, "a");
	if (f == NULL)
		return false;
	fprintf(f, "%.2u:%.2u:%.2u.%.3ld", tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec / 1000);
	if (title != NULL)
		fprintf(f, " [%s]", title);
	fputc('\n', f);
	char txt[LINE_LEN];
	size_t l = ((len + LINE_LEN - 1) / LINE_LEN) * LINE_LEN;
	for (size_t i = 0, j = 0; i <= l; i++){
		if ((i % LINE_LEN) == 0){
			if (i > 0)
				fprintf(f, "       %.*s\n", (int)j, txt);
			if (i < l)
				fprintf(f, "%.4zx ", i);
			else{
				fputc('\n', f);
				break;
			}
			j = 0;
		}else if ((i > 0) && ((i % HALF_LINE_LEN) == 0))
			fprintf(f, (i < len) ? " --" : "   ");
		if (i < len){
			uint8_t b = data[i];
			fprintf(f, " %.2hhx", b);
			txt[j++] = (b < 0x20) ? '.' : recode(b);
		}else
			fprintf(f, "   ");
	}
	fflush(f);
	fclose(f);
	if (tv.tv_sec > (last_write_date + SECONDS_IN_DAY)){
		del_excess_logs(LOG_FOLDER, prefix, "txt", MAX_LOG_FILES);
		last_write_date = (tv.tv_sec / SECONDS_IN_DAY) * SECONDS_IN_DAY;
	}
	return true;
}
