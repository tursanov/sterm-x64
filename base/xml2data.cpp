#include "xml2data.h"

template <typename T> T &replace(T &s, const T &what, const T &txt)
{
	T::size_type l1 = what.size();
	if (l1 > 0){
		T::size_type i = 0, l2 = txt.size();
		while (i != s.npos){
			i = s.find(what, i);
			if (i != s.npos){
				s.replace(i, l1, txt);
				i += l2;
			}
		}
	}
	return s;
}

static vector<pair<string, string>> subst_tbl;
static vector<pair<string, string>> pre_subst_tbl;

static ssize_t get_c_str(const char *buf, char *str, size_t str_len)
{
	if ((buf == NULL) || (str == NULL) || (str_len == 0))
		return -1;
	ssize_t ret = -1;
	enum st {
		start,
		val,
		val_esc,
		val_hex,
		val_oct,
		val_end,
		err,
	} st = start;
	size_t buf_idx = 0;
	int cnt = 0;
	char v = 0;
	for (size_t str_idx = 0; (st != val_end) && (st != err) && (str_idx < str_len); buf_idx++){
		char ch = buf[buf_idx];
		if (ch == 0)
			break;
		switch (st){
			case start:
				if (ch == '\"')
					st = val;
				else if ((ch != ' ') && (ch != '\t'))
					st = err;
				break;
			case val:
				if (ch == '\"'){
					str[str_idx] = 0;
					st = val_end;
				}else if (ch == '\\')
					st = val_esc;
				else
					str[str_idx++] = ch;
				break;
			case val_esc:
				if (ch == 'x'){
					cnt = v = 0;
					st = val_hex;
				}else if ((ch >= '0') && (ch <= '7')){
					cnt = v = 0;
					st = val_oct;
					buf_idx--;
				}else if (ch == 'a')
					str[str_idx++] = 0x07;
				else if (ch == 'b')
					str[str_idx++] = 0x08;
				else if (ch == 't')
					str[str_idx++] = 0x09;
				else if (ch == 'n')
					str[str_idx++] = 0x0a;
				else if (ch == 'v')
					str[str_idx++] = 0x0b;
				else if (ch == 'f')
					str[str_idx++] = 0x0c;
				else if (ch == 'r')
					str[str_idx++] = 0x0d;
				else
					str[str_idx++] = ch;
				if (st == val_esc)
					st = val;
				break;
			case val_hex:
				if ((ch >= '0') && (ch <= '9'))
					ch -= 0x30;
				else if ((ch >= 'A') && (ch <= 'F'))
					ch -= 0x37;
				else if ((ch >= 'a') && (ch <= 'f'))
					ch -= 0x57;
				else
					st = err;
				if (st != err){
					v <<= 4;
					v |= ch;
					if (++cnt == 2){
						str[str_idx++] = v;
						st = val;
					}
				}
				break;
			case val_oct:
				if ((ch >= '0') && (ch <= '7')){
					v <<= 3;
					v |= (ch - 0x30);
					if (++cnt == 3){
						str[str_idx++] = v;
						st = val;
					}
				}else
					st = err;
		}
	}
	if (st == val_end)
		ret = buf_idx;
	return ret;
}

static bool parse_subst_entry(const char *txt, string &name, string &val)
{
	if (txt == NULL)
		return false;
	log_dbg("txt = '%s'.", txt);
	int ret = false;
	name.clear();
	val.clear();
	enum st {
		start,
		name_end,
		end,
		err,
	} st = start;
	char what[256], subst[256];
	ssize_t idx = 0;
	for (size_t src_idx = 0; (st != end) && (st != err);){
		char ch = txt[src_idx++];
		if (ch == 0)
			break;
		switch (st){
			case start:
				idx = get_c_str(txt, what, ASIZE(what));
				if (idx == -1)
					st = err;
				else{
					src_idx = idx;
					st = name_end;
				}
				break;
			case name_end:
				if (ch == '='){
					idx = get_c_str(txt + src_idx, subst, sizeof(subst));
					if (idx == -1)
						st = err;
					else
						st = end;
				}else if ((ch != ' ') && (ch != '\t'))
					st = err;
				break;
		}
	}
	if (st == end){
		name.assign(what);
		val.assign(subst);
		ret = true;
	}else
		log_err("Ошибка разбора строки (st = %d).", st);
	return ret;
}

static bool is_comment_str(const char *str)
{
	if (str == NULL)
		return false;
	bool ret = false;
	for (; *str != 0; str++){
		if ((*str != ' ') && (*str != '\t')){
			ret = *str == ';';
			break;
		}
	}
	return ret;
}

static regex_t reg;

static int find_selector(const struct dirent *entry)
{
	return regexec(&reg, entry->d_name, 0, NULL, 0) == REG_NOERROR;
}

ssize_t read_subst_tbl(const char *id, vector<pair<string, string>> &tbl)
{
	tbl.clear();
	char path[PATH_MAX];
	char pattern[64];
	snprintf(pattern, ASIZE(pattern), "^T%s[0-9]{5}\\.[Xx][Ss][Ll]$", id);
	int rc = regcomp(&reg, pattern, REG_EXTENDED | REG_NOSUB);
	if (rc != REG_NOERROR){
		log_err("Ошибка компиляции регулярного выражения для %s: %d.", pattern, rc);
		return -1;
	}
	struct dirent **names = NULL;
	int n = scandir(XSLT_FOLDER, &names, find_selector, alphasort);
	if (n == -1){
		log_sys_err("Ошибка поиска таблицы замен в каталоге " XSLT_FOLDER ":");
		regfree(&reg);
		return -1;
	}else if (n == 0){
		log_err("В каталоге " XSLT_FOLDER " не найдена таблица замен.");
		logfree(&reg);
		if (names != NULL)
			free(names);
		return -1;
	}else if (n > 1)
		log_warn("В каталоге " XSLT_FOLDER " обнаружено более одной таблицы замен (%d). "
			"Будет использована таблица %s.", n, names[0]->d_name);
	ssize_t ret = -1;
	snprintf(path, ASIZE(patn), XSLT_FOLDER "/%s", names[0]->d_name);
	FILE *f = fopen(path, "r");
	if (f != NULL){
		char str[1024];
		string name, val;
		ret = 0;
		while (fgets(str, ASIZE(str), f) != NULL){
			if (!is_comment_str(str)){
				if (parse_subst_entry(str, name, val)){
					tbl.push_back(pair<string, string>(name, val));
					ret++;
				}
			}
		}
		if (fclose(f) != 0)
			log_sys_err("Ошибка закрытия файла замен %s:", path);
		log_dbg("Из файла замен %s прочитано %d строк.", path, ret);
	}else
		log_sys_err("Ошибка открытия файла замен %s для чтения:", path);
	free(names);
	regfree(&reg);
	return ret;
}

static vector<uint8_t> &ins_char(vector<uint8_t> &str, uint8_t ch, int pos)
{
	if ((pos == -1) || (pos == str.size()))
		str.push_back(ch);
	else if (static_cast<vector<string>::size_type>(pos) < str.size())
		str[pos] = ch;
	else if (static_cast<vector<string>::size_type>(pos) > str.size()){
		size_t n = pos - str.size();
		str.insert(str.cend(), n, 0x20);
		str.push_back(ch);
	}
	return str;
}

static vector<vector<uint8_t>>::iterator adjust_buf(vector<vector<uint8_t>> &buf,
	int &row, int &col)
{
	if (col > 79){
		row  += col / 80;
		col %= 80;
	}
	if (static_cast<vector<string>::size_type>(row) >= buf.size()){
		size_t m = row - buf.size();
		for (size_t i = 0; i <= m; i++)
			buf.push_back(vector<uint8_t>());
	}
	return buf.begin() + row;;
}

static void preprocess_data(vector<uint8_t> &data)
{
	vector<vector<uint8_t>> buf;
	buf.push_back(vector<uint8_t>());
	vector<vector<uint8_t>>::iterator str = buf.begin();
	int row = 0, col = 0, v = 0, n = 0;
	bool esc = false, pos = false;
	for (uint8_t b : data){
		if (esc){
			esc = false;
			if (b == KKT_POSN){
				v = n = 0;
				pos = true;
			}else{
				str = adjust_buf(buf, row, col);
				ins_char(*str, X_DLE, col++);
				ins_char(*str, b, col++);
			}
		}else if (pos){
			if (b == KKT_POSN_HPOS_RIGHT){
				if (n > 0)
					col += v;
				pos = false;
			}else if (b == KKT_POSN_HPOS_LEFT){
				if (n > 0){
					col -= v;
					if (col < 0)
						col = 0;
				}
				pos = false;
			}else if (b == KKT_POSN_HPOS_ABS){
				if (n > 0)
					col = v;
				pos = false;
			}else if (b == KKT_POSN_VPOS_FW){
				if (n > 0)
					row += (v + 1) / 2;
				pos = false;
			}else if (b == KKT_POSN_VPOS_BK){
				if (n > 0){
					row -= v / 2;
					if (row < 0)
						row = 0;
				}
				pos = false;
			}else if (b == KKT_POSN_VPOS_ABS){
				if (n > 0)
					row = (v + 1) / 2 - 1;
				pos = false;
			}else if (n < 3){
				if ((b >= 0x30) && (b <= 0x39)){
					v *= 10;
					v += b - 0x30;
					n++;
				}else
					pos = false;
			}else
				pos = false;
		}else if (is_escape(b))
			esc = true;
		else{
			if (b == KKT_LF)
				row++;
			else if (b == KKT_CR)
				col = 0;
			else{
				str = adjust_buf(buf, row, col);
				ins_char(*str, b, col++);
			}
		}
	}
	data.clear();
	for (str = buf.begin(); str != buf.end(); str++){
		str->push_back(KKT_CR);
		str->push_back(KKT_LF);
		data.insert(data.cend(), str->begin(), str->end());
	}
	char path[PATH_MAX];
	snprintf(path, ASIZE(path), XSLT_FOLDER "/scr.bin");
	int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (fd != -1){
		write(fd, data.data(), data.size());
		close(fd);
	}
}

#if 0
typedef struct _XsltErrorCtx {
	uint8_t recode;
	const vector<pair<string, string>> &subst_tbl;
} XsltErrorCtx;

tstring xslt_err;

static void XMLCDECL onXsltError(void *ctx, const char *fmt, ...) LIBXML_ATTR_FORMAT(2,3)
{
	XsltErrorCtx *ec = reinterpret_cast<XsltErrorCtx *>(ctx);
	va_list ap;
	va_start(ap, fmt);
	char buf[1024];
	int l = vsnprintf_s(buf, ASIZE(buf), _TRUNCATE, fmt, ap);
	va_end(ap);
	txt::CodePage cp = (ec->recode == RECODE_CP866) ? txt::CodePage::CP866 : txt::CodePage::Win1251;
	txt::recodeData(reinterpret_cast<LPBYTE>(buf), l, cp);
	string str(buf);
	for (const auto &p : ec->subst_tbl)
		txt::replace(str, p.first, p.second);
	int n = 0;
	for (string::const_reverse_iterator p = str.crbegin(); p != str.crend(); ++p, n++){
		if ((*p != '\r') && (*p != '\n'))
			break;
	}
	if (n > 0)
		str.resize(str.length() - n);
	if (!str.empty()){
		xslt_err.assign(ansi2tstr(str));
		log_err(xslt_err.c_str());
	}
}
#endif

static bool transform_xml(xmlDocPtr xml, const char *xslt_id, uint8_t *out, size_t &out_len,
	uint8_t recode)
{
	xslt_err.clear();
	bool ret = false, embedded = (strlen(xslt_id) != 2);
	char path[PATH_MAX];
	if (embedded)
		snprintf(path, ASIZE(path), XSLT_FOLDER "/%s.xsl", xslt_id);
	else{
		char pattern[128];
		snprintf(pattern, sizeof(pattern), "^T%s[0-9]{5}\\.[Xx][Ss][Ll]$", xslt_id);
		int rc = regcomp(&reg, pattern, REG_EXTENDED | REG_NOSUB);
		if (rc != REG_NOERROR){
			log_err("Ошибка компиляции регулярного выражения для %s: %d.",
				pattern, rc);
			return false;
		}
		struct dirent **names = NULL;
		int n = scandir(XSLT_FOLDER, &names, find_selector, alphasort);
		if (n == -1){
			log_sys_err("Ошибка поиска файла XSLT %s в каталоге " XSLT_FOLDER ":",
				xslt_id);
			regfree(&reg);
			return false;
		}else if (n == 0){
			log_err("В каталоге " XSLT_FOLDER " не найден файл XSLT %s.", xslt_id);
			logfree(&reg);
			if (names != NULL)
				free(names);
			return false;
		}else if (n > 1)
			log_warn("В каталоге " XSLT_FOLDER " обнаружено более одного файла XSLT %s (%d). "
				"Будет использован файл %s.", xslt_id, n, names[0]->d_name);
		snprintf(path, ASIZE(patn), XSLT_FOLDER "/%s", names[0]->d_name);
		free(names);
		regfree(&reg);
	}
	xsltStylesheetPtr xslt = xsltParseStylesheetFile((const xmlChar *)path);
	if (xslt != NULL){
		xmlDocPtr doc = xsltApplyStylesheet(xslt, xml, NULL);
		if (doc != NULL){
			snprintf(path, ASIZE(path), XSLT_FOLDER "/out.bin");
			int len = xsltSaveResultToFilename(path, doc, xslt, 0);
			if (len != -1){
				log_dbg("%d байт записано в файл %s.", len, path);
				int fd = open(path, O_RDONLY);
				if (fd != -1){
					char *buf = new char[len + 1];
					int rc = read(fd, buf, len);
					if (rc == len){
						buf[len] = 0;
						string str;
						str.assign(buf, buf + len);
						for (const auto &p : subst_tbl)
							replace(str, p.first, p.second);
						size_t data_len = str.size();
						if (data_len > out_len)
							data_len = out_len;
						else
							out_len = data_len;
						if (out_len > 0)
							memcpy(out, str.c_str(), out_len);
						ret = true;
					}else if (rc == -1)
						log_sys_err("Ошибка чтения из %s:", path);
					else
						log_err("Из %s прочитано %d байт вместо %d.", path, rc, len);
					delete [] buf;
				}else
					log_sys_err("Ошибка открытия файла %s для чтения:", path);
				if (close(f) != 0)
					log_sys_err("Ошибка закрытия файла %s:", path);
			}else
				log_err("Ошибка записи результата трансформации в файл %s.", path);
		}else
			log_err("Ошибка трансформации XML.");
	}else
		log_err("Ошибка разбора XSLT в %s.", path);
	return ret;
}

static bool store_embedded_xslt(const uint8_t *xslt, size_t xslt_len, const char *fname)
{
	if ((xslt == NULL) || (xslt_len == 0) || (fname == NULL))
		return false;
	bool ret = false;
	char path[PATH_MAX];
	snprintf(path, ASIZE(path), XSLT_FOLDER "/%s.xsl", fname);
	int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (fd != -1){
		ssize_t rc = write(fd, xslt, xslt_len);
		if (rc == xslt_len){
			log_dbg("Данные для трансформации записаны в %s.", path);
			ret = true;
		}else if (rc == -1)
			log_sys_err("Ошибка записи в %s:", path);
		else
			log_sys_err("В %s записано %d байт вместо %d:", path, rc, xslt_len);
		if (close(f) != 0)
			log_sys_err("Ошибка закрытия %s:", path);
	}
	return ret;
}

static char get_dst_char(int dst)
{
	char ret ='?';
	switch (dst){
		case dst_text:
			ret = X_SCR;
			break;
		case dst_log:
			ret = 'E';
			break;
		case dst_xprn:
			ret = X_XPRN;
			break;
		case dst_kprn:
			ret = X_KPRN;
			break;
		case dst_lprn:
			ret = X_TPRN;
			break;
	}
	return ret;
}

static const uint8_t RECODE_NONE	= 0x30;
static const uint8_t RECODE_CP866	= 0x31;

enum class TransformType {
	None,
	Embedded,
	Prn,
	Explicit,
};

uint8_t *check_xml(uint8_t *p, size_t l, int dst, int *ecode,
	uint8_t *data_buf, size_t *data_buf_len, uint8_t *scr_buf, size_t *scr_buf_len)
{
#define set_ecode(e)		\
	if (ecode != NULL)	\
		*ecode = e
	if (data_buf_len != NULL)
		*data_buf_len = 0;
	if (scr_buf_len != NULL)
		*scr_buf_len = 0;
	int idx = 0;
	if (l < 9){
		set_ecode(E_XML_SHORT);
		return p + idx;
	}
	*ecode = E_OK;
	uint8_t recode = p[idx];
	if ((recode != RECODE_NONE) && (recode != RECODE_CP866)){
		set_ecode(E_XML_RECODE);
		return p + idx;
	}
	idx++;
	TransformType scr_transform = TransformType::None;
	char scr_transform_idx[3] = {0, 0, 0};
	if (p[idx] == 0x2a){
		switch (p[idx + 1]){
			case 0x2a:
				scr_transform = TransformType::None;
				break;
			case 0x50:
				scr_transform = TransformType::Prn;
				break;
			case 0x45:
				scr_transform = TransformType::Embedded;
				break;
			default:
				set_ecode(E_XML_SCR_TRANSFORM_TYPE);
				return p + idx + 1;
		}
	}else if (((p[idx] == 0x30) && (p[idx + 1] == 0x30)) ||
			((p[idx] == 0x5a) && (p[idx + 1] == 0x5a))){
		set_ecode(E_XML_SCR_TRANSFORM_TYPE);
		return p + idx;
	}else{
		scr_transform_idx[0] = p[idx];
		scr_transform_idx[1] = p[idx + 1];
		scr_transform = TransformType::Explicit;
	}
	idx += 2;
	TransformType prn_transform = TransformType::None;
	char prn_transform_idx[3] = {0, 0, 0};
	if (p[idx] == 0x2a){
		switch (p[idx + 1]){
			case 0x2a:
			case 0x3f:
				prn_transform = TransformType::None;
				break;
			case 0x45:
				prn_transform = TransformType::Embedded;
				break;
			default:
				set_ecode(E_XML_PRN_TRANSFORM_TYPE);
				return p + idx + 1;
		}
	}else if (((p[idx] == 0x30) && (p[idx + 1] == 0x30)) ||
			((p[idx] == 0x5a) && (p[idx + 1] == 0x5a))){
		set_ecode(E_XML_SCR_TRANSFORM_TYPE);
		return p + idx;
	}else{
		prn_transform_idx[0] = p[idx];
		prn_transform_idx[1] = p[idx + 1];
		prn_transform = TransformType::Explicit;
	}
	if ((scr_transform == TransformType::Prn) && (prn_transform == TransformType::None))
		scr_transform = TransformType::None;
	idx += 2;
	uint8_t *scr_xslt = NULL;
	uint16_t scr_xslt_len = read_hex_word(p + idx);
	if (number_error){
		set_ecode(E_XML_XSLT_LEN);
		return p + idx;
	}else if ((idx + scr_xslt_len) > l){
		set_ecode(E_XML_XSLT_SHORT);
		return p + idx;
	}
	idx += 4;
	if (scr_xslt_len > 0){
		scr_xslt = new uint8_t[scr_xslt_len];
		memcpy(scr_xslt, p + idx, scr_xslt_len);
		idx += scr_xslt_len;
	}else if (scr_transform == TransformType::Embedded){
		set_ecode(E_XML_NO_SCR_TRANSFORM);
		return p + idx;
	}
	uint8_t *prn_xslt = NULL;
	uint16_t prn_xslt_len = read_hex_word(p + idx);
	if (number_error){
		set_ecode(E_XML_XSLT_LEN);
		return p + idx;
	}else if ((idx + prn_xslt_len) > l){
		set_ecode(E_XML_XSLT_SHORT);
		return p + idx;
	}
	idx += 4;
	if (prn_xslt_len > 0){
		prn_xslt = new uint8_t[prn_xslt_len];
		memcpy(prn_xslt, p + idx, prn_xslt_len);
		idx += prn_xslt_len;
	}else if (prn_transform == TransformType::Embedded){
		set_ecode(E_XML_NO_PRN_TRANSFORM);
		return p + idx;
	}
	static char xml_hdr[] = "<?XML VERSION=\"1.0\" ENCODING=\"KOI-7\"?>";
	const uint16_t XML_HDR_LEN = ASIZE(xml_hdr) - 1;
	uint16_t xml_len = read_hex_word(p + idx);
	if (number_error){
		set_ecode(E_XML_LEN);
		return p + idx;
	}else if ((xml_len < XML_HDR_LEN) || ((idx + xml_len) > l)){
		set_ecode(E_XML_SHORT);
		return p + idx;
	}
	idx += 4;
	uint8_t *xml_ptr = memmem(p + idx, xml_len, xml_hdr, XML_HDR_LEN);
	if (xml_ptr == NULL){
		set_ecode(E_XML_HDR);
		return p + idx;
	}
	ssize_t xml_offs = xml_ptr - p; xml_idx = xml_offs - idx;
	if ((xml_idx + XML_HDR_LEN) >= xml_len){
		set_ecode(E_XML_SHORT);
		return p + idx;
	}
	xml_len -= xml_idx + XML_HDR_LEN;
	if (xml_len == 0){
		set_ecode(E_XML_SHORT);
		return p + idx;
	}
	idx += xml_idx + XML_HDR_LEN;
	char *xml0 = new char[xml_len + 1];
	memcpy(xml0, p + idx, xml_len);
	xml0[xml_len] = 0;
	idx += xml_len;
	if (recode == RECODE_CP866)
		recode_str(xml0, -1);
	xmlDocPtr xml_doc = NULL;
	uint8_t *out_buf = NULL;
	size_t out_buf_len = 0;
	const size_t MAX_DATA_LEN = 65536;
	char hdr[256];
	string xml_scr, xml_prn;
	if ((scr_transform != TransformType::None) && (scr_transform != TransformType::Prn)){
		snprintf(hdr, ASIZE(hdr), "<?xml version=\"1.0\" encoding=\"%s\"?>\r\n"
				"<A H=\"%d\" W=\"%d\" T=\"S%c\" D=\"P\">\r\n",
			(recode == RECODE_NONE) ? "us-ascii" : "cp866",
			(scr_mode == m32x8) ? 8  : 20,
			(scr_mode == m32x8) ? 32 : 80,
			get_dst_char(dst));
		xml_scr.assign(hdr);
		xml_scr += xml0;
		xml_scr += "</A>";
		for (const auto &p : pre_subst_tbl)
			replace(xml_scr, p.first, p.second);
	}
	if (prn_transform != TransformType::None){
		snprintf(hdr, ASIZE(hdr), "<?xml version=\"1.0\" encoding=\"%s\"?>\r\n"
				"<A H=\"%d\" W=\"%d\" T=\"P%c\" D=\"P\">\r\n",
			(recode == RECODE_NONE) ? "us-ascii" : "cp866",
			(scr_mode == m32x8) ? 8  : 20,
			(scr_mode == m32x8) ? 32 : 80,
			get_dst_char(dst));
		xml_prn.assign(hdr);
		xml_prn += xml0;
		xml_prn += "</A>";
		for (const auto &p : pre_subst_tbl)
			txt::replace(xml_prn, p.first, p.second);
	}
	const char *scr_xslt_id = NULL, *prn_xslt_id = NULL;
	if (prn_transform == TransformType::Embedded){
		if (store_embedded_xslt(prn_xslt, prn_xslt_len, "pscr"))
			prn_xslt_id = "pscr";
	}else if (prn_transform == TransformType::Explicit)
		prn_xslt_id = prn_transform_idx;
	if (scr_transform == TransformType::Embedded){
		if (store_embedded_xslt(scr_xslt, scr_xslt_len, "escr"))
			scr_xslt_id = "escr";
	}else if (scr_transform == TransformType::Prn)
		scr_xslt_id = prn_xslt_id;
	else if (scr_transform == TransformType::Explicit)
		scr_xslt_id = scr_transform_idx;
	if ((scr_xslt_id == NULL) && (scr_transform != TransformType::None)){
		set_ecode(E_XML_SCR_TRANSFORM_TYPE);
		return p + 1;
	}else if ((prn_xslt_id == NULL) && (prn_transform != TransformType::None)){
		set_ecode(E_XML_PRN_TRANSFORM_TYPE);
		return p + 1;
	}
	out_buf = new uint8_t[MAX_DATA_LEN];
	if (prn_transform == TransformType::None){
		if ((data_buf != NULL) && (data_buf_len != NULL) && (dst != dst_log)){
			if (xml_len < *data_buf_len)
				*data_buf_len = xml_len;
			memcpy(data_buf, xml0, *data_buf_len);
		}
	}else{
		xml_doc = xmlReadMemory(xml_prn.c_str(), xml_prn.size(), NULL, NULL, XML_PARSE_COMPACT);
		if (xml_doc != NULL){
			log_dbg("XML для печати из ответа успешно разобран.");
			out_buf_len = MAX_DATA_LEN;
			bool rc = transform_xml(xml_doc, prn_xslt_id, out_buf, out_buf_len, recode);
			xmlFreeDoc(xml_doc);
			if (!rc){
				set_ecode(E_XML_PRN_TRANSFORM);
				return p + xml_offs;
			}
		}else{
			xmlErrorPtr err = xmlGetLastError();
			log_err("Ошибка разбора XML для печати: %d (%s).", err->code, err->message);
			set_ecode(E_XML_PRN_PARSE);
			return p + xml_offs;
		}
/*		try {
			uint32_t dummy_idx = 0;
			parseInternalRecursive(out_buf, dummy_idx, out_buf_len);
		} catch (SYNTAX_ERROR err){
			onSyntaxError(err, idx);
		}*/
	}
	if (scr_transform == TransformType::None){
		if ((scr_buf != NULL) && (scr_buf_len != NULL)){
			if (xml_len < *scr_buf_len)
				*scr_buf_len = xml_len;
			memcpy(scr_buf, xml0, *scr_buf_len);
		}
	else if (scr_transform == TransformType::Prn){
		log_dbg("На экране будет отображён результат трансформации для принтера.");
/*		_scr_data.assign(out_buf, out_buf + out_buf_len);
		preprocess_data(_scr_data);*/
	}else{
		xml_doc = xmlReadMemory(xml_scr.c_str(), xml_scr.size(), NULL, NULL, XML_PARSE_COMPACT);
		if (xml_doc != NULL){
			log_dbg("XML для экрана из ответа успешно разобран.");
			out_buf_len = MAX_DATA_LEN;
			bool rc = transform_xml(xml_doc, scr_xslt_id, out_buf, out_buf_len, recode);
			xmlFreeDoc(xml_doc);
			if (rc){
/*				_scr_data.assign(out_buf, out_buf + out_buf_len);
				preprocess_data(_scr_data);*/
			}else{
				set_ecode(E_XML_SCR_TRANSFORM);
				return p + xml_offs;
			}
		}else{
			xmlErrorPtr err = xmlGetLastError();
			log_err("Ошибка разбора XML для экрана: %d (%s).", err->code, err->message);
			set_ecode(E_XML_SCR_PARSE);
			return p + xml_offs;
		}
	}
	if ((data_buf != NULL) && (data_buf_len != NULL) &&
			(scr_buf != NULL) && (scr_buf_len != NULL) &&
			(prn_transform == TransformType::None) && (dst() == ParaDst::log)){
		if (*scr_buf_len < *data_buf_len)
			*data_buf_len = *scr_buf_len;
		memcpy(data_buf, scr_buf, *scr_buf_len);
	}
	if (scr_xslt != NULL)
		delete [] scr_xslt;
	if (prn_xslt != NULL)
		delete [] prn_xslt;
	delete [] xml0;
	if (out_buf != NULL)
		delete [] out_buf;
}
