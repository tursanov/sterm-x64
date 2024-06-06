/* ����� � ࠧ��⪠�� ������� ���/���. (c) gsr, 2015-2016, 2019, 2022, 2024 */

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <zlib.h>
#include "gui/scr.h"
#include "kkt/cmd.h"
#include "kkt/fdo.h"
#include "kkt/io.h"
#include "kkt/kkt.h"
#include "x3data/bmp.h"
#include "x3data/grids.h"
#include "x3data/grids.hpp"
#include "x3data/icons.h"
#include "express.h"
#include "paths.h"
#include "sterm.h"
#include "termlog.h"
#include "transport.h"
#include "xbase64.h"

bool GridInfo::parse(const char *name)
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
	char nm[SPRN_MAX_GRID_NAME_LEN + 1];
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
		_name.assign(nm);
	}
	return ret;
}

static list<GridInfo> grids_to_create_xprn;
static list<GridInfo>::const_iterator grids_to_create_xprn_ptr = grids_to_create_xprn.cbegin();
static list<GridInfo> grids_to_remove_xprn;
static list<GridInfo> grids_failed_xprn;

static list<GridInfo> grids_to_create_kkt;
static list<GridInfo>::const_iterator grids_to_create_kkt_ptr = grids_to_create_kkt.cbegin();
static list<GridInfo> grids_to_remove_kkt;
static list<GridInfo> grids_failed_kkt;

static void clr_grid_lists(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove)
{
	grids_to_create.clear();
	grids_to_remove.clear();
}

static inline void clr_grid_lists_xprn()
{
	clr_grid_lists(grids_to_create_xprn, grids_to_remove_xprn);
	grids_to_create_xprn_ptr = grids_to_create_xprn.cbegin();
}

static inline void clr_grid_lists_kkt()
{
	clr_grid_lists(grids_to_create_kkt, grids_to_remove_kkt);
	grids_to_create_kkt_ptr = grids_to_create_kkt.cbegin();
}

static inline void clr_grid_lists()
{
	clr_grid_lists_xprn();
	clr_grid_lists_kkt();
}

bool need_grids_update_xprn()
{
	return !grids_to_create_xprn.empty() || !grids_to_remove_xprn.empty();
}

bool need_grids_update_kkt()
{
	return !grids_to_create_kkt.empty() || !grids_to_remove_kkt.empty();
}

static regex_t reg;

static int grid_selector(const struct dirent *entry)
{
	log_dbg("d_name = '%s'.", entry->d_name);
	return regexec(&reg, entry->d_name, 0, NULL, 0) == REG_NOERROR;
}

/* ���� �� ��᪥ ࠧ��⮪ �� ॣ��୮�� ��ࠦ���� */
static bool find_stored_grids(list<GridInfo> &stored_grids, const char *pattern)
{
	if (pattern == NULL)
		return false;
	int rc = regcomp(&reg, pattern, REG_EXTENDED | REG_NOSUB);
	if (rc != REG_NOERROR){
		log_err("�訡�� �������樨 ॣ��୮�� ��ࠦ���� ��� '%s': %d.", pattern, rc);
		return false;
	}
	struct dirent **names;
	int n = scandir(GRIDS_FOLDER, &names, grid_selector, alphasort);
	if (n == -1){
		log_sys_err("�訡�� ���᪠ ࠧ��⮪ ������� � ��⠫��� " GRIDS_FOLDER ":");
		regfree(&reg);
		return false;
	}
	bool ret = false;
	for (int i = 0; i < n; i++){
		static char path[PATH_MAX];
		snprintf(path, ASIZE(path), GRIDS_FOLDER "/%s", names[i]->d_name);
		GridInfo gi;
		if (gi.parse(names[i]->d_name)){
			log_dbg("�����㦥�� ࠧ��⪠ %s #%d (%hc).", gi.name().c_str(), gi.nr(), gi.id());
			stored_grids.push_back(gi);
			ret = true;
		}
	}
	free(names);
	regfree(&reg);
	return ret;
}

/* ���� �� ��᪥ ࠧ��⮪ ��� ��� */
static bool find_stored_grids_xprn(list<GridInfo> &stored_grids)
{
	stored_grids.clear();
	find_stored_grids(stored_grids, "^HU[1-9].[Bb][Mm][Pp]$");
	find_stored_grids(stored_grids, "^L[0-9]{7}\\.[Bb][Mm][Pp]$");
	return !stored_grids.empty();
}

/* ���� �� ��᪥ ࠧ��⮪ ��� ��� */
static bool find_stored_grids_kkt(list<GridInfo> &stored_grids)
{
	stored_grids.clear();
	find_stored_grids(stored_grids, "^M[0-9]{7}\\.[Bb][Mm][Pp]$");
	return !stored_grids.empty();
}

/*
 * �������� ᯨ᪮� ࠧ��⮪ ��� ����窨/㤠����� �� �᭮����� ᯨ᪠
 * �� "������" � 䠩��� �� ��᪥.
 */
static void check_stored_grids(const list<GridInfo> &x3_grids, list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove,
	bool (*find_fn)(list<GridInfo> &))
{
	clr_grid_lists(grids_to_create, grids_to_remove);
/* ������ ᯨ᮪ ࠧ��⮪, �࠭����� � ��� ��� */
	log_dbg("�饬 ࠧ��⪨ � ��⠫��� %s...", GRIDS_FOLDER);
	list<GridInfo> stored_grids;
	find_fn(stored_grids);
	log_dbg("������� ࠧ��⮪: %d.", stored_grids.size());
/* �饬 ࠧ��⪨ ��� ����窨 */
	log_dbg("�饬 ࠧ��⪨ ��� ����窨...");
	for (const auto &p : x3_grids){
		bool found = false;
		for (const auto &p1 : stored_grids){
			if (p1.id() != p.id())
				continue;
			else if (p.isNewer(p1)){
				log_dbg("�����⪠ %s #%d (%hc) ��॥ ����ᮢ᪮� � �㤥� 㤠����.",
					p1.name().c_str(), p1.nr(), p1.id());
				grids_to_remove.push_back(p1);
			}else if (p1.isNewer(p)){
				log_dbg("�����⪠ %s #%d (%hc) ����� ����ᮢ᪮� � �㤥� 㤠���.",
					p1.name().c_str(), p1.nr(), p1.id());
				grids_to_remove.push_back(p1);
			}else
				found = true;
		}
		if (found)
			log_dbg("�����⪠ %s #%d (%hc) �� �ॡ�� ����������.", p.name().c_str(), p.nr(), p.id());
		else{
			log_dbg("�����⪠ %s #%d (%hc) ��������� � ��� ��� � �㤥� ����㦥�� �� \"������-3\").",
				p.name().c_str(), p.nr(), p.id());
			grids_to_create.push_back(p);
		}
	}
/* �饬 ࠧ��⪨ ��� 㤠����� */
/*	log_dbg("�饬 ࠧ��⪨ ��� 㤠�����...");
	for (const auto &p : stored_grids){
		if (find_if(x3_grids, [p](const GridInfo &gi) {return gi.id() == p.id();}) == x3_grids.end()){
			log_dbg("�����⪠ %s #%d (%hc) ����祭� ��� 㤠�����.", p.name().c_str(), p.nr(), p.id());
			grids_to_remove.push_back(p);
		}
	}*/
	log_dbg("������� ࠧ��⮪: ��� ����窨: %d; ��� 㤠�����: %d.", grids_to_create.size(), grids_to_remove.size());
}

/* �������� ᯨ᪠ ࠧ��⮪ ��� ��� */
static inline void check_stored_grids_xprn(const list<GridInfo> &x3_grids)
{
	check_stored_grids(x3_grids, grids_to_create_xprn, grids_to_remove_xprn, find_stored_grids_xprn);
}

/* �������� ᯨ᪠ ࠧ��⮪ ��� ��� */
static inline void check_stored_grids_kkt(const list<GridInfo> &x3_grids)
{
	check_stored_grids(x3_grids, grids_to_create_kkt, grids_to_remove_kkt, find_stored_grids_kkt);
}

/* ���� � ����� �⢥� �����䨪��஢ ࠧ��⮪ ��� ���/��� */
static bool check_x3_grids(const uint8_t *data, size_t len, list<GridInfo> &x3_grids_xprn, list<GridInfo> &x3_grids_kkt)
{
	if ((data == NULL) || (len == 0))
		return false;
	x3_grids_xprn.clear();
	x3_grids_kkt.clear();
	for (size_t i = 0; (data[i] == 0x3c) && ((i + 1) < len); i++){
		const uint8_t *p = (const uint8_t *)memchr(data + i + 1, 0x3e, len - i - 1);
		if (p == NULL)
			break;
		GridInfo gi;
		if (gi.parse(data + i, len - 1)){
			if ((gi.prefix() == "HU") || (gi.prefix() == "L")){
				log_dbg("������� ࠧ��⪠ ��� ���: %s.", gi.name().c_str());
				x3_grids_xprn.push_back(gi);
			}else if (gi.prefix() == "M"){
				log_dbg("������� ࠧ��⪠ ��� ���: %s.", gi.name().c_str());
				x3_grids_kkt.push_back(gi);
			}
		}
		i += p - (data + i);
	}
	return !x3_grids_xprn.empty() || !x3_grids_kkt.empty();
}

bool check_x3_grids(const uint8_t *data, size_t len)
{
	list<GridInfo> x3_grids_xprn, x3_grids_kkt;
	bool ret = check_x3_grids(data, len, x3_grids_xprn, x3_grids_kkt);
	if (ret){
		check_stored_grids_xprn(x3_grids_xprn);
		check_stored_grids_kkt(x3_grids_kkt);
	}
	return ret;
}

/* ���ᨬ���� ࠧ��� ���� ᦠ⮩ ࠧ��⪨ */
static const size_t MAX_COMPRESSED_GRID_LEN = 65536;
/* ���� ��� ��� ᦠ⮩ ࠧ��⪨ */
static uint8_t grid_buf[MAX_COMPRESSED_GRID_LEN];
/* ����騩 ࠧ��� �ਭ���� ������ ᦠ⮩ ࠧ��⪨ */
static size_t grid_buf_idx = 0;

/* ��⮧���� ��� ����祭�� ࠧ��⪨ �� ���� */
static uint8_t grid_auto_req[REQ_BUF_LEN];
/* ����� ��⮧���� */
static size_t grid_auto_req_len = 0;

static x3_sync_callback_t grid_sync_cbk = NULL;

/* �������쭠� �ਭ� ࠧ��⪨ ������ � �� */
#define GRID_MIN_WIDTH		1
/* ���ᨬ��쭠� �ਭ� ࠧ��⪨ ������ � �� */
#define GRID_MAX_WIDTH		204
/* �������쭠� ���� ࠧ��⪨ ������ � �� */
#define GRID_MIN_HEIGHT		1
/* ���ᨬ��쭠� ���� ࠧ��⪨ ������ � �� */
#define GRID_MAX_HEIGHT		82

/* �஢�ઠ ��������� ࠧ��⪨ */
static bool check_grid_header(const struct pic_header *hdr, uint8_t id)
{
	bool ret = false;
	if (hdr == NULL)
		log_err("hdr = NULL");
	else if (hdr->hdr_len != sizeof(*hdr))
		log_err("len mismatch");
	else if (hdr->id != id)
		log_err("id mismatch");
	else if ((hdr->w < GRID_MIN_WIDTH) || (hdr->w > GRID_MAX_WIDTH))
		log_err("illegal w");
	else if ((hdr->h < GRID_MIN_HEIGHT) || (hdr->h > GRID_MAX_HEIGHT))
		log_err("illegal h");
	else if (!GridInfo::isIdValid(hdr->id) || (hdr->id == 0x38))
		log_err("illegal id");
	else if ((hdr->data_len == 0) || (hdr->data_len > MAX_COMPRESSED_GRID_LEN))
		log_err("illegal data len");
	else{
		ret = true;
		for (size_t i = 0; i < sizeof(hdr->name); i++){
			char ch = hdr->name[i];
			if (ch == 0)
				break;
			else if (ch < 0x20){
				log_err("illegal name");
				ret = false;
				break;
			}
		}
	}
	return ret;
}

/* ��ࠢ�� ��砫쭮�� ����� �� ����祭�� ࠧ��⪨ */
static void send_grid_request(const GridInfo &grid)
{
	log_dbg("name = %s.", grid.name().c_str());
	int offs = get_req_offset();
	req_len = offs;
	req_len += snprintf((char *)req_buf + req_len, ASIZE(req_buf) - req_len,
		"P55TR(EXP.DATA.MAKET   %-8s)000000000", grid.name().c_str());
	set_scr_request(req_buf + offs, req_len - offs, true);
	wrap_text();
}

/* ��ࠢ�� ��⮧���� ��� ����祭�� ࠧ��⪨ */
static void send_grid_auto_request()
{
	req_len = get_req_offset();
	memcpy(req_buf + req_len, grid_auto_req, grid_auto_req_len);	/* FIXME */
	set_scr_request(grid_auto_req, grid_auto_req_len, true);
	req_len += grid_auto_req_len;
	wrap_text();
}

/* ������஢���� ࠧ��⪨, �ᯠ����� � ��࠭���� � 䠩�� �� ��᪥ */
static bool store_grid(const GridInfo &gi)
{
	log_info("path = %s; id = %hc.", gi.name().c_str(), gi.id());
	if (grid_buf_idx == 0){
		log_err("���� ������ ����.");
		return false;
	}
	size_t len = 0;
	if (!xbase64_decode(grid_buf, grid_buf_idx, grid_buf, grid_buf_idx, &len) || (len == 0)){
		log_err("�訡�� ������஢���� ������.");
		return false;
	}
	BITMAPFILEHEADER bmp_hdr;
	size_t l = sizeof(bmp_hdr);
	int rc = uncompress((uint8_t *)&bmp_hdr, &l, grid_buf, len);
	if ((rc != Z_OK) && (rc != Z_BUF_ERROR)){
		log_err("�訡�� �ᯠ����� (%d).", rc);
		return false;
	}else if ((bmp_hdr.bfType != 0x4d42) || (bmp_hdr.bfSize == 0)){
		log_err("������ �ଠ� ��������� BMP (bfType = 0x%.hx; bfSize = %u; bfOffBits = %u).",
			bmp_hdr.bfType, bmp_hdr.bfSize, bmp_hdr.bfOffBits);
		return false;
	}
	size_t bmp_len = bmp_hdr.bfSize;
	if (bmp_len > 262144){
		log_err("������ 䠩�� BMP (%u) �ॢ�蠥� ���ᨬ��쭮 �����⨬�.", bmp_len);
		return false;
	}
	scoped_ptr<uint8_t> bmp_data(new uint8_t[bmp_len]);
	rc = uncompress(bmp_data.get(), &bmp_len, grid_buf, len);
	if (rc != Z_OK){
		log_err("�訡�� �ᯠ����� BMP (%d).", rc);
		return false;
	}
	char path[PATH_MAX];
	snprintf(path, ASIZE(path), GRIDS_FOLDER "/%s.BMP", gi.name().c_str());
	int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (fd == -1){
		log_sys_err("�訡�� ᮧ����� 䠩�� %s:", path);
		return false;
	}
	bool ret = false;
	rc = write(fd, bmp_data.get(), bmp_len);
	if (rc == bmp_len){
		log_info("�����⪠ �ᯥ譮 ����ᠭ� � 䠩� %s.", path);
		ret = true;
	}else if (rc == -1)
		log_sys_err("�訡�� ����� � 䠩� %s:", path);
	else
		log_err("� %s ����� %u ����ᠭ� %d ����.", path, bmp_len, rc);
	close(fd);
	if (!ret)
		unlink(path);
	return ret;
}

/* �������� ࠧ��⪨ � ��᪠ */
static bool remove_grid(const GridInfo &gi)
{
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), GRIDS_FOLDER "/%s.BMP", gi.name().c_str());
	log_info("����塞 ࠧ���� %s...", path);
	bool ret = unlink(path) == 0;
	if (ret)
		log_info("�����⪠ %s �ᯥ譮 㤠����.", path);
	else
		log_sys_err("�訡�� 㤠����� ࠧ��⪨ %s:", path);
	return ret;
}

/* ��뢠���� ��᫥ ����祭�� �⢥� �� ����� ࠧ��⪨ ���/��� */
void on_response_grid(void)
{
	grid_auto_req_len = 0;
	static char err_msg[1024];
	err_msg[0] = 0;
/* 0 -- ����� ࠧ��⪨; 1 -- ��㣮� �⢥�, ��ࠡ�⪠ �� �ॡ����; 2 -- ��㣮� �⢥�, �ॡ���� ��ࠡ�⪠ */
	int non_grid_resp = 0;
	set_term_state(st_resp);
	int grid_para = -1, req_para = -1;
	size_t grid_len = 0, req_len = 0;
	if (find_pic_data(&grid_para, &req_para) && (grid_para != -1)){
		grid_len = handle_para(grid_para);
		log_info("�����㦥�� ����� ࠧ��⪨ (����� #%d; %zd ����).",
			grid_para + 1, grid_len);
		if (grid_len > (ASIZE(grid_buf) - grid_buf_idx)){
			snprintf(err_msg, ASIZE(err_msg), "��९������� ���� ������ ࠧ��⪨.");
			non_grid_resp = 1;
		}else if (grid_len > 0){
			memcpy(grid_buf + grid_buf_idx, text_buf, grid_len);
			grid_buf_idx += grid_len;
			if (req_para != -1){
				req_len = handle_para(req_para);
				if (req_len > 0){
					log_info("�����㦥� ��⮧���� (����� %d; %zd ����).",
						req_para + 1, req_len);
					memcpy(grid_auto_req, text_buf, req_len);
					grid_auto_req_len = req_len;
					send_grid_auto_request();
				}
			}else if (req_type == req_grid_xprn){
				log_info("�����⪠ %s ����祭� ���������. ���࠭塞 � 䠩�...",
					grids_to_create_xprn_ptr->name().c_str());
				store_grid(*grids_to_create_xprn_ptr++);
				grid_buf_idx = 0;
				if (grids_to_create_xprn_ptr == grids_to_create_xprn.cend()){
					log_info("����㧪� ࠧ��⮪ ��� ��� �����襭�.");
					sync_grids_kkt(NULL);
				}else
					send_grid_request(*grids_to_create_xprn_ptr);
			}else if (req_type == req_grid_kkt){
				log_info("�����⪠ %s ����祭� ���������. ���࠭塞 � 䠩�...",
					grids_to_create_kkt_ptr->name().c_str());
				store_grid(*grids_to_create_kkt_ptr++);
				grid_buf_idx = 0;
				if (grids_to_create_kkt_ptr == grids_to_create_kkt.cend()){
					log_info("����㧪� ࠧ��⮪ ��� ��� �����襭�.");
					sync_icons_xprn(NULL);
				}else
					send_grid_request(*grids_to_create_kkt_ptr);
			}
		}else{
			snprintf(err_msg, ASIZE(err_msg), "����祭� ����� ࠧ��⪨ �㫥��� �����.");
			non_grid_resp = 1;
		}
	}else{
		snprintf(err_msg, ASIZE(err_msg), "�� ������� ����� ࠧ��⪨.");
		non_grid_resp = 2;
	}
	if (err_msg[0] != 0){
		log_err(err_msg);
		grid_buf_idx = 0;
//		termcore_callbacks.callMessageBox("������", err_msg);
	}
	if (non_grid_resp){
		if (grid_sync_cbk != NULL)
			grid_sync_cbk(true, NULL);
		if (non_grid_resp == 2)
			execute_resp();
	}
}

/* ��砫� ᨭ�஭���樨 ࠧ��⮪ � "������" */
static bool sync_grids(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove, list<GridInfo> &grids_failed,
       x3_sync_callback_t cbk)
{
	if (!need_grids_update()){
		log_info("���������� ࠧ��⮪ �� �ॡ����.");
		return true;
	}
	bool ok = true;
	char txt[256];
	size_t n = 0;
	grid_sync_cbk = cbk;
	if (cbk != NULL)
		cbk(false, "����㧪� ࠧ��⮪ �� \"������-3\"");
	grids_failed.clear();
	list<GridInfo> _grids_to_create, _grids_to_remove;
/* ���砫� 㤠�塞 ���� ࠧ��⪨ */
	for (const auto &p : grids_to_remove){
		if (remove_grid(p))
			n++;
		else
			_grids_to_remove.push_back(p);
	}
	grids_to_remove.assign(_grids_to_remove.cbegin(), _grids_to_remove.cend());
/* ��⥬ ����稢��� ���� */
	for (const auto &p : grids_to_create){
		snprintf(txt, ASIZE(txt), "����㧪� ࠧ��⪨ %s (%zu �� %zu)",
			p.name().c_str(), (n + 1), grids_to_create.size());
		if (cbk != NULL)
			cbk(false, txt);
		log_dbg(txt);
		send_grid_request(p);
		break;
/*		int rc = download_grid(p);
		if (rc == TC_OK)
			n++;
		else if (inquirer->first_req()){
			ok = false;
			break;
		}else{
			_grids_to_create.push_back(p);
			grids_failed.push_back(p);
		}*/
	}
/*	if (ok){
		grids_to_create.assign(_grids_to_create.cbegin(), _grids_to_create.cend());
	}
	if (ok && (n > 0))
		ok = update_xprn_grids(cbk) && update_kkt_grids(cbk);*/
	if (ok){
		if (cbk != NULL)
			cbk(false, "�����⪨ �ᯥ譮 ����㦥��");
	}/*else
		showXPrnPicSyncError(grids_failed_xprn, grids_failed_kkt, icons_failed_xprn, icons_failed_kkt);*/
	grid_sync_cbk = NULL;
	return ok;
}

bool sync_grids_xprn(x3_sync_callback_t cbk)
{
	log_dbg("");
	bool ret = true;
	if (need_grids_update_xprn()){
		req_type = req_grid_xprn;
		grids_to_create_xprn_ptr = grids_to_create_xprn.cbegin();
		ret = sync_grids(grids_to_create_xprn, grids_to_remove_xprn, grids_failed_xprn, cbk);
	}else
		ret = sync_grids_kkt(cbk);
	return ret;
}

bool sync_grids_kkt(x3_sync_callback_t cbk)
{
	bool ret = true;
	if (need_grids_update_kkt()){
		req_type = req_grid_kkt;
		grids_to_create_kkt_ptr = grids_to_create_kkt.cbegin();
		ret = sync_grids(grids_to_create_kkt, grids_to_remove_kkt, grids_failed_kkt, cbk);
	}else
		ret = sync_icons_xprn(cbk);
	return ret;
}

#if 0
/* ����祭�� ᯨ᪠ ࠧ��⮪ �� ��� */
static bool find_xprn_grids(list<GridInfo> &xprn_grids)
{
	bool ret = false;
	xprn_grids.clear();
	log_info("�⥭�� ᯨ᪠ ࠧ��⮪ �� ���...");
	if (xprn->beginGridLst(xprnOpCallback)){
		xprn_wait.wait();
		if (xprn->statusOK()){
			xprn_grids.assign(xprn->grids().cbegin(), xprn->grids().cend());
			log_info("������� ࠧ��⮪ � ���: %d.", xprn_grids.size());
			ret = true;
		}else
			log_err("�訡�� ����祭�� ᯨ᪠ ࠧ��⮪ �� ���: %s.", XPrn::status_str(xprn->status()));
	}else
		log_err("���������� ����� ����祭�� ᯨ᪠ ࠧ��⮪ �� ���.");
	return ret;
}

/* �������� ᯨ᪠ ࠧ��⮪ ��� ��� ����㧪� � 㤠����� */
static void check_xprn_grids(const list<GridInfo> &stored_grids, list<GridInfo> &grids_to_load,
	list<GridInfo> &grids_to_erase)
{
	grids_to_load.clear();
	grids_to_erase.clear();
/* ������ ᯨ᮪ ࠧ��⮪ � ��� */
	list<GridInfo> xprn_grids;
	if (!find_xprn_grids(xprn_grids))
		return;
/* �饬 ࠧ��⪨ ��� ����㧪� */
	log_dbg("�饬 ࠧ��⪨ ��� ����㧪�...");
	for (const auto &p : stored_grids){
		auto p1 = find_if(xprn_grids, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 == xprn_grids.end()){
			log_dbg("�����⪠ %s #%d (%hc) ��������� � ��� � �㤥� �㤠 ����㦥��.",
				p.name().c_str(), p.nr(), p.id());
			grids_to_load.push_back(p);
		}else if (p.isNewer(*p1)){
			log_dbg("������� %s #%d (%hc) ����室��� ��������.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			grids_to_load.push_back(p);
			grids_to_erase.push_back(*p1);
		}else
			log_dbg("�����⪠ %s #%d (%hc) �� �ॡ�� ����������.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
	}
/* �饬 ࠧ��⪨ ��� 㤠����� */
	log_dbg("�饬 ࠧ��⪨ ��� 㤠�����...");
	for (const auto &p : xprn_grids){
		if (find_if(stored_grids, [p](const GridInfo &gi) {return gi.id() == p.id();}) == stored_grids.end()){
			log_dbg("�����⪠ %s #%d (%hc) ����祭� ��� 㤠�����.", p.name().c_str(), p.nr(), p.id());
			grids_to_erase.push_back(p);
		}
	}
	log_dbg("������� ࠧ��⮪: ��� ����㧪�: %d; ��� 㤠�����: %d.", grids_to_load.size(), grids_to_erase.size());
}

/* ������ �������� ࠧ��⪨ � ��� */
static bool write_grid_to_xprn(const GridInfo &gi)
{
	if (xprn == NULL)
		return false;
	bool ret = false;
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), "%s\\%s.BMP", grid_folder, gi.name().c_str());
	log_info("����㦠�� � ��� ࠧ���� %s...", path);
	if (xprn->beginGridLoad(path, gi.nr(), xprnOpCallback)){
		xprn_wait.wait();
		ret = xprn->statusOK();
		if (ret)
			log_info("�����⪠ %s �ᯥ譮 ����㦥�� � ���.", path);
		else
			log_err("�訡�� ����㧪� ࠧ��⪨ %s � ���: %s.", path, XPrn::status_str(xprn->status()));
	}else
		log_err("���������� ����� ����㧪� ࠧ��⪨ %s � ���: %s.", path, XPrn::status_str(xprn->status()));
	return ret;
}

/* �������� �������� ࠧ��⪨ �� ��� */
static bool erase_grid_from_xprn(const GridInfo &gi)
{
	if (xprn == NULL)
		return false;
	bool ret = false;
	log_info("����塞 �� ��� ࠧ��⪨ %s...", gi.name().c_str());
	if (xprn->beginGridErase(gi.nr(), xprnOpCallback)){
		xprn_wait.wait();
		ret = xprn->statusOK();
		if (ret)
			log_info("�����⪠ %s �ᯥ譮 㤠���� �� ���.", gi.name().c_str());
		else
			log_err("�訡�� 㤠����� ࠧ��⪨ %s �� ���: %s.", gi.name().c_str(),
				XPrn::status_str(xprn->status()));
	}else
		log_err("���������� ����� 㤠����� ࠧ��⪨ %s �� ���: %s.", gi.name().c_str(),
			XPrn::status_str(xprn->status()));
	return ret;
}

/* ���������� ࠧ��⮪ � ��� "�����⮭" */
static bool update_grids_vtsv(const list<GridInfo> &grids_to_load, const list<GridInfo> &grids_to_erase,
	list<GridInfo> &grids_failed, InitializationNotify_t init_notify)
{
//	bool ret = true;
	static char txt[64];
	size_t n = 1;
/* ���砫� 㤠�塞 ���㦭� ࠧ��⪨ �� ��� */
	for (const auto &p : grids_to_erase){
		snprintf(txt, ASIZE(txt), "�������� �� ��� ࠧ��⪨ %s (%u �� %u)",
			p.name().c_str(), n++, grids_to_erase.size());
		init_notify(false, txt);
		erase_grid_from_xprn(p);
/*		if (!erase_grid_from_xprn(p)){
			ret = false;
			break;
		}*/
	}
/*	if (!ret)
		return ret;*/
/* ��⥬ ����㦠�� ���� ࠧ��⪨ */
	n = 1;
	for (const auto &p : grids_to_load){
		snprintf(txt, ASIZE(txt), "����㧪� � ��� ࠧ��⪨ %s (%u �� %u)",
			p.name().c_str(), n++, grids_to_load.size());
		init_notify(false, txt);
		if (!write_grid_to_xprn(p))
			grids_failed.push_back(p);
/*		{
			ret = false;
			break;
		}*/
	}
/* �஢��塞, �� ࠧ��⪨ �ᯥ譮 ����㧨���� */
	log_info("�஢��塞 ����ᠭ�� ࠧ��⪨...");
	Sleep(3000);
	list<GridInfo> xprn_grids;
	if (!find_xprn_grids(xprn_grids))
		return false;
	bool ok = true;
	for (const auto &p : grids_to_load){
		auto p1 = find_if(grids_failed, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 != grids_failed.end())
			continue;
		p1 = find_if(xprn_grids, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 == xprn_grids.end()){
			log_dbg("�����⪠ %s #%d (%hc) ��������� � ��� ��᫥ ����㧪�.",
				p.name().c_str(), p.nr(), p.id());
			grids_failed.push_back(p);
			ok = false;
		}else if (p.isNewer(*p1)){
			log_dbg("�����⪠ %s #%d (%hc) �� �뫠 㤠���� �� ���.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			grids_failed.push_back(p);
			ok = false;
		}
	}
	if (ok)
		log_info("�����⪨ � ��� �ᯥ譮 ���������.");
	return true; /*ret;*/
}

/* ���������� ࠧ��⮪ � ��� "������-��" */
static bool update_grids_sprn(const list<GridInfo> &stored_grids, list<GridInfo> &grids_failed,
	InitializationNotify_t init_notify)
{
/* ���砫� 㤠�塞 �� ࠧ��⪨ �� ��� */
	log_info("����塞 �� ����騥�� ࠧ��⪨ �� ���...");
	init_notify(false, "�������� ࠧ��⮪ �� ���");
	if (!xprn->beginGridEraseAll(xprnOpCallback)){
		log_err("���������� ����� 㤠����� ࠧ��⮪ �� ���: %s.", XPrn::status_str(xprn->status()));
		return false;
	}
	xprn_wait.wait();
	if (!xprn->statusOK()){
		log_err("�訡�� 㤠����� ࠧ��⮪ �� ���: %s.", XPrn::status_str(xprn->status()));
		return false;
	}
//	bool ret = true;
/* ��⥬ ����㦠�� ���� ࠧ��⪨ */
	size_t n = 1;
	for (const auto &p : stored_grids){
		static char txt[64];
		snprintf(txt, ASIZE(txt), "����㧪� � ��� ࠧ��⪨ %s (%u �� %u)",
			p.name().c_str(), n++, stored_grids.size());
		init_notify(false, txt);
		if (!write_grid_to_xprn(p))
			grids_failed.push_back(p);
/*		{
			ret = false;
			break;
		}*/
	}
	return true; /*ret;*/
}

/* ���������� ࠧ��⮪ � ��� */
bool update_xprn_grids(InitializationNotify_t init_notify)
{
	if (xprn == NULL){
		log_err("��� ���������.");
		return false;
	}
	bool ret = false;
	init_notify(false, "���������� ࠧ��⮪ � ���");
	Sleep(100);
	list<GridInfo> stored_grids, grids_to_load, grids_to_erase;
	find_stored_grids_xprn(stored_grids);
	check_xprn_grids(stored_grids, grids_to_load, grids_to_erase);
	if (!grids_to_load.empty() || !grids_to_erase.empty()){
		if (xprn->type() == XPrnType::Videoton)
			ret = update_grids_vtsv(grids_to_load, grids_to_erase, grids_failed_xprn, init_notify); 
		else if (xprn->type() == XPrnType::Spectrum)
			ret = update_grids_sprn(stored_grids, grids_failed_xprn, init_notify);
	}else
		ret = true;
	if (!ret && !isTermcoreState(TS_INIT)){
		static char txt[256];
		uint8_t status = xprn->status();
		if (xprn->statusOK(status))
			snprintf(txt, ASIZE(txt), "�訡�� ���������� ࠧ��⮪ � ���.");
		else
			snprintf(txt, ASIZE(txt), "�訡�� ���������� ࠧ��⮪ � ���: %s.",
				xprn->status_str(status));
		termcore_callbacks.callMessageBox("���������� ��������", txt);
	}
	return ret;
}
#endif

/* ����祭�� ᯨ᪠ ࠧ��⮪ �� ��� */
static bool find_kkt_grids(list<GridInfo> &kkt_grids)
{
	bool ret = false;
	kkt_grids.clear();
	log_info("�⥭�� ᯨ᪠ ࠧ��⮪ �� ���...");
	size_t len = sizeof(grid_buf);
	uint8_t status = kkt_get_grid_lst(grid_buf, &len);
	if (status == KKT_STATUS_OK){
		size_t n = grid_buf[0];
		log_dbg("n = %zu.", n);
		const struct pic_header *hdr = (const struct pic_header *)(grid_buf + 1);
		for (size_t i = 0; i < n; i++, hdr++){
			if (check_grid_header(hdr, hdr->id)){
				char name[SPRN_MAX_GRID_NAME_LEN + 1];
				memcpy(name, hdr->name, sizeof(hdr->name));
				name[SPRN_MAX_GRID_NAME_LEN] = 0;
				log_dbg("name = %s", name);
				GridInfo gi;
				if (gi.parse(name))
					kkt_grids.push_back(gi);
				else
					log_err("parse error.");
			}
		}
		log_info("������� ࠧ��⮪ � ���: %zu.", kkt_grids.size());
		ret = true;
	}else
		log_err("�訡�� ����祭�� ᯨ᪠ ࠧ��⮪ �� ���: 0x%.2hhx (%s).",
			status, kkt_status_str(status));
	return ret;
}

/* �������� ᯨ᪠ ࠧ��⮪ ��� ��� ����㧪� � 㤠����� */
static void check_kkt_grids(const list<GridInfo> &stored_grids, list<GridInfo> &grids_to_load,
	list<GridInfo> &grids_to_erase)
{
	grids_to_load.clear();
	grids_to_erase.clear();
/* ������ ᯨ᮪ ࠧ��⮪ � ��� */
	list<GridInfo> kkt_grids;
	if (!find_kkt_grids(kkt_grids))
		return;
/* �饬 ࠧ��⪨ ��� ����㧪� */
	log_dbg("�饬 ࠧ��⪨ ��� ����㧪�...");
	for (const auto &p : stored_grids){
		auto p1 = find_if(kkt_grids, [p](const GridInfo &gi) {return gi.id() == p.id();});
		if (p1 == kkt_grids.end()){
			log_dbg("�����⪠ %s #%d (%c) ��������� � ��� � �㤥� �㤠 ����㦥��.",
				p.name().c_str(), p.nr(), p.id());
			grids_to_load.push_back(p);
		}else if (p.isNewer(*p1)){
			log_dbg("������� %s #%d (%c) ����室��� ��������.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
			grids_to_load.push_back(p);
			grids_to_erase.push_back(*p1);
		}else
			log_dbg("�����⪠ %s #%d (%c) �� �ॡ�� ����������.",
				(*p1).name().c_str(), (*p1).nr(), (*p1).id());
	}
/* �饬 ࠧ��⪨ ��� 㤠����� */
	log_dbg("�饬 ࠧ��⪨ ��� 㤠�����...");
	for (const auto &p : kkt_grids){
		if (find_if(stored_grids, [p](const GridInfo &gi) {return gi.id() == p.id();}) == stored_grids.end()){
			log_dbg("�����⪠ %s #%d (%c) ����祭� ��� 㤠�����.", p.name().c_str(), p.nr(), p.id());
			grids_to_erase.push_back(p);
		}
	}
	log_dbg("������� ࠧ��⮪: ��� ����㧪�: %zu; ��� 㤠�����: %zu.", grids_to_load.size(), grids_to_erase.size());
}

/* ������ �������� ࠧ��⪨ � ��� */
static bool write_grid_to_kkt(const GridInfo &gi)
{
	if (kkt == NULL)
		return false;
	bool ret = false;
	size_t len = 0, w = 0, h = 0;
	static char path[PATH_MAX];
	snprintf(path, ASIZE(path), GRIDS_FOLDER "/%s.BMP", gi.name().c_str());
	log_info("����㦠�� � ��� ࠧ���� %s...", path);
	const uint8_t *data = read_bmp(path, len, w, h, GRID_MIN_WIDTH, GRID_MAX_WIDTH,
		GRID_MIN_HEIGHT, GRID_MAX_HEIGHT);
	log_dbg("w = %zu; h = %zu.", w, h);
	if (data != NULL){
		vector<uint8_t> cdata;
		if (compress_picture(data, len, w, h, cdata) && !cdata.empty()){
			log_dbg("������ ������ ��᫥ ᦠ��: %zu ����.", cdata.size());
			char name[SPRN_MAX_GRID_NAME_LEN + 1];
			snprintf(name, sizeof(name), "%s.BMP", gi.name().c_str());
			if (kkt_load_grid(cdata.data(), cdata.size(), gi.id(), w, h, name) == KKT_STATUS_OK){
				log_info("�����⪠ %s �ᯥ譮 ����㦥�� � ���.", path);
				ret = true;
			}else
				log_err("�訡�� ����㧪� ࠧ��⪨ %s � ���: 0x%.2hhx.", path, kkt_status);
		}else
			log_err("�訡�� ᦠ�� ࠧ��⪨ %s.", path);
	}
	return ret;
}

/* ���������� ࠧ��⮪ � ��� "������-�" */
static bool update_grids_kkt(const list<GridInfo> &stored_grids, list<GridInfo> &grids_failed)
{
/* ���砫� 㤠�塞 �� ࠧ��⪨ �� ��� */
	log_info("����塞 �� ����騥�� ࠧ��⪨ �� ���...");
	set_term_state(st_kkt_grids);
	set_term_astate((intptr_t)"�������� ���� ��������");
	if (kkt_erase_all_grids() == KKT_STATUS_OK)
		log_info("�����⪨ 㤠���� �� ���.");
	else{
		static char txt[MAX_TERM_ASTATE_LEN + 1];
		log_err("�訡�� 㤠����� ࠧ��⮪ �� ���: 0x%.2hhx.", kkt_status);
		snprintf(txt, sizeof(txt), "������ �������� %.2hhx", kkt_status);
		set_term_astate((intptr_t)txt);
		return false;
	}
/* ��⥬ ����㦠�� ���� ࠧ��⪨ */
	size_t n = 1;
	kkt_begin_batch_mode();
	static char txt[MAX_TERM_ASTATE_LEN + 1];
	for (const auto &p : stored_grids){
		log_info("����㧪� � ��� ࠧ��⪨ %s (%zu �� %zu)",
			p.name().c_str(), n, stored_grids.size());
		snprintf(txt, sizeof(txt), "%s (%zu �� %zu)",
			p.name().c_str(), n++, stored_grids.size());
		set_term_astate((intptr_t)txt);
		if (!write_grid_to_kkt(p))
			grids_failed.push_back(p);
	}
	kkt_end_batch_mode();
	return true;
}

/* ���������� ࠧ��⮪ � ��� */
bool update_kkt_grids()
{
	if (kkt == NULL){
		log_err("��� �� ������祭�.");
		return false;
	}
	fdo_suspend();
	bool ret = false;
	list<GridInfo> stored_grids, grids_to_load, grids_to_erase;
	find_stored_grids_kkt(stored_grids);
	check_kkt_grids(stored_grids, grids_to_load, grids_to_erase);
	if (!grids_to_load.empty() || !grids_to_erase.empty()){
		ret = update_grids_kkt(stored_grids, grids_failed_kkt);
		set_term_state(st_input);
	}else
		ret = true;
	fdo_resume();
	return ret;
}
