/* Работа с билетопечатающим устройством (БПУ). (c) gsr 2009, 2026 */

#if !defined PRN_SPRN_H
#define PRN_SPRN_H

#if defined __cplusplus
extern "C" {
#endif

#include "prn/express.h"
#include "cfg.h"
#include "devinfo.h"
#include "gd.h"

extern const struct dev_info *sprn;
extern const struct dev_info *rfid;

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
#define SPRN_NUMBER_TIMEOUT	400	/* чтение номера бланка (4 сек) */
#define SPRN_MEDIA_TIMEOUT	400	/* запрос типа носителя (4 сек) */
#define SPRN_ID_TIMEOUT		250	/* запрос идентификатора (2.5 сек) */
#define SPRN_STATUS_TIMEOUT	1200	/* запрос статуса (12 сек) */
#define SPRN_RD_BCODE_TIMEOUT	1600	/* печать с проверкой номера бланка (12 сек) */
#define SPRN_LOG_TIMEOUT	1200	/* печать КЛ (12 сек на абзац) */
#define SPRN_BCODE_CTL_TIMEOUT	2200	/* печать билета с контролем штрих-кода (22 сек) */
#define SPRN_TEXT_TIMEOUT	1200	/* печать текста без контроля штрих-кода (12 сек) */
#define SPRN_RD_PARAMS_TIMEOUT	500	/* получение параметров БПУ (5 сек) */
#define SPRN_WR_PARAM_TIMEOUT	300	/* установка параметра БПУ (3 сек) */

#define SPRN_INFINITE_TIMEOUT	0	/* бесконечный таймаут */

/* Тип носителя в БПУ */
#define SPRN_MEDIA_UNKNOWN	0x00	/* неизвестный тип */
#define SPRN_MEDIA_BLANK	0x30	/* БСО */
#define SPRN_MEDIA_PAPER	0x31	/* рулонная бумага */
#define SPRN_MEDIA_BOTH		0x32	/* оба носителя */

/* Статус завершения команд БПУ */
#define SPRN_STATUS_OK		0x00	/* нормальное завершение */
#define SPRN_STATUS_NO_NUMBER	0x01	/* номер БПУ не прописан в памяти */
#define SPRN_STATUS_NO_BCODE	0x30	/* штриховой код отсутствует */
#define SPRN_STATUS_BCODE_LEN	0x36	/* длина штрихового кода не равна 13 */
#define SPRN_STATUS_BLANK_NR	0x37	/* несовпадение номера бланка */
#define SPRN_STATUS_BCODE_CRC	0x38	/* ошибка контрольной суммы штрих-кода */
#define SPRN_STATUS_ZERO_NR	0x39	/* считался номер 000000 */
#define SPRN_STATUS_PAPER_END	0x41	/* конец бумаги */
#define SPRN_STATUS_COVER_OPEN	0x42	/* крышка открыта */
#define SPRN_STATUS_PAPER_LOCK	0x43	/* бумага застряла на выходе */
#define SPRN_STATUS_PAPER_WRACK	0x44	/* бумага замялась */
#define SPRN_STATUS_NOTCH_ERR	0x45	/* ошибка чтения реперной метки */
#define SPRN_STATUS_SCANNER_ERR	0x46	/* аппаратная ошибка сканера штрих-кода */
#define SPRN_STATUS_MEDIA_ERR	0x47	/* ошибка носителя */
#define SPRN_STATUS_BLANK_SKEW	0x4a	/* перекос бланка */
#define SPRN_STATUS_HW_ERR	0x4f	/* общая аппаратная ошибка принтера */
#define SPRN_STATUS_NO_FFEED	0x50	/* нет команды отрезки бланка */
#define SPRN_STATUS_VPOS_OVER	0x51	/* превышение объёма текста по вертикальным позициям */
#define SPRN_STATUS_HPOS_OVER	0x52	/* превышение объёма текста по горизонтальным позициям */
#define SPRN_STATUS_LOG_ERR	0x53	/* нарушение структуры информации при печати КЛ */
#define SPRN_STATUS_BC_CMD_ERR	0x54	/* нарушение формата команды анализа штрихового кода */
#define SPRN_STATUS_GRID_ERROR	0x55	/* нарушение параметров нанесения макетов */
#define SPRN_STATUS_INVALID_ARG	0x70	/* неверный параметр */
#define SPRN_STATUS_NO_ICON	0x71	/* пиктограмма не найдена в БПУ */
#define SPRN_STATUS_GRID_WIDTH	0x72	/* ширина сетки больше ширины бланка в установках */
#define SPRN_STATUS_GRID_HEIGHT	0x73	/* высота сетки больше высоты бланка в установках */
#define SPRN_STATUS_GRID_NM_FMT	0x74	/* неправильный формат имени сетки */
#define SPRN_STATUS_GRID_NM_LEN	0x75	/* длина имени сетки больше допустимой */
#define SPRN_STATUS_GRID_NR	0x76	/* неверный формат номера сетки */

/* Статус выполнения команд работы с RFID */
#define RFID_STATUS_OK		0x00	/* нормальное завершения */
#define RFID_STATUS_INVALID_ARG	0x70	/* неверный параметр */
#define RFID_STATUS_NONFUNCTION	0xf1	/* считыватель неработоспособен */
#define RFID_STATUS_NO_TAG	0xf2	/* метка не обнаружена */
#define RFID_STATUS_TAG_UNKNOWN	0xf3	/* считыватель не может работать с меткой */
#define RFID_STATUS_COLLISION	0xf4	/* найдено более одной метки */
#define RFID_STATUS_ACCESS_ERR	0xf5	/* ошибка доступа */
#define RFID_STATUS_READ_ERR	0xf6	/* ошибка чтения */
#define RFID_STATUS_HW_ERR	0xff	/* аппаратная ошибка */

/* Загрузка разметок бланков (нет в ТЗ ВНИИЖТ) */
#define SPRN_STATUS_GRID_ILLEGAL_ID	0xb0	/* неверный идентификатор разметки */
#define SPRN_STATUS_GRID_TOO_SHORT	0xb1	/* длина разметки меньше допустимой */
#define SPRN_STATUS_GRID_TOO_LONG	0xb2	/* длина разметки превышает допустимую */
#define SPRN_STATUS_GRID_CRC_ERR	0xb3	/* ошибка контрольной суммы разметки */
#define SPRN_STATUS_GRID_WRITE_ERR	0xb4	/* ошибка записи разметки в память БПУ */
#define SPRN_STATUS_GRID_TIMEOUT	0xb5	/* таймаут приема разметки */

/* Максимальное количество разметок */
#define MAX_GRIDS			36	/* 0-9 A-Z */

/* Загрузка пиктограмм (нет в ТЗ ВНИИЖТ) */
#define SPRN_STATUS_ICON_ILLEGAL_ID	0xb8	/* неверный идентификатор пиктограммы */
#define SPRN_STATUS_ICON_TOO_SHORT	0xb9	/* длина данных пиктограмм меньше допустимой */
#define SPRN_STATUS_ICON_TOO_LONG	0xba	/* длина данных пиктограмм превышает допустимую */
#define SPRN_STATUS_ICON_CRC_ERR	0xbb	/* ошибка контрольной суммы данных пиктограмм */
#define SPRN_STATUS_ICON_WRITE_ERR	0xbc	/* ошибка записи пиктограмм в память БПУ */
#define SPRN_STATUS_ICON_TIMEOUT	0xbd	/* таймаут приема пиктограмм */

/* Максимальное количество пиктограмм */
#define MAX_ICONS			36	/* 0-9 A-Z */

static inline bool sprn_ok(uint8_t status)
{
	return status == SPRN_STATUS_OK;
}

static inline bool rfid_ok(uint8_t status)
{
	return status == RFID_STATUS_OK;
}

/* Коды завершения функций для работы с БПУ */
enum {
	SPRN_RET_OK,		/* операция выполнена без ошибок */
	SPRN_RET_ERR,		/* операция выполнена с ошибкой */
	SPRN_RET_RST,		/* при выполнении операции произошёл сброс терминала */
};

#define SPRN_RX_BUF_LEN		4096
#define SPRN_TX_BUF_LEN		262144

/* Инициализация БПУ и получение его заводского номера */
extern int sprn_init(void);
/* Закрытие устройства для работы с БПУ */
extern void sprn_close(void);
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

/* Расшифровка ошибок БПУ */
struct sprn_error_txt {
	int len;		/* длина сообщения */
	char *txt;		/* сообщение об ошибке (koi7) */
};

/* Получение информации об ошибке БПУ на основании её кода */
extern const struct sprn_error_txt *sprn_get_error_txt(uint8_t code);
/* Получение информации об ошибке считывателя ЭМТТ на основании её кода */
extern const struct sprn_error_txt *rfid_get_error_txt(uint8_t code);

#if defined __cplusplus
}
#endif

#endif		/* PRN_SPRN_H */
