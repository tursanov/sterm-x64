/* Работа с разметками бланков БПУ/ККТ. (c) gsr, 2015-2016, 2019, 2022, 2024 */

#include <zlib.h>
#include "x3data/grids.h"
#include "termlog.h"
#include "xbase64.h"

bool GridInfo::parse(const char * name)
{
	if (name == NULL)
		return false;
	size_t len = strlen(name);
	if (len == 0)
		return false;
	char *nm = new char[len + 1];
	for (size_t i = 0; i <= len; i++)
		nm[i] = (i < len) ? toupper(name[i]) : 0;
	bool ret = false;
	static const char * bmp_ext = ".BMP";
	int nr;
	char ext[5], pr, ch;
	if ((sscanf(nm, "HU%d%4s%c", &nr, ext, &ch) == 2) && (strcmp(ext, bmp_ext) == 0)){
		if (isNrValid(nr)){
			_prefix = "HU";
			_date = 0;
			ret = true;
		}
	}else{
		int y, d;
		if (sscanf(nm, "%c%2d%2d%3d%4s%c", &pr, &nr, &y, &d, ext, &ch) == 5){
			pr = toupper(pr);
			if (((pr == 'L') || (pr == 'M')) && isNrValid(nr) &&
					(strcmp(ext, bmp_ext) == 0) && (y > 14) && (d > 0) && (d < 367)){
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

bool GridInfo::parse(const uint8_t *data, size_t len)
{
	if ((data == NULL) || (len == 0))
		return false;
	bool ret = false;
	int nr = 0;
	char nm[XPrn::SPRN_MAX_GRID_NAME_LEN + 1];
	static const char old_prefix[] = "<HU";
	if (memcmp((const char *)data, old_prefix, ASIZE(old_prefix) - 1) == 0){
		bool tail = false;
		for (int i = ASIZE(old_prefix) - 1; i < 10; i++){
			uint8_t b = data[i];
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
					snprintf(nm, ASIZE(nm), "HU%d", nr);
					_prefix = "HU";
					_date = 0;
					ret = true;
				}
			}
		}
	}else{
		char pr, ch;
		int y, d;
		if (sscanf((const char *)data, "<%c%2d%2d%3d%c", &pr, &nr, &y, &d, &ch) == 5){
			pr &= ~0x20;
			if (((pr == 'L') || (pr == 'M')) && (ch == '>') && isNrValid(nr) && (y > 14) && (d > 0) && (d < 367)){
				snprintf(nm, ASIZE(nm), "%c%.2d%.2d%.3d", pr, nr, y, d);
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

void clrGridLists(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove)
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
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), "%s\\%s", grid_folder, pattern);
	WIN32_FIND_DATA fd;
	HANDLE fh = FindFirstFile(path, &fd);
	if (fh != INVALID_HANDLE_VALUE){
		do {
			GridInfo gi;
			if (gi.parse(fd.cFileName)){
				log_dbg("Обнаружена разметка %s #%d (%hc).", gi.name().c_str(), gi.nr(), gi.id());
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
	findStoredGrids(stored_grids, "HU?*.BMP");
	findStoredGrids(stored_grids, "L???????.BMP");
	return !stored_grids.empty();
}

static bool findStoredGridsKkt(list<GridInfo> &stored_grids)
{
	stored_grids.clear();
	findStoredGrids(stored_grids, "M???????.BMP");
	return !stored_grids.empty();
}

void checkStoredGrids(const list<GridInfo> &x3_grids, list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove,
	bool (*find_fn)(list<GridInfo> &))
{
	clrGridLists(grids_to_create, grids_to_remove);
/* Создаём список разметок, хранящихся в ПАК РМК */
	log_dbg("Ищем разметки в каталоге %s...", grid_folder);
	list<GridInfo> stored_grids;
	find_fn(stored_grids);
	log_dbg("Найдено разметок: %d.", stored_grids.size());
/* Ищем разметки для закачки */
	log_dbg("Ищем разметки для закачки...");
	for (const auto &p : x3_grids){
		bool found = false;
		for (const auto &p1 : stored_grids){
			if (p1.id() != p.id())
				continue;
			else if (p.isNewer(p1)){
				log_dbg("Разметка %s #%d (%hc) старее экспрессовской и будет удалена.",
					p1.name().c_str(), p1.nr(), p1.id());
				grids_to_remove.push_back(p1);
			}else if (p1.isNewer(p)){
				log_dbg("Разметка %s #%d (%hc) новее экспрессовской и будет удална.",
					p1.name().c_str(), p1.nr(), p1.id());
				grids_to_remove.push_back(p1);
			}else
				found = true;
		}
		if (found)
			log_dbg("Разметка %s #%d (%hc) не требует обновления.", p.name().c_str(), p.nr(), p.id());
		else{
			log_dbg("Разметка %s #%d (%hc) отсутствует в ПАК РМК и будет загружена из \"Экспресс-3\").",
				p.name().c_str(), p.nr(), p.id());
			grids_to_create.push_back(p);
		}
	}
/* Ищем разметки для удаления */
/*	log_dbg("Ищем разметки для удаления...");
	for (const auto &p : stored_grids){
		if (find_if(x3_grids, [p](const GridInfo &gi) {return gi.id() == p.id();}) == x3_grids.end()){
			log_dbg("Разметка %s #%d (%hc) помечена для удаления.", p.name().c_str(), p.nr(), p.id());
			grids_to_remove.push_back(p);
		}
	}*/
	log_dbg("Найдено разметок: для закачки: %d; для удаления: %d.", grids_to_create.size(), grids_to_remove.size());
}

void checkStoredGridsXPrn(const list<GridInfo> &x3_grids)
{
	checkStoredGrids(x3_grids, grids_to_create_xprn, grids_to_remove_xprn, findStoredGridsXPrn);
}

void checkStoredGridsKkt(const list<GridInfo> &x3_grids)
{
	checkStoredGrids(x3_grids, grids_to_create_kkt, grids_to_remove_kkt, findStoredGridsKkt);
}

bool checkX3Grids(const uint8_t *data, size_t len, list<GridInfo> &x3_grids_xprn, list<GridInfo> &x3_grids_kkt)
{
	if ((data == NULL) || (len == 0))
		return false;
	x3_grids_xprn.clear();
	x3_grids_kkt.clear();
	for (size_t i = 0; (data[i] == 0x3c) && ((i + 1) < len); i++){
		const uint8_t *p = (uint8_t *)memchr(data + i + 1, 0x3e, len - i - 1);
		if (p == NULL)
			break;
		GridInfo gi;
		if (gi.parse(data + i, len - 1)){
			if ((gi.prefix() == "HU") || (gi.prefix() == "L"))
				x3_grids_xprn.push_back(gi);
			else if (gi.prefix() == "M")
				x3_grids_kkt.push_back(gi);
		}
		i += p - (data + i);
	}
	return !x3_grids_xprn.empty() || !x3_grids_kkt.empty();
}

/* Максимальный размер буфера сжатой разметки */
static const size_t MAX_COMPRESSED_GRID_LEN = 65536;
/* Буфер для приёма сжатой разметки */
static uint8_t grid_buf[MAX_COMPRESSED_GRID_LEN];
/* Текущий размер принятых данных сжатой разметки */
static size_t grid_buf_idx = 0;

static uint32_t grid_req_t0 = 0;

static const uint8_t *grid_auto_req = NULL;
static size_t grid_auto_req_len = 0;

static WaitTool gridReqWait;

static X3SyncCallback_t grid_sync_cbk = NULL;

static void onEndGridRequest(CwbSdk *cwb_sdk)
{
	log_info("cwb_sdk = %p.", cwb_sdk);
	grid_auto_req = NULL;
	grid_auto_req_len = 0;
	static char err_msg[1024];
	err_msg[0] = 0;
/* 0 -- данные разметки; 1 -- другой ответ, обработка не требуется; 2 -- другой ответ, требуется обработка */
	int non_grid_resp = 0;
	scoped_ptr<DMap> dmap(new DMap());
	scoped_ptr<X3Response> rsp(new X3Response(inquirer, term_keys, term_smap, dmap.get(), xprn, tprn, kkt,
		xlog, termcore_callbacks));
	if (cwb_sdk->error() == CwbSdkError::OK){
		setTermcoreState(TS_RESPONSE);
		InquirerError inq_err = inquirer->checkResp(cwb_sdk->resp(), cwb_sdk->resp_len());
		if (inq_err == InquirerError::OK){
			if (inquirer->is_specific_error()){
				uint8_t code = inquirer->x3_specific_error();
				snprintf(err_msg, ASIZE(err_msg),
					"Сообщение об ошибке \"Экспресс\": %.2hX (%s).", (uint16_t)code,
					Inquirer::specific_error_str(code));
				xlog->writeSpecial(0, code);
				non_grid_resp = 1;
			}else if (isXmlResp(inquirer->resp(), inquirer->resp_len())){
				snprintf(err_msg, ASIZE(err_msg), "В ответ на запрос разметки пришёл XML.");
				non_grid_resp = 1;
			}else{
				UINT idx = 0;
				uint32_t rt = GetTickCount() - grid_req_t0;
				try {
					log_info("Начинаем синтаксический разбор ответа.");
					bool has_tprn = cfg->has_tprn();
					cfg->set_has_tprn(true);
					rsp->parse(inquirer->resp(), idx, inquirer->resp_len(), rt);
					cfg->set_has_tprn(has_tprn);
					cfg->clrChanged();
					log_info("Ответ разобран.");
					const uint8_t *grid = NULL, req = NULL;
					size_t grid_len = 0, req_len = 0;
					if (rsp->getPicData(grid, grid_len, req, req_len)){
						log_info("Обнаружены данные разметки (%d байт).", grid_len);
						if (grid_len > (ASIZE(grid_buf) - grid_buf_idx)){
							snprintf(err_msg, ASIZE(err_msg),
								"Переполнение буфера данных разметки.");
							non_grid_resp = 1;
						}else if (grid_len > 0){
							memcpy(grid_buf + grid_buf_idx, grid, grid_len);
							grid_buf_idx += grid_len;
							if ((req != NULL) && (req_len > 0)){
								log_info("Обнаружен автозапрос (%d байт).", req_len);
								grid_auto_req = req;
								grid_auto_req_len = req_len;
							}
						}else{
							snprintf(err_msg, ASIZE(err_msg),
								"Получены данные разметки нулевой длины.");
							non_grid_resp = 1;
						}
					}else{
						snprintf(err_msg, ASIZE(err_msg),
							"Не найдены данные разметки.");
						non_grid_resp = 2;
					}
				} catch (SYNTAX_ERROR e){
					snprintf(err_msg, ASIZE(err_msg),
						"Синтаксическая ошибка ответа %s по смещению %d (0x%.4X).",
						X3Response::getErrorName(e), idx, idx);
					inquirer->ZBp().setSyntaxError();
					non_grid_resp = 1;
				}
			}
		}else if (inq_err == InquirerError::ForeignResp){
			const uint8_t *addr = cwb_sdk->resp() + 4;
			snprintf(err_msg, ASIZE(err_msg),
				"Получен ответ для другого терминала (%.2hX:%.2hX).", (uint16_t)addr[0], (uint16_t)addr[1]);
			xlog->writeForeign(0, addr[0], addr[1]);
			non_grid_resp = 1;
		}else{
			snprintf(err_msg, ASIZE(err_msg), "Ошибка сеансового уровня: %s.",
				inquirer->error_str(inq_err));
			non_grid_resp = 1;
		}
	}else{
		snprintf(err_msg, ASIZE(err_msg), "Ошибка SDK РМК: %s", cwb_sdk->error_msg().c_str());
		non_grid_resp = 1;
	}
	if (err_msg[0] != 0){
		log_err(err_msg);
		grid_buf_idx = 0;
		termcore_callbacks.callMessageBox("ОШИБКА", err_msg);
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
	char req[256];
	sprintf(req, ASIZE(req), "P55TR(EXP.DATA.MAKET   %-8ls)000000000", grid.name().c_str());
	if (inquirer->writeRequest((uint8_t *)req, strlen(req), 0) == InquirerError::OK){
		TERMCORE_STATE prev_ts = getTermcoreState();
		setTermcoreState(TS_REQUEST);
		CwbSdkError cwb_err = cwb_sdk->beginHostRequest(inquirer->req(),
			inquirer->req_len(), onEndGridRequest);
		if (cwb_err == CwbSdkError::OK)
			grid_req_t0 = GetTickCount();
		else{
			setTermcoreState(prev_ts);
			if (cwb_err == CwbSdkError::Cwb)
				log_err("Ошибка SDK РМК: %s.", cwb_sdk->error_msg().c_str());
			else
				log_err("Ошибка взаимодействия с SDK РМК: %s.",
					(cwb_sdk->error() == CwbSdkError::OK) ? CwbSdk::error_msg(cwb_err) : cwb_sdk->error_msg().c_str());
			ret = TC_CWB_FAULT;
		}
		processExitFlag();
	}else{
		log_err("Ошибка создания запроса для получения разметки.");
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
	uint32_t rt = GetTickCount() - grid_req_t0;
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
				log_err("Ошибка SDK РМК: %s.", cwb_sdk->error_msg().c_str());
			else
				log_err("Ошибка взаимодействия с SDK РМК: %s.",
					(cwb_sdk->error() == CwbSdkError::OK) ? CwbSdk::error_msg(cwb_err) : cwb_sdk->error_msg().c_str());
			ret = TC_CWB_FAULT;
		}
		processExitFlag();
	}else{
		log_err("Ошибка создания запроса для получения разметки.");
		ret = TC_INVALID_REQ;
	}
	leaveCriticalSection(CS_HOST_REQ);
	return ret;
}

/* Декодирование разметки, распаковка и сохранение в файле на диске */
static bool storeGrid(const GridInfo &gi)
{
	log_info("name = %s; id = %hc.", gi.name().c_str(), gi.id());
	if (grid_buf_idx == 0){
		log_err("Буфер данных пуст.");
		return false;
	}
	size_t len = 0;
	if (!xbase64Decode(grid_buf, grid_buf_idx, grid_buf, grid_buf_idx, len) || (len == 0)){
		log_err("Ошибка декодирования данных.");
		return false;
	}
	BITMAPFILEHEADER bmp_hdr;
	size_t l = sizeof(bmp_hdr);
	int rc = uncompress((LPBYTE)&bmp_hdr, &l, grid_buf, len);
	if ((rc != Z_OK) && (rc != Z_BUF_ERROR)){
		log_err("Ошибка распаковки (%d).", rc);
		return false;
	}else if ((bmp_hdr.bfType != 0x4d42) || (bmp_hdr.bfSize == 0)){
		log_err("Неверный формат заголовка BMP.");
		return false;
	}
	size_t bmp_len = bmp_hdr.bfSize;
	if (bmp_len > 262144){
		log_err("Размер файла BMP (%u) превышает максимально допустимый.", bmp_len);
		return false;
	}
	scoped_ptr<uint8_t> bmp_data(new uint8_t[bmp_len]);
	rc = uncompress(bmp_data.get(), &bmp_len, grid_buf, len);
	if (rc != Z_OK){
		log_err("Ошибка распаковки BMP (%d).", rc);
		return false;
	}
	char name[PATH_MAX];
	snprintf(name, ASIZE(name), "%s\\%s.BMP", grid_folder, gi.name().c_str());
	int fd = _topen(name, _O_WRONLY | _O_BINARY | _O_TRUNC | _O_CREAT, _S_IWRITE);
	if (fd == -1){
		log_win_err("Ошибка создания файла %s:", name);
		return false;
	}
	bool ret = false;
	rc = _write(fd, bmp_data.get(), bmp_len);
	if (rc == bmp_len){
		log_info("Разметка успешно записана в файл %s.", name);
		ret = true;
	}else if (rc == -1)
		log_win_err("Ошибка записи в файл %s:", name);
	else
		log_err("В %s вместо %u записано %d байт.", name, bmp_len, rc);
	_close(fd);
	if (!ret)
		_tremove(name);
	return ret;
}

/* Удаление разметки с диска */
static bool removeGrid(const GridInfo &gi)
{
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), "%s\\%s.BMP", grid_folder, gi.name().c_str());
	log_info("Удаляем разметку %s...", path);
	bool ret = _tunlink(path) == 0;
	if (ret)
		log_info("Разметка %s успешно удалена.", path);
	else
		log_win_err("Ошибка удаления разметки %s:", path);
	return ret;
}

/* Загрузка заданной разметки печати из "Экспресс-3" */
static TERMCORE_RETURN downloadGrid(const GridInfo &gi)
{
	TERMCORE_RETURN ret = TC_OK;
	bool flag = false, need_req = true;
	log_info("Загружаем разметку %s...", gi.name().c_str());
	grid_buf_idx = 0;
	for (uint32_t n = 0; need_req; n++){
		flag = false;
		ret = (n == 0) ? sendGridRequest(gi) : sendGridAutoRequest();
		if (ret == TC_OK){
			gridReqWait.wait();
			if (isTermcoreState(TS_READY) && (grid_buf_idx > 0)){
				if (grid_auto_req_len == 0){
					log_info("Разметка %s получена полностью. Сохраняем в файл...",
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
		log_err("Ошибка загрузки разметки %s.", gi.name().c_str());
	return ret;
}

bool syncGrids(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove, list<GridInfo> &grids_failed,
	       X3SyncCallback_t cbk)
{
	if (!needGridsUpdate()){
		log_info("Обновление разметок не требуется.");
		return true;
	}
	bool ok = true;
	char txt[256];
	size_t n = 0;
	grid_sync_cbk = cbk;
	cbk(false, "Загрузка разметок из \"Экспресс-3\"");
	Sleep(100);
	grids_failed.clear();
	list<GridInfo> _grids_to_create, _grids_to_remove;
/* Сначала закачиваем новые разметки */
	for (const auto &p : grids_to_create){
		snprintf(txt, ASIZE(txt), "Загрузка разметки %s (%u из %u)",
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
			cbk(false, "Разметки успешно загружены");
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
	log_info("Чтение списка разметок из БПУ...");
	if (xprn->beginGridLst(xprnOpCallback)){
		xprn_wait.wait();
		if (xprn->statusOK()){
			xprn_grids.assign(xprn->grids().cbegin(), xprn->grids().cend());
			log_info("Найдено разметок в БПУ: %d.", xprn_grids.size());
			ret = true;
		}else
			log_err("Ошибка получения списка разметок из БПУ: %s.", XPrn::status_str(xprn->status()));
	}else
		log_err("Невозможно начать получение списка разметок из БПУ.");
	return ret;
}

/* Создание списка разметок БПУ для загрузки и удаления */
static void checkXPrnGrids(const list<GridInfo> &stored_grids, list<GridInfo> &grids_to_load,
	list<GridInfo> &grids_to_erase)
{
	grids_to_load.clear();
	grids_to_erase.clear();
/* Создаём список разметок в БПУ */
	list<GridInfo> xprn_grids;
	if (!findXPrnGrids(xprn_grids))
		return;
/* Ищем разметки для загрузки */
	log_dbg("Ищем разметки для загрузки...");
	for (const auto &p : stored_grids){
		auto p1 = find_if(xprn_grids, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 == xprn_grids.end()){
			log_dbg("Разметка %s #%d (%hc) отсутствует в БПУ и будет туда загружена.",
				p.name().c_str(), p.nr(), p.id());
			grids_to_load.push_back(p);
		}else if (p.isNewer(*p1)){
			log_dbg("Разметку %s #%d (%hc) необходимо обновить.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			grids_to_load.push_back(p);
			grids_to_erase.push_back(*p1);
		}else
			log_dbg("Разметка %s #%d (%hc) не требует обновления.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
	}
/* Ищем разметки для удаления */
	log_dbg("Ищем разметки для удаления...");
	for (const auto &p : xprn_grids){
		if (find_if(stored_grids, [p](const GridInfo &gi) {return gi.id() == p.id();}) == stored_grids.end()){
			log_dbg("Разметка %s #%d (%hc) помечена для удаления.", p.name().c_str(), p.nr(), p.id());
			grids_to_erase.push_back(p);
		}
	}
	log_dbg("Найдено разметок: для загрузки: %d; для удаления: %d.", grids_to_load.size(), grids_to_erase.size());
}

/* Запись заданной разметки в БПУ */
static bool writeGridToXPrn(const GridInfo &gi)
{
	if (xprn == NULL)
		return false;
	bool ret = false;
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), "%s\\%s.BMP", grid_folder, gi.name().c_str());
	log_info("Загружаем в БПУ разметку %s...", path);
	if (xprn->beginGridLoad(path, gi.nr(), xprnOpCallback)){
		xprn_wait.wait();
		ret = xprn->statusOK();
		if (ret)
			log_info("Разметка %s успешно загружена в БПУ.", path);
		else
			log_err("Ошибка загрузки разметки %s в БПУ: %s.", path, XPrn::status_str(xprn->status()));
	}else
		log_err("Невозможно начать загрузку разметки %s в БПУ: %s.", path, XPrn::status_str(xprn->status()));
	return ret;
}

/* Удаление заданной разметки из БПУ */
static bool eraseGridFromXPrn(const GridInfo &gi)
{
	if (xprn == NULL)
		return false;
	bool ret = false;
	log_info("Удаляем из БПУ разметки %s...", gi.name().c_str());
	if (xprn->beginGridErase(gi.nr(), xprnOpCallback)){
		xprn_wait.wait();
		ret = xprn->statusOK();
		if (ret)
			log_info("Разметка %s успешно удалена из БПУ.", gi.name().c_str());
		else
			log_err("Ошибка удаления разметки %s из БПУ: %s.", gi.name().c_str(),
				XPrn::status_str(xprn->status()));
	}else
		log_err("Невозможно начать удаление разметки %s из БПУ: %s.", gi.name().c_str(),
			XPrn::status_str(xprn->status()));
	return ret;
}

/* Обновление разметок в БПУ "Видеотон" */
static bool updateGridsVtsv(const list<GridInfo> &grids_to_load, const list<GridInfo> &grids_to_erase,
	list<GridInfo> &grids_failed, InitializationNotify_t init_notify)
{
//	bool ret = true;
	static char txt[64];
	size_t n = 1;
/* Сначала удаляем ненужные разметки из БПУ */
	for (const auto &p : grids_to_erase){
		snprintf(txt, ASIZE(txt), "Удаление из БПУ разметки %s (%u из %u)",
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
		snprintf(txt, ASIZE(txt), "Загрузка в БПУ разметки %s (%u из %u)",
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
	log_info("Проверяем записанные разметки...");
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
			log_dbg("Разметка %s #%d (%hc) отсутствует в БПУ после загрузки.",
				p.name().c_str(), p.nr(), p.id());
			grids_failed.push_back(p);
			ok = false;
		}else if (p.isNewer(*p1)){
			log_dbg("Разметка %s #%d (%hc) не была удалена из БПУ.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			grids_failed.push_back(p);
			ok = false;
		}
	}
	if (ok)
		log_info("Разметки в БПУ успешно обновлены.");
	return true; /*ret;*/
}

/* Обновление разметок в БПУ "СПЕКТР-ДС" */
static bool updateGridsSPrn(const list<GridInfo> &stored_grids, list<GridInfo> &grids_failed,
	InitializationNotify_t init_notify)
{
/* Сначала удаляем все разметки из БПУ */
	log_info("Удаляем все имеющиеся разметки из БПУ...");
	init_notify(false, "Удаление разметок из БПУ");
	if (!xprn->beginGridEraseAll(xprnOpCallback)){
		log_err("Невозможно начать удаление разметок из БПУ: %s.", XPrn::status_str(xprn->status()));
		return false;
	}
	xprn_wait.wait();
	if (!xprn->statusOK()){
		log_err("Ошибка удаления разметок из БПУ: %s.", XPrn::status_str(xprn->status()));
		return false;
	}
//	bool ret = true;
/* Затем загружаем новые разметки */
	size_t n = 1;
	for (const auto &p : stored_grids){
		static char txt[64];
		snprintf(txt, ASIZE(txt), "Загрузка в БПУ разметки %s (%u из %u)",
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
		log_err("БПУ отсутствует.");
		return false;
	}
	bool ret = false;
	init_notify(false, "Обновление разметок в БПУ");
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
		static char txt[256];
		uint8_t status = xprn->status();
		if (xprn->statusOK(status))
			snprintf(txt, ASIZE(txt), "Ошибка обновления разметок в БПУ.");
		else
			snprintf(txt, ASIZE(txt), "Ошибка обновления разметок в БПУ: %s.",
				xprn->status_str(status));
		termcore_callbacks.callMessageBox("ОБНОВЛЕНИЕ РАЗМЕТОК", txt);
	}
	return ret;
}

/* Получение списка разметок из ККТ */
static bool findKktGrids(list<GridInfo> &kkt_grids)
{
	bool ret = false;
	kkt_grids.clear();
	log_info("Чтение списка разметок из ККТ...");
	if (kkt->getGridLst()){
		KktRespGridLst *rsp = dynamic_cast<KktRespGridLst *>(kkt->rsp());
		if ((rsp != NULL) && rsp->statusOk()){
			kkt_grids.assign(rsp->grids().cbegin(), rsp->grids().cend());
			log_info("Найдено разметок в ККТ: %d.", kkt_grids.size());
			ret = true;
		}else
			log_err("Ошибка получения списка разметок из ККТ: 0x%.2hx.", (uint16_t)kkt->status());
	}else
		log_err("Невозможно начать получение списка разметок из ККТ: 0x%.2hx.", (uint16_t)kkt->status());
	return ret;
}

/* Создание списка разметок ККТ для загрузки и удаления */
static void checkKktGrids(const list<GridInfo> &stored_grids, list<GridInfo> &grids_to_load,
	list<GridInfo> &grids_to_erase)
{
	grids_to_load.clear();
	grids_to_erase.clear();
/* Создаём список разметок в ККТ */
	list<GridInfo> kkt_grids;
	if (!findKktGrids(kkt_grids))
		return;
/* Ищем разметки для загрузки */
	log_dbg("Ищем разметки для загрузки...");
	for (const auto &p : stored_grids){
		auto p1 = find_if(kkt_grids, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 == kkt_grids.end()){
			log_dbg("Разметка %s #%d (%hc) отсутствует в ККТ и будет туда загружена.",
				p.name().c_str(), p.nr(), p.id());
			grids_to_load.push_back(p);
		}else if (p.isNewer(*p1)){
			log_dbg("Разметку %s #%d (%hc) необходимо обновить.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			grids_to_load.push_back(p);
			grids_to_erase.push_back(*p1);
		}else
			log_dbg("Разметка %s #%d (%hc) не требует обновления.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
	}
/* Ищем разметки для удаления */
	log_dbg("Ищем разметки для удаления...");
	for (const auto &p : kkt_grids){
		if (find_if(stored_grids, [p](const GridInfo &gi) {return gi.id() == p.id();}) == stored_grids.end()){
			log_dbg("Разметка %s #%d (%hc) помечена для удаления.", p.name().c_str(), p.nr(), p.id());
			grids_to_erase.push_back(p);
		}
	}
	log_dbg("Найдено разметок: для загрузки: %d; для удаления: %d.", grids_to_load.size(), grids_to_erase.size());
}

/* Запись заданной разметки в ККТ */
static bool writeGridToKkt(const GridInfo &gi)
{
	if (kkt == NULL)
		return false;
	bool ret = false;
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), "%s\\%s.BMP", grid_folder, gi.name().c_str());
	log_info("Загружаем в ККТ разметку %s...", path);
	if (kkt->loadGrid(path, gi.nr())){
		KktRespGridLoad *rsp = dynamic_cast<KktRespGridLoad *>(kkt->rsp());
		if ((rsp != NULL) && rsp->statusOk()){
			ret = true;
			log_info("Разметка %s успешно загружена в ККТ.", path);
		}else
			log_err("Ошибка загрузки разметки %s в ККТ: 0x%.2hx.", path, (uint16_t)kkt->status());
	}else
		log_err("Невозможно начать загрузку разметки %s в ККТ: 0x%.2hx.", path, (uint16_t)kkt->status());
	return ret;
}

/* Обновление разметок в ККТ "СПЕКТР-Ф" */
static bool updateGridsKkt(const list<GridInfo> &stored_grids, list<GridInfo> &grids_failed,
	InitializationNotify_t init_notify)
{
/* Сначала удаляем все разметки из ККТ */
	log_info("Удаляем все имеющиеся разметки из ККТ...");
	init_notify(false, "Удаление разметок из ККТ");
	if (kkt->eraseAllGrids()){
		KktRespGridEraseAll *rsp = dynamic_cast<KktRespGridEraseAll *>(kkt->rsp());
		if ((rsp != NULL) && rsp->statusOk())
			log_info("Разметки удалены из ККТ.");
		else{
			log_err("Ошибка удаления разметок из ККТ: 0x%.2hx.", (uint16_t)kkt->status());
			return false;
		}
	}else{
		log_err("Невозможно начать удаление разметок из ККТ: 0x%.2hx.", (uint16_t)kkt->status());
		return false;
	}
/* Затем загружаем новые разметки */
	size_t n = 1;
	kkt->beginBatchMode();
	for (const auto &p : stored_grids){
		static char txt[64];
		snprintf(txt, ASIZE(txt), "Загрузка в ККТ разметки %s (%u из %u)",
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
	log_info("kkt = %p.", kkt);
	if (kkt == NULL)
		return false;
	bool ret = false;
	init_notify(false, "Обновление разметок в ККТ");
	Sleep(100);
	list<GridInfo> stored_grids, grids_to_load, grids_to_erase;
	findStoredGridsKkt(stored_grids);
	checkKktGrids(stored_grids, grids_to_load, grids_to_erase);
	if (!grids_to_load.empty() || !grids_to_erase.empty())
		ret = updateGridsKkt(stored_grids, grids_failed_kkt, init_notify);
	else
		ret = true;
	if (!ret && !isTermcoreState(TS_INIT)){
		static char txt[256];
		snprintf(txt, ASIZE(txt), "Ошибка обновления разметок в ККТ: 0x%.2hx.",
			(uint16_t)kkt->status());
		termcore_callbacks.callMessageBox("ОБНОВЛЕНИЕ РАЗМЕТОК", txt);
	}
	return ret;
}
