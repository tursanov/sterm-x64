/* ��騥 �㭪樨 ��� ࠡ��� � ����஫�묨 ���⠬�. (c) gsr 2008. */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "log/generic.h"
#include "log/logdbg.h"
#include "genfunc.h"
#include "sterm.h"

/* �������� ����஫쭮� ����� ��������� ⨯� */
bool log_create(struct log_handle *hlog)
{
	int fd;
	bool ret = false;
	hlog->init_log_hdr(hlog);
/*	if (!remount_home(true)){
		logdbg("%s: �� ���� ��६���஢��� ���⥫� �ନ���� "
			"��� �����: %s.\n", __func__, strerror(errno));
		return false;
	}*/
	fd = open(hlog->log_name, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
		S_IRUSR | S_IWUSR);
	if (fd == -1)
		logdbg("%s: �訡�� ᮧ����� 䠩�� %s: %s.\n", __func__,
			hlog->log_type, strerror(errno));
	else if (write(fd, hlog->hdr, hlog->hdr_len) != hlog->hdr_len)
		logdbg("%s: �訡�� ����� ��������� %s: %s.\n", __func__,
			hlog->log_type, strerror(errno)); 
	else if (!fill_file(fd, hlog->hdr->len))
		logdbg("%s: �訡�� ����� � 䠩� %s: %s.\n", __func__,
			hlog->log_type, strerror(errno));
	else
		ret = true;
	fsync(fd);
	close(fd);
/*	remount_home(false);*/
	flush_home();
	return ret;
}

/* ����樮��஢���� � �⥭�� �����뢭��� ����� ������ �� �� */
bool log_atomic_read(struct log_handle *hlog, uint32_t offs, uint8_t *buf, uint32_t l)
{
	bool ret = false;
	if (lseek(hlog->rfd, offs, SEEK_SET) == (off_t)-1)
		logdbg("%s: �訡�� ����樮��஢���� %s �� ᬥ饭�� %u "
			"��� �⥭��: %s.\n", __func__, hlog->log_type, offs, strerror(errno));
	else if (read(hlog->rfd, buf, l) != l)
		logdbg("%s: �訡�� �⥭�� %u ���� %s �� ᬥ饭�� %u: %s.\n", __func__,
			l, hlog->log_type, offs, strerror(errno));
	else
		ret = true;
	return ret;
}

/* �⥭�� 䠩�� ����஫쭮� ����� */
bool log_read(struct log_handle *hlog, uint32_t offs, uint8_t *buf, uint32_t l)
{
	uint32_t l1, l2 = 0;
	if ((hlog == NULL) || (hlog->rfd == -1) || (buf == NULL) ||
			(l == 0) || (offs >= hlog->full_len))
		return false;
	l1 = hlog->full_len - offs;
	if (l1 > l)
		l1 = l;
	else
		l2 = l - l1;
	return	log_atomic_read(hlog, offs, buf, l1) && ((l2 == 0) ||
		log_atomic_read(hlog, hlog->hdr_len, buf + l1, l2));
}

/* ����樮��஢���� � ������ �����뢭��� ����� ������ �� �� */
bool log_atomic_write(struct log_handle *hlog, uint32_t offs, uint8_t *buf, uint32_t l)
{
	bool ret = false;
	if (lseek(hlog->wfd, offs, SEEK_SET) == (off_t)-1)
		logdbg("%s: �訡�� ����樮��஢���� %s �� ᬥ饭�� %u "
			"��� �����: %s.\n", __func__, hlog->log_type, offs, strerror(errno));
	else if (write(hlog->wfd, buf, l) != l)
		logdbg("%s: �訡�� ����� %u ���� �� %s �� ᬥ饭�� %u: %s.\n", __func__,
			l, hlog->log_type, offs, strerror(errno));
	else
		ret = true;
	return ret;
}

/* ������ � 䠩� ����஫쭮� ����� */
bool log_write(struct log_handle *hlog, uint32_t offs, uint8_t *buf, uint32_t l)
{
	uint32_t l1, l2 = 0;
	if ((hlog == NULL) || (hlog->wfd == -1) || (buf == NULL) ||
			(l == 0) || (offs >= hlog->full_len))
		return false;
	l1 = hlog->full_len - offs;
	if (l1 > l)
		l1 = l;
	else
		l2 = l - l1;
	return	log_atomic_write(hlog, offs, buf, l1) && ((l2 == 0) ||
		log_atomic_write(hlog, hlog->hdr_len, buf + l1, l2));
}

/* ��砫� ����� �� ����஫��� ����� */
bool log_begin_write(struct log_handle *hlog)
{
	bool ret = false;
/*	remount_home(true);*/
	hlog->wfd = open(hlog->log_name, O_WRONLY | O_SYNC);
	if (hlog->wfd == -1)
		logdbg("%s: �訡�� ������ %s ��� �����: %s.\n", __func__,
			hlog->log_type, strerror(errno));
	else
		ret = true;
	return ret;
}

/* ����砭�� ����� �� ����஫��� ����� */
void log_end_write(struct log_handle *hlog)
{
	if (hlog->wfd != -1){
		fsync(hlog->wfd);
		close(hlog->wfd);
		hlog->wfd = -1;
	}
/*	remount_home(false);*/
}

/* �����ﭨ� ����� ���� ᬥ饭�ﬨ � ����楢�� ���� */
uint32_t log_delta(struct log_handle *hlog, uint32_t tail, uint32_t head)
{
	return (tail - head + hlog->hdr->len) % hlog->hdr->len;
}

/* �����祭�� ������ � ����楢�� ���� */
uint32_t log_inc_index(struct log_handle *hlog, uint32_t index, uint32_t delta)
{
	return (index - hlog->hdr_len + delta) % hlog->hdr->len + hlog->hdr_len;
}

/* ��।������ ᢮������� ����࠭�⢠ �� �� */
uint32_t log_free_space(struct log_handle *hlog)
{
	if (hlog->hdr->n_recs == 0)
		return hlog->hdr->len;
	else if (hlog->hdr->head == hlog->hdr->tail)
		return 0;
	else
		return log_delta(hlog, hlog->hdr->head, hlog->hdr->tail);
}

/* ��।������ ����஢ �ࠩ��� ����ᥩ �� �� */
bool log_get_boundary_numbers(struct log_handle *hlog,
		uint32_t *n0, uint32_t *n1)
{
	if ((n0 == NULL) || (n1 == NULL) || (hlog->hdr->n_recs == 0))
		return false;
	*n0 = hlog->map[hlog->map_head].number;
	*n1 = hlog->map[(hlog->map_head + hlog->hdr->n_recs - 1) % hlog->map_size].number;
	return true;
}

/* ��।������ ����� ���������, ��࠭�祭���� �����ᠬ� */
uint32_t log_get_interval_len(struct log_handle *hlog,
		uint32_t index1, uint32_t index2)
{
	if ((index1 >= hlog->map_size) || (index2 >= hlog->map_size))
		return 0;
	else
		return (index2 + hlog->map_size - index1) % hlog->map_size + 1;
}

/*
 * ��।������ ������ ����� � ������� ����஬. �����頥� -1UL,
 * �᫨ ������ �� �������.
 */
uint32_t log_index_for_number(struct log_handle *hlog, uint32_t number)
{
	for (uint32_t i = 0; i < hlog->hdr->n_recs; i++){
		if (hlog->map[(hlog->map_head + i) % hlog->map_size].number == number)
			return i;
	}
	return -1U;
}

/*
 * ���� ����� �� �� ������. �����頥� ������ ����� �� �� ��� -1,
 * �᫨ ������ �� �������
 */
uint32_t log_find_rec_by_number(struct log_handle *hlog, uint32_t number)
{
	uint32_t i;
	for (i = 0; i < hlog->hdr->n_recs; i++){
		if (hlog->map[(hlog->map_head + i) % hlog->map_size].number == number)
			return i;
	}
	return -1U;
}

/*
 * ���� ����� �� �� �� ��� ᮧ�����. �����頥� ������ ����� � ������襩
 * ��⮩ ᮧ�����.
 */
uint32_t log_find_rec_by_date(struct log_handle *hlog, struct date_time *dt)
{
	if (dt == NULL)
		return -1U;
	uint32_t d = -1U, index = -1U;
	for (uint32_t i = 0; i < hlog->hdr->n_recs; i++){
		uint32_t dd = *(uint32_t *)&hlog->map[(hlog->map_head + i) % hlog->map_size].dt -
			*(uint32_t *)dt;
		if (dd & 0x80000000)
			dd = ~dd + 1;	/* NB: LONG_MIN �� ��������� */
		if (dd < d){
			d = dd;
			index = i;
		}
	}
	return index;
}

/* ���⪠ ����஫쭮� ����� */
bool log_clear(struct log_handle *hlog, bool need_write)
{
	bool ret = false;
	bool flag = need_write ? log_begin_write(hlog) : true;
	if (flag){
		hlog->clear(hlog);
		ret = need_write ? hlog->write_header(hlog) : true;
	}
	if (need_write)
		log_end_write(hlog);
	return ret;
}

/*
 * �᫨ �� �஢�થ ����� �� �� �㤥� �����㦥�� �訡��, �� � ��
 * ��᫥���騥 ����� ���� ���襭�.
 */
bool log_truncate(struct log_handle *hlog, uint32_t index, uint32_t tail)
{
	bool ret = false;
	if (index <= hlog->hdr->n_recs){
		hlog->hdr->n_recs = index;
		hlog->hdr->tail = tail;
		if (log_begin_write(hlog)){
			ret = hlog->write_header(hlog);
			log_end_write(hlog);
		}
		memset(hlog->map + index, 0, sizeof(struct map_entry_t));
	}
	if (ret){
		logdbg("%s: �� %s ��࠭��� %u ����ᥩ.\n", __func__, hlog->log_type, index);
		flush_home();
	}
	return ret;
}

/* ����⨥ ����஫쭮� ����� */
bool log_open(struct log_handle *hlog, bool can_create)
{
/* FIXME: �� ᮧ����� �� on_open ����� ��뢠���� ������ */
	if (hlog->on_open != NULL)
		hlog->on_open(hlog);
	hlog->rfd = open(hlog->log_name, O_RDONLY);
	if ((hlog->rfd != -1) && hlog->read_header(hlog) && hlog->fill_map(hlog))
		return true;
	else if (can_create){
		if (hlog->rfd != -1)
			close(hlog->rfd);
		logdbg("%s: %s �㤥� ᮧ���� ������.\n", __func__, hlog->log_type);
		return log_create(hlog) && log_open(hlog, false);
	}else
		return false;
}

/* �����⨥ ����஫쭮� ����� */
void log_close(struct log_handle *hlog)
{
	if (hlog->on_close != NULL)
		hlog->on_close(hlog);
	if (hlog->rfd != -1){
		close(hlog->rfd);
		hlog->rfd = -1;
	}
}

/* ���������� ���� �����᪮�� ����� �ਭ�� */
void fill_prn_number(uint8_t *dst, const char *src, size_t len)
{
	size_t i;
	if ((dst != NULL) && (src != NULL)){
		for (i = 0; (i < len) && *src; i++)
			dst[i] = src[i];
		for (; i < len; i++)
			dst[i] = 0x30;
	}
}


/* ����� ����஫쭮� ����� */

/* ���� ���� ����஫쭮� ����� */
uint8_t log_prn_buf[PRN_BUF_LEN] = {[0 ... PRN_BUF_LEN - 1] = 0};
/* ����� ������ � ���� ���� */
uint32_t log_prn_data_len = 0;

/* ������ ᨬ���� � ���� ���� ��� ��४���஢�� */
bool prn_write_char_raw(uint8_t c)
{
	if (log_prn_data_len < sizeof(log_prn_buf)){
		log_prn_buf[log_prn_data_len++] = c;
		return true;
	}else
		return false;
}

/* ������ ᨬ���� � ���� ���� � ��४���஢��� */
bool prn_write_char(uint8_t c)
{
	return prn_write_char_raw(recode(c));
}

/* ������ ��㯯� ᨬ����� ��� ��४���஢�� */
bool prn_write_chars_raw(const char *s, int n)
{
	if (n == -1)
		n = strlen(s);
	for (; n > 0; n--)
		try_fn(prn_write_char_raw(*s++));
	return true;
}

/* ������ � ���� ��ப� � ��४���஢��� */
bool prn_write_str(const char *str)
{
	for (; *str; str++)
		try_fn(prn_write_char(*str));
	return true;
}

/* ������ � ���� ��ப� �� �ଠ�� � ��४���஢��� */
bool prn_write_str_fmt(const char *fmt, ...)
{
	static char str[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);
	return prn_write_str(str);
}

/* ������ � ���� ���� ᨬ���� ���� ��ப� */
bool prn_write_eol(void)
{
	return prn_write_str("\x0a\x0d");
}

/* ������ � ���� ���� ⥪��� ���� � �६��� */
bool prn_write_cur_date_time(void)
{
	time_t t;
	struct tm *tm;
	char s[128];
	t = time(NULL) + time_delta;
	tm = localtime(&t);
	snprintf(s, sizeof(s), "%.2d.%.2d.%d %.2d:%.2d:%.2d",
		tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	return prn_write_str(s);
}

/* ������ � ���� ���� �������� ���� � �६��� */
bool prn_write_date_time(struct date_time *dt)
{
	char s[128];
	if (dt == NULL)
		return false;
	snprintf(s, sizeof(s), "%.2d.%.2d.%d %.2d:%.2d:%.2d",
		dt->day + 1, dt->mon + 1, dt->year + 2000,
		dt->hour, dt->min, dt->sec);
	return prn_write_str(s);
}

/* ���� ���� ���� � ��砫쭮� ���ﭨ� */
bool log_reset_prn_buf(void)
{
	log_prn_data_len = 0;
	return true;
}

/* �뢮� �� ����� ����-���� */
bool log_print_bcode(const uint8_t *log_data, uint32_t log_data_len, uint32_t *log_data_index)
{
	enum {
		st_start,
		st_digit1,
		st_semi1,
		st_digit2,
		st_semi2,
		st_stop,
	};
	int st = st_start, n = 0;
	uint8_t b;
	bool need_print;
	for (; (*log_data_index < log_data_len) && (st != st_stop); (*log_data_index)++){
		b = log_data[*log_data_index];
		need_print = true;
		switch (st){
			case st_start:
				if (b == ';')
					st = st_semi1;
				else if (isdigit(b)){
					n = 2;
					(*log_data_index)--;
					st = st_digit1;
				}else
					return false;
				need_print = false;
				break;
			case st_semi1:
				if (b == ';')
					st = st_semi2;
				else if (isdigit(b)){
					n = 15;
					(*log_data_index)--;
					st = st_digit2;
				}else
					return false;
				need_print = false;
				break;
			case st_digit1:
				if (n > 0){
					if (!isdigit(b))
						return false;
					n--;
				}else if (b == ';')
					st = st_semi2;
				else if (isdigit(b)){
					n = 15;
					(*log_data_index)--;
					st = st_digit2;
				}else
					return false;
				need_print = false;
				break;
			case st_semi2:
				if (isdigit(b)){
					n = 13;
					(*log_data_index)--;
					st = st_digit2;
				}else
					return false;
				need_print = false;
				break;
			case st_digit2:
				if (!isdigit(b))
					return false;
				if (--n == 0)
					st = st_stop;
				need_print = n < 13;
				break;
		}
		if (need_print)
			try_fn(prn_write_char_raw(b));
	}
	if (st == st_stop){
		(*log_data_index)--;
		return true;
	}else
		return false;
}
