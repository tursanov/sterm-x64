/* ����� � 蠡������ ���� ���. (c) gsr, 2022, 2024 */

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <zlib.h>
#include "gui/scr.h"
#include "express.h"
#include "paths.h"
#include "sterm.h"
#include "termlog.h"
#include "transport.h"
#include "xbase64.h"

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
		log_err("� ��⠫��� " PATTERNS_DIR " �� ������ 䠩� ����.");
		regfree(&reg);
		return -1;
	}else if (n > 1)
		log_info("� ��⠫��� " PATTERNS_DIR " ������� ����� ������ 䠩�� ���� (%d); "
			"�㤥� �ᯮ�짮��� %s.", n, names[0]->d_name);
	else
		log_dbg("�����㦥� 䠩� %s.", names[0]->d_name);
	time_t ret = jul_date_to_unix_date(names[0]->d_name + 3);
	free(names);
	regfree(&reg);
	return ret;
}

static bool need_patterns_update(const string &version)
{
	bool ret = false;
	time_t old_t = get_local_patterns_date(), new_t = jul_date_to_unix_date(version.c_str());
	if (old_t == -1)
		ret = new_t != -1;
	else
		ret = (new_t != -1) && (new_t > old_t);
	return ret;
}

static int clr_selector(const struct dirent *entry)
{
	return	(strcmp(entry->d_name, ".") != 0) &&
		(strcmp(entry->d_name, "..") != 0);
}

static int clr_old_patterns(void)
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

static x3_sync_callback_t patterns_sync_cbk = NULL;

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

static void onEndPatternsRequest(CwbSdk *cwb_sdk)
{
	log_info("cwb_sdk = %p."), cwb_sdk);
	patterns_auto_req = NULL;
	patterns_auto_req_len = 0;
	static char err_msg[1024];
	err_msg[0] = 0;
/* 0 -- ����� 蠡����� ����; 1 -- ��㣮� �⢥�, ��ࠡ�⪠ �� �ॡ����; 2 -- ��㣮� �⢥�, �ॡ���� ��ࠡ�⪠ */
	int non_patterns_resp = 0;
	scoped_ptr<DMap> dmap(new DMap());
	scoped_ptr<X3Response> rsp(new X3Response(inquirer, term_keys, term_smap, dmap.get(), xprn, tprn, kkt,
		xlog, termcore_callbacks));
	if (cwb_sdk->error() == CwbSdkError::OK){
		setTermcoreState(TS_RESPONSE);
		InquirerError inq_err = inquirer->checkResp(cwb_sdk->resp(), cwb_sdk->resp_len());
		if (inq_err == InquirerError::OK){
			if (inquirer->is_specific_error()){
				uint8_t code = inquirer->x3_specific_error();
				snprintf(err_msg, ASIZE(err_msg), _TRUNCATE,
					"����饭�� �� �訡�� \"������\": %.2hX (%s)."), (WORD)code,
					Inquirer::specific_error_str(code));
				xlog->writeSpecial(0, code);
				non_patterns_resp = 1;
			}else if (isXmlResp(inquirer->resp(), inquirer->resp_len())){
				snprintf(err_msg, ASIZE(err_msg), _TRUNCATE, "� �⢥� �� ����� 蠡����� ���� ��� ���� XML."));
				non_patterns_resp = 1;
			}else{
				UINT idx = 0;
				uint32_t rt = GetTickCount() - patterns_req_t0;
				try {
					log_info("��稭��� ᨭ⠪��᪨� ࠧ��� �⢥�."));
					bool has_tprn = cfg->has_tprn();
					cfg->set_has_tprn(true);
					rsp->parse(inquirer->resp(), idx, inquirer->resp_len(), rt);
					cfg->set_has_tprn(has_tprn);
					cfg->clrChanged();
					log_info("�⢥� ࠧ��࠭."));
					const uint8_t *patterns = NULL, req = NULL;
					size_t patterns_len = 0, req_len = 0;
					if (rsp->getPicData(patterns, patterns_len, req, req_len)){
						log_info("�����㦥�� ����� 蠡����� ���� ��� (%d ����)."), patterns_len);
						if (patterns_len > (ASIZE(patterns_buf) - patterns_buf_idx)){
							snprintf(err_msg, ASIZE(err_msg), _TRUNCATE,
								"��९������� ���� ������ 蠡����� ���� ���."));
							non_patterns_resp = 1;
						}else if (patterns_len > 0){
							memcpy(patterns_buf + patterns_buf_idx, patterns, patterns_len);
							patterns_buf_idx += patterns_len;
							if ((req != NULL) && (req_len > 0)){
								log_info("�����㦥� ��⮧���� (%d ����)."), req_len);
								patterns_auto_req = req;
								patterns_auto_req_len = req_len;
							}
						}else{
							snprintf(err_msg, ASIZE(err_msg), _TRUNCATE,
								"����祭� ����� 蠡����� ���� ��� �㫥��� �����."));
							non_patterns_resp = 1;
						}
					}else{
						snprintf(err_msg, ASIZE(err_msg), _TRUNCATE,
							"�� ������� ����� 蠡����� ���� ���."));
						non_patterns_resp = 2;
					}
				} catch (SYNTAX_ERROR e){
					snprintf(err_msg, ASIZE(err_msg), _TRUNCATE,
						"���⠪��᪠� �訡�� �⢥� %s �� ᬥ饭�� %d (0x%.4X)."),
						X3Response::getErrorName(e), idx, idx);
					inquirer->ZBp().setSyntaxError();
					non_patterns_resp = 1;
				}
			}
		}else if (inq_err == InquirerError::ForeignResp){
			const uint8_t *addr = cwb_sdk->resp() + 4;
			snprintf(err_msg, ASIZE(err_msg), _TRUNCATE,
				"����祭 �⢥� ��� ��㣮�� �ନ���� (%.2hX:%.2hX)."), (WORD)addr[0], (WORD)addr[1]);
			xlog->writeForeign(0, addr[0], addr[1]);
			non_patterns_resp = 1;
		}else{
			snprintf(err_msg, ASIZE(err_msg), _TRUNCATE, "�訡�� ᥠ�ᮢ��� �஢��: %s."),
				inquirer->error_str(inq_err));
			non_patterns_resp = 1;
		}
	}else{
		snprintf(err_msg, ASIZE(err_msg), _TRUNCATE, "�訡�� SDK ���: %s"), cwb_sdk->error_msg().c_str());
		non_patterns_resp = 1;
	}
	if (err_msg[0] != 0){
		log_err(err_msg);
		patterns_buf_idx = 0;
		termcore_callbacks.callMessageBox("������"), err_msg);
	}
	if (!isTermcoreState(TS_CLOSE_WAIT)){
		if (non_patterns_resp){
			if (patterns_sync_cbk != NULL)
				patterns_sync_cbk(true, NULL);
			if (non_patterns_resp == 2){
				setTermcoreState(TS_RESPONSE);
				handleResp(rsp.get(), false);
			}
		}else
			setTermcoreState(TS_READY);
	}
	processExitFlag();
	patternsReqWait.stop();
}

static TERMCORE_RETURN sendPatternsRequest(const string &version)
{
	if (!enterCriticalSection(CS_HOST_REQ)){
		log_err(LOG_REENTRANCE);
		return TC_REENTRANT;
	}
	TERMCORE_RETURN ret = TC_OK;
	char req[256];
	snprintf(req, ASIZE(req), _TRUNCATE, "P55TR(EXP.DATA.MAKET   S00%s)000000000", tstr2ansi(version).c_str());
	if (inquirer->writeRequest((LPCBYTE)req, strlen(req), 0) == InquirerError::OK){
		TERMCORE_STATE prev_ts = getTermcoreState();
		setTermcoreState(TS_REQUEST);
		CwbSdkError cwb_err = cwb_sdk->beginHostRequest(inquirer->req(), inquirer->req_len(), onEndPatternsRequest);
		if (cwb_err == CwbSdkError::OK)
			patterns_req_t0 = GetTickCount();
		else{
			setTermcoreState(prev_ts);
			if (cwb_err == CwbSdkError::Cwb)
				log_err("�訡�� SDK ���: %s."), cwb_sdk->error_msg().c_str());
			else
				log_err("�訡�� ����������⢨� � SDK ���: %s."),
					(cwb_sdk->error() == CwbSdkError::OK) ? CwbSdk::error_msg(cwb_err) : cwb_sdk->error_msg().c_str());
			ret = TC_CWB_FAULT;
		}
		processExitFlag();
	}else{
		log_err("�訡�� ᮧ����� ����� ��� ����祭�� 蠡����� ���� ���."));
		ret = TC_INVALID_REQ;
	}
	leaveCriticalSection(CS_HOST_REQ);
	return ret;
}

static TERMCORE_RETURN sendPatternsAutoRequest()
{
	if (!enterCriticalSection(CS_HOST_REQ)){
		log_err(LOG_REENTRANCE);
		return TC_REENTRANT;
	}
	TERMCORE_RETURN ret = TC_OK;
	uint32_t rt = GetTickCount() - patterns_req_t0;
	if (inquirer->writeRequest(patterns_auto_req, patterns_auto_req_len, rt) == InquirerError::OK){
		TERMCORE_STATE prev_ts = getTermcoreState();
		setTermcoreState(TS_REQUEST);
		CwbSdkError cwb_err = cwb_sdk->beginHostRequest(inquirer->req(), inquirer->req_len(), onEndPatternsRequest);
		if (cwb_err == CwbSdkError::OK)
			patterns_req_t0 = GetTickCount();
		else{
			setTermcoreState(prev_ts);
			if (cwb_err == CwbSdkError::Cwb)
				log_err("�訡�� SDK ���: %s."), cwb_sdk->error_msg().c_str());
			else
				log_err("�訡�� ����������⢨� � SDK ���: %s."),
					(cwb_sdk->error() == CwbSdkError::OK) ? CwbSdk::error_msg(cwb_err) : cwb_sdk->error_msg().c_str());
			ret = TC_CWB_FAULT;
		}
		processExitFlag();
	}else{
		log_err("�訡�� ᮧ����� ����� ��� ����祭�� ����⮢ ࠧ��⮪."));
		ret = TC_INVALID_REQ;
	}
	leaveCriticalSection(CS_HOST_REQ);
	return ret;
}

static inline bool isCrLf(char b)
{
	return (b == 0x0a) || (b == 0x0d);
}

static LPCSTR nextStr(LPCSTR buf, size_t len)
{
	LPCSTR ret = NULL;
	bool crlf = false;
	for (size_t i = 0; i < len; i++){
		char ch = buf[i];
		if (!crlf)
			crlf = isCrLf(ch);
		else if (!isCrLf(ch)){
			ret = buf + i;
			break;
		}
	}
	return ret;
}

static const size_t MAX_KKT_PATTERNS_DATA_LEN	= 65536;

/* ������஢���� 蠡����� ���� ���, �ᯠ����� � ��࠭���� � 䠩�� �� ��᪥ */
static bool storePatterns(void)
{
	log_info(""));
	if (patterns_buf_idx == 0){
		log_err("���� ������ ����."));
		return false;
	}
	size_t len = 0;
	if (!xbase64Decode(patterns_buf, patterns_buf_idx, patterns_buf, patterns_buf_idx, len) || (len == 0)){
		log_err("�訡�� ������஢���� ������."));
		return false;
	}else
		log_info("����� 蠡����� ���� ��� ������஢���; ����� ��᫥ ������஢���� %u ����."), len);
	scoped_ptr<uint8_t> patterns_data(new uint8_t[MAX_KKT_PATTERNS_DATA_LEN]);
	size_t patterns_data_len = MAX_KKT_PATTERNS_DATA_LEN;
	int rc = uncompress(patterns_data.get(), &patterns_data_len, patterns_buf, len);
	if (rc == Z_OK)
		log_info("����� 蠡����� ���� ��� �ᯠ������; ����� ������ ��᫥ �ᯠ����� %u ����."), patterns_data_len);
	else{
		log_err("�訡�� �ᯠ����� 蠡����� ���� ��� (%d)."), rc);
		return false;
	}
	LPCSTR p = reinterpret_cast<LPCSTR>(patterns_data.get());
	log_dbg("p = %p."), p);
	size_t nr_files = 0;
	if (sscanf(p, "%u", &nr_files) == 1)
		log_info("� ��娢� 蠡����� ���� ��� %u 䠩���."), nr_files);
	else{
		log_err("�訡�� �⥭�� ������⢠ 䠩��� � ��娢� 蠡����� ���� ���."));
		return false;
	}
	bool ret = false;
	boost::container::vector<string> names;
	boost::container::vector<size_t> lengths;
	char fname[PATH_MAX + 1];
	size_t flen;
	for (size_t i = 0; i <= nr_files; i++){
		p = nextStr(p, patterns_data_len - (p - (LPCSTR)patterns_data.get()));
		if (i == nr_files){
			ret = true;
			break;
		}else if (p == NULL)
			break;
		else if (sscanf(p, "%[^*]*%u", fname, &flen) != 2){
			log_err("�訡�� �⥭�� ���ଠ樨 � 䠩�� �%u."), i);
			break;
		}
		log_info("���� �%u: %hs (%u ����)"), i, fname, flen);
		names.push_back(ansi2tstr(fname));
		lengths.push_back(flen);
	}
	if (!ret)
		return ret;
	const uint8_t *data = (LPCBYTE)p;
	clrOldPatterns();
	char path[PATH_MAX + 1];
	ret = false;
	for (size_t i = 0, idx = data - patterns_data.get(); i < nr_files; i++){
		if ((patterns_data_len - idx) < lengths[i]){
			log_err("�������筮 ������ ��� 䠩�� %s."), names[i]);
			break;
		}
		snprintf(path, ASIZE(path), _TRUNCATE, "%s\\%s"), patterns_folder, names[i].c_str());
		int fd = _topen(path, _O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE);
		if (fd == -1){
			log_win_err("�訡�� ������ %s ��� �����:"), path);
			break;
		}else if (_write(fd, data, lengths[i]) != lengths[i]){
			log_win_err("�訡�� ����� � %s:"), path);
			break;
		}else if (_close(fd) != 0){
			log_win_err("�訡�� ������� %s:"), path);
			break;
		}else
			log_info("���� %s ��࠭� �� ��᪥."), path);
		idx += lengths[i];
		data += lengths[i];
		if ((i + 1) == nr_files){
			if (idx < patterns_data_len)
				log_warn("� ���� 蠡����� ���� ��� ���������� ��譨� ����� (%u ����)."),
					patterns_data_len - idx);
			ret = true;
		}
	}
	ret = false;
	snprintf(path, ASIZE(path), _TRUNCATE, "%s\\S00%s"), patterns_folder, x3_kkt_patterns_version.c_str());
	int fd = _topen(path, _O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE);
	if (fd == -1)
		log_win_err("�訡�� ᮧ����� 䠩�� ���ᨨ 蠡����� ���� ��� %s:"), path);
	else if (_close(fd) != 0)
		log_win_err("�訡�� ������� 䠩�� ���ᨨ 蠡����� ���� ��� %s:"), path);
	else{
		log_info("���� ���ᨨ 蠡����� ���� ��� %s ��࠭� �� ��᪥."), path);
		ret = true;
	}
	return ret;
}

/* ����㧪� 蠡����� ���� ��� �� "������-3" */
static TERMCORE_RETURN downloadPatterns(const string &version)
{
	TERMCORE_RETURN ret = TC_OK;
	bool flag = false, need_req = true;
	log_info("��稭��� ����㧪� 蠡����� ���� ���."));
	patterns_buf_idx = 0;
	for (uint32_t n = 0; need_req; n++){
		flag = false;
		ret = (n == 0) ? sendPatternsRequest(version) : sendPatternsAutoRequest();
		if (ret == TC_OK){
			patternsReqWait.wait();
			if (isTermcoreState(TS_READY) && (patterns_buf_idx > 0)){
				if (patterns_auto_req_len == 0){
					log_info("������� ���� ��� ����祭�. ���࠭塞 �� ���..."));
					need_req = false;
					if (storePatterns())
						flag = true;
				}else
					flag = true;
			}
		}
		if (!flag){
			if (ret == TC_OK)
				ret = TC_PATTERNS_LOAD;
			break;
		}
	}
	if ((ret != TC_OK) && (ret != TS_CLOSE_WAIT))
		log_err("�訡�� ����㧪� 蠡����� ���� ���."));
	return ret;
}

uint32_t WINAPI syncPatterns(X3SyncCallback_t cbk)
{
	bool ok = true;
	size_t n = 0;
	patterns_sync_cbk = cbk;
	cbk(false, "����㧪� 蠡����� ���� ��� �� \"������\""));
	Sleep(100);
	TERMCORE_RETURN rc = downloadPatterns(x3_kkt_patterns_version);
	if (rc != TC_OK)
		ok = false;
	if (ok){
		if (cbk != NULL)
			cbk(false, "������� ���� ��� �ᯥ譮 ����㦥��"));
	}else
		termcore_callbacks.callMessageBox("��������"), "�訡�� ����㧪� 蠡����� ���� ���."));
	return ok;
}

string x3_kkt_patterns_version;

bool checkX3KktPatterns(const uint8_t *data, size_t len, string &version)
{
	log_info("data = %p; len = %u."), data, len);
	if ((data == NULL) || (len == 0))
		return false;
	static uint8_t tag[] = {0x3c, 0x53, 0x30, 0x30};	/* <S00 */
	bool ret = false;
	for (size_t i = 0; (data[i] == 0x3c) && ((i + 1) < len); i++){
		const uint8_t *p = (LPCBYTE)memchr(data + i + 1, 0x3e, len - i - 1);
		if (p == NULL)
			break;
		else if ((memcmp(data + i, tag, ASIZE(tag)) == 0) && ((p - (data + i)) == 9)){
			version = ansi2tstr(reinterpret_cast<LPCSTR>(data + i + 4), 5);
			log_info("�����㦥�� 蠡���� ���� ��� (����� %s)."), version.c_str());
			ret = true;
			break;
		}else
		i += p - (data + i);
	}
	return ret;
}
