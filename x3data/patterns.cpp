/* ����� � 蠡������ ���� ���. (c) gsr 2022, 2024 */

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <zlib.h>
#include "x3data/common.hpp"
#include "x3data/patterns.h"
#include "x3data/xslts.h"
#include "gui/scr.h"
#include "express.h"
#include "paths.h"
#include "sterm.h"
#include "termlog.h"
#include "transport.h"
#include "xbase64.h"

/* ����� 蠡����� ���� � "������" */
static string x3_kkt_patterns_version;

static time_t jul_date_to_unix_date(const char *jul_date)
{
	if ((jul_date == NULL) || (strlen(jul_date) != 5))
		return -1;
	struct tm tm;
	if (sscanf(jul_date, "%2u%3u", &tm.tm_year, &tm.tm_mday) != 2)
		return -1;
	tm.tm_year += 100;
	tm.tm_mon = 0;
	tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
	tm.tm_yday = tm.tm_wday = 0;
	tm.tm_isdst = -1;
	return mktime(&tm);
}

static regex_t reg;

static int pattern_selector(const struct dirent *entry)
{
	return regexec(&reg, entry->d_name, 0, NULL, 0) == REG_NOERROR;
}

static time_t get_local_patterns_date()
{
	if (!create_folder_if_need(PATTERNS_FOLDER)){
		log_err("��⠫�� " PATTERNS_FOLDER " �� ������� � �� ����� ���� ᮧ���.");
		return -1;
	}
	int rc = regcomp(&reg, "^S00[0-9]{5}$", REG_EXTENDED | REG_NOSUB);
	if (rc != REG_NOERROR){
		log_err("�訡�� �������樨 ॣ��୮�� ��ࠦ���� ���: %d.", rc);
		return -1;
	}
	struct dirent **names;
	int n = scandir(PATTERNS_FOLDER, &names, pattern_selector, alphasort);
	if (n == -1){
		log_sys_err("�訡�� ���᪠ 蠡����� ���� � ��⠫��� " PATTERNS_FOLDER ":");
		regfree(&reg);
		return -1;
	}else if (n == 0){
		log_err("� ��⠫��� " PATTERNS_FOLDER " �� ������ 䠩� ����.");
		regfree(&reg);
		return -1;
	}else if (n > 1)
		log_info("� ��⠫��� " PATTERNS_FOLDER " ������� ����� ������ 䠩�� ���� (%d); "
			"�㤥� �ᯮ�짮��� %s.", n, names[0]->d_name);
	else
		log_dbg("�����㦥� 䠩� %s.", names[0]->d_name);
	time_t ret = jul_date_to_unix_date(names[0]->d_name + 3);
	free(names);
	regfree(&reg);
	return ret;
}

static bool need_patterns_update(const string &x3_version)
{
	bool ret = false;
	time_t old_t = get_local_patterns_date(), new_t = jul_date_to_unix_date(x3_version.c_str());
	if (old_t == -1)
		ret = new_t != -1;
	else
		ret = (new_t != -1) && (new_t > old_t);
	return ret;
}

bool need_patterns_update(void)
{
	return need_patterns_update(x3_kkt_patterns_version);
}

static int clr_selector(const struct dirent *entry)
{
	return	(strcmp(entry->d_name, ".") != 0) &&
		(strcmp(entry->d_name, "..") != 0);
}

static int clr_old_patterns()
{
	struct dirent **names;
	int n = scandir(PATTERNS_FOLDER, &names, clr_selector, alphasort);
	if (n == -1){
		log_sys_err("�訡�� ��ᬮ�� ��⠫��� " PATTERNS_FOLDER ":");
		return -1;
	}
	int ret = 0;
	for (int i = 0; i < n; i++){
		static char path[PATH_MAX];
		snprintf(path, ASIZE(path), PATTERNS_FOLDER "/%s", names[i]->d_name);
		struct stat st;
		if (stat(path, &st) == -1)
			log_sys_err("�訡�� ����祭�� ���ଠ樨 � 䠩�� %s:", path);
		else if (((st.st_mode & S_IFMT) == S_IFREG) ||
				((st.st_mode & S_IFMT) == S_IFLNK)){
			if (unlink(path) == -1)
				log_sys_err("�訡�� 㤠����� 䠩�� %s:", path);
			else
				ret++;
		}
	}
	free(names);
	return ret;
}

static bool check_x3_kkt_patterns(const uint8_t *data, size_t len, string &version)
{
	log_info("data = %p; len = %u.", data, len);
	if ((data == NULL) || (len == 0))
		return false;
	static uint8_t tag[] = {0x3c, 0x53, 0x30, 0x30};	/* <S00 */
	bool ret = false;
	for (size_t i = 0; (data[i] == 0x3c) && ((i + 1) < len); i++){
		const uint8_t *p = (const uint8_t *)memchr(data + i + 1, 0x3e, len - i - 1);
		if (p == NULL)
			break;
		else if ((memcmp(data + i, tag, ASIZE(tag)) == 0) && ((p - (data + i)) == 9)){
			version.assign((const char *)data + i + 4, 5);
			log_info("�����㦥�� 蠡���� ���� ��� (����� %s).", version.c_str());
			ret = true;
			break;
		}else
			i += p - (data + i);
	}
	return ret;
}

bool check_x3_kkt_patterns(const uint8_t *data, size_t len)
{
	return check_x3_kkt_patterns(data, len, x3_kkt_patterns_version);
}

/* ���ᨬ���� ࠧ��� ���� ᦠ��� 蠡����� ���� */
static const size_t MAX_COMPRESSED_PATTERNS_LEN = 65536;
/* ���� ��� ��� ᦠ��� 蠡����� ���� */
static uint8_t patterns_buf[MAX_COMPRESSED_PATTERNS_LEN];
/* ����騩 ࠧ��� �ਭ���� ������ ᦠ��� 蠡����� ���� */
static size_t patterns_buf_idx = 0;

/* ��⮧���� ��� ����祭�� 蠡����� ���� �� ���� */
static uint8_t patterns_auto_req[REQ_BUF_LEN];
/* ����� ��⮧���� */
static size_t patterns_auto_req_len = 0;

/* ��ࠢ�� ��砫쭮�� ����� �� ����祭�� 蠡����� ���� */
static void send_patterns_request(const string &version)
{
	int offs = get_req_offset();
	req_len = offs;
	req_len += snprintf((char *)req_buf + req_len, ASIZE(req_buf) - req_len,
		"P55TR(EXP.DATA.MAKET   S00%s)000000000", version.c_str());
	set_scr_request(req_buf + offs, req_len - offs, true);
	wrap_text();
}

/* ��ࠢ�� ��⮧���� ��� ����祭�� 蠡����� ���� */
static void send_patterns_auto_request()
{
	req_len = get_req_offset();
	memcpy(req_buf + req_len, patterns_auto_req, patterns_auto_req_len);	/* FIXME */
	set_scr_request(patterns_auto_req, patterns_auto_req_len, true);
	req_len += patterns_auto_req_len;
	wrap_text();
}

static inline bool is_cr_lf(char b)
{
	return (b == 0x0a) || (b == 0x0d);
}

static char *next_str(char *buf, size_t len)
{
	char *ret = NULL;
	bool crlf = false;
	for (size_t i = 0; i < len; i++){
		char ch = buf[i];
		if (!crlf)
			crlf = is_cr_lf(ch);
		else if (!is_cr_lf(ch)){
			ret = buf + i;
			break;
		}
	}
	return ret;
}

static const size_t MAX_KKT_PATTERNS_DATA_LEN = 65536;

/* ������஢���� 蠡����� ���� ���, �ᯠ����� � ��࠭���� � 䠩�� �� ��᪥ */
static bool store_patterns()
{
	log_info("");
	if (patterns_buf_idx == 0){
		log_err("���� ������ ����.");
		return false;
	}
	size_t len = 0;
	if (!xbase64_decode(patterns_buf, patterns_buf_idx, patterns_buf, patterns_buf_idx, &len) ||
			(len == 0)){
		log_err("�訡�� ������஢���� ������.");
		return false;
	}else
		log_info("����� 蠡����� ���� ��� ������஢���; "
			"����� ��᫥ ������஢���� %u ����.", len);
	scoped_ptr<uint8_t> patterns_data(new uint8_t[MAX_KKT_PATTERNS_DATA_LEN]);
	size_t patterns_data_len = MAX_KKT_PATTERNS_DATA_LEN;
	int rc = uncompress(patterns_data.get(), &patterns_data_len, patterns_buf, len);
	if (rc == Z_OK)
		log_info("����� 蠡����� ���� ��� �ᯠ������; "
			"����� ������ ��᫥ �ᯠ����� %u ����.", patterns_data_len);
	else{
		log_err("�訡�� �ᯠ����� 蠡����� ���� ��� (%d).", rc);
		return false;
	}
	char *p = (char *)patterns_data.get();
	size_t nr_files = 0;
	if (sscanf(p, "%zu", &nr_files) == 1)
		log_info("� ��娢� 蠡����� ���� ��� %u 䠩���.", nr_files);
	else{
		log_err("�訡�� �⥭�� ������⢠ 䠩��� � ��娢� 蠡����� ���� ���.");
		return false;
	}
	bool ret = false;
	boost::container::vector<string> names;
	boost::container::vector<size_t> lengths;
	char fname[PATH_MAX];
	size_t flen;
	for (size_t i = 0; i <= nr_files; i++){
		p = next_str(p, patterns_data_len - (p - (char *)patterns_data.get()));
		if (i == nr_files){
			ret = true;
			break;
		}else if (p == NULL)
			break;
		else if (sscanf(p, "%[^*]*%zu", fname, &flen) != 2){
			log_err("�訡�� �⥭�� ���ଠ樨 � 䠩�� �%u.", i);
			break;
		}
		log_info("���� �%u: %s (%u ����)", i + 1, fname, flen);
		names.push_back(fname);
		lengths.push_back(flen);
	}
	if (!ret)
		return ret;
	const uint8_t *data = (uint8_t *)p;
	clr_old_patterns();
	char path[PATH_MAX];
	ret = false;
	for (size_t i = 0, idx = data - patterns_data.get(); i < nr_files; i++){
		if ((patterns_data_len - idx) < lengths[i]){
			log_err("�������筮 ������ ��� 䠩�� %s.", names[i]);
			break;
		}
		snprintf(path, ASIZE(path), PATTERNS_FOLDER "/%s", names[i].c_str());
		int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
		if (fd == -1){
			log_sys_err("�訡�� ������ %s ��� �����:", path);
			break;
		}
		int rc = write(fd, data, lengths[i]);
		if (rc == lengths[i])
			log_info("������ %s �ᯥ譮 ����ᠭ � 䠩� %s.", names[i].c_str(), path);
		else if (rc == -1){
			log_sys_err("�訡�� ����� � %s:", path);
			break;
		}else{
			log_err("� %s ����� %zu ����ᠭ� %d ����.", path, lengths[i], rc);
			break;
		}
		close(fd);
		idx += lengths[i];
		data += lengths[i];
		if ((i + 1) == nr_files){
			if (idx < patterns_data_len)
				log_warn("� ���� 蠡����� ���� ��� ���������� ��譨� ����� (%u ����).",
					patterns_data_len - idx);
			ret = true;
		}
	}
	snprintf(path, ASIZE(path), PATTERNS_FOLDER "/S00%s", x3_kkt_patterns_version.c_str());
	int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (fd == -1){
		log_sys_err("�訡�� ᮧ����� 䠩�� ���ᨨ 蠡����� ���� ��� %s:", path);
		ret = false;
	}else
		log_info("���� ���ᨨ 蠡����� ���� ��� %s ��࠭� �� ��᪥.", path);
	close(fd);
	return ret;
}

/* ��뢠���� ��᫥ ����祭�� �⢥� �� ����� 蠡����� ���� ��� */
void on_response_patterns(void)
{
	patterns_auto_req_len = 0;
	static char err_msg[1024];
	err_msg[0] = 0;
/* 0 -- ����� 蠡����� ����; 1 -- ��㣮� �⢥�, ��ࠡ�⪠ �� �ॡ����; 2 -- ��㣮� �⢥�, �ॡ���� ��ࠡ�⪠ */
	int non_patterns_resp = 0;
	set_term_state(st_resp);
	int patterns_para = -1, req_para = -1;
	size_t patterns_len = 0, req_len = 0;
	if (find_pic_data(&patterns_para, &req_para) && (patterns_para != -1)){
		patterns_len = handle_para(patterns_para);
		log_info("�����㦥�� ����� 蠡����� ���� (����� #%d; %zd ����).",
			patterns_para + 1, patterns_len);
		if (patterns_len > (ASIZE(patterns_buf) - patterns_buf_idx)){
			snprintf(err_msg, ASIZE(err_msg), "��९������� ���� ������ 蠡����� ����.");
			non_patterns_resp = 1;
		}else if (patterns_len > 0){
			memcpy(patterns_buf + patterns_buf_idx, text_buf, patterns_len);
			patterns_buf_idx += patterns_len;
			if (req_para != -1){
				req_len = handle_para(req_para);
				if (req_len > 0){
					log_info("�����㦥� ��⮧���� (����� %d; %zd ����).",
						req_para + 1, req_len);
					memcpy(patterns_auto_req, text_buf, req_len);
					patterns_auto_req_len = req_len;
					send_patterns_auto_request();
				}
			}else{
				log_info("������� ���� ����祭� ���������. ���࠭塞 �� ���...");
				x3data_sync_ok |= X3_SYNC_KKT_PATTERNS;
				store_patterns();
				sync_xslt();
			}
		}else{
			snprintf(err_msg, ASIZE(err_msg), "����祭� ����� 蠡����� ���� �㫥��� �����.");
			non_patterns_resp = 1;
		}
	}else{
		snprintf(err_msg, ASIZE(err_msg), "�� ������� ����� 蠡����� ����.");
		non_patterns_resp = 2;
	}
	if ((err_msg[0] != 0) || (non_patterns_resp != 0))
		x3data_sync_fail |= X3_SYNC_KKT_PATTERNS;
	if (err_msg[0] != 0){
		log_err(err_msg);
		patterns_buf_idx = 0;
	}
	if (non_patterns_resp != 0){
		req_type = req_regular;
		x3data_sync_report_dlg();
		if (non_patterns_resp == 2){
			log_dbg("���室�� � ��ࠡ�⪥ �⢥�.");
			execute_resp();
		}
	}
}

static bool download_patterns()
{
	patterns_buf_idx = 0;
	log_info("��稭��� ����㧪� 蠡����� ���� ���.");
	send_patterns_request(x3_kkt_patterns_version);
	return true;
}

bool sync_patterns()
{
	log_dbg("");
	bool ret = true;
	if (need_patterns_update(x3_kkt_patterns_version)){
		req_type = req_patterns;
		ret = download_patterns();
	}else
		ret = sync_xslt();
	return ret;
}
