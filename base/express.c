/*
 * Синтаксический разбор и обработка текста ответа из "Экспресс-3".
 * (c) gsr 2009-2020, 2022, 2024.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/dialog.h"
#include "gui/scr.h"
#include "kkt/fd/ad.h"
#include "kkt/kkt.h"
#include "kkt/xml.h"
#include "log/express.h"
#include "log/local.h"
#include "log/pos.h"
#include "prn/aux.h"
#include "prn/express.h"
#include "prn/local.h"
#include "x3data/grids.h"
#include "x3data/icons.h"
#include "x3data/patterns.h"
#include "x3data/xslts.h"
#include "bscsym.h"
#include "express.h"
#include "hash.h"
#include "keys.h"
#include "numbers.h"
#include "sterm.h"
#include "termlog.h"
#include "tki.h"
#include "transport.h"
#include "xml2data.h"

/* Тип текущего запроса */
int req_type = req_regular;

static uint32_t log_number;		/* номер текущей записи на КЛ при обработке ответа */
uint32_t log_para = 0;			/* номер абзаца ответа на ЦКЛ при обработке ответа */

/* Описание кода синтаксической ошибки ответа */
const char *get_syntax_error_txt(uint8_t code)
{
	static struct {
		uint8_t code;
		const char *txt;
	} map[] = {
		{2,	"Нет конца текста."},
		{4,	"Повторная или вложенная команда записи\000"
			"ключей в ОЗУ ключей\000."},
		{5,	"Внешняя команда записи информации в ДЗУ.\000"},
		{6,	"В тексте присутствует команда конца\000"
			"записи в ДЗУ, но отсутствует команда\000"
			"начала записи.\000"},
		{7,	"Вложенная команда обработки вывода\000"
			"на внешнее устройство.\000"},
		{8,	"Нет конца записи информации в ДЗУ.\000"},
		{9,	"Команда преобразования символов\000"
			"расположена в ключах или до появления\000"
			"команды вывода информации на внешнее\000"
			"устройство.\000"},
		{10,	"Несуществующая команда преобразования\000"
			"(Ар2V).\000"},
		{11,	"Команда преобразования не соответствует\000"
			"внешнему устройству.\000"},
		{15,	"Команда внутри ОЗУ ключей.\000"},
		{16,	"Команды работы со штрих-кодом должны быть\000"
			"первыми в абзаце.\000"},
		{17,	"Отсутствуют параметры работы с БСО\000"
			"для ППУ.\000"},
		{18,	"Нет команды отрезки БСО для ППУ.\000"},
		{35,	"Попытка вывода на несуществующее\000"
			"устройство.\000"},
		{40,	"Переполнение ОЗУ ключей.\000"},
		{50,	"Переполнение ДЗУ.\000"},
		{70,	"Нет конца абзаца.\000"},
		{71,	"Не окончена запись в ОЗУ ключей.\000"},
		{72,	"Не окончена запись в ДЗУ.\000"},
		{73,	"В принятом технологическом тексте нет\000"
			"команд обработки, связанных с указанием\000"
			"внешнего устройства, на которое\000"
			"производится вывод информации.\000"},
		{77,	"Ошибка ключей -- длина поля при ветвлении\000"
			"ключей информации более 10 символов.\000"},
		{78,	"Ошибка в разделителе ключей.\000"},
		{79,	"Ошибка в концевых разделителях ключей.\000"},
		{80,	"Обращение к ОЗУ констант или ДЗУ\000"
			"по несуществующему адресу.\000"},
		{81,	"Внутри команд обработки, занесенных\000"
			"в ДЗУ, содержится несуществующая команда.\000"},
		{82,	"Слишком длинный абзац.\000"},
		{83,	"Длина прикладного текста после обработки\000"
			"меньше либо равна 0.\000"},
		{86,	"Ошибка в команде штрихового кода.\000"},
		{87,	"Неверное расположение команды\000"
			"переключения разрешения экрана.\000"},
		{89,	"Неверный операнд команды переключения\000"
			"разрешения.\000"},
		{90,	"Неверный формат абзаца для работы с ИПТ.\000"},
		{91,	"Терминал не настроен для работы с ИПТ.\000"},
		{98,	"В начале абзаца для ДПУ должна стоять\000"
			"команда длины бланка.\000"},
		{99,	"Неизвестная ошибка.\000"},
	};
	int i;
	for (i = 0; i < ASIZE(map); i++){
		if ((map[i].code == code) || (i == (ASIZE(map) - 1)))
			return map[i].txt;
	}
	return NULL;	/* только если ASIZE(map) == 0 */
}

/* Информация для ИПТ */
struct bank_data bd;

/* Очистка банковской информации */
void clear_bank_info(void)
{
	memset(&bd, 0, sizeof(bd));
}

/* Получение информации о банковском абзаце */
ssize_t get_bank_info(struct bank_info *items, size_t nr_items)
{
	if ((items == NULL) || (nr_items == 0))
		return -1;
	size_t ret = bd.nr_docs;
	if (ret > nr_items)
		ret = nr_items;
	for (size_t i = 0; i < nr_items; i++){
		struct bank_info *p = items + i;
		p->req_id = bd.req_id;
		strncpy(p->term_id, bd.term_id, BNK_TERM_ID_LEN);
		p->term_id[BNK_TERM_ID_LEN] = 0;
		p->op = bd.op;
		p->ticket = bd.ticket;
		p->repayment = bd.repayment;
		strncpy(p->prev_blank_nr, bd.prev_blank_nr, BNK_BLANK_NR_LEN);
		p->prev_blank_nr[BNK_BLANK_NR_LEN] = 0;
		strncpy(p->blank_nr, bd.doc_info[i].blank_nr, BNK_BLANK_NR_LEN);
		p->blank_nr[BNK_BLANK_NR_LEN] = 0;
		p->amount = bd.doc_info[i].amount;
	}
	return ret;
}

/* Проверка атрибутов символа (Ар2 V) */
static bool check_attr(uint8_t bg, uint8_t fg)
{
/* Бит D6 всегда 1 */
	if (((bg & 0x40) == 0) || ((fg & 0x40) == 0))
		return false;
	return ((bg == 0x70) && (fg == 0x70)) ||	/* отмена выделения */
		((bg >= 0x40) && (bg <= 0x47) && (fg >= 0x40) && (fg <= 0x47));
}

/* Проверка команды автоповтора символа (Ар2 S) */
static bool check_repeat(uint8_t x)
{
	return (x <= 0x7f) && (x & 0x40);
}

/* Проверка команды штрихового кода */
static uint8_t *check_bcode(uint8_t *p, int l, int *ecode)
{
	enum {
		st_start,
		st_number1,
		st_semi1,
		st_number2,
		st_semi2,
		st_number3,
		st_stop
	};
	int i, n = 0, st = st_start;
	uint8_t b;
	*ecode = E_BCODE;
	if ((p == NULL) || (l <= 0))
		return p;
	for (i = 0; (i < l) && (st != st_stop); i++){
		b = p[i];
		switch (st){
			case st_start:
				if (b == ';')
					st = st_semi1;
				else if (isdigit(b)){
					st = st_number1;
					n = 2;
				}else
					return p + i;
				break;
			case st_semi1:
				if (b == ';')
					st = st_semi2;
				else if (isdigit(b)){
					n = 2;
					st = st_number2;
				}else
					return p + i;
				break;
			case st_semi2:
				if (isdigit(b)){
					n = 13;
					st = st_number3;
				}else
					return p + i;
				break;
			case st_number1:
				n--;
				if (n == 0){
					if (isdigit(b)){
						st = st_number2;
						n = 2;
					}else if (b == ';')
						st = st_semi2;
					else
						return p + i;
				}else if (!isdigit(b))
					return p + i;
				break;
			case st_number2:
				n--;
				if (n == 0){
					if (isdigit(b)){
						st = st_number3;
						n = 13;
					}else
						return p + i;
				}else if (!isdigit(b))
					return p + i;
				break;
			case st_number3:
				n--;
				if (n == 0)
					st = st_stop;
				else if (!isdigit(b))
					return p + i;
				break;
		}
	}
	if (st == st_stop)
		*ecode = E_OK;
	return p + i - 1;
}

/* Проверка команды нанесения штрихового кода для ППУ (Ар2 0x1a) */
static uint8_t *check_bcode2(uint8_t *p, int l, int *ecode)
{
	enum {
		st_type,
		st_x,
		st_y,
		st_number,
		st_len,
		st_data,
		st_stop,
	};
	int i, n = 0, data_len = 0, st = st_type;
	uint8_t b;
	*ecode = E_BCODE;
	if ((p == NULL) || (l <= 0))
		return p;
	for (i = 0; (i < l) && (st != st_stop); i++){
		b = p[i];
		switch (st){
			case st_type:
				if (isdigit(b)){
					n = 0;
					st = st_x;
				}else
					return p + i;
				break;
			case st_x:
				if ((n == 0) && (b == 0x3b))
					st = st_number;
				else if (!isdigit(b))
					return p + i;
				else if (++n == 3){
					n = 0;
					st = st_y;
				}
				break;
			case st_y:
				if (!isdigit(b))
					return p + i;
				else if (++n == 3){
					n = 0;
					st = st_len;
				}
				break;
			case st_number:
				if ((b != 0x31) && (b != 0x32) && (b != 0x33))
					return p + i;
				n = 0;
				st = st_len;
				break;
			case st_len:
				if (!isdigit(b))
					return p + i;
				data_len *= 10;
				data_len += b - 0x30;
				if (++n == 3){
					if (data_len == 0)
						return p + i - 2;
					else{
						n = 0;
						st = st_data;
					}
				}
				break;
			case st_data:
				if (b < 0x30)
					return p + i;
				else if (++n == data_len)
					st = st_stop;
				break;
		}
	}
	if (st == st_stop)
		*ecode = E_OK;
	return p + i;
}

/*
 * Проверка команды нанесения штрихового кода для ППУ (Ар2 0x1a).
 * Формат команды нанесения штрих-кода:
 * 1b 1a LLL N T1 XXX1 YYY1 lll1 [T2 XXX2 YYY2 lll2 [...]] ddd...ddd
 * В данных могут быть либо собственно данные, либо обращение к СОЛЭБ (1b 19).
 * NB: Если ecode != NULL происходит синтаксическая проверка команды,
 * иначе -- её выполнение (в этом случае используются dst и dst_len).
 */
static uint8_t *check_kkt_bcode(uint8_t *p, size_t l, int *ecode,
	uint8_t *dst, uint32_t *dst_len)
{
	if (ecode != NULL)
		*ecode = E_UNKNOWN;
	if ((p == NULL) || (l == 0)){
		return p;
	}else if ((ecode == NULL) && ((dst == NULL) || (dst_len == NULL)))
		return p;
	if (ecode != NULL)
		*ecode = E_BCODE;
	off_t idx = 0;
	if (l < 15)		/* заголовок команды плюс хотя бы один байт данных */
		return p + idx;
/* Общая длина данных штрих-кодов */
	uint32_t total_len = 0;
	if (!read_uint(p + idx, 3, &total_len) || (total_len < 12) ||
			((idx + total_len + 3) > l))
		return p;
	idx += 3;
	l = idx + total_len;	/* будут обработаны только данные штрих-кода */
/* Количество штрих-кодов */
	uint8_t n = p[idx];
	if ((n < 0x31) || (n > 0x39))
		return p + idx;
	n -= 0x30;
	idx++;
	struct {
		uint8_t type;
		uint32_t x;
		uint32_t y;
		uint32_t len;
	} bcodes[n];
	memset(bcodes, 0, sizeof(bcodes));
/* Обработка каждого из штрих-кодов */
	for (uint8_t i = 0; i < n; i++){
//		off_t idx0 = idx;
		if ((idx + 11) > l)
			return p + idx;
/* Тип штрих-кода */
		uint8_t type = p[idx];
		if ((type < 0x30) || (type > 0x39))
			return p + idx;
		idx++;
/* Координаты штрих-кода */
		uint32_t x = 0, y = 0;
		if (p[idx] == 0x3b){
			x = UINT32_MAX;
			idx++;
		}else if (read_uint(p + idx, 3, &x))
			idx += 3;
		else
			return p + idx;
		if (p[idx] == 0x3b){
			y = UINT32_MAX;
			idx++;
		}else if (read_uint(p + idx, 3, &y))
			idx += 3;
		else
			return p + idx;
/* Длина штрих-кода (ПОСЛЕ всех обращений к СОЛЭБ, если таковые имеются) */
		uint32_t len = 0;
		if (!read_uint(p + idx, 3, &len))
			return p + idx;
		idx += 3;
		bcodes[i].type = type;
		bcodes[i].x = x;
		bcodes[i].y = y;
		bcodes[i].len = len;
	}
	size_t dst_idx = 0;
	for (int i = 0; i < ASIZE(bcodes) && (bcodes[i].len > 0); i++){
		typeof(*bcodes) *bc = bcodes + i;
		if ((bc->len > 2) && is_escape(p[idx]) && (p[idx + 1] == 0x19))
			break;
		else if (ecode != NULL){
			if ((idx + bc->len) > l)
				break;
		}else if ((dst_idx + 12 + bc->len) <= *dst_len){
			dst[dst_idx++] = KKT_ESC;
			dst[dst_idx++] = KKT_WR_BCODE;
			dst[dst_idx++] = bc->type;
			if (bc->x == UINT32_MAX)
				dst[dst_idx++] = 0x3b;
			else{
				write_uint(dst + dst_idx, bc->x, 3);
				dst_idx += 3;
			}
			if (bc->y == UINT32_MAX)
				dst[dst_idx++] = 0x3b;
			else{
				write_uint(dst + dst_idx, bc->y, 3);
				dst_idx += 3;
			}
			write_uint(dst + dst_idx, bc->len, 3);
			dst_idx += 3;
			memcpy(dst + dst_idx, p + idx, bc->len);
			dst_idx += bc->len;
			idx += bc->len;
			FILE *f = fopen("bcode.bin", "wb");
			if (f != NULL){
				fwrite(dst, dst_idx, 1, f);
				fclose(f);
			}
		}
	}
	if (dst_len != NULL)
		*dst_len = dst_idx;
	if (ecode != NULL)
		*ecode = E_OK;
	return p + 3 + total_len - 1;
}

static uint8_t *check_keys(uint8_t *keys, int l, int *ecode);
static int prom_total_len = 0;

/* Проверка данных для ДЗУ */
static uint8_t *check_prom(uint8_t *txt, int l, int *ecode, int dst)
{
	static uint8_t prom_end[] = {X_DLE, X_PROM_END};
	uint8_t *p, *pp, *eptr, n, b;
	int arr_len;
	if ((txt == NULL) || (l <= 0)){
		*ecode = E_NOEPROM;
		return NULL;
	}else
		*ecode = E_OK;
	p = txt;
	pp = memfind(p, l, prom_end, sizeof(prom_end));
	if (pp == NULL){
		*ecode = E_NOEPROM;
		return p;
	}
	arr_len = pp - p - 1;		/* was -3 ! */
	prom_total_len += arr_len;
	if (prom_total_len > PROM_BUF_LEN){
		*ecode = E_PROMOVER;
		return p;
	}
	n = *p++;
	if (!IS_HASH_ID(n)){
		*ecode = E_ADDR;
		return p - 1;
	}
	eptr = p + arr_len;
	while (p < eptr){
		if (*p == X_DLE){
			if ((p + 2) > eptr){
				*ecode = E_ARRUNK;
				return p;
			}
			b = p[1];
			p += 2;
			switch (b){
				case X_SWRES:	/* Ар2Ф не подлежит записи в ДЗУ */
					*ecode = E_MISPLACE;
					return p - 2;
/*				case XPRN_NO_BCODE:
					if (dst == dst_sprn){
						*ecode = E_MISPLACE;
						return p - 2;
					}*/		/* fall through */
				case XPRN_FONT:
				case XPRN_VPOS:
				case XPRN_PRNOP:
				case XPRN_WR_BCODE:
				case XPRN_RD_BCODE:
					if ((dst != dst_xprn) && (dst != dst_aprn) &&
							(dst != dst_lprn)){
						*ecode = E_MISPLACE;
						return p - 2;
					}
					if ((b == XPRN_WR_BCODE) || (b == XPRN_RD_BCODE)){
						pp = check_bcode(p, txt + l - p, ecode);
						if (*ecode != E_OK)
							return pp;
						else
							p = pp;
					}
					break;
				case LPRN_NO_BCODE:
					if (dst != dst_lprn){
						*ecode = E_MISPLACE;
						return p - 2;
					}
					break;
				case X_ATTR:
					if ((p + 2) > eptr){
						*ecode = E_ARRUNK;
						return p - 2;
					}else if (!check_attr(p[0], p[1])){
						*ecode = E_ILLATTR;
						return p;
					}else if (dst != dst_text){
						*ecode = E_MISPLACE;
						return p - 2;
					}
					p += 2;
					break;
				case X_REPEAT:
					if ((p + 2) > eptr){
						*ecode = E_ARRUNK;
						return p - 2;
					}else if (!check_repeat(p[1])){
						*ecode = E_ILLATTR;
						return p;
					}
					p += 2;
					break;
				case X_RD_ROM:
					if ((p + 1) > eptr){
						*ecode = E_ARRUNK;
						return p - 2;
					}else if (!in_hash(rom, *p)){
						*ecode = E_ADDR;
						return p;
					}
					p++;
					break;
				case X_KEY:
					p = check_keys(p, eptr - p, ecode);
					if (*ecode != E_OK)
						return p;
					break;
				default:
					*ecode = E_ARRUNK;
					return p - 2;
			}
		}else
			p++;
	}
	return p;
}

/* Проверка данных для ОЗУ ключей */
static uint8_t *check_keys(uint8_t *keys, int l, int *ecode)
{
#define N_MAX 10
#define MAX_LEVEL 3
	enum {
		kc_key,		/* kc -- key context */
		kc_key_id
	};
	static uint8_t delim[] = {0x5e, 0x5d, 0x5c, 0x5b};
	static uint8_t kend[] = {0x5e, 0x5e};
	int level = -1, n = 0, key_len;
	int context = kc_key;
	uint8_t *p, *pp, *_p, *ep = NULL, b;
	if ((keys == NULL) || (l <= 0)){
		*ecode = E_NOKEND;
		return NULL;
	}else
		*ecode = E_OK;
	p = keys;
	pp = memfind(p, l, kend, sizeof(kend));
	if (pp == NULL){
		*ecode = E_NOKEND;
		return p;
	}else
		ep = pp + 2;
/* Ключи можно записать в ДЗУ */
	if (((p + 2) <= ep) && (p[0] == X_DLE) && (p[1] == X_W_PROM)){
/* При записи в ДЗУ после ^^ стоит Ар2 ^ */
		if ((ep + 2) > (keys + l)){
			*ecode = E_NOEPROM;
			return keys + l - 1;
		}
		p += 2;
		_p = check_prom(p, ep + 2 - p, ecode, dst_keys);
		if (*ecode != E_OK)
			return _p;
		if (!in_hash(prom, *p))
			hash_set(prom, *p, (uint8_t *)" ");
		p++;
/*		pp -= 2;*/
	}
	key_len = pp - p + 1 /*-1*/;
	if (p[key_len - 1] != delim[0]){
		*ecode = E_KEYEDELIM;
		return p + key_len - 1;
	}else if (key_len > KEY_BUF_LEN){
		*ecode = E_KEYOVER;
		return p + KEY_BUF_LEN;
	}
	ep = p + key_len;
	while (p < ep){
		if (*p == X_DLE){
			if ((p + 2) > ep){
				*ecode = E_NOKEND;
				return ep;
			}
			b = p[1];
			switch (b){
				case X_KEY:
					*ecode = E_REPKEYS;
					break;
				case X_ATTR:
				case X_REPEAT:
					*ecode = E_DUMMYATTR;
					break;
				default:
					*ecode = E_CMDINKEYS;
					break;
			}
			return p;
		}else{
			uint8_t c = *p;
			if (is_key(c)){
				context = kc_key;
				n = 0;
			}else if (is_key_id(c)){
				if (context == kc_key){
					level++;
					if (level > MAX_LEVEL){
						*ecode = E_KEYDELIM;
						return p;
					}
				}
				context = kc_key_id;
				if (++n > N_MAX){
					*ecode = E_KEYID;
					return p;
				}
			}else if (is_key_delim(c)){
				context = kc_key_id;
				if (c != delim[level]){
					if (level > 0)
						level--;
					if (c != delim[level]){
						*ecode = E_KEYDELIM;
						return p;
					}
				}
			}else{
				*ecode = E_UNKNOWN;
				return p;
			}
			p++;
		}
	}
	return ep + 1;
}

/* Проверка абзаца для ОЗУ констант */
static uint8_t *check_rom(uint8_t *txt, int l, int *ecode)
{
	uint8_t *p, *eptr, b;
	if ((txt == NULL) || (l <= 0)){
		*ecode = E_NOPEND;
		return NULL;
	}else
		*ecode = E_OK;
	p = txt;
	eptr = txt + l;
	while (p < eptr){
		if (*p == X_DLE){
			if ((p + 2) > eptr){
				*ecode = E_NOPEND;
				return eptr - 1;
			}
			b = p[1];
			p += 2;
			switch (b){
				case X_PARA_END:
				case X_PARA_END_N:
					return p;
				default:
					*ecode = E_UNKNOWN;
					return p - 2;
			}
		}else if (*p == HASH_DELIM){
			if ((p + 1) > eptr){
				*ecode = E_NOPEND;
				return eptr - 1;
			}
			p++;
			if (!IS_HASH_ID(*p)){
				*ecode = E_ADDR;
				return p;
			}else
				if (!in_hash(rom, *p))
					hash_set(rom, *p, (uint8_t *)" ");
		}else
			p++;
	}
	*ecode = E_NOPEND;
	return p;
}

/* Проверка банковского абзаца */
static uint8_t *check_bank_info(uint8_t *txt, int l, int *ecode)
{
	enum {
		st_req_id,
		st_term_id,
		st_op_type,
		st_ticket,
		st_reissue,
		st_reissue_nr,
		st_blank_nr,
		st_amount_quot,
		st_amount_rem,
		st_doc_end,
		st_stop,
		st_err,
	};
	int i, st = st_req_id, n = 0, m = 0;
	uint8_t b;
	*ecode = E_BANK;
	for (i = 0; (i < l) && (st != st_stop) && (st != st_err); i++){
		b = txt[i];
		switch (st){
			case st_req_id:
				if (n < BNK_REQ_ID_LEN){
					if (!isdigit(b))
						st = st_err;
				}else if (n == BNK_REQ_ID_LEN){
					if (b == ';'){
						n = 0;
						st = st_term_id;
					}else
						st = st_err;
				}
				if (st == st_req_id)
					n++;
				break;
			case st_term_id:
				if (n == BNK_TERM_ID_LEN){
					if (b == ';'){
						n = 0;
						st = st_op_type;
					}else
						st = st_err;
				}
				if (st == st_term_id)
					n++;
				break;
			case st_op_type:
				if (n == 0){
					if ((b != '+') && (b =! '-') && (b != '*'))
						st = st_err;
				}else if (b == ';'){
					n = 0;
					st = st_ticket;
				}else
					st = st_err;
				if (st == st_op_type)
					n++;
				break;
			case st_ticket:
				if (n == 0){
					if ((b != 'p') && (b != 'u'))
						st = st_err;
				}else if (b == ';'){
					n = 0;
					st = st_reissue;
				}
				if (st == st_ticket)
					n++;
				break;
			case st_reissue:
				if (n == 0){
					if (b == '1'){
						n = 0;
						st = st_reissue_nr;
					}else if (b == ';')
						st = st_blank_nr;
					else if (b != '0')
						st = st_err;
				}else if (b == ';')
					st = st_blank_nr;
				else
					st = st_err;
				if (st == st_reissue)
					n++;
				break;
			case st_reissue_nr:
				if (n == 0){
					if (b != '(')
						st = st_err;
				}else if (n < (BNK_BLANK_NR_LEN + 1)){
					if (!isdigit(b) && ((i < 2) || (b > 4)))
						st = st_err;
				}else if (n == (BNK_BLANK_NR_LEN + 1)){
					if (b != ')')
						st = st_err;
				}else if (b == ';'){
					n = m = 0;
					st = st_blank_nr;
				}else
					st = st_err;
				if (st == st_reissue_nr)
					n++;
				break;
			case st_blank_nr:
				if (n < BNK_BLANK_NR_LEN){
					if (!isdigit(b))
						st = st_err;
				}else if (b == '/'){
					n = 0;
					st = st_amount_quot;
				}else
					st = st_err;
				if (st == st_blank_nr)
					n++;
				break;
			case st_amount_quot:
				if (isdigit(b)){
					if (n == BNK_AMOUNT_MAX_LEN)
						st = st_err;
					else if ((i + 1) == l)
						st = st_stop;
				}else if (b == '.'){
					if ((n > 0) && ((n + 1) < BNK_AMOUNT_MAX_LEN))
						st = st_amount_rem;
					else
						st = st_err;
				}else{
					i--;
					st = st_doc_end;
				}
				if (st == st_amount_quot)
					n++;
				break;
			case st_amount_rem:
				if (isdigit(b)){
					if (n == BNK_AMOUNT_MAX_LEN)
						st = st_err;
					else if ((i + 1) == l)
						st = st_stop;
				}else{
					i--;
					st = st_doc_end;
				}
				if (st == st_amount_rem)
					n++;
				break;
			case st_doc_end:
				if (b == ';'){
					m++;
					if (m < BNK_MAX_DOCS){
						n = 0;
						st = st_blank_nr;
					}else
						st = st_err;
				}else
					st = st_err;
				break;
		}
	}
	if (st == st_stop)
		*ecode = E_OK;
	return txt + i;
}

/* Сканирование банковского абзаца (синтаксис уже проверен) */
static void scan_bank_info(uint8_t *txt)
{
	clear_bank_info();
	int idx = 0;
/* Номер заказа в системе */
	size_t len;
	read_var_uint(txt, &len, BNK_REQ_ID_LEN, &bd.req_id);
	idx += BNK_REQ_ID_LEN + 1;
/* Технологический номер терминала */
	memcpy(bd.term_id, txt + idx, BNK_TERM_ID_LEN);
	bd.term_id[BNK_TERM_ID_LEN] = 0;
	idx += BNK_TERM_ID_LEN + 1;
/* Тип платежа */
	bd.op = txt[idx];
	idx += 2;
/* ОД/ПВД */
	bd.ticket = txt[idx] == 'p';
	idx += 2;
/* Признак переоформления */
	if (txt[idx] == '0'){
		bd.repayment = '0';
		idx += 2;
	}else if (txt[idx] == '1'){
		bd.repayment = '1';
		idx += 2;
		memcpy(bd.prev_blank_nr, txt + idx, BNK_BLANK_NR_LEN);
		bd.prev_blank_nr[BNK_BLANK_NR_LEN] = 0;
		idx += BNK_BLANK_NR_LEN + 2;
	}else{
		bd.repayment = 0;
		idx++;
	}
/* Информация о документах в заказе */
	for (int i = 0; i < BNK_MAX_DOCS; i++){
		if (!isdigit(txt[idx]))
			break;
		struct bank_doc_info *di = bd.doc_info + i;
		memcpy(di->blank_nr, txt + idx, BNK_BLANK_NR_LEN);
		di->blank_nr[BNK_BLANK_NR_LEN] = 0;
		idx += BNK_BLANK_NR_LEN + 1;
		uint64_t q = 0, r = 0;
		bool quot = true;
		for (int j = 0; j < BNK_AMOUNT_MAX_LEN; j++){
			uint8_t b = txt[idx++];
			if (isdigit(b)){
				b -= 0x30;
				if (quot){
					q *= 10;
					q += b;
				}else{
					r *= 10;
					r += b;
				}
			}else if (b == '.')
				quot = false;
			else if (b == ';')
				break;
		}
		idx++;
		if (r < 10)
			r *= 10;
		di->amount = q * 100 + r;
		bd.nr_docs++;
	}
}

/* Определение назначения абзаца ответа */
static int get_dest(uint8_t b)
{
	int ret = dst_none;
	switch (b){
		case X_SCR:
			ret = dst_text;
			break;
		case X_XPRN:
			ret = dst_xprn;
			break;
		case X_APRN:
			ret = dst_aprn;
			break;
		case X_SPRN:
			ret = dst_sprn;
			break;
		case X_KPRN:
			ret = dst_kprn;
			break;
		case X_OUT:
			ret = dst_out;
			break;
		case X_QOUT:
			ret = dst_qout;
			break;
		case X_ROM:
			ret = dst_hash;
			break;
		case X_KEY:
			ret = dst_keys;
			break;
		case X_CLEAR:
			ret = dst_sys;
			break;
		case X_BANK:
			ret = dst_bank;
			break;
		case X_KKT:
			ret = dst_kkt;
			break;
		case X_WR_LOG:
			ret = dst_log;
			break;
	}
	return ret;
}

static int para_len(int offset)
{
	int i, l = 0;
	bool dle = false;
	uint8_t p_end[] = {
		X_PARA_END_N,
		X_SCR,
		X_XPRN,
		X_APRN,
		X_SPRN,
		X_KPRN,
		X_OUT,
		X_QOUT,
		X_ROM,
		X_KEY,
		X_PARA_END,
		X_REQ,
		X_WR_LOG,
	};
	if ((offset == -1) || (offset >= (text_offset + text_len)))
		return 0;
	for (i = offset; i < (text_offset + text_len); i++, l++){
		if (dle){
			dle = false;
			if (memchr(p_end, resp_buf[i], sizeof(p_end)) != NULL){
				l--;
				break;
			}
		}else
			dle = is_escape(resp_buf[i]);
	}
	return l;
}

/* Проверка абзаца для ДПУ */
static uint8_t *check_aprn(uint8_t *txt, int l, int *ecode)
{
	enum {
		st_start,
		st_dle,
		st_digit,
		st_ok,
	};
	int i, st = st_start, n = 0;
	uint8_t b;
	for (i = 0; i < l; i++){
		b = txt[i];
		if (b == APRN_NUL)
			continue;
		switch (st){
			case st_start:
				if (b == APRN_DLE)
					st = st_dle;
				else{
					*ecode = E_LBL_LEN;
					return txt + i;
				}
				break;
			case st_dle:
				if (b == APRN_LBL_LEN){
					st = st_digit;
					n = 3;
				}else{
					*ecode = E_LBL_LEN;
					return txt + i;
				}
				break;
			case st_digit:
				if ((b >= 0x30) && (b <= 0x39)){
					if (!--n)
						st = st_ok;
				}else{
					*ecode = E_LBL_LEN;
					return txt + i + n - 3;
				}
				break;
			case st_ok:
				*ecode = E_OK;
				return txt + i;
		}
	}
	*ecode = E_LBL_LEN;
	return txt + i;
}

/* Проверка абзаца для ППУ */
static uint8_t *check_lprn(uint8_t *txt, int l, int *ecode)
{
	enum {
		st_start,
		st_dle,
		st_data,
		st_data_dle,
		st_ok,
		st_err,
	};
	int i, st = st_start;
	uint8_t b, b1 = 0;
	*ecode = E_OK;
	for (i = 0; (i < l) && (st != st_err) && (st != st_ok); i++){
		b = txt[i];
		switch (st){
			case st_start:
				if (b == LPRN_DLE)
					st = st_dle;
				else{
					*ecode = E_NO_LPRN_CMD;
					st = st_err;
				}
				break;
			case st_dle:
				if ((b == LPRN_RD_BCODE) ||
						(b == LPRN_NO_BCODE))
					st = st_data;
				else{
					*ecode = E_NO_LPRN_CMD;
					st = st_err;
				}
				break;
			case st_data:
				if (b == X_DLE)
					st = st_data_dle;
				else
					b1 = b;
				break;
			case st_data_dle:
				if ((b == X_PARA_END_N) || (b == X_PARA_END)){
					if ((b1 == LPRN_FORM_FEED) ||
							(b1 == LPRN_FORM_FEED1))
						st = st_ok;
					else{
						txt += i - 1;
						*ecode = E_NO_LPRN_CUT;
						st = st_err;
					}
				}else
					st = st_data;
				break;
		}
	}
	return txt;
}

/* Проверка абзаца для ККТ */
static uint8_t *check_kprn(uint8_t *txt, int l, int *ecode)
{
	enum {
		st_data,
		st_dle,
		st_ok,
		st_err,
	};
	int i, st = st_data;
	uint8_t b, b1 = 0;
	*ecode = E_OK;
	for (i = 0; (i < l) && (st != st_err) && (st != st_ok); i++){
		b = *txt++;
		switch (st){
			case st_data:
				if (b == KKT_ESC)
					st = st_dle;
				else
					b1 = b;
				break;
			case st_dle:
				if ((b == X_PARA_END_N) || (b == X_PARA_END)){
					if ((b1 == KKT_FF) || (b1 == KKT_END_BLOCK))
						st = st_ok;
					else{
						*ecode = E_NO_LPRN_CUT;	/* FIXME */
						st = st_err;
					}
				}else
					st = st_data;
				break;
		}
	}
	if ((st != st_ok) && (st != st_err))
		*ecode = E_NOPEND;
	return txt;
}


/* Проверка XML для ККТ */
static uint8_t *check_kkt_xml(uint8_t *txt, int l, int *ecode)
{
	uint8_t *ret = txt;
	*ecode = E_OK;
	int len = -1;
	bool esc = false;
	for (int i = 0; i < l; i++){
		uint8_t b = txt[i];
		if (esc){
			if ((b == X_PARA_END) || (b == X_PARA_END_N))
				len = i - 1;
			else{
				*ecode = E_KKT_XML;
				ret = txt + i - 1;
			}
			break;
		}else
			esc = b == X_DLE;
	}
	if (*ecode == E_OK){
		if (len == -1){
			ret = txt + l;
			*ecode = E_NOPEND;
		}else if (len > TEXT_BUF_LEN)
			*ecode = E_BIGPARA;
	}
	if (*ecode != E_OK)
		return ret;
	memcpy(text_buf, txt, len);
	text_buf[len] = 0;
	parse_kkt_xml((const char *)text_buf, true, kkt_xml_callback, ecode);
	printf("%s: ecode = %.2x\n", __func__, *ecode);
	if (*ecode == E_OK)
		ret = txt + len + 2;
	else
		ret = txt;
	return ret;
}

/* Проверка абзаца ответа */
static uint8_t *check_para(uint8_t *txt, int l, int *ecode, int n_para)
{
	uint8_t *p, *pp, *eptr, b;
	int dst = get_dest(txt[-1]);
	bool has_warray = false;
	if ((txt == NULL) || (l <= 0) || (ecode == NULL) || (n_para < 0) || (n_para >= MAX_PARAS))
		return NULL;
	*ecode = E_OK;
	if (para_len(txt - resp_buf) > TEXT_BUF_LEN){
		*ecode = E_BIGPARA;
		return txt;
	}
	if (dst == dst_aprn)
		p = check_aprn(txt, l, ecode);
	else if (dst == dst_lprn)
		p = check_lprn(txt, l, ecode);
	else if (dst == dst_kprn)
		return check_kprn(txt, l, ecode);
	else if (dst == dst_kkt)
		return check_kkt_xml(txt, l, ecode);
	else
		p = txt;
	if (*ecode != E_OK)
		return p;
	eptr = txt + l;
	while (p < eptr){
		if (is_escape(*p)){
			if ((p + 2) > eptr){
				*ecode = E_NOPEND;
				return eptr - 1;
			}
			b = p[1];
			p += 2;
			switch (b){
				case X_SWRES:
					if ((p + 1) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}else if (p > (txt + 2)){
						*ecode = E_SWPOS;
						return p - 2;
					}else if ((*p != 0x30) && (*p != 0x31)){
						*ecode = E_SWOP;
						return p;
					}
					break;
				case X_PARA_END:
				case X_PARA_END_N:
				case X_REQ:
					return p - 2;
				case X_SCR:
				case X_XPRN:
				case X_APRN:
				case X_SPRN:
				case X_KPRN:
				case X_OUT:
				case X_QOUT:
				case X_ROM:
				case X_KEY:
				case X_WR_LOG:
					*ecode = E_NESTEDOUT;
					return p - 2;
				case X_ATTR:
					if ((p + 2) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}else if (dst != dst_text){
						*ecode = E_MISPLACE;
						return p - 2;
					}else if (!check_attr(p[0], p[1])){
						*ecode = E_ILLATTR;
						return p - 2;
					}
					p += 2;
					break;
				case X_REPEAT:
					if ((p + 2) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}else if (dst == dst_keys){
						*ecode = E_MISPLACE;
						return p - 2;
					}else if (!check_repeat(p[1])){
						*ecode = E_ILLATTR;
						return p;
					}
					p += 2;
					break;
				case X_W_PROM:
					if ((p + 1) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}
					has_warray = true;
					pp = check_prom(p, txt + l - p, ecode, dst);
					if (*ecode != E_OK)
						return pp;
					if (!in_hash(prom, *p))
						hash_set(prom, *p, (uint8_t *)" ");
					p = pp;
					break;
				case X_RD_PROM:
					if ((p + 1) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}else if (!in_hash(prom, *p)){
						*ecode = E_ADDR;
						return p;
					}
					p++;
					break;
				case X_RD_ROM:
					if ((p + 1) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}else if (!in_hash(rom, *p)){
						*ecode = E_ADDR;
						return p;
					}
					p++;
					break;
				case XPRN_FONT:
				case XPRN_VPOS:
					if ((p + 1) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}else if (dst != dst_xprn){
						*ecode = E_MISPLACE;
						return p - 2;
					}		/* fall through */
				case XPRN_AUXLNG:
				case XPRN_MAINLNG:
				case XPRN_PRNOP:
				case XPRN_WR_BCODE:
				case XPRN_RD_BCODE:
				case XPRN_NO_BCODE:
					if ((dst != dst_xprn) && (dst != dst_aprn)){
						if ((b == XPRN_NO_BCODE) ||
								((dst != dst_lprn) &&
								 (dst != dst_log))){
							*ecode = E_MISPLACE;
							return p - 2;
						}
					}
					if ((b == XPRN_RD_BCODE) || (b == XPRN_NO_BCODE)){
						if (p > (txt + 2)){
							*ecode = E_LCODEINBODY;
							return p - 2;
						}
						pp = check_bcode(p, txt + l - p, ecode);
						if (*ecode != E_OK)
							return pp;
						else
							p = pp;
					}
					break;
				case LPRN_INTERLINE:
				case LPRN_NO_BCODE:
				case LPRN_WR_BCODE2:
					if ((dst != dst_lprn) && (dst != dst_log)){
						*ecode = E_MISPLACE;
						return p - 2;
					}
					if (b == LPRN_WR_BCODE2){
//						pp = check_bcode2(p, txt + l - p, ecode);
						pp = check_kkt_bcode(p, txt + l - p, ecode, NULL, NULL);
						if (*ecode != E_OK)
							return pp;
						else
							p = pp;
					}
					break;
				case X_DELAY:
					if ((p + 2) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}
					break;
				case X_PROM_END:
					if (!has_warray){
						*ecode = E_NOSPROM;
						return p - 2;
					}else
						has_warray = false;
					break;
				case X_XML:
					printf("%s: n_para = %d\n", __func__, n_para);
					pp = check_xml(p, txt + l - p, dst, ecode, get_xml_data(n_para));
					if (*ecode != E_OK)
						return pp;
					else
						p = pp;
					break;
				default:
					*ecode = E_UNKNOWN;
					return p - 1;
			}
		}else
			p++;
	}
	*ecode = E_NOPEND;
	return eptr - 1;
}

/* Проверка синтаксиса ответа */
uint8_t *check_syntax(uint8_t *txt, int l, int *ecode)
{
	bool keys = false;
	bool has_dst = false;
	int n_dsts = 0, n_para = 0;
	bool has_clear = false;
	uint8_t *p = txt, *eptr = txt + l, b;
	*ecode = E_OK;
	prom_total_len = 0;
	if (txt == NULL)
		return NULL;
	if (l <= 0){
		*ecode = E_SHORT;
		return p;
	}
	while (p < eptr){
		if (is_escape(*p)){
			if ((p + 2) > eptr){
				*ecode = E_NOPEND;
				return eptr - 1;
			}
			b = p[1];
			p += 2;
			switch (b){
				case X_DELAY:
					if ((p + 2) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}
					p += 2;
					break;
				case X_CLEAR:
					if (has_clear){
						*ecode = E_DBLCLEAR;
						return p - 2;
					}else{
						has_clear = true;
						if (p > (txt + 2)){
							*ecode = E_CLEAR;
							return p - 2;
						}
						n_para++;
					}
					break;
				case X_80_20:
					break;
				case X_SWRES:
					if ((p + 1) > eptr){
						*ecode = E_NOPEND;
						return eptr - 1;
					}else if ((*p != 0x30) && (*p != 0x31)){
						*ecode = E_SWOP;
						return p;
					}
					p++;
					break;
				case X_KEY:
					if (keys){
						*ecode = E_REPKEYS;
						return p - 2;
					}
					keys = true;
					p = check_keys(p, txt + l - p, ecode);
					if (*ecode != E_OK)
						return p;
					n_dsts++;
					break;
				case X_XPRN:
#if !defined __FAKE_PRINT__
					if (!cfg.has_xprn /*|| (wm == wm_local)*/){
						*ecode = E_NODEVICE;
						return p;
					}
#endif
					goto main_chk;	/* I'm sorry :-) */
				case X_APRN:
					if (!cfg.has_aprn /*|| (wm == wm_local)*/){
						*ecode = E_NODEVICE;
						return p;
					}
					goto main_chk;
				case X_BANK:
					if (!cfg.bank_system){
						*ecode = E_NO_BANK;
						return p - 2;
					}
					p = check_bank_info(p, txt + l - p, ecode);
					if (*ecode != E_OK)
						return p;
					n_dsts++;
					break;
				case X_KPRN:
				case X_KKT:
					if (!cfg.has_kkt || (kkt == NULL)){
						*ecode = E_NO_KKT;
						return p - 2;
					}else if (!cfg.fiscal_mode){
						*ecode = E_KKT_NONFISCAL;
						return p - 2;
					}
					goto main_chk;
				case X_WR_LOG:
/*					if (wm != wm_local){
						*ecode = E_NODEVICE;
						return p - 2;
					}*/		/* fall through */
				case X_SCR:
				case X_OUT:
				case X_QOUT:
main_chk:				has_dst = true;
					p = check_para(p, txt + l - p, ecode, n_para);
					if (*ecode != E_OK)
						return p;
					n_dsts++;
					break;
				case X_ROM:
					has_dst = true;
					p = check_rom(p, txt + l - p, ecode);
					if (*ecode != E_OK)
						return p;
					n_dsts++;
					break;
				case X_PARA_END:
				case X_PARA_END_N:
				case X_REQ:
					has_dst = false;
					n_para++;
					break;
				case X_REPEAT:
				case X_ATTR:
					if (!has_dst){
						*ecode = E_DUMMYATTR;
						return p - 2;
					}
					break;
				default:
					if (!has_dst){
						*ecode = E_UNKDEVICE;
						return p - 2;
					}
			}
		}else if (has_dst)
			p++;
		else{
			if (((p + 1) == eptr) && (*p == BSC_ETX))
				p++;
			else{
				*ecode = E_UNKDEVICE;
				return p;
			}
		}
	}
	if (n_dsts == 0){
		*ecode = E_UNKDEVICE;
		return p;
	}
	return NULL;
}

/* Обнуление "карты" ответа */
static void reset_resp_map(void)
{
	int i;
	for (i = 0; i < ASIZE(map); i++){
		map[i].offset = -1;
		map[i].dst = dst_none;
		map[i].can_print = false;
		map[i].jump_next = false;
		map[i].auto_handle = false;
		map[i].handled = true;		/* sic */
		map[i].log_handled = false;
		map[i].jit_req = false;
		map[i].delay = 0;
		map[i].scr_mode = m_undef;
	}
	n_paras = 0;
}

/* Создание "карты" ответа */
int make_resp_map(void)
{
	int i, n, dst;
	uint8_t *p = resp_buf + text_offset;
	struct para_info *pi = map;
	bool dle = false;
	bool auto_handle = false;
	bool next_para = false;
	bool first_print = true;
	reset_resp_map();
	set_resp_mode(cfg.scr_mode);
	for (i = n = 0; (i <= text_len) && (n < MAX_PARAS); i++){
		if (next_para){
			next_para = false;
			if (((pi->dst == dst_xprn) || (pi->dst == dst_aprn) ||
					(pi->dst == dst_lprn) || (pi->dst == dst_kprn)) &&
					first_print){
				first_print = false;
				pi->auto_handle = false;
			}else
				pi->auto_handle = auto_handle;
			auto_handle = pi->jump_next;
			pi++;
			n++;
		}
		if (i == text_len)
			break;
		if (is_escape(p[i]))
			dle = true;
		else if (dle){
			dle = false;
			dst = get_dest(p[i]);
			if (dst != dst_none){
				if ((dst != dst_sys) && (dst != dst_xprn) &&
						(dst != dst_aprn) &&
						(dst != dst_lprn) && (dst != dst_kprn))
					first_print = false;
				if (pi->dst == dst_none){
					pi->dst = dst;
					if ((dst == dst_xprn) || (dst == dst_aprn) ||
							(dst == dst_lprn) || (dst == dst_kprn))
						pi->can_print = true;
					pi->offset = text_offset + i + 1;
				}else{
					next_para = true;
					pi->jump_next = (dst != dst_xprn) &&
						(dst != dst_aprn) &&
						(dst != dst_lprn) && (dst != dst_kprn);
					i -= 2;
				}
			}else{
				switch (p[i]){
					case X_80_20:
						set_resp_mode(m80x20);
/*						set_scr_mode(m80x20, true, false);*/
						break;
					case X_SWRES:
						i++;
						if (p[i] == 0x30)
							pi->scr_mode = m32x8;
						else if (p[i] == 0x31)
							pi->scr_mode = m80x20;
						break;
					case X_DELAY:
						if (n > 0){
							i++;
							map[n - 1].delay = read_hex_byte(p + i);
						}
						i++;
						break;
					case X_PARA_END_N:
						pi->jump_next = true;
						first_print = false;
						__fallthrough__;
					case X_PARA_END:
					case X_REQ:
						next_para = true;
						pi->jit_req = (p[i] == X_REQ);
						if (pi->dst == dst_none){
							pi->dst = dst_sys;
							pi->offset = text_offset + i - 1;
						}
				}
			}
		}
	}
	if ((n > 0) && (n < MAX_PARAS)){
		pi = map;
		if ((pi->dst == dst_xprn) || (pi->dst == dst_aprn) ||
				(pi->dst == dst_lprn) || (pi->dst == dst_kprn)){
			for (i = n; i > 0; i--)
				map[i] = map[i - 1];
			shr_xml_data();
//			memmove(map + 1, map, n * sizeof(*map));
			pi->dst = dst_sys;
			pi->jump_next = false;
			pi->auto_handle = false;
			pi->handled = false;
			pi->log_handled = false;
			pi->jit_req = false;
			pi->delay = 0;
			n++;
		}
	}
	for (i = 0; i < n; i++)
		map[i].handled = false;
	return n;
}

/* Пометить все абзацы как обработанные */
void mark_all(void)
{
	int i;
	for (i = 0; i < ASIZE(map); i++)
		map[i].handled = true;
}

/* Пометить все абзацы для печати как обработанные */
static void mark_prn(void)
{
#if !defined __DEBUG_PRINT__
	for (int i = 0; i < n_paras; i++)
		if ((map[i].dst == dst_xprn) || (map[i].dst == dst_aprn) ||
				(map[i].dst == dst_lprn) || (map[i].dst == dst_kprn))
			map[i].can_print = false;
#endif
}

/* Число необработанных абзацев */
int n_unhandled(void)
{
	int i, n;
	for (i = n = 0; i < ASIZE(map); i++){
		if (!map[i].handled)
			n++;
	}
	return n;
}

/* Можно ли вывести абзац на экран */
bool can_show(int dst)
{
	return	(dst == dst_text) || (dst == dst_xprn) ||
		(dst == dst_aprn) || (dst == dst_lprn) || (dst == dst_kprn) ||
		(dst == dst_out) || (dst == dst_log);
}

/* Можно ли вывести абзац на принтер */
bool can_print(int n)
{
	return !map[n].handled && ((map[n].dst == dst_xprn) ||
			(map[n].dst == dst_aprn) || (map[n].dst == dst_lprn) ||
			(map[n].dst == dst_kprn));
}

/* Можно ли остановиться на заданном абзаце */
bool can_stop(int n)
{
	return ((map[n].dst != dst_sys) && (map[n].dst != dst_keys) &&
		(map[n].dst != dst_bank) && (map[n].dst != dst_kkt) &&
		(map[n].dst != dst_log)) || !map[n].handled;
}

/* Обработаны ли все предыдущие абзацы */
bool prev_handled(void)
{
	bool ret = true;
	for (int i = 0; i < cur_para; i++){
		if (!map[i].handled){
			ret = false;
			break;
		}
	}
	return ret;
}

/* Перейти к следующему абзацу */
int next_para(int n)
{
	return (n + 1) % n_paras;
}

/* Индекс следующего абзаца для вывода на печать */
int next_printable(void)
{
	int i;
	for (i = 0; map[i].offset != -1; i++)
		if (can_print(i))
			return i;
	return -1;
}

/* Проверка наличия КТ в тексте ответа */
static bool check_etx(void)
{
	size_t i, k;
	for (i = k = 0; i < sizeof(resp_buf); i++){
		if (resp_buf[i] == BSC_ETX){
			resp_len = i + 1;
			return true;
		}else if (resp_buf[i] != 0)
			k = i;
	}
	ecode = E_NOETX;
	if (k < (sizeof(resp_buf) - 1))
		k++;
	err_ptr = resp_buf + k;
	n_paras = 0;
	show_error();
	return false;
}

/* Проверка ответа, введенного вручную, а также ОЗУ ключей */
bool check_raw_resp(void)
{
	if (scr_text_modified){
		if (cur_buf == resp_buf){
			if (!check_etx() || !prepare_raw_resp())
				return false;
			clear_hash(prom);
			err_ptr = check_syntax(resp_buf + text_offset, text_len, &ecode);
			clear_hash(prom);
			if (err_ptr == NULL){
				n_paras = make_resp_map();
				mark_prn();
				return true;
			}else{
				show_error();
				n_paras = 0;
			}
		}else if (cur_buf == keys){
			rollback_keys(true);
			return true;
		}
	}
	return false;
}

/* Многопроходный разбор */
#define MAX_PARSE_STEPS		100
int handle_para(int n_para)
{
	bool prn_dst = n_para < 0;
	if (n_para < 0)
		n_para += MAX_PARAS;
	static uint8_t buf[TEXT_BUF_LEN];
	struct para_info *pi = map + n_para;
	uint8_t *p, *eptr, id;
	uint32_t i, n, m = 0, l = 0, ll;
	bool dle = false;
	if ((pi == NULL) || (pi->offset == -1))
		return 0;
	else if (pi->dst == dst_sys)
		return 0;
	l = para_len(pi->offset);
	if ((l + 1) > TEXT_BUF_LEN)
		return 0;
	memset(buf, 0, TEXT_BUF_LEN);
	memcpy(buf, resp_buf + pi->offset, l);
	memset(text_buf, 0, sizeof(text_buf));
	eptr = buf + l;
	do {
		n = 0;
		p = buf;
		i = 0;
		while (p < eptr){
			if (is_escape(*p))
				dle = true;
			else if (dle){
				dle = false;
				switch (*p){
					case X_RD_ROM:
						if ((p + 1) >= eptr)
							return 0;
						id = p[1];
						ll = hash_get_len(rom, id);
						if ((i + ll + 1) > sizeof(text_buf))
							return 0;
						hash_get(rom, id, text_buf + i);
						i += ll;
						p++;
						n++;
						break;
					case X_W_PROM:
						if ((p + 2) >= eptr)
							return 0;
						if (!pi->handled)
/*
 * В конце buf всегда находится 0, поэтому hash_set в любом случае не выйдет
 * за границы buf при чтении данных.
 */
							hash_set(prom, p[1], p + 2);
						p++;
						n++;
						break;
					case X_PROM_END:
						break;
					case X_RD_PROM:
						if ((p + 1) >= eptr)
							return 0;
						id = p[1];
						ll = hash_get_len(prom, id);
						if ((i + ll + 1) > sizeof(text_buf))
							return 0;
						hash_get(prom, id, text_buf + i);
						i += ll;
						p++;
						n++;
						break;
					case X_REPEAT:
						if ((p + 2) >= eptr)
							return 0;
						ll = p[2] & 0x3f;
						if ((i + ll + 1) > sizeof(text_buf))
							return 0;
						memset(text_buf + i, p[1], ll);
						i += ll;
						p += 2;
						n++;
						break;
					case X_XML:
					{
						struct xml_data *xml_data = get_xml_data(n_para);
						if (xml_data == NULL)
							return 0;
						uint8_t *data = NULL;
						size_t data_len = 0;
						if (prn_dst && (xml_data->prn_data != NULL)){
							data = xml_data->prn_data;
							data_len = xml_data->prn_data_len;
						}else if (!prn_dst && (xml_data->scr_data != NULL)){
							data = xml_data->scr_data;
							data_len = xml_data->scr_data_len;
						}
						if (data == NULL)
							return 0;
						else if ((i + data_len) > sizeof(text_buf))
							return 0;
						if (data_len > 0){
							memcpy(text_buf + i, data, data_len);
							i += data_len;
							p += xml_data->cmd_len;
							n++;
						}
						break;
					}
					case LPRN_WR_BCODE2:
						if (m == 0){	/* штрих-код обрабатывается только один раз */
							ll = sizeof(text_buf) - i;
							p = check_kkt_bcode(p + 1, eptr - p - 1, NULL,
								text_buf + i, &ll);
							i += ll;
							break;
						}
						__fallthrough__;
					default:		/* Неизвестная команда */

						if ((i + 3) > sizeof(text_buf))
							return 0;
						text_buf[i++] = p[-1];
						text_buf[i++] = p[0];
						break;
				}
			}else if ((*p != BSC_RUS) && (*p != BSC_LAT)){
				if ((i + 1) > sizeof(text_buf))
					return 0;
				else
					text_buf[i++] = *p;
			}
			p++;
		}
		m++;
		text_buf[i] = 0;
		l = i;
		memset(buf, 0, TEXT_BUF_LEN);
		memcpy(buf, text_buf, l);
		eptr = buf + l;
	} while (n && (m < MAX_PARSE_STEPS));
	pi->log_handled = true;
	return l;
}

/* Синхронизация даты с хост-ЭВМ при инициализации */
static void sync_date(void)
{
	struct tm tm;
	time_t t;
	char dcookie[] = "kl`~i ", *p;
	bool lprn_sync_fail = false;
	p = (char *)memfind(resp_buf + text_offset, resp_len,
		(const uint8_t *)dcookie, sizeof(dcookie) - 1);
	if (p != NULL){
		p += strlen(dcookie);
		t = time(NULL);
		memcpy(&tm, localtime(&t), sizeof(tm));	/* is_dst */
		tm.tm_sec = 0;
		if (sscanf(p, " %d.%d.%d %d.%d",
				&tm.tm_mday, &tm.tm_mon, &tm.tm_year,
				&tm.tm_hour, &tm.tm_min) == 5){
			tm.tm_mon--;
			tm.tm_year += 100;
			time_t xt = mktime(&tm);
			if (xt > 0){
				time_delta = xt - time(NULL);
				kbd_reset_idle_interval();
				if ((wm == wm_local) && cfg.has_lprn &&
						cfg.has_sd_card){
					int ret = lprn_sync_time();
					if ((ret == LPRN_RET_ERR) ||
							((ret == LPRN_RET_OK) &&
							 (lprn_sd_status > 0x01)))
						lprn_sync_fail = true;
				}
#if defined __SYNC_KKT_RTC__
				if (cfg.has_kkt && (kkt != NULL))
					kkt_set_rtc(xt + cfg.tz_offs * 3600);
#endif
			}
		}
	}
	xlog_write_rec(hxlog, NULL, 0, XLRT_INIT, 0);
}

bool use_integrator = false;

static bool check_integrator(const uint8_t *data, size_t len)
{
	const uint8_t integrator[] = "<INTEGRATOR>";
	return memmem(data, len, integrator, sizeof(integrator) - 1) != NULL;
}

/* Предварительная обработка ответа */
static void preexecute_resp(void)
{
	int i, l, transition_flag = 0;
	bool no_print = true, init = TST_FLAG(ZBp, GDF_REQ_INIT);
	if (init){
		sync_date();
		if (!resp_executing)
			return;
	}
	for (i = log_para = 0; i < n_paras; i++){
		l = handle_para(i);
		switch (map[i].dst){
			case dst_keys:
				memset(keys, 0, KEY_BUF_LEN);
				strcpy((char *)keys, (char *)text_buf);
				if (i > 0)
					map[i - 1].jump_next = true;
				break;
			case dst_hash:
				hash_set_all(rom, text_buf);
				if (i > 0)
					map[i-1].jump_next = true;
				break;
			case dst_xprn:
			case dst_kprn:
			case dst_sprn:
				no_print = false;
				transition_flag = -1;
				log_number = xlog_write_rec(hxlog,
					text_buf, l, XLRT_NORMAL, log_para++);
				break;
			case dst_aprn:
				no_print = false;
				transition_flag = -1;
				log_number = xlog_write_rec(hxlog,
					text_buf, l, XLRT_AUX, log_para++);
				break;
			case dst_bank:
				scan_bank_info(text_buf);
				log_number = xlog_write_rec(hxlog, text_buf, l,
					XLRT_BANK, log_para++);
				break;
			case dst_log:
				no_print = false;
				log_number = xlog_write_rec(hxlog, text_buf, l,
					XLRT_NORMAL, log_para++);
				break;
			case dst_kkt:
				no_print = false;
				if (wm == wm_main){
					log_number = xlog_write_rec(hxlog, text_buf, l,
						XLRT_KKT, log_para);
					if (cfg.kkt_apc)
						xlog_set_rec_flags(hxlog, log_number, log_para,
							XLOG_REC_APC);
					log_para++;
				}
				break;
			case dst_text:
				if (init){
					bool old_use_integrator = use_integrator;
					use_integrator = check_integrator(text_buf, l);
					if (use_integrator != old_use_integrator)
						RedrawScr(false, get_main_title());
					log_dbg("use_integrator = %d.", use_integrator);
					check_x3_grids(text_buf, l);
					log_dbg("need_grids_update_xprn = %d; need_grids_update_kkt = %d.",
						need_grids_update_xprn(), need_grids_update_kkt());
					check_x3_icons(text_buf, l);
					log_dbg("need_icons_update_xprn = %d; need_icons_update_kkt = %d.",
						need_icons_update_xprn(), need_icons_update_kkt());
					check_x3_kkt_patterns(text_buf, l);
					log_dbg("need_patterns_update = %d.", need_patterns_update());
					check_x3_xslt(text_buf, l);
					log_dbg("need_xslt_update = %d.", need_xslt_update());
				}
				if (transition_flag != -1)
					transition_flag++;
				break;
		}
	}
	if (no_print && TST_FLAG(ZBp, GDF_REQ_APP))
		log_number = xlog_write_rec(hxlog, NULL, 0, XLRT_NOTIFY, 0);
	wm_transition_interactive = wm_transition && (transition_flag != 1);
}

/* Формирование текста сообщения об ошибке ППУ */
static const char *make_lprn_err_msg(int lprn_ret)
{
	char *ret = NULL;
	if (lprn_ret == LPRN_RET_OK){
		if (lprn_status == 0){
			if (lprn_sd_status > 0x01)
				ret = lprn_get_sd_error_txt(lprn_sd_status)->txt;
		}else
			ret = lprn_get_error_txt(lprn_status)->txt;
		if ((ret == NULL) && (lprn_media != LPRN_MEDIA_BLANK) &&
				(lprn_media != LPRN_MEDIA_BOTH))
			ret = "НЕВЕРН\x9bЙ НОСИТЕЛЬ";
	}else if (lprn_ret == LPRN_RET_ERR){
		if (lprn_timeout)
			ret = "ТАЙМАУТ";
		else
			ret = "ОБЩАЯ ОШИБКА ППУ";
	}
	return ret;
}

/* Вывод окна с сообщением об ошибке ППУ в случае возможности отказа от заказа */
static int show_lprn_error_rejectable(int lprn_ret)
{
	static struct custom_btn btns[] = {
		{
			.text	= "Повторная печать",
			.cmd	= cmd_print
		},
		{
			.text	= "Отказ от заказа",
			.cmd	= cmd_reject
		},
		{
			.text	= NULL,
			.cmd	= cmd_none
		}
	};
	static char msg[256];
	snprintf(msg, sizeof(msg), "При попытке печати на БСО произошла ошибка:\n"
		"%s\nВы можете повторить печать или отказаться от заказа, "
		"выбрав соответствующую кнопку.\n"
		"Для перемещения между кнопками используйте Tab.",
		make_lprn_err_msg(lprn_ret));
	return message_box("Ошибка ППУ", msg, (intptr_t)btns, 0, al_center);
}

/* Вывод окна с сообщением об ошибке ППУ в случае невозможности отказа от заказа */
static int show_lprn_error_nonrejectable(int lprn_ret)
{
	static struct custom_btn btns[] = {
		{
			.text	= "Повторная печать",
			.cmd	= cmd_print
		},
		{
			.text	= NULL,
			.cmd	= cmd_none
		}
	};
	static char msg[256];
	snprintf(msg, sizeof(msg), "При попытке печати на БСО произошла ошибка:\n"
		"%s\nВы можете повторить печать.",
		make_lprn_err_msg(lprn_ret));
	return message_box("Ошибка ППУ", msg, (intptr_t)btns, 0, al_center);
}

/* Вывод окна с сообщением об ошибке ППУ при печати на БСО */
static int show_lprn_error(int lprn_ret, bool rejectable)
{
	int ret;
	online = false;
/*	guess_term_state();
	push_term_info();
	hide_cursor();
	scr_visible = false;*/
	set_term_busy(true);
/*	ClearScreen(clBlack);*/
	ret = rejectable ? show_lprn_error_rejectable(lprn_ret) :
		show_lprn_error_nonrejectable(lprn_ret);
	online = true;
/*	pop_term_info();
	ClearScreen(clBtnFace);*/
	redraw_term(true, main_title);
	return ret;
}

/* Вывод окна с сообщением об ошибке ККППУ при печати на ленте */
static int show_kprn_error(uint8_t status, bool rejectable)
{
	struct custom_btn btns[3];
	int n = 0;
	btns[n].text = "Повторная печать";
	btns[n++].cmd = cmd_print;
	if (rejectable){
		btns[n].text = "Отказ от заказа";
		btns[n++].cmd = cmd_reject;
	}
	btns[n].text = NULL;
	btns[n].cmd = cmd_none;
	static char msg[256];
	if (rejectable)
		snprintf(msg, sizeof(msg), "При попытке печати на ККТ произошла ошибка:\n"
			"%hhx (%s)\nВы можете повторить печать или отказаться от заказа, "
			"выбрав соответствующую кнопку.\n"
			"Для перемещения между кнопками используйте Tab.",
			status, kkt_status_str(status));
	else
		snprintf(msg, sizeof(msg), "При попытке печати на ККТ произошла ошибка:\n"
			"%hhx (%s)\nВы можете повторить печать.",
			status, kkt_status_str(status));
	int ret;
	online = false;
	set_term_busy(true);
	ret = message_box("Ошибка ККТ", msg, (intptr_t)btns, 0, al_center);
	online = true;
	redraw_term(true, main_title);
	return ret;
}

static bool kprn_print(const uint8_t *data, size_t len)
{
	bool ret = false;
	while (true){
		uint8_t status = kkt_print_vf(data, len);
		can_reject = false;
		if (status == KKT_STATUS_OK){
			set_term_astate(ast_none);
			ret = true;
			break;
		}else{
			set_term_astate(ast_no_kkt);
			err_beep();
			int cmd = show_kprn_error(kkt_status, can_reject);
			if (cmd == cmd_reject){
				reject_req();
				break;
			}else if (cmd == cmd_reset)
				break;
		}
	}
	return ret;
}

/*
 * Обработка абзаца для печати. Возвращает false, если дальнейшая обработка
 * ответа невозможна.
 */
static bool execute_prn(struct para_info *p, int l, int n_para)
{
	bool ret = true, printed = false;
	set_term_astate(ast_none);
	if (p->dst == dst_xprn){
		if (cfg.has_xprn)
			ret = printed = xprn_print((char *)text_buf, l);
	}else if (p->dst == dst_aprn){
		if (cfg.has_aprn)
			ret = printed = aprn_print((char *)text_buf, l);
	}else if (p->dst == dst_kprn){
		if (cfg.has_kkt && (kkt != NULL))
			ret = printed = kprn_print(text_buf, l);
	}else{		/* dst_sprn */
#if defined INSERT_SPRN_CODE_HERE
		while (true){
			bool sent_to_prn = false;
			int lprn_ret = lprn_print_ticket(text_buf, l, &sent_to_prn);
			if (sent_to_prn)
				can_reject = false;
			if (lprn_ret == LPRN_RET_OK){
				if ((lprn_status == 0) && (!cfg.has_sd_card ||
						(lprn_sd_status <= 0x01))){
					if ((lprn_media == LPRN_MEDIA_BLANK) ||
							(lprn_media == LPRN_MEDIA_BOTH)){
						set_term_astate(ast_none);
						printed = true;
						break;
					}else
						set_term_astate(ast_lprn_ch_media);
				}else{
					if (sent_to_prn){
						if (cfg.has_sd_card && (lprn_sd_status > 0x01)){
							set_term_astate(ast_lprn_sd_err);
							llog_write_sd_error(hllog, lprn_sd_status, log_para++);
						}else{
							set_term_astate(ast_lprn_err);
							if (lprn_has_blank_number)
								llog_write_pr_error_bcode(hllog,
									lprn_status, lprn_blank_number,
									log_para++);
							else
								llog_write_pr_error(hllog, lprn_status,
									log_para++);
						}
						err_beep();
						lprn_error_shown = true;
						break;
					}
				}
			}else if (lprn_ret == LPRN_RET_ERR){
				if (sent_to_prn){
					if (lprn_timeout)
						llog_write_error(hllog, LLRT_ERROR_TIMEOUT, log_para++);
					else
						llog_write_error(hllog, LLRT_ERROR_GENERIC, log_para++);
					set_term_astate(ast_nolprn);
					err_beep();
					lprn_error_shown = true;
					break;
				}
			}
			if (lprn_ret == LPRN_RET_RST){
				ret = false;
				break;
			}else{
				int cmd = show_lprn_error(lprn_ret, can_reject);
				if (cmd == cmd_reject){
					reject_req();
					lprn_error_shown = false;
					ret = false;
					break;
				}else if (cmd == cmd_reset){
					ret = false;
					break;
				}
			}
		}
#endif		/* INSERT_SPRN_CODE_HERE */
	}
	if (printed){	/* FIXME */
		xlog_set_rec_flags(hxlog, log_number, n_para, XLOG_REC_PRINTED);
	}
	return ret;
}

/* Возвращает true, если в правой части строки статуса выводится сообщение об ошибке */
bool is_rstatus_error_msg(void)
{
	int errors[] = {
		ast_noxprn,
		ast_noaprn,
		ast_nolprn,
		ast_lprn_err,
		ast_lprn_ch_media,
		ast_lprn_sd_err,
		ast_rejected,
		ast_repeat,
		ast_nokey,
		ast_finit,
		ast_illegal,
		ast_error,
		ast_prn_number,
		ast_pos_error,
		ast_pos_need_init,
		ast_no_kkt,
	};
	int i;
	for (i = 0; i < ASIZE(errors); i++){
		if (_term_aux_state == errors[i])
			return true;
	}
	return false;
}

static void execute_kkt(struct para_info *pi __attribute__((unused)), int len)
{
	int err = E_OK;
	text_buf[len] = 0;
	parse_kkt_xml((const char *)text_buf, false, kkt_xml_callback, &err);
}

/* Получение информации банковского абзаца во время обработки ответа */
const struct bank_data *get_bi(void)
{
	const struct bank_data *ret = NULL;
	if (resp_executing){
		for (int i = 0; i < n_paras; i++){
			if (map[i].dst == dst_bank){
				ret = &bd;
				break;
			}
		}
	}
	return ret;
}

/* Получение данных изображения для БПУ */
bool find_pic_data(int *data, int *req)
{
	bool ret = false;
	*data = *req = -1;
	int n = 0, m = 0, pic_para = -1, req_para = -1;
	for (int i = 0; i < n_paras; i++){
		if (map[i].dst == dst_tprn){
			if (++n > 1)
				break;
			pic_para = i;
		}else if (map[i].dst == dst_out){
			if (++m > 1)
				break;
			req_para = i;
		}
	}
	if ((n == 1) && (pic_para != -1)){
		*data = pic_para;
		ret = true;
	}
	if ((m == 1) && (req_para != -1))
		*req = req_para;
	return ret;
}

/* Проверка необходимости синхронизация данных с "Экспресс-3" */
uint32_t need_x3_sync(void)
{
	uint32_t ret = X3_SYNC_NONE;
	if (need_grids_update_xprn())
		ret |= X3_SYNC_XPRN_GRIDS;
	if (need_grids_update_kkt())
		ret |= X3_SYNC_KKT_GRIDS;
	if (need_icons_update_xprn())
		ret |= X3_SYNC_XPRN_ICONS;
	if (need_icons_update_kkt())
		ret |= X3_SYNC_KKT_ICONS;
	if (need_patterns_update())
		ret |= X3_SYNC_KKT_PATTERNS;
	if (need_xslt_update())
		ret |= X3_SYNC_XSLT;
	return ret;
}

/* Обработка текста ответа. Возвращает false, если надо перейти к ОЗУ заказа */
bool execute_resp(void)
{
	set_term_state(st_resp);
	int n = n_unhandled();
	set_term_led(hbyte = n2hbyte(n));
	astate_for_req = ast_none;
	cur_para = -1;
	if (next_printable() == -1)
		can_reject = false;
	else
		can_reject = TST_FLAG(OBp, GDF_RESP_REJ_ENABLED);
	resp_executing = true;
	resp_printed = has_bank_data = has_kkt_data = false;
	lprn_error_shown = false;
	preexecute_resp();
	if (need_lock_term)
		return true;	/* не переключаться на ОЗУ заказа */
	int n_para = 0;		/* номер абзаца на ЦКЛ */
	int next = 0;
	struct para_info *p = NULL;
	bool parsed = false;
	bool jump_next = true;
	bool has_req = false;
	bool ndest_shown = false;
	while ((n > 0) && !has_req && resp_executing){
		int l = 0;
		if (jump_next && !lprn_error_shown){
			jump_next = false;
			for (int k = 0; k < n_paras; k++){
				cur_para = next_para(cur_para);
				if (prev_handled()){
					if (can_stop(cur_para))
						break;
				}else if (can_show(map[cur_para].dst))
					break;
			}
			parsed = false;
			ndest_shown = false;
		}
		if (!parsed && !lprn_error_shown){
			parsed = true;
			p = map + cur_para;
			l = handle_para(cur_para);
			if (can_show(p->dst)){
				if (p->scr_mode == m_undef)
					set_scr_text(text_buf, l, txt_rich, true);
				else{
					set_scr_text(text_buf, l, txt_rich, false);
					set_scr_mode(p->scr_mode, true, false);
				}
				show_dest(p->dst);
			}
		}else if (!ndest_shown && !lprn_error_shown){
			ndest_shown = true;
			show_ndest(next);
		}
		if (!p->handled && !lprn_error_shown && prev_handled()){
			switch (p->dst){
				case dst_xprn:
				case dst_aprn:
				case dst_lprn:
				case dst_kprn:
					if (p->auto_handle && (next_printable() == cur_para)){
						if (p->can_print){
							l = handle_para(n_para - MAX_PARAS);
							if (execute_prn(p, l, n_para++)){
								if ((p->dst == dst_xprn) || (p->dst == dst_kprn))
									resp_printed = true;
								ndest_shown = false;
							}else{
								lprn_error_shown = false;
								if (hbyte != HB_INIT)
									set_term_led(hbyte = n2hbyte(0));
								return resp_executing = false;
							}
						}
						can_reject = false;
						p->handled = true;
						n--;
					}
					break;
				case dst_out:
				case dst_qout:
					if (p->auto_handle){
						set_scr_request(text_buf, l, false);
						p->handled = true;
						n--;
						if (p->jit_req)
							has_req = prev_handled();
					}
					break;
				case dst_log:
					p->handled = true;
					n--;
					n_para++;
					break;
				case dst_kkt:{
					has_kkt_data = true;
					execute_kkt(p, l);
					struct AD_state ads;
					if (AD_get_state(&ads))
						xlog_set_rec_flags(hxlog, log_number, n_para, XLOG_REC_CPC);
					__fallthrough__;
				}
				case dst_bank:
					if (p->dst == dst_bank)
						has_bank_data = true;
					n_para++;
					__fallthrough__;
				default:
					p->handled = true;
					n--;
			}
			if (p->handled){
				next = next_para(cur_para);
				set_term_led(hbyte = n2hbyte(n));
				jump_next = p->jump_next;
				if ((p->delay > 0) && !term_delay(p->delay)){
					lprn_error_shown = false;
					return resp_executing = false;
				}
			}
		}
		switch (get_cmd(false, false)){
			case cmd_none:
				break;
			case cmd_view_req:
				show_req();
				break;
			case cmd_view_resp:
				jump_next = true;
				break;
			case cmd_print:
				if ((map[next].dst == dst_xprn) ||
						(map[next].dst == dst_aprn) ||
						(map[next].dst == dst_lprn) ||
						(map[next].dst == dst_kprn)){
					cur_para = next;
					map[cur_para].auto_handle = true;
					parsed = false;
				}else
					err_beep();
				break;
			case cmd_enter:
				if ((map[next].dst == dst_out) || (map[next].dst == dst_qout)){
					cur_para = next;
					map[cur_para].auto_handle = true;
					parsed = false;
				}else
					err_beep();
				break;
			case cmd_reject:
				if (can_reject){
					lprn_error_shown = false;
					reject_req();
					return resp_executing = false;
				}else{
/*					set_term_astate(ast_illegal);*/
					err_beep();
				}
				break;
			case cmd_switch_res:
				switch_term_mode();
				break;
			case cmd_pgup:
				cm_pgup(NULL);
				break;
			case cmd_pgdn:
				cm_pgdn(NULL);
				break;
			case cmd_reset:
				if (reset_term(false))
					return resp_executing = false;
				break;
			case cmd_continue:
				set_term_astate(ast_none);
				lprn_error_shown = false;
				break;
			default:
				err_beep();
		}
	}
	resp_executing = false;
	can_reject = false;
	lprn_error_shown = false;

	if (has_kkt_data)
		show_hints();

	bool ret = true;
	if (kt != key_none){
		if (!is_rstatus_error_msg())
			set_term_astate(ast_none);
		if (has_req && resp_handling)
			send_request();
		if (TST_FLAG(OBp, GDF_RESP_INIT))
			ret = false;
		else
			ret = (p != NULL) ? !p->jump_next : false;
	}
	return ret;
}
