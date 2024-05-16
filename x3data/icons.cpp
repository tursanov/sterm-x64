/* Работа с пиктограммами БПУ/ККТ. (c) gsr, 2015-2016, 2019, 2022, 2024 */

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <zlib.h>
#include "gui/scr.h"
#include "kkt/cmd.h"
#include "x3data/bmp.h"
#include "x3data/icons.h"
#include "x3data/icons.hpp"
#include "express.h"
#include "paths.h"
#include "sterm.h"
#include "termlog.h"
#include "transport.h"
#include "xbase64.h"

bool IconInfo::parse(const char *name)
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
	static const char *bmp_ext = ".BMP";
	int nr;
	char ext[5], pr, ch;
	int y, d;
	if (sscanf(nm, "%c%2d%2d%3d%4s%c", &pr, &nr, &y, &d, ext, &ch) == 5){
		pr = toupper(pr);
		if (((pr == 'Y') || (pr == 'Z')) && isNrValid(nr) &&
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
	if (ret){
		_nr = nr;
		_id = nrToId(nr);
		_name.assign(nm, len - 4);
	}
	delete [] nm;
	return ret;
}

bool IconInfo::parse(const uint8_t *data, size_t len)
{
	if ((data == NULL) || (len == 0))
		return false;
	bool ret = false;
	int nr = 0;
	char nm[SPRN_MAX_ICON_NAME_LEN + 1];
	char pr, ch;
	int y, d;
	if (sscanf((const char *)data, "<%c%2d%2d%3d%c", &pr, &nr, &y, &d, &ch) == 5){
		pr &= ~0x20;
		if (((pr == 'Y') || (pr == 'Z')) && (ch == '>') && isNrValid(nr) && (y > 14) && (d > 0) && (d < 367)){
			snprintf(nm, ASIZE(nm), "%c%.2d%.2d%.3d", pr, nr, y, d);
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
	if (ret){
		_nr = nr;
		_id = nrToId(nr);
		_name.assign(nm);
	}
	return ret;
}

static list<IconInfo> icons_to_create_xprn;
static list<IconInfo>::const_iterator icons_to_create_xprn_ptr = icons_to_create_xprn.cbegin();
static list<IconInfo> icons_to_remove_xprn;
static list<IconInfo> icons_failed_xprn;

static list<IconInfo> icons_to_create_kkt;
static list<IconInfo>::const_iterator icons_to_create_kkt_ptr = icons_to_create_kkt.cbegin();
static list<IconInfo> icons_to_remove_kkt;
static list<IconInfo> icons_failed_kkt;

static void clr_icon_lists(list<IconInfo> &icons_to_create, list<IconInfo> &icons_to_remove)
{
	icons_to_create.clear();
	icons_to_remove.clear();
}

static inline void clr_icon_lists_xprn()
{
	clr_icon_lists(icons_to_create_xprn, icons_to_remove_xprn);
	icons_to_create_xprn_ptr = icons_to_create_xprn.cbegin();
}

static inline void clr_icon_lists_kkt()
{
	clr_icon_lists(icons_to_create_kkt, icons_to_remove_kkt);
	icons_to_create_kkt_ptr = icons_to_create_kkt.cbegin();
}

static inline void clr_icon_lists()
{
	clr_icon_lists_xprn();
	clr_icon_lists_kkt();
}

bool need_icons_update_xprn()
{
	return !icons_to_create_xprn.empty() || !icons_to_remove_xprn.empty();
}

bool need_icons_update_kkt()
{
	return !icons_to_create_kkt.empty() || !icons_to_remove_kkt.empty();
}

static regex_t reg;

static int icon_selector(const struct dirent *entry)
{
	log_dbg("d_name = '%s'.", entry->d_name);
	return regexec(&reg, entry->d_name, 0, NULL, 0) == REG_NOERROR;
}

/* Поиск на диске пиктограмм по регулярному выражению */
static bool find_stored_icons(list<IconInfo> &stored_icons, const char *pattern)
{
	if (pattern == NULL)
		return false;
	int rc = regcomp(&reg, pattern, REG_EXTENDED | REG_NOSUB);
	if (rc != REG_NOERROR){
		log_err("Ошибка компиляции регулярного выражения для '%s': %d.", pattern, rc);
		return false;
	}
	struct dirent **names;
	int n = scandir(ICONS_FOLDER, &names, icon_selector, alphasort);
	if (n == -1){
		log_sys_err("Ошибка поиска пиктограмм в каталоге " ICONS_FOLDER ":");
		regfree(&reg);
		return false;
	}
	bool ret = false;
	for (int i = 0; i < n; i++){
		static char path[PATH_MAX];
		snprintf(path, ASIZE(path), ICONS_FOLDER "/%s", names[i]->d_name);
		IconInfo gi;
		if (gi.parse(names[i]->d_name)){
			log_dbg("Обнаружена пиктограмма %s #%d (%hc).", gi.name().c_str(), gi.nr(), gi.id());
			stored_icons.push_back(gi);
			ret = true;
		}
	}
	free(names);
	regfree(&reg);
	return ret;
}

/* Поиск на диске пиктограмм для БПУ */
static bool find_stored_icons_xprn(list<IconInfo> &stored_icons)
{
	stored_icons.clear();
	find_stored_icons(stored_icons, "^Z[0-9]{7}\\.[Bb][Mm][Pp]$");
	return !stored_icons.empty();
}

/* Поиск на диске пиктограмм для ККТ */
static bool find_stored_icons_kkt(list<IconInfo> &stored_icons)
{
	stored_icons.clear();
	find_stored_icons(stored_icons, "^Y[0-9]{7}\\.[Bb][Mm][Pp]$");
	return !stored_icons.empty();
}

/*
 * Создание списков пиктограмм для закачки/удаления на основании списка
 * из "Экспресс" и файлов на диске.
 */
static void check_stored_icons(const list<IconInfo> &x3_icons, list<IconInfo> &icons_to_create, list<IconInfo> &icons_to_remove,
	bool (*find_fn)(list<IconInfo> &))
{
	clr_icon_lists(icons_to_create, icons_to_remove);
/* Создаём список пиктограмм, хранящихся в ПАК РМК */
	log_dbg("Ищем пиктограммы в каталоге %s...", ICONS_FOLDER);
	list<IconInfo> stored_icons;
	find_fn(stored_icons);
	log_dbg("Найдено пиктограмм: %d.", stored_icons.size());
/* Ищем пиктограммы для закачки */
	log_dbg("Ищем пиктограммы для закачки...");
	for (const auto &p : x3_icons){
		bool found = false;
		for (const auto &p1 : stored_icons){
			if (p1.id() != p.id())
				continue;
			else if (p.isNewer(p1)){
				log_dbg("Пиктограмма %s #%d (%hc) старее экспрессовской и будет удалена.",
					p1.name().c_str(), p1.nr(), p1.id());
				icons_to_remove.push_back(p1);
			}else if (p1.isNewer(p)){
				log_dbg("Пиктограмма %s #%d (%hc) новее экспрессовской и будет удална.",
					p1.name().c_str(), p1.nr(), p1.id());
				icons_to_remove.push_back(p1);
			}else
				found = true;
		}
		if (found)
			log_dbg("Пиктограмма %s #%d (%hc) не требует обновления.", p.name().c_str(), p.nr(), p.id());
		else{
			log_dbg("Пиктограмма %s #%d (%hc) отсутствует в ПАК РМК и будет загружена из \"Экспресс-3\").",
				p.name().c_str(), p.nr(), p.id());
			icons_to_create.push_back(p);
		}
	}
/* Ищем пиктограммы для удаления */
/*	log_dbg("Ищем пиктограммы для удаления...");
	for (const auto &p : stored_icons){
		if (find_if(x3_icons, [p](const IconInfo &gi) {return gi.id() == p.id();}) == x3_icons.end()){
			log_dbg("Пиктограмма %s #%d (%hc) помечена для удаления.", p.name().c_str(), p.nr(), p.id());
			icons_to_remove.push_back(p);
		}
	}*/
	log_dbg("Найдено пиктограмм: для закачки: %d; для удаления: %d.", icons_to_create.size(), icons_to_remove.size());
}

/* Создание списка пиктограмм для БПУ */
static inline void check_stored_icons_xprn(const list<IconInfo> &x3_icons)
{
	check_stored_icons(x3_icons, icons_to_create_xprn, icons_to_remove_xprn, find_stored_icons_xprn);
}

/* Создание списка пиктограмм для ККТ */
static inline void check_stored_icons_kkt(const list<IconInfo> &x3_icons)
{
	check_stored_icons(x3_icons, icons_to_create_kkt, icons_to_remove_kkt, find_stored_icons_kkt);
}

/* Поиск в абзаце ответа идентификаторов пиктограмм для БПУ/ККТ */
static bool check_x3_icons(const uint8_t *data, size_t len, list<IconInfo> &x3_icons_xprn, list<IconInfo> &x3_icons_kkt)
{
	log_dbg("data = %p; len = %zu.", data, len);
	if ((data == NULL) || (len == 0))
		return false;
	x3_icons_xprn.clear();
	x3_icons_kkt.clear();
	for (size_t i = 0; (data[i] == 0x3c) && ((i + 1) < len); i++){
		const uint8_t *p = (uint8_t *)memchr(data + i + 1, 0x3e, len - i - 1);
		if (p == NULL)
			break;
		IconInfo gi;
		if (gi.parse(data + i, len - 1)){
			if (gi.prefix() == "Z"){
				log_dbg("Найдена пиктограмма для БПУ: %s.", gi.name().c_str());
				x3_icons_xprn.push_back(gi);
			}else if (gi.prefix() == "Y"){
				log_dbg("Найдена пиктограмма для ККТ: %s.", gi.name().c_str());
				x3_icons_kkt.push_back(gi);
			}
		}
		i += p - (data + i);
	}
	return !x3_icons_xprn.empty() || !x3_icons_kkt.empty();
}

bool check_x3_icons(const uint8_t *data, size_t len)
{
	list<IconInfo> x3_icons_xprn, x3_icons_kkt;
	bool ret = check_x3_icons(data, len, x3_icons_xprn, x3_icons_kkt);
	if (ret){
		check_stored_icons_xprn(x3_icons_xprn);
		check_stored_icons_kkt(x3_icons_kkt);
	}
	return ret;
}

/* Максимальный размер буфера сжатой пиктограммы */
static const size_t MAX_COMPRESSED_ICON_LEN = 65536;
/* Буфер для приёма сжатой пиктограммы */
static uint8_t icon_buf[MAX_COMPRESSED_ICON_LEN];
/* Текущий размер принятых данных сжатой пиктограммы */
static size_t icon_buf_idx = 0;

/* Автозапрос для получения пиктограммы по частям */
static uint8_t icon_auto_req[REQ_BUF_LEN];
/* Длина автозапроса */
static size_t icon_auto_req_len = 0;

static x3_sync_callback_t icon_sync_cbk = NULL;

/* Отправка начального запроса на получение пиктограммы */
static void send_icon_request(const IconInfo &icon)
{
	log_dbg("name = %s.", icon.name().c_str());
	int offs = get_req_offset();
	req_len = offs;
	req_len += snprintf((char *)req_buf + req_len, ASIZE(req_buf) - req_len,
		"P55TR(EXP.DATA.MAKET   %-8s)000000000", icon.name().c_str());
	set_scr_request(req_buf + offs, req_len - offs, true);
	wrap_text();
}

/* Отправка автозапроса для получения пиктограммы */
static void send_icon_auto_request()
{
	req_len = get_req_offset();
	memcpy(req_buf + req_len, icon_auto_req, icon_auto_req_len);	/* FIXME */
	set_scr_request(icon_auto_req, icon_auto_req_len, true);
	req_len += icon_auto_req_len;
	wrap_text();
}

/* Декодирование пиктограммы, распаковка и сохранение в файле на диске */
static bool store_icon(const IconInfo &gi)
{
	log_info("path = %s; id = %hc.", gi.name().c_str(), gi.id());
	if (icon_buf_idx == 0){
		log_err("Буфер данных пуст.");
		return false;
	}
	size_t len = 0;
	if (!xbase64_decode(icon_buf, icon_buf_idx, icon_buf, icon_buf_idx, &len) || (len == 0)){
		log_err("Ошибка декодирования данных.");
		return false;
	}
	BITMAPFILEHEADER bmp_hdr;
	size_t l = sizeof(bmp_hdr);
	int rc = uncompress((uint8_t *)&bmp_hdr, &l, icon_buf, len);
	if ((rc != Z_OK) && (rc != Z_BUF_ERROR)){
		log_err("Ошибка распаковки (%d).", rc);
		return false;
	}else if ((bmp_hdr.bfType != 0x4d42) || (bmp_hdr.bfSize == 0)){
		log_err("Неверный формат заголовка BMP (bfType = 0x%.hx; bfSize = %u; bfOffBits = %u).",
			bmp_hdr.bfType, bmp_hdr.bfSize, bmp_hdr.bfOffBits);
		return false;
	}
	size_t bmp_len = bmp_hdr.bfSize;
	if (bmp_len > 262144){
		log_err("Размер файла BMP (%u) превышает максимально допустимый.", bmp_len);
		return false;
	}
	scoped_ptr<uint8_t> bmp_data(new uint8_t[bmp_len]);
	rc = uncompress(bmp_data.get(), &bmp_len, icon_buf, len);
	if (rc != Z_OK){
		log_err("Ошибка распаковки BMP (%d).", rc);
		return false;
	}
	char path[PATH_MAX];
	snprintf(path, ASIZE(path), ICONS_FOLDER "/%s.BMP", gi.name().c_str());
	int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (fd == -1){
		log_sys_err("Ошибка создания файла %s:", path);
		return false;
	}
	bool ret = false;
	rc = write(fd, bmp_data.get(), bmp_len);
	if (rc == bmp_len){
		log_info("Пиктограмма успешно записана в файл %s.", path);
		ret = true;
	}else if (rc == -1)
		log_sys_err("Ошибка записи в файл %s:", path);
	else
		log_err("В %s вместо %u записано %d байт.", path, bmp_len, rc);
	close(fd);
	if (!ret)
		unlink(path);
	return ret;
}

/* Удаление пиктограммы с диска */
static bool remove_icon(const IconInfo &gi)
{
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), ICONS_FOLDER "/%s.BMP", gi.name().c_str());
	log_info("Удаляем пиктограмму %s...", path);
	bool ret = unlink(path) == 0;
	if (ret)
		log_info("Пиктограмма %s успешно удалена.", path);
	else
		log_sys_err("Ошибка удаления пиктограммы %s:", path);
	return ret;
}

/* Вызывается после получения ответа на запрос пиктограммы БПУ/ККТ */
void on_response_icon(void)
{
	icon_auto_req_len = 0;
	static char err_msg[1024];
	err_msg[0] = 0;
/* 0 -- данные пиктограммы; 1 -- другой ответ, обработка не требуется; 2 -- другой ответ, требуется обработка */
	int non_icon_resp = 0;
	set_term_state(st_resp);
	int icon_para = -1, req_para = -1;
	size_t icon_len = 0, req_len = 0;
	if (find_pic_data(&icon_para, &req_para) && (icon_para != -1)){
		icon_len = handle_para(icon_para);
		log_info("Обнаружены данные пиктограммы (абзац #%d; %zd байт).",
		icon_para + 1, icon_len);
		if (icon_len > (ASIZE(icon_buf) - icon_buf_idx)){
			snprintf(err_msg, ASIZE(err_msg), "Переполнение буфера данных пиктограммы.");
			non_icon_resp = 1;
		}else if (icon_len > 0){
			memcpy(icon_buf + icon_buf_idx, text_buf, icon_len);
			icon_buf_idx += icon_len;
			if (req_para != -1){
				req_len = handle_para(req_para);
				if (req_len > 0){
					log_info("Обнаружен автозапрос (абзац %d; %zd байт).",
						req_para + 1, req_len);
					memcpy(icon_auto_req, text_buf, req_len);
					icon_auto_req_len = req_len;
					send_icon_auto_request();
				}
			}else if (req_type == req_icon_xprn){
				log_info("Пиктограмма %s получена полностью. Сохраняем в файл...",
					icons_to_create_xprn_ptr->name().c_str());
				store_icon(*icons_to_create_xprn_ptr++);
				icon_buf_idx = 0;
				if (icons_to_create_xprn_ptr == icons_to_create_xprn.cend()){
					log_info("Загрузка пиктограмм для БПУ завершена.");
					sync_icons_kkt(NULL);
				}else
					send_icon_request(*icons_to_create_xprn_ptr);
			}else if (req_type == req_icon_kkt){
				log_info("Пиктограмма %s получена полностью. Сохраняем в файл...",
					icons_to_create_kkt_ptr->name().c_str());
				store_icon(*icons_to_create_kkt_ptr++);
				icon_buf_idx = 0;
				if (icons_to_create_kkt_ptr == icons_to_create_kkt.cend())
					log_info("Загрузка пиктограмм для ККТ завершена.");
				else
					send_icon_request(*icons_to_create_kkt_ptr);
			}
		}else{
			snprintf(err_msg, ASIZE(err_msg), "Получены данные пиктограммы нулевой длины.");
			non_icon_resp = 1;
		}
	}else{
		snprintf(err_msg, ASIZE(err_msg), "Не найдены данные пиктограммы.");
		non_icon_resp = 2;
	}
	if (err_msg[0] != 0){
		log_err(err_msg);
		icon_buf_idx = 0;
//		termcore_callbacks.callMessageBox("ОШИБКА", err_msg);
	}
	if (non_icon_resp){
		if (icon_sync_cbk != NULL)
			icon_sync_cbk(true, NULL);
		if (non_icon_resp == 2)
			execute_resp();
	}
}

/* Начало синхронизации пиктограмм с "Экспресс" */
static bool sync_icons(list<IconInfo> &icons_to_create, list<IconInfo> &icons_to_remove, list<IconInfo> &icons_failed,
       x3_sync_callback_t cbk)
{
	if (!need_icons_update()){
		log_info("Обновление пиктограмм не требуется.");
		return true;
	}
	bool ok = true;
	char txt[256];
	size_t n = 0;
	icon_sync_cbk = cbk;
	if (cbk != NULL)
		cbk(false, "Загрузка пиктограмм из \"Экспресс-3\"");
	icons_failed.clear();
	list<IconInfo> _icons_to_create, _icons_to_remove;
/* Сначала удаляем старые пиктограммы */
	for (const auto &p : icons_to_remove){
		if (remove_icon(p))
			n++;
		else
			_icons_to_remove.push_back(p);
	}
	icons_to_remove.assign(_icons_to_remove.cbegin(), _icons_to_remove.cend());
/* Затем закачиваем новые */
	for (const auto &p : icons_to_create){
		snprintf(txt, ASIZE(txt), "Загрузка пиктограммы %s (%zu из %zu)",
			p.name().c_str(), (n + 1), icons_to_create.size());
		if (cbk != NULL)
			cbk(false, txt);
		log_dbg(txt);
		send_icon_request(p);
		break;
/*		int rc = download_icon(p);
		if (rc == TC_OK)
			n++;
		else if (inquirer->first_req()){
			ok = false;
			break;
		}else{
			_icons_to_create.push_back(p);
			icons_failed.push_back(p);
		}*/
	}
/*	if (ok){
		icons_to_create.assign(_icons_to_create.cbegin(), _icons_to_create.cend());
	}
	if (ok && (n > 0))
		ok = update_xprn_icons(cbk) && update_kkt_icons(cbk);*/
	if (ok){
		if (cbk != NULL)
			cbk(false, "Пиктограммы успешно загружены");
	}/*else
		showXPrnPicSyncError(icons_failed_xprn, icons_failed_kkt, icons_failed_xprn, icons_failed_kkt);*/
	icon_sync_cbk = NULL;
	return ok;
}

bool sync_icons_xprn(x3_sync_callback_t cbk)
{
	log_dbg("");
	bool ret = true;
	if (need_icons_update_xprn()){
		req_type = req_icon_xprn;
		icons_to_create_xprn_ptr = icons_to_create_xprn.cbegin();
		ret = sync_icons(icons_to_create_xprn, icons_to_remove_xprn, icons_failed_xprn, cbk);
	}else
		ret = sync_icons_kkt(cbk);
	return ret;
}

bool sync_icons_kkt(x3_sync_callback_t cbk)
{
	bool ret = true;
	if (need_icons_update_kkt()){
		req_type = req_icon_kkt;
		icons_to_create_kkt_ptr = icons_to_create_kkt.cbegin();
		ret = sync_icons(icons_to_create_kkt, icons_to_remove_kkt, icons_failed_kkt, cbk);
	}
	return ret;
}

#if 0
/* Получение списка пиктограмм из БПУ */
static bool find_xprn_icons(list<IconInfo> &xprn_icons)
{
	bool ret = false;
	xprn_icons.clear();
	log_info("Чтение списка пиктограмм из БПУ...");
	if (xprn->beginGridLst(xprnOpCallback)){
		xprn_wait.wait();
		if (xprn->statusOK()){
			xprn_icons.assign(xprn->icons().cbegin(), xprn->icons().cend());
			log_info("Найдено пиктограмм в БПУ: %d.", xprn_icons.size());
			ret = true;
		}else
			log_err("Ошибка получения списка пиктограмм из БПУ: %s.", XPrn::status_str(xprn->status()));
	}else
		log_err("Невозможно начать получение списка пиктограмм из БПУ.");
	return ret;
}

/* Создание списка пиктограмм БПУ для загрузки и удаления */
static void check_xprn_icons(const list<IconInfo> &stored_icons, list<IconInfo> &icons_to_load,
	list<IconInfo> &icons_to_erase)
{
	icons_to_load.clear();
	icons_to_erase.clear();
/* Создаём список пиктограмм в БПУ */
	list<IconInfo> xprn_icons;
	if (!find_xprn_icons(xprn_icons))
		return;
/* Ищем пиктограммы для загрузки */
	log_dbg("Ищем пиктограммы для загрузки...");
	for (const auto &p : stored_icons){
		auto p1 = find_if(xprn_icons, [p](const IconInfo &gi) {return gi.id() == p.id();});
		if (p1 == xprn_icons.end()){
			log_dbg("Пиктограмма %s #%d (%hc) отсутствует в БПУ и будет туда загружена.",
				p.name().c_str(), p.nr(), p.id());
			icons_to_load.push_back(p);
		}else if (p.isNewer(*p1)){
			log_dbg("Пиктограмму %s #%d (%hc) необходимо обновить.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			icons_to_load.push_back(p);
			icons_to_erase.push_back(*p1);
		}else
			log_dbg("Пиктограмма %s #%d (%hc) не требует обновления.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
	}
/* Ищем пиктограммы для удаления */
	log_dbg("Ищем пиктограммы для удаления...");
	for (const auto &p : xprn_icons){
		if (find_if(stored_icons, [p](const IconInfo &gi) {return gi.id() == p.id();}) == stored_icons.end()){
			log_dbg("Пиктограмма %s #%d (%hc) помечена для удаления.", p.name().c_str(), p.nr(), p.id());
			icons_to_erase.push_back(p);
		}
	}
	log_dbg("Найдено пиктограмм: для загрузки: %d; для удаления: %d.", icons_to_load.size(), icons_to_erase.size());
}

/* Запись заданной пиктограммы в БПУ */
static bool write_icon_to_xprn(const IconInfo &gi)
{
	if (xprn == NULL)
		return false;
	bool ret = false;
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), "%s\\%s.BMP", icon_folder, gi.name().c_str());
	log_info("Загружаем в БПУ пиктограмму %s...", path);
	if (xprn->beginGridLoad(path, gi.nr(), xprnOpCallback)){
		xprn_wait.wait();
		ret = xprn->statusOK();
		if (ret)
			log_info("Пиктограмма %s успешно загружена в БПУ.", path);
		else
			log_err("Ошибка загрузки пиктограммы %s в БПУ: %s.", path, XPrn::status_str(xprn->status()));
	}else
		log_err("Невозможно начать загрузку пиктограммы %s в БПУ: %s.", path, XPrn::status_str(xprn->status()));
	return ret;
}

/* Удаление заданной пиктограммы из БПУ */
static bool erase_icon_from_xprn(const IconInfo &gi)
{
	if (xprn == NULL)
		return false;
	bool ret = false;
	log_info("Удаляем из БПУ пиктограммы %s...", gi.name().c_str());
	if (xprn->beginGridErase(gi.nr(), xprnOpCallback)){
		xprn_wait.wait();
		ret = xprn->statusOK();
		if (ret)
			log_info("Пиктограмма %s успешно удалена из БПУ.", gi.name().c_str());
		else
			log_err("Ошибка удаления пиктограммы %s из БПУ: %s.", gi.name().c_str(),
				XPrn::status_str(xprn->status()));
	}else
		log_err("Невозможно начать удаление пиктограммы %s из БПУ: %s.", gi.name().c_str(),
			XPrn::status_str(xprn->status()));
	return ret;
}

/* Обновление пиктограмм в БПУ "Видеотон" */
static bool update_icons_vtsv(const list<IconInfo> &icons_to_load, const list<IconInfo> &icons_to_erase,
	list<IconInfo> &icons_failed, InitializationNotify_t init_notify)
{
//	bool ret = true;
	static char txt[64];
	size_t n = 1;
/* Сначала удаляем ненужные пиктограммы из БПУ */
	for (const auto &p : icons_to_erase){
		snprintf(txt, ASIZE(txt), "Удаление из БПУ пиктограммы %s (%u из %u)",
			p.name().c_str(), n++, icons_to_erase.size());
		init_notify(false, txt);
		erase_icon_from_xprn(p);
/*		if (!erase_icon_from_xprn(p)){
			ret = false;
			break;
		}*/
	}
/*	if (!ret)
		return ret;*/
/* Затем загружаем новые пиктограммы */
	n = 1;
	for (const auto &p : icons_to_load){
		snprintf(txt, ASIZE(txt), "Загрузка в БПУ пиктограммы %s (%u из %u)",
			p.name().c_str(), n++, icons_to_load.size());
		init_notify(false, txt);
		if (!write_icon_to_xprn(p))
			icons_failed.push_back(p);
/*		{
			ret = false;
			break;
		}*/
	}
/* Проверяем, что пиктограммы успешно загрузились */
	log_info("Проверяем записанные пиктограммы...");
	Sleep(3000);
	list<IconInfo> xprn_icons;
	if (!find_xprn_icons(xprn_icons))
		return false;
	bool ok = true;
	for (const auto &p : icons_to_load){
		auto p1 = find_if(icons_failed, [p](const IconInfo &gi) {return gi.id() == p.id();});
		if (p1 != icons_failed.end())
			continue;
		p1 = find_if(xprn_icons, [p](const IconInfo &gi) {return gi.id() == p.id();});
		if (p1 == xprn_icons.end()){
			log_dbg("Пиктограмма %s #%d (%hc) отсутствует в БПУ после загрузки.",
				p.name().c_str(), p.nr(), p.id());
			icons_failed.push_back(p);
			ok = false;
		}else if (p.isNewer(*p1)){
			log_dbg("Пиктограмма %s #%d (%hc) не была удалена из БПУ.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			icons_failed.push_back(p);
			ok = false;
		}
	}
	if (ok)
		log_info("Пиктограммы в БПУ успешно обновлены.");
	return true; /*ret;*/
}

/* Обновление пиктограмм в БПУ "СПЕКТР-ДС" */
static bool update_icons_sprn(const list<IconInfo> &stored_icons, list<IconInfo> &icons_failed,
	InitializationNotify_t init_notify)
{
/* Сначала удаляем все пиктограммы из БПУ */
	log_info("Удаляем все имеющиеся пиктограммы из БПУ...");
	init_notify(false, "Удаление пиктограмм из БПУ");
	if (!xprn->beginGridEraseAll(xprnOpCallback)){
		log_err("Невозможно начать удаление пиктограмм из БПУ: %s.", XPrn::status_str(xprn->status()));
		return false;
	}
	xprn_wait.wait();
	if (!xprn->statusOK()){
		log_err("Ошибка удаления пиктограмм из БПУ: %s.", XPrn::status_str(xprn->status()));
		return false;
	}
//	bool ret = true;
/* Затем загружаем новые пиктограммы */
	size_t n = 1;
	for (const auto &p : stored_icons){
		static char txt[64];
		snprintf(txt, ASIZE(txt), "Загрузка в БПУ пиктограммы %s (%u из %u)",
			p.name().c_str(), n++, stored_icons.size());
		init_notify(false, txt);
		if (!write_icon_to_xprn(p))
			icons_failed.push_back(p);
/*		{
			ret = false;
			break;
		}*/
	}
	return true; /*ret;*/
}

/* Обновление пиктограмм в БПУ */
bool update_xprn_icons(InitializationNotify_t init_notify)
{
	if (xprn == NULL){
		log_err("БПУ отсутствует.");
		return false;
	}
	bool ret = false;
	init_notify(false, "Обновление пиктограмм в БПУ");
	Sleep(100);
	list<IconInfo> stored_icons, icons_to_load, icons_to_erase;
	find_stored_icons_xprn(stored_icons);
	check_xprn_icons(stored_icons, icons_to_load, icons_to_erase);
	if (!icons_to_load.empty() || !icons_to_erase.empty()){
		if (xprn->type() == XPrnType::Videoton)
			ret = update_icons_vtsv(icons_to_load, icons_to_erase, icons_failed_xprn, init_notify); 
		else if (xprn->type() == XPrnType::Spectrum)
			ret = update_icons_sprn(stored_icons, icons_failed_xprn, init_notify);
	}else
		ret = true;
	if (!ret && !isTermcoreState(TS_INIT)){
		static char txt[256];
		uint8_t status = xprn->status();
		if (xprn->statusOK(status))
			snprintf(txt, ASIZE(txt), "Ошибка обновления пиктограмм в БПУ.");
		else
			snprintf(txt, ASIZE(txt), "Ошибка обновления пиктограмм в БПУ: %s.",
				xprn->status_str(status));
		termcore_callbacks.callMessageBox("ОБНОВЛЕНИЕ РАЗМЕТОК", txt);
	}
	return ret;
}

/* Получение списка пиктограмм из ККТ */
static bool find_kkt_icons(list<IconInfo> &kkt_icons)
{
	bool ret = false;
	kkt_icons.clear();
	log_info("Чтение списка пиктограмм из ККТ...");
	if (kkt->getGridLst()){
		KktRespGridLst *rsp = dynamic_cast<KktRespGridLst *>(kkt->rsp());
		if ((rsp != NULL) && rsp->statusOk()){
			kkt_icons.assign(rsp->icons().cbegin(), rsp->icons().cend());
			log_info("Найдено пиктограмм в ККТ: %d.", kkt_icons.size());
			ret = true;
		}else
			log_err("Ошибка получения списка пиктограмм из ККТ: 0x%.2hx.", (uint16_t)kkt->status());
	}else
		log_err("Невозможно начать получение списка пиктограмм из ККТ: 0x%.2hx.", (uint16_t)kkt->status());
	return ret;
}

/* Создание списка пиктограмм ККТ для загрузки и удаления */
static void check_kkt_icons(const list<IconInfo> &stored_icons, list<IconInfo> &icons_to_load,
	list<IconInfo> &icons_to_erase)
{
	icons_to_load.clear();
	icons_to_erase.clear();
/* Создаём список пиктограмм в ККТ */
	list<IconInfo> kkt_icons;
	if (!find_kkt_icons(kkt_icons))
		return;
/* Ищем пиктограммы для загрузки */
	log_dbg("Ищем пиктограммы для загрузки...");
	for (const auto &p : stored_icons){
		auto p1 = find_if(kkt_icons, [p](const IconInfo &gi) {return gi.id() == p.id();});
		if (p1 == kkt_icons.end()){
			log_dbg("Пиктограмма %s #%d (%hc) отсутствует в ККТ и будет туда загружена.",
				p.name().c_str(), p.nr(), p.id());
			icons_to_load.push_back(p);
		}else if (p.isNewer(*p1)){
			log_dbg("Пиктограмму %s #%d (%hc) необходимо обновить.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			icons_to_load.push_back(p);
			icons_to_erase.push_back(*p1);
		}else
			log_dbg("Пиктограмма %s #%d (%hc) не требует обновления.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
	}
/* Ищем пиктограммы для удаления */
	log_dbg("Ищем пиктограммы для удаления...");
	for (const auto &p : kkt_icons){
		if (find_if(stored_icons, [p](const IconInfo &gi) {return gi.id() == p.id();}) == stored_icons.end()){
			log_dbg("Пиктограмма %s #%d (%hc) помечена для удаления.", p.name().c_str(), p.nr(), p.id());
			icons_to_erase.push_back(p);
		}
	}
	log_dbg("Найдено пиктограмм: для загрузки: %d; для удаления: %d.", icons_to_load.size(), icons_to_erase.size());
}

/* Запись заданной пиктограммы в ККТ */
static bool write_icon_to_kkt(const IconInfo &gi)
{
	if (kkt == NULL)
		return false;
	bool ret = false;
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), "%s\\%s.BMP", icon_folder, gi.name().c_str());
	log_info("Загружаем в ККТ пиктограмму %s...", path);
	if (kkt->loadGrid(path, gi.nr())){
		KktRespGridLoad *rsp = dynamic_cast<KktRespGridLoad *>(kkt->rsp());
		if ((rsp != NULL) && rsp->statusOk()){
			ret = true;
			log_info("Пиктограмма %s успешно загружена в ККТ.", path);
		}else
			log_err("Ошибка загрузки пиктограммы %s в ККТ: 0x%.2hx.", path, (uint16_t)kkt->status());
	}else
		log_err("Невозможно начать загрузку пиктограммы %s в ККТ: 0x%.2hx.", path, (uint16_t)kkt->status());
	return ret;
}

/* Обновление пиктограмм в ККТ "СПЕКТР-Ф" */
static bool update_icons_kkt(const list<IconInfo> &stored_icons, list<IconInfo> &icons_failed,
	InitializationNotify_t init_notify)
{
/* Сначала удаляем все пиктограммы из ККТ */
	log_info("Удаляем все имеющиеся пиктограммы из ККТ...");
	init_notify(false, "Удаление пиктограмм из ККТ");
	if (kkt->eraseAllGrids()){
		KktRespGridEraseAll *rsp = dynamic_cast<KktRespGridEraseAll *>(kkt->rsp());
		if ((rsp != NULL) && rsp->statusOk())
			log_info("Пиктограммы удалены из ККТ.");
		else{
			log_err("Ошибка удаления пиктограмм из ККТ: 0x%.2hx.", (uint16_t)kkt->status());
			return false;
		}
	}else{
		log_err("Невозможно начать удаление пиктограмм из ККТ: 0x%.2hx.", (uint16_t)kkt->status());
		return false;
	}
/* Затем загружаем новые пиктограммы */
	size_t n = 1;
	kkt->beginBatchMode();
	for (const auto &p : stored_icons){
		static char txt[64];
		snprintf(txt, ASIZE(txt), "Загрузка в ККТ пиктограммы %s (%u из %u)",
			p.name().c_str(), n++, stored_icons.size());
		init_notify(false, txt);
		if (!write_icon_to_kkt(p))
			icons_failed.push_back(p);
	}
	kkt->endBatchMode();
	return true;
}

/* Обновление пиктограмм в ККТ */
bool update_kkt_icons(InitializationNotify_t init_notify)
{
	log_info("kkt = %p.", kkt);
	if (kkt == NULL)
		return false;
	bool ret = false;
	init_notify(false, "Обновление пиктограмм в ККТ");
	Sleep(100);
	list<IconInfo> stored_icons, icons_to_load, icons_to_erase;
	find_stored_icons_kkt(stored_icons);
	check_kkt_icons(stored_icons, icons_to_load, icons_to_erase);
	if (!icons_to_load.empty() || !icons_to_erase.empty())
		ret = update_icons_kkt(stored_icons, icons_failed_kkt, init_notify);
	else
		ret = true;
	if (!ret && !isTermcoreState(TS_INIT)){
		static char txt[256];
		snprintf(txt, ASIZE(txt), "Ошибка обновления пиктограмм в ККТ: 0x%.2hx.",
			(uint16_t)kkt->status());
		termcore_callbacks.callMessageBox("ОБНОВЛЕНИЕ РАЗМЕТОК", txt);
	}
	return ret;
}
#endif
