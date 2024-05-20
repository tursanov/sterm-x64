/* ����� � ⠡��栬� XSLT. (c) gsr, 2023, 2024 */

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <zlib.h>
#include "x3data/common.hpp"
#include "x3data/xslts.h"
#include "gui/scr.h"
#include "express.h"
#include "paths.h"
#include "sterm.h"
#include "termlog.h"
#include "transport.h"
#include "xbase64.h"

bool XSLTInfo::isIdxValid(const char *idx)
{
	return (idx != NULL) && (_tcslen(idx) == 2) &&
		(((idx[0] >= _T('0')) && (idx[0] <= _T('9'))) || ((idx[0] >= _T('A')) && (idx[0] <= _T('Z')))) &&
		(((idx[1] >= _T('0')) && (idx[1] <= _T('9'))) || ((idx[1] >= _T('A')) && (idx[1] <= _T('Z'))));
}

bool XSLTInfo::parse(const char *name)
{
	if (name == NULL)
		return false;
	size_t len = _tcslen(name);
	if (len == 0)
		return false;
	char *nm = new char[len + 1];
	for (size_t i = 0; i <= len; i++)
		nm[i] = (i < len) ? toupper(name[i]) : 0;
	bool ret = false;
	static const char *xslt_ext = _T(".XSL");
	char pr, idx[3], ext[5], ch;
	int y, d;
	if (sscanf(nm, _T("%c%2s%2d%3d%4s%c"), &pr, &idx, &y, &d, ext, &ch) == 5){
		pr = toupper(pr);
		if ((pr == _T('T')) && isIdxValid(idx) &&
				(strcmp(ext, xslt_ext) == 0) && (y > 14) && (d > 0) && (d < 367)){
			struct tm tm;
			tm.tm_year = 100 + y;
			tm.tm_mon = 0;
			tm.tm_mday = d;
			tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
			tm.tm_isdst = -1;
			time_t t = mktime(&tm);
			if (t != -1){
				_date = t;
				ret = true;
			}
		}
	}
	if (ret){
		strcpy(_idx, idx);
		_name.assign(nm, len - 4);
	}
	delete [] nm;
	return ret;
}

bool XSLTInfo::parse(const uint8_t *data, size_t len)
{
	if ((data == NULL) || (len == 0))
		return false;
	bool ret = false;
	char nm[MAX_XSLT_NAME_LEN + 1];
	char pr, idx[3], ch;
	int y, d;
	if (_snscanf((LPCCH)data, len, "<%c%2s%2d%3d%c", &pr, &idx, &y, &d, &ch) == 5){
		pr &= ~0x20;
		if ((pr == 'T') && (ch == '>') && isIdxValid(idx) && (y > 14) && (d > 0) && (d < 367)){
			snprintf(nm, ARRAYSIZE(nm), _TRUNCATE, _T("%hc%.2hs%.2d%.3d"), pr, idx, y, d);
			struct tm tm;
			tm.tm_year = 100 + y;
			tm.tm_mon = 0;
			tm.tm_mday = d;
			tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
			tm.tm_isdst = -1;
			time_t t = mktime(&tm);
			if (t != -1){
				_date = t;
				ret = true;
			}
		}
	}
	if (ret){
		strcpy(_idx, ansi2tstr(idx).c_str());
		_name.assign(nm);
	}
	return ret;
}

list<XSLTInfo> xslt_to_create;
list<XSLTInfo> xslt_to_remove;
list<XSLTInfo> xslt_failed;

bool needXSLTUpdate()
{
	return !xslt_to_create.empty() || !xslt_to_remove.empty();
}

void checkStoredXSLT(const list<XSLTInfo> &x3_xslt)
{
	xslt_to_create.clear();
	xslt_to_remove.clear();
/* ������ ᯨ᮪ ⠡��� XSLT, �࠭����� � ��� ��� */
	log_dbg(_T("�饬 ⠡���� XSLT � ��⠫��� %s..."), xslt_folder);
	list<XSLTInfo> stored_xslt;
	static char path[MAX_PATH + 1];
	snprintf(path, ARRAYSIZE(path), _TRUNCATE, _T("%s\\T???????.XSL"), xslt_folder);
	WIN32_FIND_DATA fd;
	HANDLE fh = FindFirstFile(path, &fd);
	if (fh != INVALID_HANDLE_VALUE){
		do {
			XSLTInfo xi;
			if (xi.parse(fd.cFileName)){
				log_dbg(_T("�����㦥�� ⠡��� XSLT %s #%s."), xi.name().c_str(), xi.idx());
				stored_xslt.push_back(xi);
			}
		} while (FindNextFile(fh, &fd));
		FindClose(fh);
	}
	log_dbg(_T("������� ⠡��� XSLT: %d."), stored_xslt.size());
/* �饬 ⠡���� XSLT ��� ����窨 */
	log_dbg(_T("�饬 ⠡���� XSLT ��� ����窨..."));
	for (const auto &p : x3_xslt){
		bool found = false;
		for (const auto &p1 : stored_xslt){
			if (strcmp(p1.idx(), p.idx()) != 0)
				continue;
			else if (p.isNewer(p1)){
				log_dbg(_T("������ XSLT %s #%s ��॥ ����ᮢ᪮� � �㤥� 㤠����."),
					p1.name().c_str(), p1.idx());
				xslt_to_remove.push_back(p1);
			}else if (p1.isNewer(p)){
				log_dbg(_T("������ XSLT %s #%s ����� ����ᮢ᪮� � �㤥� 㤠����."),
					p1.name().c_str(), p1.idx());
				xslt_to_remove.push_back(p1);
			}else
				found = true;
		}
		if (found)
			log_dbg(_T("������ XSLT %s #%s �� �ॡ�� ����������."), p.name().c_str(), p.idx());
		else{
			log_dbg(_T("������ XSLT %s #%s ��������� � ��� ��� � �㤥� ����㦥�� �� \"������-3\")."),
				p.name().c_str(), p.idx());
			xslt_to_create.push_back(p);
		}
	}
	log_dbg(_T("������� ⠡��� XSLT: ��� ����窨: %d; ��� 㤠�����: %d."), xslt_to_create.size(), xslt_to_remove.size());
}

bool checkX3XSLT(const uint8_t *data, size_t len, list<XSLTInfo> &x3_xslt)
{
	if ((data == NULL) || (len == 0))
		return false;
	x3_xslt.clear();
	for (size_t i = 0; (data[i] == 0x3c) && ((i + 1) < len); i++){
		const uint8_t *p = (const uint8_t *)memchr(data + i + 1, 0x3e, len - i - 1);
		if (p == NULL)
			break;
		XSLTInfo xi;
		if (xi.parse(data + i, len - 1))
			x3_xslt.push_back(xi);
		i += p - (data + i);
	}
	return !x3_xslt.empty();
}

/* ���ᨬ���� ࠧ��� ���� ᦠ⮩ ⠡���� XSLT */
static const size_t MAX_COMPRESSED_XSLT_LEN = 65536;
/* ���� ��� ��� ᦠ⮩ ⠡���� XSLT */
static uint8_t xslt_buf[MAX_COMPRESSED_XSLT_LEN];
/* ����騩 ࠧ��� �ਭ���� ������ ᦠ⮩ ⠡���� XSLT */
static size_t xslt_buf_idx = 0;

static uint32_t xslt_req_t0 = 0;

static const uint8_t *xslt_auto_req = NULL;
static size_t xslt_auto_req_len = 0;

static X3SyncCallback_t xslt_sync_cbk = NULL;

static void callSyncCbk(bool done, const char *message)
{
	if (xslt_sync_cbk != NULL)
		xslt_sync_cbk(done, message);
}

static WaitTool xsltReqWait;

static void onEndXSLTRequest(CwbSdk *cwb_sdk)
{
	log_info(_T("cwb_sdk = %p."), cwb_sdk);
	xslt_auto_req = NULL;
	xslt_auto_req_len = 0;
	static char err_msg[1024];
	err_msg[0] = 0;
/* 0 -- ����� ⠡���� XSLT; 1 -- ��㣮� �⢥�, ��ࠡ�⪠ �� �ॡ����; 2 -- ��㣮� �⢥�, �ॡ���� ��ࠡ�⪠ */
	int non_xslt_resp = 0;
	scoped_ptr<DMap> dmap(new DMap());
	scoped_ptr<X3Response> rsp(new X3Response(inquirer, term_keys, term_smap, dmap.get(), xprn, tprn, kkt,
		xlog, termcore_callbacks));
	if (cwb_sdk->error() == CwbSdkError::OK){
		setTermcoreState(TS_RESPONSE);
		InquirerError inq_err = inquirer->checkResp(cwb_sdk->resp(), cwb_sdk->resp_len());
		if (inq_err == InquirerError::OK){
			if (inquirer->is_specific_error()){
				uint8_t code = inquirer->x3_specific_error();
				snprintf(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
					_T("����饭�� �� �訡�� \"������\": %.2hX (%s)."), (WORD)code,
					Inquirer::specific_error_str(code));
				xlog->writeSpecial(0, code);
				non_xslt_resp = 1;
			}else if (isXmlResp(inquirer->resp(), inquirer->resp_len())){
				snprintf(err_msg, ARRAYSIZE(err_msg), _TRUNCATE, _T("� �⢥� �� ����� ⠡���� XSLT ���� XML."));
				non_xslt_resp = 1;
			}else{
				UINT idx = 0;
				uint32_t rt = GetTickCount() - xslt_req_t0;
				try {
					log_info(_T("��稭��� ᨭ⠪��᪨� ࠧ��� �⢥�."));
					bool has_tprn = cfg->has_tprn();
					cfg->set_has_tprn(true);
					rsp->parse(inquirer->resp(), idx, inquirer->resp_len(), rt);
					cfg->set_has_tprn(has_tprn);
					cfg->clrChanged();
					log_info(_T("�⢥� ࠧ��࠭."));
					const uint8_t *xslt = NULL, req = NULL;
					size_t xslt_len = 0, req_len = 0;
					if (rsp->getPicData(xslt, xslt_len, req, req_len)){
						log_info(_T("�����㦥�� ����� ⠡���� XSLT (%d ����)."), xslt_len);
						if ((xslt_buf_idx + xslt_len) > ARRAYSIZE(xslt_buf)){
							snprintf(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
								_T("��९������� ���� ������ ⠡���� XSLT."));
							non_xslt_resp = 1;
						}else if (xslt_len > 0){
							memcpy(xslt_buf + xslt_buf_idx, xslt, xslt_len);
							xslt_buf_idx += xslt_len;
							if ((req != NULL) && (req_len > 0)){
								log_info(_T("�����㦥� ��⮧���� ⠡���� XSLT (%d ����)."), req_len);
								xslt_auto_req = req;
								xslt_auto_req_len = req_len;
							}
						}else{
							snprintf(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
								_T("����祭� ����� ⠡���� XSLT �㫥��� �����."));
							non_xslt_resp = 1;
						}
					}else{
						snprintf(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
							_T("�� ������� ����� ⠡���� XSLT."));
						non_xslt_resp = 2;
					}
				} catch (SYNTAX_ERROR e){
					snprintf(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
						_T("���⠪��᪠� �訡�� �⢥� %s �� ᬥ饭�� %d (0x%.4X)."),
						X3Response::getErrorName(e), idx, idx);
					inquirer->ZBp().setSyntaxError();
					non_xslt_resp = 1;
				}
			}
		}else if (inq_err == InquirerError::ForeignResp){
			const uint8_t *addr = cwb_sdk->resp() + 4;
			snprintf(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
				_T("����祭 �⢥� ��� ��㣮�� �ନ���� (%.2hX:%.2hX)."), (WORD)addr[0], (WORD)addr[1]);
			xlog->writeForeign(0, addr[0], addr[1]);
			non_xslt_resp = 1;
		}else{
			snprintf(err_msg, ARRAYSIZE(err_msg), _TRUNCATE, _T("�訡�� ᥠ�ᮢ��� �஢��: %s."),
				inquirer->error_str(inq_err));
			non_xslt_resp = 1;
		}
	}else{
		snprintf(err_msg, ARRAYSIZE(err_msg), _TRUNCATE, _T("�訡�� SDK ���: %s"), cwb_sdk->error_msg().c_str());
		non_xslt_resp = 1;
	}
	if (err_msg[0] != 0){
		log_err(err_msg);
		xslt_buf_idx = 0;
		termcore_callbacks.callMessageBox(_T("������"), err_msg);
/*		clrGridLists();
		clrIconLists();*/
	}
	if (!isTermcoreState(TS_CLOSE_WAIT)){
		if (non_xslt_resp){
			callSyncCbk(true, NULL);
			if (non_xslt_resp == 2){
				setTermcoreState(TS_RESPONSE);
				handleResp(rsp.get());
			}
		}else
			setTermcoreState(TS_READY);
	}
	processExitFlag();
	xsltReqWait.stop();
}

static TERMCORE_RETURN sendXSLTRequest(const XSLTInfo &xslt)
{
	if (!enterCriticalSection(CS_HOST_REQ)){
		log_err(LOG_REENTRANCE);
		return TC_REENTRANT;
	}
	TERMCORE_RETURN ret = TC_OK;
	char req[256];
	snprintf(req, ARRAYSIZE(req), _TRUNCATE, "P55TR(EXP.DATA.MAKET   %-8ls)000000000", xslt.name().c_str());
	uint32_t rt = (xslt_req_t0 == 0) ? 0 : GetTickCount() - xslt_req_t0;
	if (inquirer->writeRequest((const uint8_t *)req, strlen(req), rt) == InquirerError::OK){
		TERMCORE_STATE prev_ts = getTermcoreState();
		setTermcoreState(TS_REQUEST);
		CwbSdkError cwb_err = cwb_sdk->beginHostRequest(inquirer->req(), inquirer->req_len(), onEndXSLTRequest);
		if (cwb_err == CwbSdkError::OK)
			xslt_req_t0 = GetTickCount();
		else{
			setTermcoreState(prev_ts);
			if (cwb_err == CwbSdkError::Cwb)
				log_err(_T("�訡�� SDK ���: %s."), cwb_sdk->error_msg().c_str());
			else
				log_err(_T("�訡�� ����������⢨� � SDK ���: %s."),
					(cwb_sdk->error() == CwbSdkError::OK) ? CwbSdk::error_msg(cwb_err) : cwb_sdk->error_msg().c_str());
			ret = TC_CWB_FAULT;
		}
		processExitFlag();
	}else{
		log_err(_T("�訡�� ᮧ����� ����� ��� ����祭�� ⠡���� XSLT."));
		ret = TC_INVALID_REQ;
	}
	leaveCriticalSection(CS_HOST_REQ);
	return ret;
}

static TERMCORE_RETURN sendXSLTAutoRequest()
{
	if (!enterCriticalSection(CS_HOST_REQ)){
		log_err(LOG_REENTRANCE);
		return TC_REENTRANT;
	}
	TERMCORE_RETURN ret = TC_OK;
	uint32_t rt = GetTickCount() - xslt_req_t0;
	if (inquirer->writeRequest(xslt_auto_req, xslt_auto_req_len, rt) == InquirerError::OK){
		TERMCORE_STATE prev_ts = getTermcoreState();
		setTermcoreState(TS_REQUEST);
		CwbSdkError cwb_err = cwb_sdk->beginHostRequest(inquirer->req(), inquirer->req_len(), onEndXSLTRequest);
		if (cwb_err == CwbSdkError::OK)
			xslt_req_t0 = GetTickCount();
		else{
			setTermcoreState(prev_ts);
			if (cwb_err == CwbSdkError::Cwb)
				log_err(_T("�訡�� SDK ���: %s."), cwb_sdk->error_msg().c_str());
			else
				log_err(_T("�訡�� ����������⢨� � SDK ���: %s."),
					(cwb_sdk->error() == CwbSdkError::OK) ? CwbSdk::error_msg(cwb_err) : cwb_sdk->error_msg().c_str());
			ret = TC_CWB_FAULT;
		}
		processExitFlag();
	}else{
		log_err(_T("�訡�� ᮧ����� ��⮧���� ��� ����祭�� ⠡���� XSLT."));
		ret = TC_INVALID_REQ;
	}
	leaveCriticalSection(CS_HOST_REQ);
	return ret;
}

/* ������஢���� ⠡���� XSLT, �ᯠ����� � ��࠭���� � 䠩�� �� ��᪥ */
static bool storeXSLT(const XSLTInfo &xi)
{
	log_info(_T("name = %s; idx = %hs."), xi.name().c_str(), xi.idx());
	if (xslt_buf_idx == 0){
		log_err(_T("���� ������ ����."));
		return false;
	}
	size_t len = 0;
	if (!xbase64Decode(xslt_buf, xslt_buf_idx, xslt_buf, xslt_buf_idx, len) || (len == 0)){
		log_err(_T("�訡�� ������஢���� ������."));
		return false;
	}else
		log_info(_T("����� ⠡���� XSLT ������஢���; ����� ��᫥ ������஢���� %u ����."), len);
	scoped_ptr<uint8_t> xslt_data(new uint8_t[MAX_COMPRESSED_XSLT_LEN]);	/* FIXME */
	size_t xslt_data_len = MAX_COMPRESSED_XSLT_LEN;
	int rc = uncompress(xslt_data.get(), &xslt_data_len, xslt_buf, len);
	if (rc == Z_OK)
		log_info(_T("����� ⠡���� XSLT �ᯠ������; ����� ������ ��᫥ �ᯠ����� %u ����."), xslt_data_len);
	else{
		log_err(_T("�訡�� �ᯠ����� ⠡���� XSLT (%d)."), rc);
		return false;
	}
	char name[MAX_PATH + 1];
	snprintf(name, ARRAYSIZE(name), _TRUNCATE, _T("%s\\%s.XSL"), xslt_folder, xi.name().c_str());
	int fd = _topen(name, _O_WRONLY | _O_BINARY | _O_TRUNC | _O_CREAT, _S_IWRITE);
	if (fd == -1){
		log_win_err(_T("�訡�� ᮧ����� 䠩�� %s:"), name);
		return false;
	}
	bool ret = false;
	rc = _write(fd, xslt_data.get(), xslt_data_len);
	if (rc == xslt_data_len){
		log_info(_T("������ XSLT �ᯥ譮 ����ᠭ� � 䠩� %s."), name);
		ret = true;
	}else if (rc == -1)
		log_win_err(_T("�訡�� ����� � 䠩� %s:"), name);
	else
		log_err(_T("� %s ����� %u ����ᠭ� %d ����."), name, xslt_data_len, rc);
	if (_close(fd) != 0)
		log_win_err(_T("�訡�� ������� 䠩�� %s:"), name);
	if (!ret)
		_tremove(name);
	return ret;
}

/* �������� ⠡���� XSLT � ��᪠ */
static bool removeXSLT(const XSLTInfo &xi)
{
	static char path[MAX_PATH + 1];
	snprintf(path, ARRAYSIZE(path), _TRUNCATE, _T("%s\\%s.XSL"), xslt_folder, xi.name().c_str());
	log_info(_T("����塞 ⠡���� XSLT %s..."), path);
	bool ret = _tunlink(path) == 0;
	if (ret)
		log_info(_T("������ %s �ᯥ譮 㤠����."), path);
	else
		log_win_err(_T("�訡�� 㤠����� ⠡���� XSLT %s:"), path);
	return ret;
}

/* ����㧪� �������� ⠡���� XSLT �� "������-3" */
static TERMCORE_RETURN downloadXSLT(const XSLTInfo &xi)
{
	TERMCORE_RETURN ret = TC_OK;
	bool flag = false, need_req = true;
	log_info(_T("����㦠�� ⠡���� XSLT %s..."), xi.name().c_str());
	xslt_buf_idx = 0;
	for (uint32_t n = 0; need_req; n++){
		flag = false;
		ret = (n == 0) ? sendXSLTRequest(xi) : sendXSLTAutoRequest();
		if (ret == TC_OK){
			xsltReqWait.wait();
			if (isTermcoreState(TS_READY) && (xslt_buf_idx > 0)){
				if (xslt_auto_req_len == 0){
					log_info(_T("������ XSLT %s ����祭� ���������. ���࠭塞 � 䠩�..."), xi.name().c_str());
					need_req = false;
					if (storeXSLT(xi))
						flag = true;
				}else
					flag = true;
			}
		}
		if (!flag){
			if (ret == TC_OK)
				ret = TC_XSLT_LOAD;
			break;
		}
	}
	if ((ret != TC_OK) && (ret != TS_CLOSE_WAIT))
		log_err(_T("�訡�� ����㧪� ⠡���� XSLT %s."), xi.name().c_str());
	return ret;
}

void showXSLTSyncError()
{
	if (xslt_failed.empty())
		return;
	static tstring msg;
	msg.assign(_T("�訡�� ᨭ�஭���樨 ᫥����� ⠡��� XSLT:\r\n"));
	bool first = true;
	for (const auto &p : xslt_failed){
		if (first)
			first = false;
		else
			msg.append(_T(";\r\n"));
		msg.append(p.name());
	}
	msg.append(_T("."));
	termcore_callbacks.callMessageBox(_T("��������"), msg.c_str());
}

bool syncXSLT(X3SyncCallback_t cbk)
{
	bool ok = true;
	char txt[256];
	size_t n = 0;
	xslt_sync_cbk = cbk;
	callSyncCbk(false, _T("����㧪� ⠡��� XSLT �� \"������\""));
	Sleep(100);
	xslt_failed.clear();
/* ���砫� ����稢��� ���� ⠡���� XSLT */
	for (const auto &p : xslt_to_create){
		snprintf(txt, ARRAYSIZE(txt), _TRUNCATE, _T("����㧪� ⠡���� XSLT %s (%u �� %u)"),
			p.name().c_str(), n + 1, xslt_to_create.size());
		callSyncCbk(false, txt);
		TERMCORE_RETURN rc = downloadXSLT(p);
		if (rc == TC_OK)
			n++;
		else{
			ok = false;
			break;
		}
	}
/* ��⥬ 㤠�塞 ���� */
	if (ok){
		xslt_to_create.clear();
		for (const auto &p : xslt_to_remove){
			if (removeXSLT(p))
				n++;
			else{
				ok = false;
				break;
			}
		}
		if (ok)
			xslt_to_remove.clear();
	}
	if (ok)
		callSyncCbk(false, _T("������� XSLT �ᯥ譮 ����㦥��"));
	else
		showXSLTSyncError();
	xslt_sync_cbk = NULL;
	return ok;
}
