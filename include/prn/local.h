/* Работа с пригородным печатающим устройством (ППУ). (c) gsr 2009 */

#if !defined PRN_LOCAL_H
#define PRN_LOCAL_H

#include "prn/express.h"
#include "cfg.h"
#include "gd.h"

/* Статус ППУ */
extern uint8_t lprn_status;
/* Статус SD-карты памяти */
extern uint8_t lprn_sd_status;
/* Длина заводского номера ППУ */
#define LPRN_NUMBER_LEN		14
/* Заводской номер ППУ */
extern uint8_t lprn_number[LPRN_NUMBER_LEN];
/* Тип носителя в ППУ */
extern uint8_t lprn_media;
/* Длина штрих-кода на БСО */
#define LPRN_BLANK_NUMBER_LEN	13
/* Номер документа в ППУ */
extern uint8_t lprn_blank_number[LPRN_BLANK_NUMBER_LEN];
/* Из ППУ получен номер бланка */
extern bool lprn_has_blank_number;
/* Флаг превышения таймаута при выполнении операции */
extern bool lprn_timeout;

/* Команды ППУ */

/* Однобайтовые команды */
#define LPRN_NUL	XPRN_NUL	/* пустая команда */
#define LPRN_STX	0x02		/* начало текста КЛ */
#define LPRN_ETX	0x03		/* конец текста КЛ */
#define LPRN_WIDE_FNT	XPRN_WIDE_FNT	/* широкий шрифт */
#define LPRN_LF		XPRN_LF		/* перевод строки */
#define LPRN_NORM_FNT	XPRN_NORM_FNT	/* нормальный шрифт */
#define LPRN_FORM_FEED	XPRN_FORM_FEED	/* отрезка с выбросом бланка */
#define LPRN_CR		XPRN_CR		/* возврат каретки */
#define LPRN_DLE1	XPRN_DLE1	/* начало ответа от ППУ */
#define LPRN_CPI15	0x19		/* плотность печати 15 cpi */
#define LPRN_DLE	XPRN_DLE	/* начало команды для ППУ */
#define LPRN_CPI12	0x1c		/* плотность печати 12 cpi */
#define LPRN_DLE2	0x1d		/* установка параметров печати */
#define LPRN_CPI20	0x1e		/* плотность печати 20 cpi */
#define LPRN_FORM_FEED1	0x1f		/* печать на БСО без отрезки */
#define LPRN_RST	XPRN_RST	/* сброс ППУ */

/* Многобайтовые команды */
#define LPRN_BCODE	0x01		/* запрос номера бланка */
#define LPRN_MEDIA	0x04		/* запрос типа носителя */
#define LPRN_INIT	0x06		/* инициализация */
#define LPRN_SNAPSHOTS	0x07		/* печать образов отпечатанных бланков */
#define LPRN_ERASE_SD	0x08		/* очистка SD-карты */
#define LPRN_AUXLNG	XPRN_AUXLNG	/* переход на дополнительную зону символов */
#define LPRN_MAINLNG	XPRN_MAINLNG	/* переход на основную зону символов */
#define LPRN_STATUS	0x12		/* запрос состояния ППУ */
#define LPRN_WR_BCODE2	0x1a		/* нанесение штрих-кода заданного типа */
#define LPRN_WR_BCODE1	XPRN_WR_BCODE	/* нанесение штрих-кода #1 (в тексте билета) */
#define LPRN_POS	XPRN_PRNOP	/* вертикальное или горизонтальное позиционирование */
#define LPRN_LOG1	0x60		/* печать на контрольной ленте абзаца Ар2 Э */
#define LPRN_INTERLINE	0x69		/* установка межстрочного интервала */
#define LPRN_LOG	0x6c		/* начало печати КЛ */
#define LPRN_RD_BCODE	XPRN_RD_BCODE	/* считывание штрих-кода */
#define LPRN_NO_BCODE	0x7b		/* печать текста без контроля штрих-кода */

/* Окончания команды LPRN_PRNOP */
#define LPRN_PRNOP_HPOS_ABS	XPRN_PRNOP_HPOS_ABS	/* абсолютное горизонтальное позиционирование */
#define LPRN_PRNOP_VPOS_ABS	XPRN_PRNOP_VPOS_ABS	/* абсолютное вертикальное позиционирование */

/* Таймауты для различных команд (в сотых долях секунды) */
#define LPRN_BCODE_TIMEOUT	1600	/* запрос номера бланка (12 сек) */
#define LPRN_MEDIA_TIMEOUT	400	/* запрос типа носителя (4 сек) */
#define LPRN_INIT_TIMEOUT	250	/* инициализация ППУ (2.5 сек) */
#define LPRN_STATUS_TIMEOUT	1200	/* запрос статуса (12 сек) */
#define LPRN_LOG_TIMEOUT	1200	/* печать КЛ (12 сек на абзац) */
#define LPRN_BCODE_CTL_TIMEOUT	2200	/* печать билета с контролем штрих-кода (22 сек) */
#define LPRN_TEXT_TIMEOUT	1200	/* печать текста без контроля штрих-кода (12 сек) */
#define LPRN_SNAPSHOTS_TIMEOUT	60000	/* печать образов бланков (10 мин) */
#define LPRN_ERASE_SD_TIMEOUT	6000	/* очистка SD-карты (60 сек) */
#define LPRN_RD_PARAMS_TIMEOUT	500	/* получение параметров ППУ (5 сек) */
#define LPRN_WR_PARAM_TIMEOUT	300	/* установка параметра ППУ (3 сек) */
#define LPRN_TIME_SYNC_TIMEOUT	300	/* синхронизация времени (3 сек) */

#define LPRN_INFINITE_TIMEOUT	0	/* бесконечный таймаут */

/* Тип носителя в ППУ */
#define LPRN_MEDIA_UNKNOWN	0x00	/* неизвестный тип */
#define LPRN_MEDIA_BLANK	0x30	/* БСО */
#define LPRN_MEDIA_PAPER	0x31	/* рулонная бумага */
#define LPRN_MEDIA_BOTH		0x32	/* оба носителя */

/* Коды завершения функций для работы с ППУ */
enum {
	LPRN_RET_OK,		/* операция выполнена без ошибок */
	LPRN_RET_ERR,		/* операция выполнена с ошибкой */
	LPRN_RET_RST,		/* при выполнении операции произошёл сброс терминала */
};

/* Инициализация ППУ и получение его заводского номера */
extern int lprn_init(void);
/* Возвращает true, если заводской номер ППУ состоит из одних нулей */
extern bool lprn_is_zero_number(void);
/* Получение статуса ППУ */
extern int lprn_get_status(void);
/* Получение типа носителя в ППУ */
extern int lprn_get_media_type(void);
/* Получение номера документа в ППУ */
extern int lprn_get_blank_number(void);
/* Печать сохранённых изображений */
extern int lprn_snapshots(void);
/* Очистка SD-карты */
extern int lprn_erase_sd(void);
/* Печать абзаца контрольной ленты */
extern int lprn_print_log(const uint8_t *data, size_t len);
/* Печать абзаца контрольной ленты, пришедшего из "Экспресс" */
extern int lprn_print_log1(const uint8_t *data, size_t len);
/* Печать текста на БСО */
extern int lprn_print_ticket(const uint8_t *data, size_t len, bool *sent_to_prn);
/* Запрос параметров работы ППУ */
extern int lprn_get_params(struct term_cfg *cfg);
/* Запись параметров работы ППУ */
extern int lprn_set_params(struct term_cfg *cfg);
/* Синхронизация времени ППУ с терминалом */
extern int lprn_sync_time(void);
/* Закрытие устройства для работы с ППУ */
extern void lprn_close(void);

/* Расшифровка ошибок ППУ */
struct lprn_error_txt {
	int len;		/* длина сообщения */
	char *txt;		/* сообщение об ошибке (koi7) */
};

/* Получение информации об ошибке ППУ на основании её кода */
extern struct lprn_error_txt *lprn_get_error_txt(uint8_t code);
/* Получение информации об ошибке карты памяти на основании её кода */
extern struct lprn_error_txt *lprn_get_sd_error_txt(uint8_t code);

#if defined __LOG_LPRN__
extern bool lprn_create_log(void);
#endif

#endif		/* PRN_LOCAL_H */
