/* Работа с таблицами XSLT. (c) gsr, 2023, 2024 */

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <zlib.h>
#include "x3data/common.hpp"
#include "x3data/xslts.h"
#include "x3data/xslts.hpp"
#include "gui/scr.h"
#include "express.h"
#include "paths.h"
#include "sterm.h"
#include "termlog.h"
#include "transport.h"
#include "xbase64.h"

bool XSLTInfo::isIdxValid(const char *idx)
{
	return (idx != NULL) && (strlen(idx) == 2) &&
		(((idx[0] >= '0') && (idx[0] <= '9')) || ((idx[0] >= 'A') && (idx[0] <= 'Z'))) &&
		(((idx[1] >= '0') && (idx[1] <= '9')) || ((idx[1] >= 'A') && (idx[1] <= 'Z')));
}

bool XSLTInfo::parse(const char *name)
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
	static const char *xslt_ext = ".XSL";
	char pr, idx[3], ext[5], ch;
	int y, d;
	if (sscanf(nm, "%c%2s%2d%3d%4s%c", &pr, idx, &y, &d, ext, &ch) == 5){
		pr = toupper(pr);
		if ((pr == 'T') && isIdxValid(idx) &&
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
	if (sscanf((const char *)data, "<%c%2s%2d%3d%c", &pr, idx, &y, &d, &ch) == 5){
		pr = toupper(pr);
		if ((pr == 'T') && (ch == '>') && isIdxValid(idx) && (y > 14) && (d > 0) && (d < 367)){
			snprintf(nm, ASIZE(nm), "%c%.2s%.2d%.3d", pr, idx, y, d);
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
		_name.assign(nm);
	}
	return ret;
}

static list<XSLTInfo> xslt_to_create;
static list<XSLTInfo>::const_iterator xslt_to_create_ptr = xslt_to_create.cbegin();
static list<XSLTInfo> xslt_to_remove;
static list<XSLTInfo> xslt_failed;

bool need_xslt_update()
{
	return !xslt_to_create.empty() || !xslt_to_remove.empty();
}

static regex_t reg;

static int xslt_selector(const struct dirent *entry)
{
	return regexec(&reg, entry->d_name, 0, NULL, 0) == REG_NOERROR;
}

/*
 * Создание списков таблиц XSLT для закачки/удаления на основании списка
 * из "Экспресс" и файлов на диске.
 */
static void check_stored_xslt(const list<XSLTInfo> &x3_xslt)
{
	xslt_to_create.clear();
	xslt_to_remove.clear();
/* Создаём список таблиц XSLT, хранящихся на диске */
	log_dbg("Ищем таблицы XSLT в каталоге " XSLT_FOLDER "...");
	int rc = regcomp(&reg, "^T[0-9A-Z]{2}[0-9]{5}\\.[Xx][Ss][Ll]$", REG_EXTENDED | REG_NOSUB);
	if (rc != REG_NOERROR){
		log_err("Ошибка компиляции регулярного выражения: %d.", rc);
		return;
	}
	list<XSLTInfo> stored_xslt;
	struct dirent **names;
	int n = scandir(XSLT_FOLDER, &names, xslt_selector, alphasort);
	if (n == -1){
		log_sys_err("Ошибка поиска таблиц XSLT в каталоге " XSLT_FOLDER ":");
		regfree(&reg);
		return;
	}
	for (int i = 0; i < n; i++){
		static char path[PATH_MAX];
		snprintf(path, ASIZE(path), XSLT_FOLDER "/%s", names[i]->d_name);
		XSLTInfo xi;
		if (xi.parse(names[i]->d_name)){
			log_dbg("Обнаружена таблица XSLT %s #%s.", xi.name().c_str(), xi.idx());
			stored_xslt.push_back(xi);
		}
	}
	free(names);
	regfree(&reg);
	log_dbg("Найдено таблиц XSLT: %d.", stored_xslt.size());
/* Ищем таблицы XSLT для закачки */
	log_dbg("Ищем таблицы XSLT для закачки...");
	for (const auto &p : x3_xslt){
		bool found = false;
		for (const auto &p1 : stored_xslt){
			if (strcmp(p1.idx(), p.idx()) != 0)
				continue;
			else if (p.isNewer(p1)){
				log_dbg("Таблица XSLT %s #%s старее экспрессовской и будет удалена.",
					p1.name().c_str(), p1.idx());
				xslt_to_remove.push_back(p1);
			}else if (p1.isNewer(p)){
				log_dbg("Таблица XSLT %s #%s новее экспрессовской и будет удалена.",
					p1.name().c_str(), p1.idx());
				xslt_to_remove.push_back(p1);
			}else
				found = true;
		}
		if (found)
			log_dbg("Таблица XSLT %s #%s не требует обновления.", p.name().c_str(), p.idx());
		else{
			log_dbg("Таблица XSLT %s #%s отсутствует в терминале и будет загружена из \"Экспресс-3\").",
				p.name().c_str(), p.idx());
			xslt_to_create.push_back(p);
		}
	}
	log_dbg("Найдено таблиц XSLT: для закачки: %d; для удаления: %d.", xslt_to_create.size(), xslt_to_remove.size());
}

/* Поиск в абзаце ответа идентификаторов таблиц XSLT */
static bool check_x3_xslt(const uint8_t *data, size_t len, list<XSLTInfo> &x3_xslt)
{
	if ((data == NULL) || (len == 0))
		return false;
	x3_xslt.clear();
	for (size_t i = 0; (data[i] == 0x3c) && ((i + 1) < len); i++){
		const uint8_t *p = (const uint8_t *)memchr(data + i + 1, 0x3e, len - i - 1);
		if (p == NULL)
			break;
		XSLTInfo xi;
		if (xi.parse(data + i, len - 1)){
			log_dbg("Найдена таблица XSLT %s.", xi.name().c_str());
			x3_xslt.push_back(xi);
		}
		i += p - (data + i);
	}
	return !x3_xslt.empty();
}

bool check_x3_xslt(const uint8_t *data, size_t len)
{
	list<XSLTInfo> x3_xslt;
	bool ret = check_x3_xslt(data, len, x3_xslt);
	if (ret)
		check_stored_xslt(x3_xslt);
	return ret;
}

/* Максимальный размер буфера сжатой таблицы XSLT */
static const size_t MAX_COMPRESSED_XSLT_LEN = 65536;
/* Буфер для приёма сжатой таблицы XSLT */
static uint8_t xslt_buf[MAX_COMPRESSED_XSLT_LEN];
/* Текущий размер принятых данных сжатой таблицы XSLT */
static size_t xslt_buf_idx = 0;

/* Автозапрос для получения таблиц XSLT по частям */
static uint8_t xslt_auto_req[REQ_BUF_LEN];
/* Длина автозапроса */
static size_t xslt_auto_req_len = 0;

static x3_sync_callback_t xslt_sync_cbk = NULL;

/* Отправка начального запроса на получение таблицы XSLT */
static void send_xslt_request(const XSLTInfo &xslt)
{
	log_dbg("name = %s.", xslt.name().c_str());
	int offs = get_req_offset();
	req_len = offs;
	req_len += snprintf((char *)req_buf + req_len, ASIZE(req_buf) - req_len,
		"P55TR(EXP.DATA.MAKET   %-8s)000000000", xslt.name().c_str());
	set_scr_request(req_buf + offs, req_len - offs, true);
	wrap_text();
}

/* Отправка автозапроса для получения таблицы XSLT */
static void send_xslt_auto_request()
{
	req_len = get_req_offset();
	memcpy(req_buf + req_len, xslt_auto_req, xslt_auto_req_len);	/* FIXME */
	set_scr_request(xslt_auto_req, xslt_auto_req_len, true);
	req_len += xslt_auto_req_len;
	wrap_text();
}

/* Декодирование таблицы XSLT, распаковка и сохранение в файле на диске */
static bool store_xslt(const XSLTInfo &xi)
{
	log_info("path = %s; idx = %s.", xi.name().c_str(), xi.idx());
	if (xslt_buf_idx == 0){
		log_err("Буфер данных пуст.");
		return false;
	}
	size_t len = 0;
	if (!xbase64_decode(xslt_buf, xslt_buf_idx, xslt_buf, xslt_buf_idx, &len) || (len == 0)){
		log_err("Ошибка декодирования данных.");
		return false;
	}else
		log_info("Данные таблицы XSLT декодированы; длина после декодирования %zu байт.", len);
	scoped_ptr<uint8_t> xslt_data(new uint8_t[MAX_COMPRESSED_XSLT_LEN]);	/* FIXME */
	size_t xslt_data_len = MAX_COMPRESSED_XSLT_LEN;
	int rc = uncompress(xslt_data.get(), &xslt_data_len, xslt_buf, len);
	if (rc == Z_OK)
		log_info("Данные таблицы XSLT распакованы; длина данных после распаковки %u байт.", xslt_data_len);
	else{
		log_err("Ошибка распаковки таблицы XSLT (%d).", rc);
		return false;
	}
	char path[PATH_MAX];
	snprintf(path, ASIZE(path), XSLT_FOLDER "/%s.XSL", xi.name().c_str());
	int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (fd == -1){
		log_sys_err("Ошибка создания файла %s:", path);
		return false;
	}
	bool ret = false;
	rc = write(fd, xslt_data.get(), xslt_data_len);
	if (rc == xslt_data_len){
		log_info("Таблица XSLT успешно записана в файл %s.", path);
		ret = true;
	}else if (rc == -1)
		log_sys_err("Ошибка записи в файл %s:", path);
	else
		log_err("В %s вместо %u записано %d байт.", path, xslt_data_len, rc);
	close(fd);
	if (!ret)
		unlink(path);

	return ret;
}

/* Удаление таблицы XSLT с диска */
static bool remove_xslt(const XSLTInfo &xi)
{
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), XSLT_FOLDER "/%s.XSL", xi.name().c_str());
	log_info("Удаляем таблицу XSLT %s...", path);
	bool ret = unlink(path) == 0;
	if (ret)
		log_info("Таблица %s успешно удалена.", path);
	else
		log_sys_err("Ошибка удаления таблицы XSLT %s:", path);
	return ret;
}

/* Вызывается после получения ответа на запрос таблицы XSLT */
void on_response_xslt(void)
{
	xslt_auto_req_len = 0;
	static char err_msg[1024];
	err_msg[0] = 0;
/* 0 -- данные XSLT; 1 -- другой ответ, обработка не требуется; 2 -- другой ответ, требуется обработка */
	int non_xslt_resp = 0;
	set_term_state(st_resp);
	int xslt_para = -1, req_para = -1;
	size_t xslt_len = 0, req_len = 0;
	if (find_pic_data(&xslt_para, &req_para) && (xslt_para != -1)){
		xslt_len = handle_para(xslt_para);
		log_info("Обнаружены данные XSLT (абзац #%d; %zd байт).",
			xslt_para + 1, xslt_len);
		if (xslt_len > (ASIZE(xslt_buf) - xslt_buf_idx)){
			snprintf(err_msg, ASIZE(err_msg), "Переполнение буфера данных XSLT.");
			non_xslt_resp = 1;
		}else if (xslt_len > 0){
			memcpy(xslt_buf + xslt_buf_idx, text_buf, xslt_len);
			xslt_buf_idx += xslt_len;
			if (req_para != -1){
				req_len = handle_para(req_para);
				if (req_len > 0){
					log_info("Обнаружен автозапрос (абзац %d; %zd байт).",
						req_para + 1, req_len);
					memcpy(xslt_auto_req, text_buf, req_len);
					xslt_auto_req_len = req_len;
					send_xslt_auto_request();
				}
			}else{
				log_info("Таблица XSLT %s получена полностью. Сохраняем в файл...",
					xslt_to_create_ptr->name().c_str());
				store_xslt(*xslt_to_create_ptr++);
				xslt_buf_idx = 0;
				if (xslt_to_create_ptr == xslt_to_create.cend())
					log_info("Загрузка таблиц XSLT завершена.");
				else
					send_xslt_request(*xslt_to_create_ptr);
			}
		}else{
			snprintf(err_msg, ASIZE(err_msg), "Получены данные таблицы XSLT нулевой длины.");
			non_xslt_resp = 1;
		}
	}else{
		snprintf(err_msg, ASIZE(err_msg), "Не найдены данные таблицы XSLT.");
		non_xslt_resp = 2;
	}
	if (err_msg[0] != 0){
		log_err(err_msg);
		xslt_buf_idx = 0;
	}
	if (non_xslt_resp){
		if (xslt_sync_cbk != NULL)
			xslt_sync_cbk(true, NULL);
		if (non_xslt_resp == 2)
			execute_resp();
	}
}

/* Начало синхронизации таблиц XSLT с "Экспресс" */
static bool sync_xslt(list<XSLTInfo> &xslt_to_create, list<XSLTInfo> &xslt_to_remove, list<XSLTInfo> &xslt_failed,
       x3_sync_callback_t cbk)
{
	if (!need_xslt_update()){
		log_info("Обновление таблиц XSLT не требуется.");
		return true;
	}
	bool ok = true;
	char txt[256];
	size_t n = 0;
	xslt_sync_cbk = cbk;
	if (cbk != NULL)
		cbk(false, "Загрузка таблиц XSLT из \"Экспресс-3\"");
	xslt_failed.clear();
	list<XSLTInfo> _xslt_to_create, _xslt_to_remove;
/* Сначала удаляем старые таблицы XSLT */
	for (const auto &p : xslt_to_remove){
		if (remove_xslt(p))
			n++;
		else
			_xslt_to_remove.push_back(p);
	}
	xslt_to_remove.assign(_xslt_to_remove.cbegin(), _xslt_to_remove.cend());
/* Затем начинаем закачку новых */
	for (const auto &p : xslt_to_create){
		snprintf(txt, ASIZE(txt), "Загрузка таблицы XSLT %s (%zu из %zu)",
			p.name().c_str(), (n + 1), xslt_to_create.size());
		log_dbg(txt);
		send_xslt_request(p);
		break;
	}
	xslt_sync_cbk = NULL;
	return ok;
}

bool sync_xslt(x3_sync_callback_t cbk)
{
	log_dbg("");
	bool ret = true;
	if (need_xslt_update()){
		req_type = req_xslt;
		xslt_to_create_ptr = xslt_to_create.cbegin();
		ret = sync_xslt(xslt_to_create, xslt_to_remove, xslt_failed, cbk);
	}
	return ret;
}
