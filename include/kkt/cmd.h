/* Команды для работы с ККТ. (c) gsr 2018-2019 */

#if !defined KKT_CMD_H
#define KKT_CMD_H

#include "sysdefs.h"

/* Однобайтовые команды */
#define KKT_NUL			0x00		/* пустая команда */
#define KKT_STX			0x02		/* начало текста КЛ */
#define KKT_ETX			0x03		/* конец текста КЛ */
#define KKT_ALIGN_LEFT		0x04		/* выравнивание по левому краю */
#define KKT_ALIGN_RIGHT		0x05		/* выравнивание по правому краю */
#define KKT_ALIGN_CENTER	0x06		/* выравнивание по центру */
#define KKT_WIDE_FNT		0x07		/* широкий шрифт */
#define KKT_BS			0x08		/* шаг назад */
#define KKT_ITALIC		0x09		/* наклонный шрифт */
#define KKT_LF			0x0a		/* перевод строки */
#define KKT_NORM_FNT		0x0b		/* нормальный шрифт */
#define KKT_FF			0x0c		/* отрезка с выбросом бланка */
#define KKT_CR			0x0d		/* возврат каретки */
#define KKT_NR			0x0e		/* порядковый номер */
#define KKT_ESC2		0x10		/* начало ответа от ФР */
#define KKT_UNBREAK_ON		0x11		/* начало неразрывного текста */
#define KKT_UNBREAK_OFF		0x12		/* конец неразрывного текста */
#define KKT_UNDERLINE		0x14		/* подчёркнутый шрифт */
#define KKT_BELL		0x16		/* звуковой сигнал */
#define KKT_CPI15		0x19		/* плотность печати 15 cpi */
#define KKT_ESC			0x1b		/* начало команды для ФР */
#define KKT_CPI12		0x1c		/* плотность печати 12 cpi */
#define KKT_CPI20		0x1e		/* плотность печати 20 cpi */
#define KKT_RST			0x7f		/* сброс ФР */

/* Многобайтовые команды */
#define KKT_FB_BEGIN		0x0c		/* начало фискального блока */
#define KKT_FB_END		0x0d		/* конец фискального блока */
#define KKT_TAG_HEADER		0x10		/* заголовок тега */
#define KKT_TAG_VALUE		0x11		/* значение тега */
#define KKT_STATUS		0x12		/* запрос статуса ФР */
/* Опрос подключенных устройств */
#define KKT_POLL		0x16		/* автоматическое определение устройства */
#define KKT_POLL_ENQ		0x7e
#define KKT_POLL_PARAMS		0x7d

/* Служебные команды */
#define KKT_SRV			0x17		/* сервисная команда */
/* Работа с ОФД */
#define KKT_SRV_FDO_IFACE	0x30		/* установка интерфейса работы с ОФД */
#define FDO_IFACE_USB		0x30		/* USB (через терминал) */
#define FDO_IFACE_ETHER		0x31		/* сетевой адаптер */
#define FDO_IFACE_GPRS		0x32		/* GPRS-модем */
#define FDO_IFACE_OTHER		0x33		/* интерфейс, установленный в ФР */
#define KKT_SRV_FDO_ADDR	0x31		/* установка ip-адреса и TCP-порта для работы с ОФД */
#define KKT_SRV_FDO_REQ		0x32		/* запрос инструкции по обмену с ОФД (USB) */
/* Инструкции по обмену с ОФД (USB) */
#define FDO_REQ_NOP		0x30		/* инструкций нет */
#define FDO_REQ_OPEN		0x31		/* установить соединение с ОФД */
#define FDO_REQ_CLOSE		0x32		/* разорвать соединение с ОФД */
#define FDO_REQ_CONN_ST		0x33		/* запрос состояния соединения с ОФД */
#define FDO_REQ_SEND		0x34		/* передать данные в ОФД */
#define FDO_REQ_RECEIVE		0x35		/* получить данные из ОФД */
#define FDO_REQ_MESSAGE		0x36		/* сообщение для оператора */

#define KKT_SRV_FDO_DATA	0x33		/* данные, принятые из ОФД (USB) */

/* Управление потоком виртуального COM-порта */
#define KKT_SRV_FLOW_CTL	0x34		/* установка управления потоком */
#define FCTL_NONE		0x30		/* нет управления потоком */
#define FCTL_XON_XOFF		0x31		/* XON/XOFF (не поддерживается) */
#define FCTL_RTS_CTS		0x32		/* RTS/CTS */
#define FCTL_DTR_DSR		0x33		/* DTR/DSR (не поддерживается) */

/* Сброс канала виртуального COM-порта */
#define KKT_SRV_RST_TYPE	0x35		/* установка метода сброса канала */
#define RST_TYPE_NONE		0x30		/* сброс канала не поддерживается */
#define RST_TYPE_BREAK_STATE	0x31		/* сброс канала по Break State */
#define RST_TYPE_RTS_CTS	0x32		/* сброс канала по RTS/CTS */
#define RST_TYPE_DTR_DSR	0x33		/* сброс канала по DTR/DSR */

/* Часы реального времени (RTC) */
#define KKT_SRV_RTC_GET		0x36		/* запрос часов реального времени */
#define KKT_SRV_RTC_SET		0x37		/* установка часов реального времени */

/* Работа с ФД */
#define KKT_SRV_LAST_DOC_INFO	0x38		/* получить информацию о последнем сформированном и последнем отпечатанном фискальном документе */
#define KKT_SRV_PRINT_LAST	0x39		/* печать последнего сформированного, но неотпечатанного документа */
#define KKT_SRV_BEGIN_DOC	0x3a		/* начать документ */
#define KKT_SRV_SEND_DOC	0x3b		/* передача данных документа */
#define KKT_SRV_END_DOC		0x3c		/* сформировать документ */
#define KKT_SRV_PRINT_DOC	0x3e		/* печать документа по номеру */

/* Настройки сетевых интерфейсов ФР */
#define KKT_SRV_NET_SETTINGS	0x41		/* настройки сети для Ethernet */
#define KKT_SRV_GPRS_SETTINGS	0x42		/* настройки GPRS-модема */

/* Яркость печати */
#define KKT_SRV_GET_BRIGHTNESS	0x46		/* определение параметров яркости печати */
#define KKT_SRV_SET_BRIGHTNESS	0x47		/* установка яркости печати */

/* Команды печати */
#define KKT_WR_BCODE		0x1a		/* нанесение штрих-кода заданного типа */
#define KKT_GRID		0x21		/* нанесение сетки БСО */
#define KKT_TITLE		0x22		/* заголовок БСО */
#define KKT_ROTATE		0x23		/* поворот текста на 90 градусов */
#define KKT_ICON		0x24		/* печать пиктограммы */
#define ICON_DIR_HORIZ		0x7b		/* горизонтальное направление печати пиктограмм (Ш) */
#define ICON_DIR_VERT		0x65		/* вертикальное направление печати пиктограмм (Е) */
#define ICON_ROTATE_0		0x30		/* печать пиктограммы без поворота */
#define ICON_ROTATE_90		0x39		/* печать пиктограммы с поворотом на 90 градусов против часовой стрелки */
#define ICON_IDSET_Z		0x5a		/* идентификатор набора пиктограмм (Z) */
#define KKT_PIC			0x25		/* печать изображения */
#define KKT_CUT_MODE		0x2a		/* задание режима отрезки ленты */
#define CUT_MODE_FULL		0x30		/* полная отрезка */
#define CUT_MODE_PARTIAL	0x31		/* частичная отрезка */
#define KKT_PIC_LOAD		0x48		/* загрузка изображения */
#define KKT_PIC_LST		0x49		/* получение списка загруженных изображений */
#define KKT_PIC_ERASE		0x4a		/* удаление изображения из ФР */
#define KKT_PIC_FIND		0x4d		/* поиск изображения по номеру */
#define KKT_POSN		0x5b		/* команда позиционирования */
#define KKT_POSN_HPOS_ABS	0x60		/* абсолютное горизонтальное позиционирование */
#define KKT_POSN_HPOS_RIGHT	0x61		/* относительное горизонтальное позиционирование вправо */
#define KKT_POSN_HPOS_LEFT	0x71		/* относительное горизонтальное позиционирование влево */
#define KKT_POSN_VPOS_ABS	0x64		/* абсолютное вертикальное позиционирование */
#define KKT_POSN_VPOS_FW	0x65		/* относительное вертикальное позиционирование вперёд */
#define KKT_POSN_VPOS_BK	0x75		/* относительное вертикальное позиционирование назад */
#define KKT_LOG			0x6c		/* начало печати КЛ */
#define KKT_VF			0x7b		/* печать текста билета */
#define KKT_CHEQUE		0x7e		/* печать чека */
#define KKT_ECHO		0x45		/* эхо (нет в документации) */
#define MAX_ECHO_LEN		4000		/* максимальная длина данных */

/* Команды для работы с ФН */
#define KKT_FS				0x66
#define KKT_FS_STATUS			0x30	/* получение статуса ФН */
#define KKT_FS_NR			0x31	/* получение номера ФН */
#define KKT_FS_LIFETIME			0x32	/* получение срока действия ФН */
#define KKT_FS_VERSION			0x33	/* определение версии ФН */
#define KKT_FS_LAST_ERROR		0x35	/* информация об ошибке */
#define KKT_FS_SHIFT_PARAMS		0x10	/* определение параметров текущей смены */
#define KKT_FS_TRANSMISSION_STATUS	0x20	/* получение статуса информационного обмена */
#define KKT_FS_FIND_DOC			0x40	/* поиск фискального документа по номеру */
#define KKT_FS_FIND_FDO_ACK		0x41	/* поиск квитанции ОФД по номеру */
#define KKT_FS_UNCONFIRMED_DOC_CNT	0x42	/* подсчёт количества документов, на которые нет квитанции ОФД */
#define KKT_FS_REGISTRATION_RESULT	0x43	/* получение итогов фискализации */
#define KKT_FS_REGISTRATION_RESULT2	0x63	/* получение итогов фискализации (вариант 2) */
#define KKT_FS_REGISTRATION_PARAM	0x44	/* получение параметра фискализации */
#define KKT_FS_REGISTRATION_PARAM2	0x64	/* получение параметра фискализации (вариант 2) */
#define KKT_FS_GET_DOC_STLV		0x45	/* получение информации об STLV фискального документа */
#define KKT_FS_READ_DOC_TLV		0x46	/* чтение TLV фискального документа */
#define KKT_FS_READ_REGISTRATION_TLV	0x47	/* чтение TLV парамертров фискализации */
#define KKT_FS_LAST_REG_DATA		0x61	/* получение параметров текущей регистрации (независимо от ФН) */
#define KKT_FS_RESET			0x7f	/* сброс ФН */


/* Обновление ПО ФР (внутренняя команда) */
#define KKT_UPDATE		0xa0		/* переход в режим обновления */

/* Статусы выполнения команд */
/* Результат печати */
#define KKT_STATUS_OK			0x00	/* нет ошибок */
#define KKT_STATUS_OK2			0x30	/* нет ошибок */
#define KKT_STATUS_PAPER_END		0x41	/* конец бумаги */
#define KKT_STATUS_COVER_OPEN		0x42	/* крышка открыта */
#define KKT_STATUS_PAPER_LOCK		0x43	/* бумага застряла на выходе */
#define KKT_STATUS_PAPER_WRACK		0x44	/* бумага замялась */
#define KKT_STATUS_FS_ERR		0x45	/* общая аппаратная ошибка ФН */
#define KKT_STATUS_CUT_ERR		0x48	/* ошибка отрезки бумаги */
#define KKT_STATUS_INIT			0x49	/* ФР находится в состоянии инициализации */
#define KKT_STATUS_THERMHEAD_ERR	0x4a	/* неполадки термоголовки */
#define KKT_STATUS_PREV_INCOMPLETE	0x4d	/* предыдущая команда была принята не полностью */
#define KKT_STATUS_CRC_ERR		0x4e	/* предыдущая команда была принята с ошибкой контрольной суммы */
#define KKT_STATUS_HW_ERR		0x4f	/* общая аппаратная ошибка ФР */
#define KKT_STATUS_NO_FFEED		0x50	/* нет команды отрезки бланка */
#define KKT_STATUS_VPOS_OVER		0x51	/* превышение объёма текста по вертикальным позициям */
#define KKT_STATUS_HPOS_OVER		0x52	/* превышение объёма текста по горизонтальным позициям */
#define KKT_STATUS_LOG_ERR		0x53	/* нарушение структуры информации при печати КЛ */
#define KKT_STATUS_GRID_ERROR		0x55	/* нарушение параметров нанесения макетов */
#define KKT_STATUS_BCODE_PARAM		0x70	/* нарушение параметров нанесения штрих-кода */
#define KKT_STATUS_NO_ICON		0x71	/* пиктограмма не найдена */
#define KKT_STATUS_GRID_WIDTH		0x72	/* ширина сетки больше ширины бланка в установках */
#define KKT_STATUS_GRID_HEIGHT		0x73	/* высота сетки больше высоты бланка в установках */
#define KKT_STATUS_GRID_NM_FMT		0x74	/* неправильный формат имени сетки */
#define KKT_STATUS_GRID_NM_LEN		0x75	/* длина имени сетки больше допустимой */
#define KKT_STATUS_GRID_NR		0x76	/* неверный формат номера сетки */
#define KKT_STATUS_INVALID_ARG		0x77	/* неверный параметр */

/* Результат опроса устройства */
#define POLL_STATUS_OK			KKT_STATUS_OK
#define POLL_STATUS_PARAM_ERR		0x80	/* ошибочные параметры */

/* Результат установки интерфейса работы с ОФД */
#define FDO_IFACE_STATUS_OK		KKT_STATUS_OK	/* интерфейс установлен */
#define FDO_IFACE_STATUS_NO		0x41	/* у ФР нет такого интерфейса */

/* Результат установки ip-адреса и TCP-порта для работы с ОФД */
#define FDO_ADDR_STATUS_OK		KKT_STATUS_OK	/* адрес ОФД установлен */
#define FDO_ADDR_STATUS_BAD_IP		0x41	/* недопустимый ip */
#define FDO_ADDR_STATUS_BAD_PORT	0x42	/* недопустимый номер порта */

/* Результат операции с ОФД */
#define FDO_OPEN_ERROR			0x00	/* ошибка соединения с ОФД */
#define FDO_OPEN_CONNECTION_STARTED	0x01	/* соединение с ОФД начато */
#define FDO_OPEN_BAD_ADDRESS		0x02	/* неверный формат адреса ОФД */
#define FDO_OPEN_ALREADY_CONNECTED	0x03	/* соединение с ОФД установлено ранее */
#define FDO_OPEN_CONNECTED		0x04	/* соединение с ОФД установлено */
#define FDO_CLOSE_ERROR			0x00	/* ошибка закрытия соединения с ОФД */
#define FDO_CLOSE_STARTED		0x01	/* началось закрытие соединения с ОФД */
#define FDO_CLOSE_NOT_CONNECTED		0x02	/* нет соединения с ОФД */
#define FDO_CLOSE_COMPLETE		0x03	/* соединение с ОФД закрыто */
#define FDO_STATUS_NOT_CONNECTED	0x00	/* нет соединения с ОФД */
#define FDO_STATUS_CONNECTED		0x01	/* есть соединение с ОФД */
#define FDO_SEND_ERROR			0x00	/* ошибка передачи данных в ОФД */
#define FDO_SEND_OK			0x01	/* данные переданы в ОФД */
#define FDO_SEND_DATA_ERROR		0x02	/* ошибка получения данных для ОФД из ФР */
#define FDO_SEND_NOT_CONNECTED		0x03	/* нет соединения с ОФД */
#define FDO_RCV_NO_DATA			0x00	/* нет данных из ОФД */
#define FDO_RCV_OK			0x01	/* данные из ОФД переданы в ФР */
#define FDO_RCV_NOT_CONNECTED		0x02	/* нет соединения с ОФД */
#define FDO_MSG_SAVE_ERROR		0x00	/* не удалось сохранить сообщение для оператора */
#define FDO_MSG_OK			0x01	/* сообщение для оператора принято */
#define FDO_MSG_RCV_ERROR		0x02	/* ошибка приёма сообщения для оператора */

/* Результат получения данных, принятых из ОФД (USB) */
#define FDO_DATA_STATUS_OK		KKT_STATUS_OK2	/* данные успешно приняты */
#define FDO_DATA_STATUS_FAULT		0x31	/* данные не удалось принять */

/* Результат установки управления потоком */
#define FLOW_CTL_STATUS_OK		KKT_STATUS_OK2	/* управление потоком установлено */
#define FLOW_CTL_STATUS_FAULT		0x31	/* указанный тип управления потоком установить не удалось */
#define FLOW_CTL_STATUS_UNSUPPORTED	0x32	/* указанный тип управления потоком не поддерживается */

/* Результат установки метода сброса канала */
#define RST_TYPE_STATUS_OK		KKT_STATUS_OK2	/* тип сброса канала установлен */
#define RST_TYPE_STATUS_FAULT		0x31	/* тип сброса канала установить не удалось */
#define RST_TYPE_STATUS_UNSUPPORTED	0x32	/* тип сброса канала не поддерживается */

/* Результат запроса часов реального времени */
#define RTC_GET_STATUS_OK		KKT_STATUS_OK2	/* данные получены */
#define RTC_GET_STATUS_FAULT		0x31	/* ошибка получения информации из RTC */

/* Результат установки часов реального времени */
#define RTC_SET_STATUS_OK		KKT_STATUS_OK2	/* RTC установлены */
#define RTC_SET_STATUS_FMT		0x31	/* ошибка формата данных */
#define RTC_SET_STATUS_FAULT		0x32	/* ошибка установки RTC */

/* Результат получения информации о последнем сформированном и последнем отпечатанном фискальном документе */
#define LAST_DOC_INFO_STATUS_OK		KKT_STATUS_OK2	/* информация получена */
#define LAST_DOC_INFO_STATUS_FAULT	0x31	/* информация не получена */

/* Результат печати последнего сформированного, но неотпечатанного фискального документа */
#define PRINT_LAST_STATUS_OK		KKT_STATUS_OK2	/* документ отпечатан */
#define PRINT_LAST_STATUS_FORM_UNKNOWN	0x31	/* неверный код формы фискального документа */
#define PRINT_LAST_STATUS_FORM_MISMATCH	0x32	/* несовпадения кода формы фискального документа */
#define PRINT_LAST_STATUS_TEMPLATE_FMT	0x33	/* ошибка структуры шаблона */
#define PRINT_LAST_STATUS_OVERFLOW	0x34	/* недостаточно памяти для приёма шаблона */
#define PRINT_LAST_STATUS_RCV_FAIL	0x35	/* ошибка приёма шаблона */
#define PRINT_LAST_STATUS_PRN_FAIL	0x36	/* ошибка печати */
#define PRINT_LAST_STATUS_NO_PRN_FLAG	0x37	/* флаг печати документа не установлен */

/* Результат установки сетевых настроек Ethernet */
#define NET_SETTINGS_STATUS_OK		KKT_STATUS_OK2	/* настройки успешно установлены */
#define NET_SETTINGS_STATUS_BAD_IP	0x31	/* неверный формат ip-адреса ФР */
#define NET_SETTINGS_STATUS_BAD_NETMASK	0x32	/* неверный формат маски подсети */
#define NET_SETTINGS_STATUS_BAD_GW	0x33	/* неверный формат ip-адреса шлюза */

/* Результат установки настроек GPRS */
#define GPRS_CFG_STATUS_OK		KKT_STATUS_OK2	/* настройки GPRS успешно установлены */
#define GPRS_CFG_STATUS_APN_LEN		0x31	/* длина APN больше допустимой */
#define GPRS_CFG_STATUS_APN_FMT		0x32	/* недопустимые символы в APN */
#define GPRS_CFG_STATUS_USER_LEN	0x33	/* длина имени пользователя больше допустимой */
#define GPRS_CFG_STATUS_USER_FMT	0x34	/* недопустимые символы в имени пользователя */
#define GPRS_CFG_STATUS_PASSWORD_LEN	0x35	/* длина пароля больше допустимой */
#define GPRS_CFG_STATUS_PASSWORD_FMT	0x36	/* недопустимые символы в пароле */

/* Результат выполнения команды эхо */
#define ECHO_STATUS_OK			KKT_STATUS_OK2	/* нет ошибок */
#define ECHO_STATUS_OVERFLOW		0x31	/* данные не помещаются в буфер */

/* Дополнительные статусы (не используются в ФР) */
#define KKT_STATUS_INTERNAL_BASE	0x80	/* начало дополнительных статусов */
#define KKT_STATUS_UPDT_NOT_READY	0x90	/* не готов к обновлению */
#define KKT_STATUS_UPDT_ERASE_FAULT	0x91	/* ошибка стирания данных FLASH */
#define KKT_STATUS_UPDT_READ_ERR	0x92	/* ошибка чтения данных из FLASH */
#define KKT_STATUS_UPDT_WRITE_ERR	0x93	/* ошибка записи данных во FLASH */
#define KKT_STATUS_UPDT_CHECK_FAULT	0x94	/* несовпадение данных при проверке */
#define KKT_STATUS_UPDT_RESP_FMT	0x95	/* неверный формат ответа контроллера */
#define KKT_STATUS_UPDT_FC_FMT		0x96	/* неверный формат фискального ядра */
#define KKT_STATUS_WR_OVERFLOW		0xf0	/* переполнение буфера передачи */
#define KKT_STATUS_OP_TIMEOUT		0xf1	/* таймаут операции */
#define KKT_STATUS_COM_ERROR		0xf2	/* ошибка COM-порта */
#define KKT_STATUS_RESP_FMT_ERROR	0xf3	/* неверный формат ответа */
#define KKT_STATUS_MUTEX_ERROR		0xf4	/* ошибка получения мьюьекса */
#define KKT_STATUS_OVERFLOW		0xf5	/* переполнение буфера */

/* Таймауты в сотых сек операций с ККТ */
extern uint32_t kkt_base_timeout;
#define KKT_BASE_TIMEOUT		10
#define KKT_DEF_TIMEOUT			10 * kkt_base_timeout	// 100
#define KKT_STATUS_TIMEOUT		kkt_base_timeout	// 10
#define KKT_FDO_IFACE_TIMEOUT		10 * kkt_base_timeout	// 100
#define KKT_FDO_ADDR_TIMEOUT		10 *kkt_base_timeout	// 100
#define KKT_FDO_DATA_TIMEOUT		30 * kkt_base_timeout	// 300
#define KKT_FR_STATUS_TIMEOUT		kkt_base_timeout	// 10
#define KKT_FR_PRINT_TIMEOUT		50 * kkt_base_timeout	// 500
#define KKT_FR_RESET_TIMEOUT		200 * kkt_base_timeout	// 2000

#endif		/* KKT_CMD_H */
