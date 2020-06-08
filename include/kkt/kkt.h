/* Работа с ККТ по виртуальному COM-порту. (c) gsr 2018-2019 */

#if !defined KKT_KKT_H
#define KKT_KKT_H

#include "kkt/cmd.h"
#include "kkt/fs.h"
#include "devinfo.h"

extern const struct dev_info *kkt;

extern uint8_t kkt_status;

#define KKT_NR_MAX_LEN		20

extern const char *kkt_nr;
extern const char *kkt_ver;
extern const char *kkt_fs_nr;

static inline bool is_bcd(uint8_t b)
{
	return ((b & 0x0f) < 0x0a) && ((b & 0xf0) < 0xa0);
}

static inline uint8_t to_bcd(uint8_t b)
{
	return (b < 100) ? (((b / 10) << 4) | (b % 10)) : 0;
}

static inline uint8_t from_bcd(uint8_t b)
{
	return is_bcd(b) ? ((b >> 4) * 10 + (b & 0x0f)) : 0;
}

extern void kkt_init(const struct dev_info *di);
extern void kkt_release(void);

/* Установить интерфейс взаимодействия с ОФД */
extern uint8_t kkt_set_fdo_iface(int fdo_iface);
/* Установить адрес ОФД */
extern uint8_t kkt_set_fdo_addr(uint32_t fdo_ip, uint16_t fdo_port);

/* Данные переменной длины */
struct kkt_var_data {
	size_t len;
	const uint8_t *data;
};

/* Инструкция по обмену с ОФД */
struct kkt_fdo_cmd {
	uint8_t cmd;
	struct kkt_var_data data;
};
/* Получить инструкцию по обмену с ОФД */
extern uint8_t kkt_get_fdo_cmd(uint8_t prev_op, uint16_t prev_op_status,
	uint8_t *cmd, uint8_t *data, size_t *data_len);
/* Передать в ККТ данные, полученные от ОФД */
extern uint8_t kkt_send_fdo_data(const uint8_t *data, size_t data_len);

/* Показания часов реального времени ККТ */
struct kkt_rtc_data {
	uint16_t year;		/* год (например, 2019) */
	uint8_t  month;		/* месяц (1-12) */
	uint8_t  day;		/* день (1-31) */
	uint8_t  hour;		/* час (1-23) */
	uint8_t  minute;	/* минута (1-59) */
	uint8_t  second;	/* секунда (1-59) */
};
/* Прочитать показания часов реального времени */
extern uint8_t kkt_get_rtc(struct kkt_rtc_data *rtc);
/* Установить часы реального времени */
extern uint8_t kkt_set_rtc(time_t t);

/* Информация о последнем сформированном и последнем отпечатанном фискальных документах */
struct kkt_last_doc_info {
	uint32_t last_nr;
	uint16_t last_type;
	uint32_t last_printed_nr;
	uint16_t last_printed_type;
};
/* Получить информацию о последнем сформированном и последнем отпечатанном документах */
extern uint8_t kkt_get_last_doc_info(struct kkt_last_doc_info *ldi, uint8_t *err_info,
	size_t *err_info_len);

/* Информация о последнем отпечатанном документе */
struct kkt_last_printed_info {
	uint32_t nr;
	uint32_t sign;
	uint16_t type;
};
/* Напечатать последний сформированный документ */
extern uint8_t kkt_print_last_doc(uint16_t doc_type, const uint8_t *tmpl, size_t tmpl_len,
	struct kkt_last_printed_info *lpi, uint8_t *err_info, size_t *err_info_len);

/* Начать формирование документа */
extern uint8_t kkt_begin_doc(uint16_t doc_type, uint8_t *err_info, size_t *err_info_len);

/* Передать данные документа */
extern uint8_t kkt_send_doc_data(const uint8_t *data, size_t len, uint8_t *err_info,
	size_t *err_info_len);

/* Информация о сформированном документе */
struct kkt_doc_info {
	uint32_t nr;
	uint32_t sign;
};
/* Завершить формирование документа */
extern uint8_t kkt_end_doc(uint16_t doc_type, const uint8_t *tmpl, size_t tmpl_len,
	uint32_t timeout_factor, struct kkt_doc_info *di,
	uint8_t *err_info, size_t *err_info_len);

/* Напечатать документ по номеру */
extern uint8_t kkt_print_doc(uint32_t doc_nr, const uint8_t *tmpl, size_t tmpl_len,
	struct kkt_last_printed_info *lpi, uint8_t *err_info, size_t *err_info_len);

/* Настроить сетевой интерфейс ККТ */
extern uint8_t kkt_set_net_cfg(uint32_t ip, uint32_t netmask, uint32_t gw);
/* Настроить GPRS в ККТ */
extern uint8_t kkt_set_gprs_cfg(const char *apn, const char *user, const char *password);

/* Информация о яркости печати ККТ */
struct kkt_brightness {
	uint8_t current;
	uint8_t def;
	uint8_t max;
};

extern struct kkt_brightness kkt_brightness;

/* Получить информацию о яркости печати ККТ */
extern uint8_t kkt_get_brightness(struct kkt_brightness *brightness);
/* Настроить яркость печати в ККТ */
extern uint8_t kkt_set_brightness(uint8_t brightness);

/* Настроить ККТ в соответствии с конфигурацией терминала */
extern uint8_t kkt_set_cfg(void);

/* Получить статус ФН */
extern uint8_t kkt_get_fs_status(struct kkt_fs_status *fs_status);

/* Получить номер ФН */
extern uint8_t kkt_get_fs_nr(char *fs_nr);

/* Получить срок действия ФН */
extern uint8_t kkt_get_fs_lifetime(struct kkt_fs_lifetime *lt);

/* Прочитать версию ФН */
extern uint8_t kkt_get_fs_version(struct kkt_fs_version *ver);

/* Прочитать информацию о последней ошибке ФН */
extern uint8_t kkt_get_fs_last_error(uint8_t *err_info, size_t *err_info_len);

/* Получить информацию о текущей смене */
extern uint8_t kkt_get_fs_shift_state(struct kkt_fs_shift_state *ss);

/* Получить статус информационного обмена с ОФД */
extern uint8_t kkt_get_fs_transmission_state(struct kkt_fs_transmission_state *ts);

/* Найти фискальный документ по номеру */
extern uint8_t kkt_find_fs_doc(uint32_t nr, bool need_print,
	struct kkt_fs_find_doc_info *fdi, uint8_t *data, size_t *data_len);

/* Найти подтверждение ОФД по номеру */
extern uint8_t kkt_find_fdo_ack(uint32_t nr, bool need_print, struct kkt_fs_fdo_ack *fdo_ack);

/* Получить количество документов, на которые нет квитанции ОФД */
extern uint8_t kkt_get_unconfirmed_docs_nr(uint32_t *nr_docs);

/* Получить данные последней регистрции */
extern uint8_t kkt_get_last_reg_data(uint8_t *data, size_t *data_len);

/* Получить информацию об STLV фискального документа */
extern uint8_t kkt_get_doc_stlv(uint32_t doc_nr, uint16_t *doc_type, size_t *len);

/* Прочитать TLV фискального документа */
extern uint8_t kkt_read_doc_tlv(uint8_t *data, size_t *len);

/* Сброс ФН */
extern uint8_t kkt_reset_fs(uint8_t b);


#endif		/* KKT_KKT_H */
