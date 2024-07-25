/* Прикладной протокол "Экспресс". (c) gsr 2000-2004, 2009, 2018, 2020, 2024 */

#if !defined EXPRESS_H
#define EXPRESS_H

#if defined __cplusplus
extern "C" {
#endif

#include "genfunc.h"

/* Признак работы терминала через интегратор */
extern bool use_integrator;

/* Типы запроса */
enum {
	req_regular,		/* обычный запрос */
	req_pos_cheque,		/* запрос чека ИПТ */
	req_grid_xprn,		/* запрос разметки бланка для БПУ */
	req_grid_kkt,		/* запрос разметки бланка для ККТ */
	req_icon_xprn,		/* запрос пиктограммы для БПУ */
	req_icon_kkt,		/* запрос пиктограммы для ККТ */
	req_patterns,		/* запрос шаблона печати ККТ */
	req_xslt,		/* запрос таблицы трансформации XML */
	req_pos,		/* запрос данных для печати квитанции ИПТ */
};

/* Тип текущего запроса */
extern int req_type;

/* Команды "Экспресс" */
#define X_DLE		0x1b	/* Esc префикс команды */
#define X_WRITE		0x31	/* 1 запись */
#define X_READ_ALL	0x32	/* 2 читать полный буфер */
#define X_CLEAR		0x35	/* 5 стирание/запись */
#define X_READ		0x36	/* 6 читать буфер */
#define X_DELAY		0x42	/* B пауза */
#define X_RD_PROM	0x43	/* C чтение из ДЗУ */
#define X_PARA_END_N	0x44	/* D конец абзаца; перейти к следующему */
#define X_80_20		0x46	/* F перейти в режим 80x20 */
#define X_RD_ROM	0x48	/* H чтение из ОЗУ констант (ПЗУ) */
#define X_SCR		0x49	/* I абзац для отображения на экране */
#define X_KEY		0x4b	/* K абзац для ОЗУ ключей */
#define X_XPRN		0x4d	/* M абзац для ОПУ */
#define X_ROM		0x4e	/* N абзац для ОЗУ констант (ПЗУ) */
#define X_QOUT		0x4f	/* O абзац для ОЗУ заказа без отображения на экране */
#define X_SPRN		0x50	/* P абзац для БПУ */
#define X_KPRN		0x51	/* Q абзац для печати на ККТ */
#define X_XML		0x52	/* R данные в формате XML */
#define X_REPEAT	0x53	/* S автоповтор символа */
#define X_REQ		0x55	/* U конец абзаца для ОЗУ заказа */
#define X_ATTR		0x56	/* V атрибут символа для вывода на экран */
#define X_W_PROM	0x57	/* W запись в ДЗУ */
#define X_APRN		0x58	/* X абзац для ДПУ */
#define X_OUT		0x5a	/* Z абзац для ОЗУ заказа */
#define X_PARA_END	0x5c	/* \ конец абзаца */
#define X_PROM_END	0x5e	/* ^ конец данных для ДЗУ */
#define X_BANK		0x62	/* Б абзац для работы с ИПТ */
#define X_SWRES		0x66	/* Ф временное переключение формата экрана */
#define X_KKT		0x79	/*   абзац для ККТ */
#define X_WR_LOG	0x7c	/* Э запись на КЛ (ЦКЛ или ПКЛ) */

/* Синтаксические ошибки ответа */
#define E_OK		0x00	/* Нет ошибки */
#define E_NOETX		0x02	/* Нет ETX */
#define E_REPKEYS	0x04	/* Повторная команда записи в ОЗУ ключей */
#define E_XPROM		0x05	/* Внешняя команда записи в ДЗУ */
#define E_NOSPROM	0x06	/* Конец записи в ДЗУ без начала записи */
#define E_NESTEDOUT	0x07	/* Вложенные команды вывода на внешнее устройство */
#define E_NOEPROM	0x08	/* Нет конца записи в ДЗУ */
#define E_DUMMYATTR	0x09	/* Команда преобразования символов стоит в ключах или вне абзаца */
#define E_ILLATTR	0x10	/* Несуществующая команда преобразования */
#define E_MISPLACE	0x11	/* Несоответствие команды выбранному внешнему устройству */
#define E_DBLCLEAR	0x12	/* Повторная команда очистки */
#define E_CLEAR		0x14	/* Команда очистки не в начале ответа */
#define E_CMDINKEYS	0x15	/* Команда внутри ОЗУ ключей */
#define E_LCODEINBODY	0x16	/* Команды работы со штрих-кодом должны быть первыми в абзаце */
#define E_NO_LPRN_CMD	0x17	/* Отсутствуют параметры работы с БСО для ППУ */
#define E_NO_LPRN_CUT	0x18	/* Нет команды отрезки БСО для ППУ */
#define E_NODEVICE	0x35	/* Попытка вывода на несуществующее устройство */
#define E_KEYOVER	0x40	/* Переполнение ОЗУ ключей */
#define E_PROMOVER	0x50	/* Переполнение ДЗУ */
#define E_NOPEND	0x70	/* Нет конца абзаца */
#define E_NOKEND	0x71	/* Не окончена запись в ОЗУ ключей */
#define E_UNKDEVICE	0x73	/* Не указано внешнее устройство */
#define E_KEYID		0x77	/* Длина идентификатора ключей больше 10 символов */
#define E_KEYDELIM	0x78	/* Ошибка в разделителях ключей */
#define E_KEYEDELIM	0x79	/* Ошибка в концевых разделителях ключей */
#define E_ADDR		0x80	/* Обращение к ОЗУ констант или ДЗУ по несуществующему адресу */
#define E_ARRUNK	0x81	/* Несуществующая команда в ДЗУ или ОЗУ констант */
#define E_BIGPARA	0x82	/* Слишком длинный абзац */
#define E_SHORT		0x83	/* Длина прикладного текста после обработки меньше либо равна 0 */
#define E_BCODE		0x86	/* Ошибка в команде штрихового кода */
#define E_SWPOS		0x87	/* Неверное расположение команды переключения разрешения */
#define E_SWOP		0x89	/* Неверный операнд команды переключения разрешения */
#define E_BANK		0x90	/* Неверный формат абзаца для работы с ИПТ */
#define E_NO_BANK	0x91	/* Терминал не настроен для работы с ИПТ */
#define E_NO_KKT	0x92	/* ККТ отсутствует */
#define E_KKT_NONFISCAL	0x93	/* ККТ находится в нефискальном режиме */
#define E_KKT_XML	0x94	/* Ошибка в XML для ККТ */
#define E_KKT_ILL_ATTR	0x95	/* Неверное значение атрибута в XML для ККТ */
#define E_KKT_NO_ATTR	0x96	/* Отсутствует обязательный атрибут для ККТ */
#define E_LBL_LEN	0x98	/* В начале абзаца для ДПУ должна стоять команда длины бланка */
#define E_XML_SHORT			0xa0	/* Слишком короткий абзац с XML */
#define E_XML_RECODE			0xa1	/* Неверный идентификатор перекодировки */
#define E_XML_SCR_TRANSFORM_TYPE	0xa2	/* Неверный тип трансформации для экрана */
#define E_XML_PRN_TRANSFORM_TYPE	0xa3	/* Неверный тип трансформации для печати */
#define E_XML_XSLT_LEN			0xa4	/* Неверный формат длины встроенной таблицы трансформации */
#define E_XML_NO_SCR_TRANSFORM		0xa5	/* Отсутствует встроенная таблица трансформации для экрана */
#define E_XML_NO_PRN_TRANSFORM		0xa6	/* Отсутствует встроенная таблица трансформации для печати */
#define E_XML_LEN			0xa7	/* Неверный формат длины XML */
#define E_XML_HDR			0xa8	/* Не найден заголовок XML */
#define E_XML_PRN_PARSE			0xa9	/* Ошибка разбора XML для печати */
#define E_XML_PRN_TRANSFORM		0xaa	/* Ошибка преобразования XML для печати */
#define E_XML_SCR_PARSE			0xab	/* Ошибка разбора XML для экрана */
#define E_XML_SCR_TRANSFORM		0xac	/* Ошибка преобразования XML для экрана */
#define E_UNKNOWN	0xff	/* Неизвестная ошибка */

/* Биты возможностей терминала (передаются во втором байте идентификатора) */
#define TCAP_TERM	0x00	/* терминал */
#define TCAP_XSLT	0x01	/* поддержка ответа в формате XML/XSLT (печать шаблонами) */
#define TCAP_BNK2	0x02	/* обработка банковских абзацев нового типа */
#define TCAP_KKT	0x04	/* ККТ */
#define TCAP_EX_BCODE	0x08	/* БПУ поддерживает расширенные штрих-коды */
#define TCAP_NO_POS	0x10	/* нет ИПТ */
#define TCAP_UNIBLANK	0x20	/* печать на универсальном бланке */
#define TCAP_FPS	0x40	/* система быстрых платежей */
#define TCAP_MASK	0x7f

/* Устройство для вывода абзаца ответа */
enum {
	dst_none,	/*  0 */
	dst_sys,	/*  1 */
	dst_text,	/*  2 */
	dst_xprn,	/*  3 ОПУ */
	dst_tprn = dst_xprn,
	dst_aprn,	/*  4 ДПУ */
	dst_sprn,	/*  5 БПУ */
	dst_out,	/*  6 */
	dst_qout,	/*  7 */
	dst_hash,	/*  8 */
	dst_keys,	/*  9 */
	dst_bank,	/* 10 */
	dst_log,	/* 11 */
	dst_kkt,	/* 12 */
	dst_kprn,	/* 13 */
};

/* Информация об абзаце ответа */
struct para_info{
	int offset;		/* смещение начала текста абзаца в resp_buf */
	int dst;		/* устройство для вывода абзаца */
	bool can_print;		/* можно ли печатать данный абзац */
	bool jump_next;		/* можно ли автоматически перейти к обработке следующего абзаца */
	bool auto_handle;	/* можно ли автоматически обработать данный абзац */
	bool handled;		/* обработан ли абзац */
	bool log_handled;	/* для обработки Ар2X */
	bool jit_req;		/* можно ли отправить запрос после обработки данного абзаца */
	uint8_t delay;		/* пауза при обработке (АрB) */
	int scr_mode;		/* разрешение экрана (Ар2Ф) */
	int xml_idx;		/* индекс в таблице XML (-1, если записи для абзаца нет */
};

/* Информация из абзаца для ИПТ */
#define BNK_REQ_ID_LEN		7
#define BNK_TERM_ID_LEN		6
#define BNK_BLANK_NR_LEN	14
#define BNK_AMOUNT_MIN_LEN	1
#define BNK_AMOUNT_MAX_LEN	10
#define BNK_MAX_DOCS		4

/* Информация о документе в заказе */
struct bank_doc_info {
	char blank_nr[BNK_BLANK_NR_LEN + 1];		/* номер документа */
	uint64_t amount;				/* сумма в копейках */
};

/* Информация о банковском абзаце */
struct bank_data {
	uint32_t req_id;				/* номер заказа в системе */
	char term_id[BNK_TERM_ID_LEN + 1];		/* технологический номер терминала */
	char op;					/* тип платежа (-;+;*) */
	bool ticket;					/* ОД/ПВД */
	char repayment;					/* признак переоформления */
	char prev_blank_nr[BNK_BLANK_NR_LEN + 1];	/* номер предыдущего документа */
	struct bank_doc_info doc_info[BNK_MAX_DOCS];	/* информация о документах в заказе */
	size_t nr_docs;					/* количество документов в заказе */
};

/* Запись в банковской корзине */
struct bank_info {
	uint32_t req_id;				/* номер заказа в системе */
	char term_id[BNK_TERM_ID_LEN + 1];		/* технологический номер терминала */
	char op;					/* тип платежа (-;+;*) */
	bool ticket;					/* ОД/ПВД */
	char blank_nr[BNK_BLANK_NR_LEN + 1];		/* номер документа */
	char repayment;					/* признак переоформления */
	char prev_blank_nr[BNK_BLANK_NR_LEN + 1];	/* номер предыдущего документа */
	uint64_t amount;				/* сумма в копейках */
};

/* Данные банковского абзаца */
extern struct bank_data bd;
/* Очистка банковской информации */
extern void clear_bank_info(void);
/* Получение информации о банковском абзаце */
extern ssize_t get_bank_info(struct bank_info *items, size_t nr_items);

/* Номер абзаца ответа на КЛ при обработке ответа */
extern uint32_t log_para;
/* Создание "карты" ответа */
extern int make_resp_map(void);
/* Пометить все абзацы как обработанные */
extern void mark_all(void);
/* Число необработанных абзацев */
extern int n_unhandled(void);
/* Можно ли вывести абзац на экран */
extern bool can_show(int dst);
/* Можно ли вывести абзац на принтер */
extern bool can_print(int n);
/* Можно ли остановиться на заданном абзаце */
extern bool can_stop(int n);
/* Обработан ли предыдущий абзац */
extern bool prev_handled(void);
/* Перейти к следующему абзацу */
extern int next_para(int n);
/* Индекс следующего абзаца для вывода на печать */
extern int next_printable(void);
/* Возвращает true, если в правой части строки статуса выводится сообщение об ошибке */
extern bool is_rstatus_error_msg(void);

/* Проверка синтаксиса ответа */
extern uint8_t *check_syntax(uint8_t *txt, int l, int *ecode);
/* Проверка ответа, введенного вручную, а также ОЗУ ключей */
extern bool check_raw_resp(void);

/* Многопроходный разбор */
extern int handle_para(int n_para);

/* Получение информации банковского абзаца во время обработки ответа */
extern const struct bank_data *get_bi(void);

/* Получение данных изображения для БПУ */
extern bool find_pic_data(int *data, int *req);

/* Категории данных, для которых требуется синхронизация с "Экспресс" */
#define X3_SYNC_NONE			0x00000000	/* синхронизация данных не требуется */
#define X3_SYNC_XPRN_GRIDS		0x00000001	/* требуется синхронизация разметок бланков БПУ */
#define X3_SYNC_XPRN_ICONS		0x00000002	/* требуется синхронизация пиктограмм БПУ */
#define X3_SYNC_KKT_GRIDS		0x00000004	/* требуется синхронизация разметок бланков ККТ */
#define X3_SYNC_KKT_ICONS		0x00000008	/* требуется синхронизация пиктограмм ККТ */
#define X3_SYNC_KKT_PATTERNS		0x00000010	/* требуется синхронизация шаблонов печати ККТ */
#define X3_SYNC_XSLT			0x00000020	/* требуется синхронизация таблиц XSLT */

/* Проверка необходимости синхронизация данных с "Экспресс" */
extern uint32_t need_x3_sync(void);

/* Обработка текста ответа. Возвращает false, если надо перейти к ОЗУ заказа */
extern bool execute_resp(void);

#if defined __cplusplus
}
#endif

#endif		/* EXPRESS_H */
