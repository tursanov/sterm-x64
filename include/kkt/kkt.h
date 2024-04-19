/* ����� � ��� �� ����㠫쭮�� COM-�����. (c) gsr 2018-2020, 2024 */

#if !defined KKT_KKT_H
#define KKT_KKT_H

#if defined __cplusplus
extern "C" {
#endif

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

/* ��⠭����� ����䥩� ����������⢨� � ��� */
extern uint8_t kkt_set_fdo_iface(int fdo_iface);
/* ��⠭����� ���� ��� */
extern uint8_t kkt_set_fdo_addr(uint32_t fdo_ip, uint16_t fdo_port);

/* ����� ��६����� ����� */
struct kkt_var_data {
	size_t len;
	const uint8_t *data;
};

/* �������� �� ������ � ��� */
struct kkt_fdo_cmd {
	uint8_t cmd;
	struct kkt_var_data data;
};
/* ������� �������� �� ������ � ��� */
extern uint8_t kkt_get_fdo_cmd(uint8_t prev_op, uint16_t prev_op_status,
	uint8_t *cmd, uint8_t *data, size_t *data_len);
/* ��।��� � ��� �����, ����祭�� �� ��� */
extern uint8_t kkt_send_fdo_data(const uint8_t *data, size_t data_len);

/* ��������� �ᮢ ॠ�쭮�� �६��� ��� */
struct kkt_rtc_data {
	uint16_t year;		/* ��� (���ਬ��, 2019) */
	uint8_t  month;		/* ����� (1-12) */
	uint8_t  day;		/* ���� (1-31) */
	uint8_t  hour;		/* �� (1-23) */
	uint8_t  minute;	/* ����� (1-59) */
	uint8_t  second;	/* ᥪ㭤� (1-59) */
};
/* ������ ��������� �ᮢ ॠ�쭮�� �६��� */
extern uint8_t kkt_get_rtc(struct kkt_rtc_data *rtc);
/* ��⠭����� ��� ॠ�쭮�� �६��� */
extern uint8_t kkt_set_rtc(time_t t);

/* ���ଠ�� � ��᫥���� ��ନ஢����� � ��᫥���� �⯥�⠭��� �᪠���� ���㬥��� */
struct kkt_last_doc_info {
	uint32_t last_nr;
	uint16_t last_type;
	uint32_t last_printed_nr;
	uint16_t last_printed_type;
};
/* ������� ���ଠ�� � ��᫥���� ��ନ஢����� � ��᫥���� �⯥�⠭��� ���㬥��� */
extern uint8_t kkt_get_last_doc_info(struct kkt_last_doc_info *ldi, uint8_t *err_info,
	size_t *err_info_len);

/* ���ଠ�� � ��᫥���� �⯥�⠭��� ���㬥�� */
struct kkt_last_printed_info {
	uint32_t nr;
	uint32_t sign;
	uint16_t type;
};
/* �������� ��᫥���� ��ନ஢���� ���㬥�� */
extern uint8_t kkt_print_last_doc(uint16_t doc_type, const uint8_t *tmpl, size_t tmpl_len,
	struct kkt_last_printed_info *lpi, uint8_t *err_info, size_t *err_info_len);

/* ����� �ନ஢���� ���㬥�� */
extern uint8_t kkt_begin_doc(uint16_t doc_type, uint8_t *err_info, size_t *err_info_len);

/* ��।��� ����� ���㬥�� */
extern uint8_t kkt_send_doc_data(const uint8_t *data, size_t len, uint8_t *err_info,
	size_t *err_info_len);

/* ���ଠ�� � ��ନ஢����� ���㬥�� */
struct kkt_doc_info {
	uint32_t nr;
	uint32_t sign;
};
/* �������� �ନ஢���� ���㬥�� */
extern uint8_t kkt_end_doc(uint16_t doc_type, const uint8_t *tmpl, size_t tmpl_len,
	uint32_t timeout_factor, struct kkt_doc_info *di,
	uint8_t *err_info, size_t *err_info_len);

/* �������� ���㬥�� �� ������ */
extern uint8_t kkt_print_doc(uint32_t doc_nr, const uint8_t *tmpl, size_t tmpl_len,
	struct kkt_last_printed_info *lpi, uint8_t *err_info, size_t *err_info_len);

/* ����ந�� �⥢�� ����䥩� ��� */
extern uint8_t kkt_set_net_cfg(uint32_t ip, uint32_t netmask, uint32_t gw);
/* ����ந�� GPRS � ��� */
extern uint8_t kkt_set_gprs_cfg(const char *apn, const char *user, const char *password);

/* ���ଠ�� � �મ�� ���� ��� */
struct kkt_brightness {
	uint8_t current;
	uint8_t def;
	uint8_t max;
};

extern struct kkt_brightness kkt_brightness;

/* ������� ���ଠ�� � �મ�� ���� ��� */
extern uint8_t kkt_get_brightness(struct kkt_brightness *brightness);
/* ����ந�� �મ��� ���� � ��� */
extern uint8_t kkt_set_brightness(uint8_t brightness);

/* ����ந�� ��� � ᮮ⢥��⢨� � ���䨣��樥� �ନ���� */
extern uint8_t kkt_set_cfg(void);

/* ������� ����� �� */
extern uint8_t kkt_get_fs_status(struct kkt_fs_status *fs_status);

/* ������� ����� �� */
extern uint8_t kkt_get_fs_nr(char *fs_nr);

/* ������� �ப ����⢨� �� */
extern uint8_t kkt_get_fs_lifetime(struct kkt_fs_lifetime *lt);

/* ������ ����� �� */
extern uint8_t kkt_get_fs_version(struct kkt_fs_version *ver);

/* ������ ���ଠ�� � ��᫥���� �訡�� �� */
extern uint8_t kkt_get_fs_last_error(uint8_t *err_info, size_t *err_info_len);

/* ������� ���ଠ�� � ⥪�饩 ᬥ�� */
extern uint8_t kkt_get_fs_shift_state(struct kkt_fs_shift_state *ss);

/* ������� ����� ���ଠ樮����� ������ � ��� */
extern uint8_t kkt_get_fs_transmission_state(struct kkt_fs_transmission_state *ts);

/* ���� �᪠��� ���㬥�� �� ������ */
extern uint8_t kkt_find_fs_doc(uint32_t nr, bool need_print,
	struct kkt_fs_find_doc_info *fdi, uint8_t *data, size_t *data_len);

/* ���� ���⢥ত���� ��� �� ������ */
extern uint8_t kkt_find_fdo_ack(uint32_t nr, bool need_print, struct kkt_fs_fdo_ack *fdo_ack);

/* ������� ������⢮ ���㬥�⮢, �� ����� ��� ���⠭樨 ��� */
extern uint8_t kkt_get_unconfirmed_docs_nr(uint32_t *nr_docs);

/* ������� ����� ��᫥���� ॣ����樨 */
extern uint8_t kkt_get_last_reg_data(uint8_t *data, size_t *data_len);

/* ������� ���ଠ�� �� STLV �᪠�쭮�� ���㬥�� */
extern uint8_t kkt_get_doc_stlv(uint32_t doc_nr, uint16_t *doc_type, size_t *len);

/* ������ TLV �᪠�쭮�� ���㬥�� */
extern uint8_t kkt_read_doc_tlv(uint8_t *data, size_t *len);

/* �������� �஥����� ���㬥�� */
extern uint8_t kkt_print_vf(const uint8_t *data, size_t len);

/* ���� �� */
extern uint8_t kkt_reset_fs(uint8_t b);

/* �����頥� ����稥 ��ࠬ��� ��� */
extern bool kkt_has_param(const char *name);

#if defined __cplusplus
}
#endif

#endif		/* KKT_KKT_H */
