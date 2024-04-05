/* Работа с разметками бланков БПУ/ККТ. (c) gsr, 2015-2016, 2019, 2022, 2024 */

#include <zlib.h>
#include "sysdefs.h"
#include "xbase64.h"
#include "grids.h"

bool GridInfo::parse(const char * name)
{
	if (name == NULL)
		return false;
	SIZE_T len = _tcslen(name);
	if (len == 0)
		return false;
	LPTSTR nm = new TCHAR[len + 1];
	for (SIZE_T i = 0; i <= len; i++)
		nm[i] = (i < len) ? _totupper(name[i]) : 0;
	bool ret = false;
	static const char * bmp_ext = _T(".BMP");
	INT nr;
	TCHAR ext[5], pr, ch;
	if ((_stscanf(nm, _T("HU%d%4s%c"), &nr, ext, &ch) == 2) && (_tcscmp(ext, bmp_ext) == 0)){
		if (isNrValid(nr)){
			_prefix = _T("HU");
			_date = 0;
			ret = true;
		}
	}else{
		INT y, d;
		if (_stscanf(nm, _T("%c%2d%2d%3d%4s%c"), &pr, &nr, &y, &d, ext, &ch) == 5){
			pr = _totupper(pr);
			if (((pr == _T('L')) || (pr == _T('M'))) && isNrValid(nr) &&
					(_tcscmp(ext, bmp_ext) == 0) && (y > 14) && (d > 0) && (d < 367)){
				struct tm tm;
				tm.tm_year = 100 + y;
				tm.tm_mon = 0;
				tm.tm_mday = d;
				tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
				tm.tm_isdst = -1;
				time_t t = mktime(&tm);
				if (t != -1){
					_prefix = pr;
					_date = t;
					ret = true;
				}
			}
		}
	}
	if (ret){
		_nr = nr;
		_id = nrToId(nr);
		_name.assign(nm, len - 4);
	}
	delete [] nm;
	return ret;
}

bool GridInfo::parse(LPCBYTE data, SIZE_T len)
{
	if ((data == NULL) || (len == 0))
		return false;
	bool ret = false;
	INT nr = 0;
	TCHAR nm[XPrn::SPRN_MAX_GRID_NAME_LEN + 1];
	static const CHAR old_prefix[] = "<HU";
	if (memcmp((LPCCH)data, old_prefix, ARRAYSIZE(old_prefix) - 1) == 0){
		bool tail = false;
		for (INT i = ARRAYSIZE(old_prefix) - 1; i < 10; i++){
			BYTE b = data[i];
			if (i < 9){
				if (tail){
					if (b != 0x20)
						break;
				}else if ((b >= 0x30) && (b <= 0x39)){
					nr *= 10;
					nr += b - 0x30;
				}else if (b == 0x20)
					tail = true;
				else
					break;
			}else if (b == '>'){
				if (isNrValid(nr)){
					_sntprintf_s(nm, ARRAYSIZE(nm), _TRUNCATE, _T("HU%d"), nr);
					_prefix = _T("HU");
					_date = 0;
					ret = true;
				}
			}
		}
	}else{
		CHAR pr, ch;
		INT y, d;
		if (_snscanf((LPCCH)data, len, "<%c%2d%2d%3d%c", &pr, &nr, &y, &d, &ch) == 5){
			pr &= ~0x20;
			if (((pr == 'L') || (pr == 'M')) && (ch == '>') && isNrValid(nr) && (y > 14) && (d > 0) && (d < 367)){
				_sntprintf_s(nm, ARRAYSIZE(nm), _TRUNCATE, _T("%c%.2d%.2d%.3d"), pr, nr, y, d);
				struct tm tm;
				tm.tm_year = 100 + y;
				tm.tm_mon = 0;
				tm.tm_mday = d;
				tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
				tm.tm_isdst = -1;
				time_t t = mktime(&tm);
				if (t != -1){
					_prefix = ansi2tstr(&pr, 1);
					_date = t;
					ret = true;
				}
			}
		}
	}
	if (ret){
		_nr = nr;
		_id = nrToId(nr);
		_name.assign(nm);
	}
	return ret;
}

list<GridInfo> grids_to_create_xprn;
list<GridInfo> grids_to_remove_xprn;
list<GridInfo> grids_failed_xprn;

list<GridInfo> grids_to_create_kkt;
list<GridInfo> grids_to_remove_kkt;
list<GridInfo> grids_failed_kkt;

VOID clrGridLists(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove)
{
	grids_to_create.clear();
	grids_to_remove.clear();
}

bool needGridsUpdate(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove)
{
	return !grids_to_create.empty() || !grids_to_remove.empty();
}

static bool findStoredGrids(list<GridInfo> &stored_grids, const char * pattern)
{
	if (pattern == NULL)
		return false;
	bool ret = false;
	static TCHAR path[MAX_PATH + 1];
	_sntprintf_s(path, ARRAYSIZE(path), _TRUNCATE, _T("%s\\%s"), grid_folder, pattern);
	WIN32_FIND_DATA fd;
	HANDLE fh = FindFirstFile(path, &fd);
	if (fh != INVALID_HANDLE_VALUE){
		do {
			GridInfo gi;
			if (gi.parse(fd.cFileName)){
				log_dbg(_T("Обнаружена разметка %s #%d (%hc)."), gi.name().c_str(), gi.nr(), gi.id());
				stored_grids.push_back(gi);
				ret = true;
			}
		} while (FindNextFile(fh, &fd));
		FindClose(fh);
	}
	return ret;
}

static bool findStoredGridsXPrn(list<GridInfo> &stored_grids)
{
	stored_grids.clear();
	findStoredGrids(stored_grids, _T("HU?*.BMP"));
	findStoredGrids(stored_grids, _T("L???????.BMP"));
	return !stored_grids.empty();
}

static bool findStoredGridsKkt(list<GridInfo> &stored_grids)
{
	stored_grids.clear();
	findStoredGrids(stored_grids, _T("M???????.BMP"));
	return !stored_grids.empty();
}

VOID checkStoredGrids(const list<GridInfo> &x3_grids, list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove,
	bool (*find_fn)(list<GridInfo> &))
{
	clrGridLists(grids_to_create, grids_to_remove);
/* Создаём список разметок, хранящихся в ПАК РМК */
	log_dbg(_T("Ищем разметки в каталоге %s..."), grid_folder);
	list<GridInfo> stored_grids;
	find_fn(stored_grids);
	log_dbg(_T("Найдено разметок: %d."), stored_grids.size());
/* Ищем разметки для закачки */
	log_dbg(_T("Ищем разметки для закачки..."));
	for (const auto &p : x3_grids){
		bool found = false;
		for (const auto &p1 : stored_grids){
			if (p1.id() != p.id())
				continue;
			else if (p.isNewer(p1)){
				log_dbg(_T("Разметка %s #%d (%hc) старее экспрессовской и будет удалена."),
					p1.name().c_str(), p1.nr(), p1.id());
				grids_to_remove.push_back(p1);
			}else if (p1.isNewer(p)){
				log_dbg(_T("Разметка %s #%d (%hc) новее экспрессовской и будет удална."),
					p1.name().c_str(), p1.nr(), p1.id());
				grids_to_remove.push_back(p1);
			}else
				found = true;
		}
		if (found)
			log_dbg(_T("Разметка %s #%d (%hc) не требует обновления."), p.name().c_str(), p.nr(), p.id());
		else{
			log_dbg(_T("Разметка %s #%d (%hc) отсутствует в ПАК РМК и будет загружена из \"Экспресс-3\")."),
				p.name().c_str(), p.nr(), p.id());
			grids_to_create.push_back(p);
		}
	}
/* Ищем разметки для удаления */
/*	log_dbg(_T("Ищем разметки для удаления..."));
	for (const auto &p : stored_grids){
		if (find_if(x3_grids, [p](const GridInfo &gi) {return gi.id() == p.id();}) == x3_grids.end()){
			log_dbg(_T("Разметка %s #%d (%hc) помечена для удаления."), p.name().c_str(), p.nr(), p.id());
			grids_to_remove.push_back(p);
		}
	}*/
	log_dbg(_T("Найдено разметок: для закачки: %d; для удаления: %d."), grids_to_create.size(), grids_to_remove.size());
}

VOID checkStoredGridsXPrn(const list<GridInfo> &x3_grids)
{
	checkStoredGrids(x3_grids, grids_to_create_xprn, grids_to_remove_xprn, findStoredGridsXPrn);
}

VOID checkStoredGridsKkt(const list<GridInfo> &x3_grids)
{
	checkStoredGrids(x3_grids, grids_to_create_kkt, grids_to_remove_kkt, findStoredGridsKkt);
}

bool checkX3Grids(LPCBYTE data, SIZE_T len, list<GridInfo> &x3_grids_xprn, list<GridInfo> &x3_grids_kkt)
{
	if ((data == NULL) || (len == 0))
		return false;
	x3_grids_xprn.clear();
	x3_grids_kkt.clear();
	for (SIZE_T i = 0; (data[i] == 0x3c) && ((i + 1) < len); i++){
		LPCBYTE p = (LPCBYTE)memchr(data + i + 1, 0x3e, len - i - 1);
		if (p == NULL)
			break;
		GridInfo gi;
		if (gi.parse(data + i, len - 1)){
			if ((gi.prefix() == _T("HU")) || (gi.prefix() == _T("L")))
				x3_grids_xprn.push_back(gi);
			else if (gi.prefix() == _T("M"))
				x3_grids_kkt.push_back(gi);
		}
		i += p - (data + i);
	}
	return !x3_grids_xprn.empty() || !x3_grids_kkt.empty();
}

/* Максимальный размер буфера сжатой разметки */
static const SIZE_T MAX_COMPRESSED_GRID_LEN = 65536;
/* Буфер для приёма сжатой разметки */
static BYTE grid_buf[MAX_COMPRESSED_GRID_LEN];
/* Текущий размер принятых данных сжатой разметки */
static SIZE_T grid_buf_idx = 0;

static DWORD grid_req_t0 = 0;

static LPCBYTE grid_auto_req = NULL;
static SIZE_T grid_auto_req_len = 0;

static WaitTool gridReqWait;

static X3SyncCallback_t grid_sync_cbk = NULL;

static VOID onEndGridRequest(CwbSdk *cwb_sdk)
{
	log_info(_T("cwb_sdk = %p."), cwb_sdk);
	grid_auto_req = NULL;
	grid_auto_req_len = 0;
	static TCHAR err_msg[1024];
	err_msg[0] = 0;
/* 0 -- данные разметки; 1 -- другой ответ, обработка не требуется; 2 -- другой ответ, требуется обработка */
	INT non_grid_resp = 0;
	scoped_ptr<DMap> dmap(new DMap());
	scoped_ptr<X3Response> rsp(new X3Response(inquirer, term_keys, term_smap, dmap.get(), xprn, tprn, kkt,
		xlog, termcore_callbacks));
	if (cwb_sdk->error() == CwbSdkError::OK){
		setTermcoreState(TS_RESPONSE);
		InquirerError inq_err = inquirer->checkResp(cwb_sdk->resp(), cwb_sdk->resp_len());
		if (inq_err == InquirerError::OK){
			if (inquirer->is_specific_error()){
				BYTE code = inquirer->x3_specific_error();
				_sntprintf_s(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
					_T("Сообщение об ошибке \"Экспресс\": %.2hX (%s)."), (WORD)code,
					Inquirer::specific_error_str(code));
				xlog->writeSpecial(0, code);
				non_grid_resp = 1;
			}else if (isXmlResp(inquirer->resp(), inquirer->resp_len())){
				_sntprintf_s(err_msg, ARRAYSIZE(err_msg), _TRUNCATE, _T("В ответ на запрос разметки пришёл XML."));
				non_grid_resp = 1;
			}else{
				UINT idx = 0;
				DWORD rt = GetTickCount() - grid_req_t0;
				try {
					log_info(_T("Начинаем синтаксический разбор ответа."));
					bool has_tprn = cfg->has_tprn();
					cfg->set_has_tprn(true);
					rsp->parse(inquirer->resp(), idx, inquirer->resp_len(), rt);
					cfg->set_has_tprn(has_tprn);
					cfg->clrChanged();
					log_info(_T("Ответ разобран."));
					LPCBYTE grid = NULL, req = NULL;
					SIZE_T grid_len = 0, req_len = 0;
					if (rsp->getPicData(grid, grid_len, req, req_len)){
						log_info(_T("Обнаружены данные разметки (%d байт)."), grid_len);
						if (grid_len > (ARRAYSIZE(grid_buf) - grid_buf_idx)){
							_sntprintf_s(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
								_T("Переполнение буфера данных разметки."));
							non_grid_resp = 1;
						}else if (grid_len > 0){
							memcpy(grid_buf + grid_buf_idx, grid, grid_len);
							grid_buf_idx += grid_len;
							if ((req != NULL) && (req_len > 0)){
								log_info(_T("Обнаружен автозапрос (%d байт)."), req_len);
								grid_auto_req = req;
								grid_auto_req_len = req_len;
							}
						}else{
							_sntprintf_s(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
								_T("Получены данные разметки нулевой длины."));
							non_grid_resp = 1;
						}
					}else{
						_sntprintf_s(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
							_T("Не найдены данные разметки."));
						non_grid_resp = 2;
					}
				} catch (SYNTAX_ERROR e){
					_sntprintf_s(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
						_T("Синтаксическая ошибка ответа %s по смещению %d (0x%.4X)."),
						X3Response::getErrorName(e), idx, idx);
					inquirer->ZBp().setSyntaxError();
					non_grid_resp = 1;
				}
			}
		}else if (inq_err == InquirerError::ForeignResp){
			LPCBYTE addr = cwb_sdk->resp() + 4;
			_sntprintf_s(err_msg, ARRAYSIZE(err_msg), _TRUNCATE,
				_T("Получен ответ для другого терминала (%.2hX:%.2hX)."), (WORD)addr[0], (WORD)addr[1]);
			xlog->writeForeign(0, addr[0], addr[1]);
			non_grid_resp = 1;
		}else{
			_sntprintf_s(err_msg, ARRAYSIZE(err_msg), _TRUNCATE, _T("Ошибка сеансового уровня: %s."),
				inquirer->error_str(inq_err));
			non_grid_resp = 1;
		}
	}else{
		_sntprintf_s(err_msg, ARRAYSIZE(err_msg), _TRUNCATE, _T("Ошибка SDK РМК: %s"), cwb_sdk->error_msg().c_str());
		non_grid_resp = 1;
	}
	if (err_msg[0] != 0){
		log_err(err_msg);
		grid_buf_idx = 0;
		termcore_callbacks.callMessageBox(_T("ОШИБКА"), err_msg);
/*		clrGridLists();
		clrIconLists();*/
	}
	if (!isTermcoreState(TS_CLOSE_WAIT)){
		if (non_grid_resp){
			if (grid_sync_cbk != NULL)
				grid_sync_cbk(true, NULL);
			if (non_grid_resp == 2){
				setTermcoreState(TS_RESPONSE);
				handleResp(rsp.get());
			}
		}else
			setTermcoreState(TS_READY);
	}
	processExitFlag();
	gridReqWait.stop();
}

static TERMCORE_RETURN sendGridRequest(const GridInfo &grid)
{
	if (!enterCriticalSection(CS_HOST_REQ)){
		log_err(LOG_REENTRANCE);
		return TC_REENTRANT;
	}
	TERMCORE_RETURN ret = TC_OK;
	CHAR req[256];
	_snprintf_s(req, ARRAYSIZE(req), _TRUNCATE, "P55TR(EXP.DATA.MAKET   %-8ls)000000000", grid.name().c_str());
	if (inquirer->writeRequest((LPCBYTE)req, strlen(req), 0) == InquirerError::OK){
		TERMCORE_STATE prev_ts = getTermcoreState();
		setTermcoreState(TS_REQUEST);
		CwbSdkError cwb_err = cwb_sdk->beginHostRequest(inquirer->req(),
			inquirer->req_len(), onEndGridRequest);
		if (cwb_err == CwbSdkError::OK)
			grid_req_t0 = GetTickCount();
		else{
			setTermcoreState(prev_ts);
			if (cwb_err == CwbSdkError::Cwb)
				log_err(_T("Ошибка SDK РМК: %s."), cwb_sdk->error_msg().c_str());
			else
				log_err(_T("Ошибка взаимодействия с SDK РМК: %s."),
					(cwb_sdk->error() == CwbSdkError::OK) ? CwbSdk::error_msg(cwb_err) : cwb_sdk->error_msg().c_str());
			ret = TC_CWB_FAULT;
		}
		processExitFlag();
	}else{
		log_err(_T("Ошибка создания запроса для получения разметки."));
		ret = TC_INVALID_REQ;
	}
	leaveCriticalSection(CS_HOST_REQ);
	return ret;
}

static TERMCORE_RETURN sendGridAutoRequest()
{
	if (!enterCriticalSection(CS_HOST_REQ)){
		log_err(LOG_REENTRANCE);
		return TC_REENTRANT;
	}
	TERMCORE_RETURN ret = TC_OK;
	DWORD rt = GetTickCount() - grid_req_t0;
	if (inquirer->writeRequest(grid_auto_req, grid_auto_req_len, rt) == InquirerError::OK){
		TERMCORE_STATE prev_ts = getTermcoreState();
		setTermcoreState(TS_REQUEST);
		CwbSdkError cwb_err = cwb_sdk->beginHostRequest(inquirer->req(),
			inquirer->req_len(), onEndGridRequest);
		if (cwb_err == CwbSdkError::OK)
			grid_req_t0 = GetTickCount();
		else{
			setTermcoreState(prev_ts);
			if (cwb_err == CwbSdkError::Cwb)
				log_err(_T("Ошибка SDK РМК: %s."), cwb_sdk->error_msg().c_str());
			else
				log_err(_T("Ошибка взаимодействия с SDK РМК: %s."),
					(cwb_sdk->error() == CwbSdkError::OK) ? CwbSdk::error_msg(cwb_err) : cwb_sdk->error_msg().c_str());
			ret = TC_CWB_FAULT;
		}
		processExitFlag();
	}else{
		log_err(_T("Ошибка создания запроса для получения разметки."));
		ret = TC_INVALID_REQ;
	}
	leaveCriticalSection(CS_HOST_REQ);
	return ret;
}

/* Декодирование разметки, распаковка и сохранение в файле на диске */
static bool storeGrid(const GridInfo &gi)
{
	log_info(_T("name = %s; id = %hc."), gi.name().c_str(), gi.id());
	if (grid_buf_idx == 0){
		log_err(_T("Буфер данных пуст."));
		return false;
	}
	SIZE_T len = 0;
	if (!xbase64Decode(grid_buf, grid_buf_idx, grid_buf, grid_buf_idx, len) || (len == 0)){
		log_err(_T("Ошибка декодирования данных."));
		return false;
	}
	BITMAPFILEHEADER bmp_hdr;
	SIZE_T l = sizeof(bmp_hdr);
	INT rc = uncompress((LPBYTE)&bmp_hdr, &l, grid_buf, len);
	if ((rc != Z_OK) && (rc != Z_BUF_ERROR)){
		log_err(_T("Ошибка распаковки (%d)."), rc);
		return false;
	}else if ((bmp_hdr.bfType != 0x4d42) || (bmp_hdr.bfSize == 0)){
		log_err(_T("Неверный формат заголовка BMP."));
		return false;
	}
	SIZE_T bmp_len = bmp_hdr.bfSize;
	if (bmp_len > 262144){
		log_err(_T("Размер файла BMP (%u) превышает максимально допустимый."), bmp_len);
		return false;
	}
	scoped_ptr<BYTE> bmp_data(new BYTE[bmp_len]);
	rc = uncompress(bmp_data.get(), &bmp_len, grid_buf, len);
	if (rc != Z_OK){
		log_err(_T("Ошибка распаковки BMP (%d)."), rc);
		return false;
	}
	TCHAR name[MAX_PATH + 1];
	_sntprintf_s(name, ARRAYSIZE(name), _TRUNCATE, _T("%s\\%s.BMP"), grid_folder, gi.name().c_str());
	INT fd = _topen(name, _O_WRONLY | _O_BINARY | _O_TRUNC | _O_CREAT, _S_IWRITE);
	if (fd == -1){
		log_win_err(_T("Ошибка создания файла %s:"), name);
		return false;
	}
	bool ret = false;
	rc = _write(fd, bmp_data.get(), bmp_len);
	if (rc == bmp_len){
		log_info(_T("Разметка успешно записана в файл %s."), name);
		ret = true;
	}else if (rc == -1)
		log_win_err(_T("Ошибка записи в файл %s:"), name);
	else
		log_err(_T("В %s вместо %u записано %d байт."), name, bmp_len, rc);
	_close(fd);
	if (!ret)
		_tremove(name);
	return ret;
}

/* Удаление разметки с диска */
static bool removeGrid(const GridInfo &gi)
{
	static TCHAR path[MAX_PATH + 1];
	_sntprintf_s(path, ARRAYSIZE(path), _TRUNCATE, _T("%s\\%s.BMP"), grid_folder, gi.name().c_str());
	log_info(_T("Удаляем разметку %s..."), path);
	bool ret = _tunlink(path) == 0;
	if (ret)
		log_info(_T("Разметка %s успешно удалена."), path);
	else
		log_win_err(_T("Ошибка удаления разметки %s:"), path);
	return ret;
}

/* Загрузка заданной разметки печати из "Экспресс-3" */
static TERMCORE_RETURN downloadGrid(const GridInfo &gi)
{
	TERMCORE_RETURN ret = TC_OK;
	bool flag = false, need_req = true;
	log_info(_T("Загружаем разметку %s..."), gi.name().c_str());
	grid_buf_idx = 0;
	for (DWORD n = 0; need_req; n++){
		flag = false;
		ret = (n == 0) ? sendGridRequest(gi) : sendGridAutoRequest();
		if (ret == TC_OK){
			gridReqWait.wait();
			if (isTermcoreState(TS_READY) && (grid_buf_idx > 0)){
				if (grid_auto_req_len == 0){
					log_info(_T("Разметка %s получена полностью. Сохраняем в файл..."),
						gi.name().c_str());
					need_req = false;
					if (storeGrid(gi))
						flag = true;
				}else
					flag = true;
			}
		}
		if (!flag){
			if (ret == TC_OK)
				ret = TC_GRID_LOAD;
			break;
		}
	}
	if ((ret != TC_OK) && (ret != TS_CLOSE_WAIT))
		log_err(_T("Ошибка загрузки разметки %s."), gi.name().c_str());
	return ret;
}

bool syncGrids(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove, list<GridInfo> &grids_failed,
	       X3SyncCallback_t cbk)
{
	if (!needGridsUpdate()){
		log_info(_T("Обновление разметок не требуется."));
		return true;
	}
	bool ok = true;
	TCHAR txt[256];
	SIZE_T n = 0;
	grid_sync_cbk = cbk;
	cbk(false, _T("Загрузка разметок из \"Экспресс-3\""));
	Sleep(100);
	grids_failed.clear();
	list<GridInfo> _grids_to_create, _grids_to_remove;
/* Сначала закачиваем новые разметки */
	for (const auto &p : grids_to_create){
		_sntprintf_s(txt, ARRAYSIZE(txt), _TRUNCATE, _T("Загрузка разметки %s (%u из %u)"),
			p.name().c_str(), (n + 1), grids_to_create.size());
		cbk(false, txt);
		TERMCORE_RETURN rc = downloadGrid(p);
		if (rc == TC_OK)
			n++;
		else /*if (inquirer->first_req())*/{
			ok = false;
			break;
		}/*else{
			_grids_to_create.push_back(p);
			grids_failed.push_back(p);
		}*/
	}
/* Затем удаляем старые */
	if (ok){
		grids_to_create.assign(_grids_to_create.cbegin(), _grids_to_create.cend());
		for (const auto &p : grids_to_remove){
			if (removeGrid(p))
				n++;
			else
				_grids_to_remove.push_back(p);
		}
		grids_to_remove.assign(_grids_to_remove.cbegin(), _grids_to_remove.cend());
	}
	if (ok && (n > 0))
		ok = updateXPrnGrids(cbk) && updateKktGrids(cbk);
	if (ok){
		if (cbk != NULL)
			cbk(false, _T("Разметки успешно загружены"));
	}else
		showXPrnPicSyncError(grids_failed_xprn, grids_failed_kkt, icons_failed_xprn, icons_failed_kkt);
	grid_sync_cbk = NULL;
	return ok;
}

/* Получение списка разметок из БПУ */
static bool findXPrnGrids(list<GridInfo> &xprn_grids)
{
	bool ret = false;
	xprn_grids.clear();
	log_info(_T("Чтение списка разметок из БПУ..."));
	if (xprn->beginGridLst(xprnOpCallback)){
		xprn_wait.wait();
		if (xprn->statusOK()){
			xprn_grids.assign(xprn->grids().cbegin(), xprn->grids().cend());
			log_info(_T("Найдено разметок в БПУ: %d."), xprn_grids.size());
			ret = true;
		}else
			log_err(_T("Ошибка получения списка разметок из БПУ: %s."), XPrn::status_str(xprn->status()));
	}else
		log_err(_T("Невозможно начать получение списка разметок из БПУ."));
	return ret;
}

/* Создание списка разметок БПУ для загрузки и удаления */
static VOID checkXPrnGrids(const list<GridInfo> &stored_grids, list<GridInfo> &grids_to_load,
	list<GridInfo> &grids_to_erase)
{
	grids_to_load.clear();
	grids_to_erase.clear();
/* Создаём список разметок в БПУ */
	list<GridInfo> xprn_grids;
	if (!findXPrnGrids(xprn_grids))
		return;
/* Ищем разметки для загрузки */
	log_dbg(_T("Ищем разметки для загрузки..."));
	for (const auto &p : stored_grids){
		auto p1 = find_if(xprn_grids, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 == xprn_grids.end()){
			log_dbg(_T("Разметка %s #%d (%hc) отсутствует в БПУ и будет туда загружена."),
				p.name().c_str(), p.nr(), p.id());
			grids_to_load.push_back(p);
		}else if (p.isNewer(*p1)){
			log_dbg(_T("Разметку %s #%d (%hc) необходимо обновить."),
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			grids_to_load.push_back(p);
			grids_to_erase.push_back(*p1);
		}else
			log_dbg(_T("Разметка %s #%d (%hc) не требует обновления."),
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
	}
/* Ищем разметки для удаления */
	log_dbg(_T("Ищем разметки для удаления..."));
	for (const auto &p : xprn_grids){
		if (find_if(stored_grids, [p](const GridInfo &gi) {return gi.id() == p.id();}) == stored_grids.end()){
			log_dbg(_T("Разметка %s #%d (%hc) помечена для удаления."), p.name().c_str(), p.nr(), p.id());
			grids_to_erase.push_back(p);
		}
	}
	log_dbg(_T("Найдено разметок: для загрузки: %d; для удаления: %d."), grids_to_load.size(), grids_to_erase.size());
}

/* Запись заданной разметки в БПУ */
static bool writeGridToXPrn(const GridInfo &gi)
{
	if (xprn == NULL)
		return false;
	bool ret = false;
	static TCHAR path[MAX_PATH + 1];
	_sntprintf_s(path, ARRAYSIZE(path), _TRUNCATE, _T("%s\\%s.BMP"), grid_folder, gi.name().c_str());
	log_info(_T("Загружаем в БПУ разметку %s..."), path);
	if (xprn->beginGridLoad(path, gi.nr(), xprnOpCallback)){
		xprn_wait.wait();
		ret = xprn->statusOK();
		if (ret)
			log_info(_T("Разметка %s успешно загружена в БПУ."), path);
		else
			log_err(_T("Ошибка загрузки разметки %s в БПУ: %s."), path, XPrn::status_str(xprn->status()));
	}else
		log_err(_T("Невозможно начать загрузку разметки %s в БПУ: %s."), path, XPrn::status_str(xprn->status()));
	return ret;
}

/* Удаление заданной разметки из БПУ */
static bool eraseGridFromXPrn(const GridInfo &gi)
{
	if (xprn == NULL)
		return false;
	bool ret = false;
	log_info(_T("Удаляем из БПУ разметки %s..."), gi.name().c_str());
	if (xprn->beginGridErase(gi.nr(), xprnOpCallback)){
		xprn_wait.wait();
		ret = xprn->statusOK();
		if (ret)
			log_info(_T("Разметка %s успешно удалена из БПУ."), gi.name().c_str());
		else
			log_err(_T("Ошибка удаления разметки %s из БПУ: %s."), gi.name().c_str(),
				XPrn::status_str(xprn->status()));
	}else
		log_err(_T("Невозможно начать удаление разметки %s из БПУ: %s."), gi.name().c_str(),
			XPrn::status_str(xprn->status()));
	return ret;
}

/* Обновление разметок в БПУ "Видеотон" */
static bool updateGridsVtsv(const list<GridInfo> &grids_to_load, const list<GridInfo> &grids_to_erase,
	list<GridInfo> &grids_failed, InitializationNotify_t init_notify)
{
//	bool ret = true;
	static TCHAR txt[64];
	SIZE_T n = 1;
/* Сначала удаляем ненужные разметки из БПУ */
	for (const auto &p : grids_to_erase){
		_sntprintf_s(txt, ARRAYSIZE(txt), _TRUNCATE, _T("Удаление из БПУ разметки %s (%u из %u)"),
			p.name().c_str(), n++, grids_to_erase.size());
		init_notify(false, txt);
		eraseGridFromXPrn(p);
/*		if (!eraseGridFromXPrn(p)){
			ret = false;
			break;
		}*/
	}
/*	if (!ret)
		return ret;*/
/* Затем загружаем новые разметки */
	n = 1;
	for (const auto &p : grids_to_load){
		_sntprintf_s(txt, ARRAYSIZE(txt), _TRUNCATE, _T("Загрузка в БПУ разметки %s (%u из %u)"),
			p.name().c_str(), n++, grids_to_load.size());
		init_notify(false, txt);
		if (!writeGridToXPrn(p))
			grids_failed.push_back(p);
/*		{
			ret = false;
			break;
		}*/
	}
/* Проверяем, что разметки успешно загрузились */
	log_info(_T("Проверяем записанные разметки..."));
	Sleep(3000);
	list<GridInfo> xprn_grids;
	if (!findXPrnGrids(xprn_grids))
		return false;
	bool ok = true;
	for (const auto &p : grids_to_load){
		auto p1 = find_if(grids_failed, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 != grids_failed.end())
			continue;
		p1 = find_if(xprn_grids, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 == xprn_grids.end()){
			log_dbg(_T("Разметка %s #%d (%hc) отсутствует в БПУ после загрузки."),
				p.name().c_str(), p.nr(), p.id());
			grids_failed.push_back(p);
			ok = false;
		}else if (p.isNewer(*p1)){
			log_dbg(_T("Разметка %s #%d (%hc) не была удалена из БПУ."),
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			grids_failed.push_back(p);
			ok = false;
		}
	}
	if (ok)
		log_info(_T("Разметки в БПУ успешно обновлены."));
	return true; /*ret;*/
}

/* Обновление разметок в БПУ "СПЕКТР-ДС" */
static bool updateGridsSPrn(const list<GridInfo> &stored_grids, list<GridInfo> &grids_failed,
	InitializationNotify_t init_notify)
{
/* Сначала удаляем все разметки из БПУ */
	log_info(_T("Удаляем все имеющиеся разметки из БПУ..."));
	init_notify(false, _T("Удаление разметок из БПУ"));
	if (!xprn->beginGridEraseAll(xprnOpCallback)){
		log_err(_T("Невозможно начать удаление разметок из БПУ: %s."), XPrn::status_str(xprn->status()));
		return false;
	}
	xprn_wait.wait();
	if (!xprn->statusOK()){
		log_err(_T("Ошибка удаления разметок из БПУ: %s."), XPrn::status_str(xprn->status()));
		return false;
	}
//	bool ret = true;
/* Затем загружаем новые разметки */
	SIZE_T n = 1;
	for (const auto &p : stored_grids){
		static TCHAR txt[64];
		_sntprintf_s(txt, ARRAYSIZE(txt), _TRUNCATE, _T("Загрузка в БПУ разметки %s (%u из %u)"),
			p.name().c_str(), n++, stored_grids.size());
		init_notify(false, txt);
		if (!writeGridToXPrn(p))
			grids_failed.push_back(p);
/*		{
			ret = false;
			break;
		}*/
	}
	return true; /*ret;*/
}

/* Обновление разметок в БПУ */
bool updateXPrnGrids(InitializationNotify_t init_notify)
{
	if (xprn == NULL){
		log_err(_T("БПУ отсутствует."));
		return false;
	}
	bool ret = false;
	init_notify(false, _T("Обновление разметок в БПУ"));
	Sleep(100);
	list<GridInfo> stored_grids, grids_to_load, grids_to_erase;
	findStoredGridsXPrn(stored_grids);
	checkXPrnGrids(stored_grids, grids_to_load, grids_to_erase);
	if (!grids_to_load.empty() || !grids_to_erase.empty()){
		if (xprn->type() == XPrnType::Videoton)
			ret = updateGridsVtsv(grids_to_load, grids_to_erase, grids_failed_xprn, init_notify); 
		else if (xprn->type() == XPrnType::Spectrum)
			ret = updateGridsSPrn(stored_grids, grids_failed_xprn, init_notify);
	}else
		ret = true;
	if (!ret && !isTermcoreState(TS_INIT)){
		static TCHAR txt[256];
		BYTE status = xprn->status();
		if (xprn->statusOK(status))
			_sntprintf_s(txt, ARRAYSIZE(txt), _TRUNCATE, _T("Ошибка обновления разметок в БПУ."));
		else
			_sntprintf_s(txt, ARRAYSIZE(txt), _TRUNCATE, _T("Ошибка обновления разметок в БПУ: %s."),
				xprn->status_str(status));
		termcore_callbacks.callMessageBox(_T("ОБНОВЛЕНИЕ РАЗМЕТОК"), txt);
	}
	return ret;
}

/* Получение списка разметок из ККТ */
static bool findKktGrids(list<GridInfo> &kkt_grids)
{
	bool ret = false;
	kkt_grids.clear();
	log_info(_T("Чтение списка разметок из ККТ..."));
	if (kkt->getGridLst()){
		KktRespGridLst *rsp = dynamic_cast<KktRespGridLst *>(kkt->rsp());
		if ((rsp != NULL) && rsp->statusOk()){
			kkt_grids.assign(rsp->grids().cbegin(), rsp->grids().cend());
			log_info(_T("Найдено разметок в ККТ: %d."), kkt_grids.size());
			ret = true;
		}else
			log_err(_T("Ошибка получения списка разметок из ККТ: 0x%.2hx."), (WORD)kkt->status());
	}else
		log_err(_T("Невозможно начать получение списка разметок из ККТ: 0x%.2hx."), (WORD)kkt->status());
	return ret;
}

/* Создание списка разметок ККТ для загрузки и удаления */
static VOID checkKktGrids(const list<GridInfo> &stored_grids, list<GridInfo> &grids_to_load,
	list<GridInfo> &grids_to_erase)
{
	grids_to_load.clear();
	grids_to_erase.clear();
/* Создаём список разметок в ККТ */
	list<GridInfo> kkt_grids;
	if (!findKktGrids(kkt_grids))
		return;
/* Ищем разметки для загрузки */
	log_dbg(_T("Ищем разметки для загрузки..."));
	for (const auto &p : stored_grids){
		auto p1 = find_if(kkt_grids, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 == kkt_grids.end()){
			log_dbg(_T("Разметка %s #%d (%hc) отсутствует в ККТ и будет туда загружена."),
				p.name().c_str(), p.nr(), p.id());
			grids_to_load.push_back(p);
		}else if (p.isNewer(*p1)){
			log_dbg(_T("Разметку %s #%d (%hc) необходимо обновить."),
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			grids_to_load.push_back(p);
			grids_to_erase.push_back(*p1);
		}else
			log_dbg(_T("Разметка %s #%d (%hc) не требует обновления."),
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
	}
/* Ищем разметки для удаления */
	log_dbg(_T("Ищем разметки для удаления..."));
	for (const auto &p : kkt_grids){
		if (find_if(stored_grids, [p](const GridInfo &gi) {return gi.id() == p.id();}) == stored_grids.end()){
			log_dbg(_T("Разметка %s #%d (%hc) помечена для удаления."), p.name().c_str(), p.nr(), p.id());
			grids_to_erase.push_back(p);
		}
	}
	log_dbg(_T("Найдено разметок: для загрузки: %d; для удаления: %d."), grids_to_load.size(), grids_to_erase.size());
}

/* Запись заданной разметки в ККТ */
static bool writeGridToKkt(const GridInfo &gi)
{
	if (kkt == NULL)
		return false;
	bool ret = false;
	static TCHAR path[MAX_PATH + 1];
	_sntprintf_s(path, ARRAYSIZE(path), _TRUNCATE, _T("%s\\%s.BMP"), grid_folder, gi.name().c_str());
	log_info(_T("Загружаем в ККТ разметку %s..."), path);
	if (kkt->loadGrid(path, gi.nr())){
		KktRespGridLoad *rsp = dynamic_cast<KktRespGridLoad *>(kkt->rsp());
		if ((rsp != NULL) && rsp->statusOk()){
			ret = true;
			log_info(_T("Разметка %s успешно загружена в ККТ."), path);
		}else
			log_err(_T("Ошибка загрузки разметки %s в ККТ: 0x%.2hx."), path, (WORD)kkt->status());
	}else
		log_err(_T("Невозможно начать загрузку разметки %s в ККТ: 0x%.2hx."), path, (WORD)kkt->status());
	return ret;
}

/* Обновление разметок в ККТ "СПЕКТР-Ф" */
static bool updateGridsKkt(const list<GridInfo> &stored_grids, list<GridInfo> &grids_failed,
	InitializationNotify_t init_notify)
{
/* Сначала удаляем все разметки из ККТ */
	log_info(_T("Удаляем все имеющиеся разметки из ККТ..."));
	init_notify(false, _T("Удаление разметок из ККТ"));
	if (kkt->eraseAllGrids()){
		KktRespGridEraseAll *rsp = dynamic_cast<KktRespGridEraseAll *>(kkt->rsp());
		if ((rsp != NULL) && rsp->statusOk())
			log_info(_T("Разметки удалены из ККТ."));
		else{
			log_err(_T("Ошибка удаления разметок из ККТ: 0x%.2hx."), (WORD)kkt->status());
			return false;
		}
	}else{
		log_err(_T("Невозможно начать удаление разметок из ККТ: 0x%.2hx."), (WORD)kkt->status());
		return false;
	}
/* Затем загружаем новые разметки */
	SIZE_T n = 1;
	kkt->beginBatchMode();
	for (const auto &p : stored_grids){
		static TCHAR txt[64];
		_sntprintf_s(txt, ARRAYSIZE(txt), _TRUNCATE, _T("Загрузка в ККТ разметки %s (%u из %u)"),
			p.name().c_str(), n++, stored_grids.size());
		init_notify(false, txt);
		if (!writeGridToKkt(p))
			grids_failed.push_back(p);
	}
	kkt->endBatchMode();
	return true;
}

/* Обновление разметок в ККТ */
bool updateKktGrids(InitializationNotify_t init_notify)
{
	log_info(_T("kkt = %p."), kkt);
	if (kkt == NULL)
		return false;
	bool ret = false;
	init_notify(false, _T("Обновление разметок в ККТ"));
	Sleep(100);
	list<GridInfo> stored_grids, grids_to_load, grids_to_erase;
	findStoredGridsKkt(stored_grids);
	checkKktGrids(stored_grids, grids_to_load, grids_to_erase);
	if (!grids_to_load.empty() || !grids_to_erase.empty())
		ret = updateGridsKkt(stored_grids, grids_failed_kkt, init_notify);
	else
		ret = true;
	if (!ret && !isTermcoreState(TS_INIT)){
		static TCHAR txt[256];
		_sntprintf_s(txt, ARRAYSIZE(txt), _TRUNCATE, _T("Ошибка обновления разметок в ККТ: 0x%.2hx."),
			(WORD)kkt->status());
		termcore_callbacks.callMessageBox(_T("ОБНОВЛЕНИЕ РАЗМЕТОК"), txt);
	}
	return ret;
}
