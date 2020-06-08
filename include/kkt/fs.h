/* Типы и константы для работы с ФН ККТ. (c) gsr 2018-2019 */

#if !defined KKT_FS_H
#define KKT_FS_H

#include <stdint.h>
#include "sysdefs.h"

/* Дата/время, используемые в ФН */
struct kkt_fs_date {
	uint32_t year;		/* год (например, 2018) */
	uint32_t month;		/* месяц (1 -- 12) */
	uint32_t day;		/* день (1 -- 31) */
};

#define KKT_FS_DATE_LEN		3

struct kkt_fs_time {
	uint32_t hour;		/* час (0 -- 23) */
	uint32_t minute;	/* минута (0 -- 59) */
};

#define KKT_FS_TIME_LEN		2

struct kkt_fs_date_time {
	struct kkt_fs_date date;
	struct kkt_fs_time time;
};

#define KKT_FS_DATE_TIME_LEN	(KKT_FS_DATE_LEN + KKT_FS_TIME_LEN)

/* Фаза жизни ФН */
#define FS_LIFESTATE_NONE	0	/* настройка */
#define FS_LIFESTATE_FREADY	1	/* готов к фискализации */
#define FS_LIFESTATE_FMODE	3	/* фискальный режим */
#define FS_LIFESTATE_POST_FMODE	7	/* постфискальный режим, идёт передача ФД в ОФД */
#define FS_LIFESTATE_ARCH_READ	15	/* чтение данных из архива ФН */

/* Текущий документ */
/* Нет открытого документа */
#define FS_CURDOC_NOT_OPEN		0x00
/* Отчёт о регистрации ККТ */
#define FS_CURDOC_REG_REPORT		0x01
/* Отчёт об открытии смены */
#define FS_CURDOC_SHIFT_OPEN_REPORT	0x02
/* Кассовый чек */
#define FS_CURDOC_CHEQUE		0x04
/* Отчёт о закрытии смены */
#define FS_CURDOC_SHIFT_CLOSE_REPORT	0x08
/* отчёт о закрытии фискального режима */
#define FS_CURDOC_FMODE_CLOSE_REPORT	0x10
/* Бланк строгой отчетности */
#define FS_CURDOC_BSO			0x11
/* Отчет об изменении параметров регистрации ККТ в связи с заменой ФН */
#define FS_CURDOC_REG_PARAMS_REPORT_ON_FS_CHANGE	0x12
/* Отчет об изменении параметров регистрации ККТ */
#define FS_CURDOC_REG_PARAMS_REPORT	0x13
/* Кассовый чек коррекции */
#define FS_CURDOC_CORRECTION_CHEQUE	0x14
/* БСО коррекции */
#define FS_CURDOC_CORRECTION_BSO	0x15
/* Отчет о текущем состоянии расчетов */
#define FS_CURDOC_CURRENT_PAY_REPORT	0x17

/* Данные документа */
#define FS_NO_DOC_DATA			0	/* нет данных документа */
#define FS_HAS_DOC_DATA			1	/* получены данные документа */

/* Состояние смены */
#define FS_SHIFT_CLOSED			0	/* смена закрыта */
#define FS_SHIFT_OPENED			1	/* смена открыта */

/* Предупреждения об исчерпании ресурсов ФН */

/* Срочная замена КС */
#define FS_ALERT_CC_REPLACE_URGENT	0x01
/* Исчерпание ресурса КС */
#define FS_ALERT_CC_EXHAUST		0x02
/* Переполнение памяти (90% заполнено) */
#define FS_ALERT_MEMORY_FULL		0x04
/* Превышено время ожидания ответа от ОФД */
#define FS_ALERT_RESP_TIMEOUT		0x08
/* Отказ по данным форматно-логического контроля (признак передается в подтверждении от ОФД) */
#define FS_ALERT_FLC			0x10
/* Требуется настройка ККТ (признак передается в подтверждении от ОФД) */
#define FS_ALERT_SETUP_REQUIRED		0x20
/* ОФД аннулирован(признак передается в подтверждении от ОФД) */
#define FS_ALERT_OFD_REVOKED		0x40
/* Критическая ошибка ФН */
#define FS_ALERT_CRITRICAL		0x80

/* Состояние ФН */
struct kkt_fs_status {
	uint8_t life_state;		/* состояние фазы жизни */
	uint8_t current_doc;		/* текущий документ */
	uint8_t doc_data;		/* данные документа */
	uint8_t shift_state;		/* состояние смены */
	uint8_t alert_flags;		/* флаги предупреждения */
	struct kkt_fs_date_time dt;	/* дата и время последнего документа */
#define KKT_FS_NR_LEN	16
	char nr[KKT_FS_NR_LEN + 1];	/* номер ФН */
	uint32_t last_doc_nr;		/* номер последнего ФД */
};

/* Срок действия ФН */
struct kkt_fs_lifetime {
	struct kkt_fs_date_time dt;
	uint32_t reg_complete;
	uint32_t reg_remain;
};

/* Версия ФН */
struct kkt_fs_version {
#define KKT_FS_VERSION_LEN		16
	char version[KKT_FS_VERSION_LEN + 1];
	uint32_t type;
};

/* Информация о смене */
struct kkt_fs_shift_state {
/* Состояние смены (0 -- закрыта, 1 -- открыта) */
	bool opened;
/* Номер смены. Если смена закрыта, то  номер последней закрытой смены,
 * если открыта, то номер текущей смены.
 */
	uint32_t shift_nr;
/* Номер чека. Если смена закрыта, то число документов в предыдущей закрытой смене
 * (0, если это первая смена). Если смена открыта, но нет ни одного чека, то 0. 
 * В остальных случаях  номер последнего сформированного чека
 */
	uint32_t cheque_nr;
};

/* Состояние обмена с ОФД */
#define FS_TS_CONNECTED		0x01	/* транспортное соединение установлено */
#define FS_TS_HAS_REQ		0x02	/* есть сообщение для передачи в ОФД */
#define FS_TS_WAIT_ACK		0x04	/* ожидание квитанции от ОФД */
#define FS_TS_HAS_CMD		0x08	/* есть команда от ОФД */
#define FS_TS_SETTINGS_CHANGED	0x10	/* изменились настройки соединения с ОФД */
#define FS_TS_WAIT_RESP		0x20	/* ожидание ответа */

/* Статус информационного обмена с ОФД */
struct kkt_fs_transmission_state {
	uint8_t state;		/* статус информационного обмена */
	bool read_msg_started;	/* началось чтение сообщения для ОФД */
	size_t sndq_len;	/* длина очереди сообщений для передачи */
	uint32_t first_doc_nr;	/* номер первого документа для передачи */
	struct kkt_fs_date_time first_doc_dt;	/* дата/время создания первого документа */
};

/* Типы фискальных документов */
#define NONE			0	/* нет */
#define REGISTRATION		1	/* отчет о регистрации */
#define RE_REGISTRATION		11	/* отчет о перерегистрации */
#define OPEN_SHIFT		2	/* открытие смены */
#define CALC_REPORT		21	/* отчет о состоянии расчетов */
#define CHEQUE			3	/* чек */
#define CHEQUE_CORR		31	/* чек корреции */
#define BSO			4	/* БСО */
#define BSO_CORR		41	/* БСО коррекции */
#define CLOSE_SHIFT		5	/* закрытие смены */
#define CLOSE_FS		6	/* закрытие ФН */
#define OP_COMMIT		7	/* подтверждение оператора */

/* Информация о различных типах документов */

/* Отчёт о регистрации (FrDocType::Registration) */
struct kkt_register_report {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	char inn[12];
	char reg_number[20];
	uint8_t tax_system;
	uint8_t mode;
} __attribute__((__packed__));

/* Отчёт о перерегистрации (FrDocType::ReRegistration) */
struct kkt_reregister_report {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	char inn[12];
	char reg_number[20];
	uint8_t tax_system;
	uint8_t mode;
	uint8_t rereg_code;
} __attribute__((__packed__));

/* Чек/чек коррекции (FrDocType::Cheque/FrDocType::ChequeCorr/FrDocType::BSO/FrDocType::BSOCorr) */
struct kkt_cheque_report {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	uint8_t type;
	uint8_t sum[5];
} __attribute__((__packed__));

/* Открытие/закрытие смены (FrDocType::OpenShift/FrDocType::CloseShift) */
struct kkt_shift_report {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	uint16_t shift_nr;
} __attribute__((__packed__));

/* Закрытие фискального режима (FrDocType::CloseFS) */
struct kkt_close_fs {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	char inn[12];
	char reg_number[20];
} __attribute__((__packed__));

/* Отчёт о состоянии расчётов (FrDocType::CalcReport) */
struct kkt_calc_report {
	uint8_t dt[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	uint32_t nr_uncommited;
	uint8_t first_uncommited_dt[3];
} __attribute__((__packed__));

/* Квитанция ОФД */
struct kkt_fdo_ack {
	uint8_t dt[5];
	uint8_t fiscal_sign[18];
	uint32_t doc_nr;
} __attribute__((__packed__));


/* Информация о найденном документе */
struct kkt_fs_find_doc_info {
	uint32_t doc_type;		/* тип документа */
	bool fdo_ack;			/* признак подтверждения ОФД */
};

/* Информация о квитанции ОФД */
struct kkt_fs_fdo_ack {
	struct kkt_fs_date_time dt;
	uint8_t fiscal_sign[18];
	uint32_t doc_nr;
};

/* Информация об STLV фискального документа */
struct kkt_fs_doc_stlv_info {
	uint32_t doc_type;		/* тип документа */
	size_t len;			/* длина документа */
};

#endif		/* KKT_FS_H */
