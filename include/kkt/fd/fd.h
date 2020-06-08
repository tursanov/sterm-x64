#ifndef FD_H
#define FD_H

//#include "kkt/fd/ad.h"

// режимы работы
#define REG_MODE_CRYPTO		0x01
#define REG_MODE_OFFLINE	0x02
#define REG_MODE_AUTOMAT	0x04
#define REG_MODE_SERVICE	0x08
#define REG_MODE_BSO		0x10
#define REG_MODE_INTERNET	0x20

// параметры регистрации
typedef struct fd_registraton_params_t {
    char user_name[256+1]; // Наименование пользователя
    char user_inn[12+1]; // ИНН пользователя
    char pay_address[256+1]; // Адрес расчетов
    char pay_place[256+1]; // Место расчетов
    uint8_t tax_systems; // системы налогообложения
    char reg_number[20+1]; // РНМ
    uint8_t reg_modes; // режимы работы
    char automat_number[20+1]; // номер автомата
    char cashier[64+1]; // кассир
    char cashier_inn[12+1]; // ИНН кассира
    char tax_service_site[256+1]; // адрес сайта ФНС
    char cheque_sender_email[64+1]; // адрес эл. почты отправителя чеков
    char ofd_name[256+1]; // наименование ОФД
    char ofd_inn[12+1]; // ИНН ОФД
    uint8_t rereg_reason; // причины перерегистрации (или 0, если выполняется регистрация)
} fd_registration_params_t;

// параметры открытия/закрытия смены
typedef struct fd_shift_params_t {
    char cashier[64+1]; // кассир
    char cashier_inn[12+1]; // инн кассира
} fd_shift_params_t;

// параметры закрытия ФН
typedef fd_shift_params_t fd_close_fs_params_t;

// тип суммы
typedef enum fd_sum_type_t {
    fd_sum_type_cash, // наличные
    fd_sum_type_electronic, // электронные
    fd_sum_type_prepayment, // предоплата
    fd_sum_type_postpayment, // постоплата
    fd_sum_type_b2b, // встречное преставление
    fd_sum_type_max
} fd_sum_type_t;

// тип НДС
typedef enum fd_vat_rate_t {
    fd_vat_rate_18 = 1, // НДС 18%
    fd_vat_rate_10, // НДС 10%
    fd_vat_rate_18_118, // НДС 18/118
    fd_vat_rate_10_110, // НДС 10/110
    fd_vat_rate_0, // НДС 0%
    fd_vat_rate_no, // НДС не облагается
    fd_vat_rate_max = fd_vat_rate_no
} fd_vat_rate_t;

// параметры чека коррекции
typedef struct fd_cheque_corr_params_t {
    uint8_t pay_type; // признак расчета
    uint8_t tax_system; // система налогообложения
    uint8_t corr_type; // тип коррекции
    struct {
        char descr[256+1]; // описание коррекции
        time_t date; // дата
        char reg_number[32+1]; // номер предписания
    } corr_reason; // основание для коррекции
    uint64_t sum[fd_sum_type_max]; // суммы
    int64_t nds[fd_vat_rate_max]; // ставки НДС. Если ставка не указана, то значение д.б. отрицательным
} fd_cheque_corr_params_t;

// получение последней ошибки
int fd_get_last_error(const char **error);
void fd_set_error(uint8_t doc_type, uint8_t status, uint8_t *err_info, size_t err_info_len);
// формирование фискального документа
int fd_create_doc(uint8_t doc_type, const uint8_t *pattern_footer, size_t pattern_footer_size);
// печать последнего фискального документа
int fd_print_last_doc(uint8_t doc_type);
// печать фискального документа по номеру
int fd_print_doc(uint8_t doc_type, uint32_t doc_no);

// регистрация/перерегистрация
int fd_registration();
// открытие смены
int fd_open_shift();
// закрытие смены
int fd_close_shift();
// отчет о текущих расчетах
int fd_calc_report();
// закрытие ФН
int fd_close_fs();


#endif
