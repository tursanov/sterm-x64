/* Работа с билетопечатающим устройством (БПУ). (c) gsr 2009, 2026 */

#if !defined PRN_SPRN_H
#define PRN_SPRN_H

#if defined __cplusplus
extern "C" {
#endif

#include "prn/express.h"
#include "cfg.h"
#include "gd.h"

/* Статус БПУ */
extern uint8_t sprn_status;
/* Длина заводского номера БПУ */
#define SPRN_NUMBER_LEN		14
/* Заводской номер БПУ */
extern uint8_t sprn_number[SPRN_NUMBER_LEN];
/* Тип носителя в БПУ */
extern uint8_t sprn_media;
/* Длина штрих-кода на БСО */
#define SPRN_BLANK_NUMBER_LEN	13
/* Номер документа в БПУ */
extern uint8_t sprn_blank_number[SPRN_BLANK_NUMBER_LEN];
/* Из БПУ получен номер бланка */
extern bool sprn_has_blank_number;
/* Флаг превышения таймаута при выполнении операции */
extern bool sprn_timeout;

/* Команды БПУ */

/* Однобайтовые команды */
#define SPRN_NUL		0x00	/* пустая команда */
#define SPRN_STX		0x02	/* начало текста КЛ */
#define SPRN_ETX		0x03	/* конец текста КЛ */
#define SPRN_WIDE_FNT		0x07	/* широкий шрифт */
#define SPRN_BS			0x08	/* шаг назад */
#define SPRN_ITALIC		0x09	/* наклонный шрифт */
#define SPRN_LF			0x0a	/* перевод строки */
#define SPRN_NORM_FNT		0x0b	/* нормальный шрифт */
#define SPRN_FORM_FEED		0x0c	/* отрезка с выбросом бланка */
#define SPRN_CR			0x0d	/* возврат каретки */
#define SPRN_DLE1		0x10	/* начало ответа от БПУ */
#define SPRN_UNDERLINE		0x14	/* подчёркнутый шрифт */
#define SPRN_BELL		0x16	/* звуковой сигнал */
#define SPRN_CPI15		0x19	/* плотность печати 15 cpi */
#define SPRN_DLE		0x1b	/* начало команды для БПУ */
#define SPRN_CPI12		0x1c	/* плотность печати 12 cpi */
#define SPRN_DLE2		0x1d	/* установка параметров печати */
#define SPRN_CPI20		0x1e	/* плотность печати 20 cpi */
#define SPRN_RST		0x7f	/* сброс БПУ */

/* Многобайтовые команды */
#define SPRN_NUMBER		0x01	/* запрос номера бланка */
#define SPRN_MEDIA		0x04	/* запрос типа носителя */
#define SPRN_ID			0x06	/* запрос идентификатора */
#define SPRN_AUXLNG		0x09	/* переход на дополнительную зону символов */
#define SPRN_MAINLNG		0x0b	/* переход на основную зону символов */
#define SPRN_STATUS		0x12	/* запрос состояния БПУ */
#define SPRN_RFID_FIND		0x14	/* поиск метки RFID */
#define SPRN_RFID_SET_KEY	0x15	/* установка ключа RFID */
#define SPRN_RFID_READ		0x16	/* чтение RFID, опрос устройства */
#define SPRN_POLL		SPRN_RFID_READ
#define SPRN_RFID_WRITE		0x17	/* запись RFID */
#define SPRN_RFID_LOAD_KEY	0x18	/* запись ключа для работы с RFID */
#define SPRN_WR_BCODE2		0x1a	/* нанесение штрих-кода заданного типа */
#define SPRN_GRID		0x21	/* нанесение сетки на БСО */
#define SPRN_TITLE		0x22	/* заголовок БСО */
#define SPRN_ROTATE		0x23	/* поворот текста на 90 градусов */
#define SPRN_ICON		0x24	/* печать пиктограммы */
#define SPRN_LOAD_GRID		0x48	/* загрузка сетки бланка */
#define SPRN_WR_BCODE1		0x4c	/* нанесение штрих-кода #1 (в тексте билета) */
#define SPRN_RD_PARAMS		0x52	/* чтение парметров БПУ */
#define SPRN_WR_PARAM		0x53	/* установка парметра БПУ */
#define SPRN_POS		0x5b	/* позиционирование */
/* Окончания команды позиционирования */
#define SPRN_POS_HPOS_ABS	0x60	/* абсолютное горизонтальное позиционирование */
#define SPRN_POS_HPOS_RIGHT	0x61	/* относительное горизонтальное позиционирование вправо */
#define SPRN_POS_HPOS_LEFT	0x71	/* относительное горизонтальное позиционирование влево */
#define SPRN_POS_VPOS_ABS	0x64	/* абсолютное вертикальное позиционирование */
#define SPRN_POS_VPOS_FW	0x65	/* относительное вертикальное позиционирование вперёд */
#define SPRN_POS_VPOS_BK	0x75	/* относительное вертикальное позиционирование назад */
#define SPRN_POS_INP_TICKET	0x74	/* ввод бланка */
#define SPRN_POS_LINE_FW	0x76	/* переключение строк вперёд */

#define SPRN_LOG1		0x60	/* печать на контрольной ленте */
#define SPRN_INTERLINE		0x69	/* установка межстрочного интервала */
#define SPRN_LOG		0x6c	/* начало печати КЛ */
#define SPRN_RD_BCODE		0x76	/* проверка номера бланка */
#define SPRN_NO_BCODE		0x7b	/* печать текста без контроля номера бланка */

/* Окончания команды SPRN_PRNOP */
#define SPRN_PRNOP_HPOS_ABS	XPRN_PRNOP_HPOS_ABS	/* абсолютное горизонтальное позиционирование */
#define SPRN_PRNOP_VPOS_ABS	XPRN_PRNOP_VPOS_ABS	/* абсолютное вертикальное позиционирование */

/* Таймауты для различных команд (в сотых долях секунды) */
#define SPRN_RD_BCODE_TIMEOUT	1600	/* запрос номера бланка (12 сек) */
#define SPRN_MEDIA_TIMEOUT	400	/* запрос типа носителя (4 сек) */
#define SPRN_ID_TIMEOUT		250	/* запрос идентификатора (2.5 сек) */
#define SPRN_STATUS_TIMEOUT	1200	/* запрос статуса (12 сек) */
#define SPRN_LOG_TIMEOUT	1200	/* печать КЛ (12 сек на абзац) */
#define SPRN_BCODE_CTL_TIMEOUT	2200	/* печать билета с контролем штрих-кода (22 сек) */
#define SPRN_TEXT_TIMEOUT	1200	/* печать текста без контроля штрих-кода (12 сек) */
#define SPRN_RD_PARAMS_TIMEOUT	500	/* получение параметров БПУ (5 сек) */
#define SPRN_WR_PARAM_TIMEOUT	300	/* установка параметра БПУ (3 сек) */
#define SPRN_TIME_SYNC_TIMEOUT	300	/* синхронизация времени (3 сек) */

#define SPRN_INFINITE_TIMEOUT	0	/* бесконечный таймаут */

/* Тип носителя в БПУ */
#define SPRN_MEDIA_UNKNOWN	0x00	/* неизвестный тип */
#define SPRN_MEDIA_BLANK	0x30	/* БСО */
#define SPRN_MEDIA_PAPER	0x31	/* рулонная бумага */
#define SPRN_MEDIA_BOTH		0x32	/* оба носителя */

/* Коды завершения функций для работы с БПУ */
enum {
	SPRN_RET_OK,		/* операция выполнена без ошибок */
	SPRN_RET_ERR,		/* операция выполнена с ошибкой */
	SPRN_RET_RST,		/* при выполнении операции произошёл сброс терминала */
};

/* Инициализация БПУ и получение его заводского номера */
extern int sprn_init(void);
/* Возвращает true, если заводской номер БПУ состоит из одних нулей */
extern bool sprn_is_zero_number(void);
/* Получение статуса БПУ */
extern int sprn_get_status(void);
/* Получение типа носителя в БПУ */
extern int sprn_get_media_type(void);
/* Получение номера документа в БПУ */
extern int sprn_get_blank_number(void);
/* Печать абзаца контрольной ленты */
extern int sprn_print_log(const uint8_t *data, size_t len);
/* Печать абзаца контрольной ленты, пришедшего из "Экспресс" */
extern int sprn_print_log1(const uint8_t *data, size_t len);
/* Печать текста на БСО */
extern int sprn_print_ticket(const uint8_t *data, size_t len, bool *sent_to_prn);
/* Запрос параметров работы БПУ */
extern int sprn_get_params(struct term_cfg *cfg);
/* Запись параметров работы БПУ */
extern int sprn_set_params(struct term_cfg *cfg);
/* Синхронизация времени БПУ с терминалом */
extern int sprn_sync_time(void);
/* Закрытие устройства для работы с БПУ */
extern void sprn_close(void);

/* Расшифровка ошибок БПУ */
struct sprn_error_txt {
	int len;		/* длина сообщения */
	char *txt;		/* сообщение об ошибке (koi7) */
};

/* Получение информации об ошибке БПУ на основании её кода */
extern struct sprn_error_txt *sprn_get_error_txt(uint8_t code);
/* Получение информации об ошибке карты памяти на основании её кода */
extern struct sprn_error_txt *sprn_get_sd_error_txt(uint8_t code);

#if defined __cplusplus
}
#endif

#endif		/* PRN_SPRN_H */
