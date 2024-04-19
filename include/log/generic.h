/* ��騥 �㭪樨 ��� ࠡ��� � ����஫�묨 ���⠬�. (c) gsr 2008. */

#if !defined LOG_GENERIC_H
#define LOG_GENERIC_H

#if defined __cplusplus
extern "C" {
#endif

#include "blimits.h"
#include "genfunc.h"
#include "sysdefs.h"

/* ���� ���� ��������� ����஫��� ���� */
struct log_header {
	uint32_t tag;		/* �ਧ��� ��������� (��� ������ �� ᢮�) */
	uint32_t len;		/* ����� �� ��� ��� ��������� */
	uint32_t n_recs;	/* �᫮ ����ᥩ */
	uint32_t head;		/* ᬥ饭�� ��ࢮ� ����� */
	uint32_t tail;		/* ᬥ饭�� ��᫥���� ����� */
	uint32_t cur_number;	/* ����� ⥪�饩 ����� */
};

/*
 * ���� ����஫쭮� �����. ����� ����� ����� ���� ᬥ饭��� ���������
 * ᮮ⢥�����饩 �����. �.�. � ��砫� ��� ���� ���������, � �� ॠ�쭮�
 * ᬥ饭�� ������ ���� ����� ���. �㫥��� ᬥ饭�� ������砥� ���ᯮ��㥬�
 * �����. ���� 横���᪠�, �.�. �� ���⨦���� ���� ���ᨢ�, ����������
 * �த�������� � ��� ��砫�.
 */
/* 30.08.04: ����⠬� ����� �⠫� �������� ⨯� map_entry_t */
/* 29.10.18: � �������� ��������� ���� tag */
struct map_entry_t {
	uint32_t offset;		/* ᬥ饭�� ��������� ����� � �� */
	uint32_t number;		/* ����� ����� */
	struct date_time dt;		/* �६� ᮧ����� ����� */
	uint32_t tag;			/* �������⥫�� ����� */
} __attribute__((__packed__));

/* ������� ��� ࠡ��� � ����஫쭮� ���⮩ ��������� ⨯� */
struct log_handle {
	struct log_header *hdr;		/* ��������� */
/*
 * log_header -- �� ���� ���� ���������, � ��� �����⭮�� ⨯� ��
 * � ��������� ����� ���� �������⥫�� ����. hdr_len -- �� ����� ���������
 * ��� �����⭮�� ⨯� ����஫쭮� �����.
 */
	uint32_t hdr_len;
	uint32_t full_len;		/* ������ ����� 䠩�� ����஫쭮� ����� */
	const char *log_type;		/* ⨯ ����஫쭮� ����� (���/���/��� � �.�.) */
	const char *log_name;		/* ��� 䠩�� ����஫쭮� ����� */
/* ���ਯ�� ��� �⥭�� 䠩�� ��. ����� �ᥣ��. */
	int rfd;
	int wfd;			/* ���ਯ�� ��� ����� �� �� */
	struct map_entry_t *map;	/* ���� ����஫쭮� ����� */
	uint32_t map_size;		/* ���ᨬ��쭮� ������⢮ ����⮢ � ���� */
	uint32_t map_head;		/* ������ ��ࢮ�� ����� ����� */
/* ������᪨� ����⢨� �� ����⨨ ����஫쭮� ����� (����� ���� NULL) */
	void (*on_open)(struct log_handle *hlog);
/* ������᪨� ����⢨� �� �����⨨ ����஫쭮� ����� (����� ���� NULL) */
	void (*on_close)(struct log_handle *hlog);
/* ���樠������ ��������� ����஫쭮� ����� */
	void (*init_log_hdr)(struct log_handle *hlog);
/* ��뢠���� �� ���⪥ ����஫쭮� ����� */
	void (*clear)(struct log_handle *hlog);
/* ���������� ����� ����஫쭮� ����� */
	bool (*fill_map)(struct log_handle *hlog);
/* �⥭�� ��������� ����஫쭮� ����� */
	bool (*read_header)(struct log_handle *hlog);
/* ������ ��������� ����஫쭮� ����� */
	bool (*write_header)(struct log_handle *hlog);
};

#define try_fn(fn) ({ if (!fn) return false; })

/* ���樠������ �����⥬� ࠡ��� � ����஫�묨 ���⠬� */
extern bool log_init_generic(void);
/* �������� ����஫쭮� ����� ��������� ⨯� */
extern bool log_create(struct log_handle *hlog);
/* ����樮��஢���� � �⥭�� �����뢭��� ����� ������ �� �� */
extern bool log_atomic_read(struct log_handle *hlog, uint32_t offs, uint8_t *buf,
		uint32_t l);
/* �⥭�� 䠩�� ����஫쭮� ����� */
extern bool log_read(struct log_handle *hlog, uint32_t offs, uint8_t *buf, uint32_t l);
/* ����樮��஢���� � ������ �����뢭��� ����� ������ �� �� */
extern bool log_atomic_write(struct log_handle *hlog, uint32_t offs, uint8_t *buf,
		uint32_t l);
/* ������ � 䠩� �������� ����� */
extern bool log_write(struct log_handle *hlog, uint32_t offs, uint8_t *buf, uint32_t l);
/* ��砫� ����� �� ����஫��� ����� */
extern bool log_begin_write(struct log_handle *hlog);
/* ����砭�� ����� �� ����஫��� ����� */
extern void log_end_write(struct log_handle *hlog);
/* �����ﭨ� ����� ���� ᬥ饭�ﬨ � ����楢�� ���� */
extern uint32_t log_delta(struct log_handle *hlog, uint32_t tail, uint32_t head);
/* �����祭�� ������ � ����楢�� ���� */
extern uint32_t log_inc_index(struct log_handle *hlog, uint32_t index, uint32_t delta);
/* ��।������ ᢮������� ����࠭�⢠ �� �� */
extern uint32_t log_free_space(struct log_handle *hlog);
/* ��।������ ����஢ �ࠩ��� ����ᥩ �� �� */
extern bool log_get_boundary_numbers(struct log_handle *hlog,
		uint32_t *n0, uint32_t *n1);
/* ��।������ ����� ���������, ��࠭�祭���� �����ᠬ� */
extern uint32_t log_get_interval_len(struct log_handle *hlog,
		uint32_t index1, uint32_t index2);
/*
 * ���� ����� �� �� ������. �����頥� ������ ����� �� �� ��� -1,
 * �᫨ ������ �� �������
 */
extern uint32_t log_find_rec_by_number(struct log_handle *hlog, uint32_t number);
/*
 * ���� ����� �� �� �� ��� ᮧ�����. �����頥� ������ ����� � ������襩
 * ��⮩ ᮧ�����.
 */
extern uint32_t log_find_rec_by_date(struct log_handle *hlog,
		struct date_time *dt);
/* ���⪠ ����஫쭮� ����� */
extern bool log_clear(struct log_handle *hlog, bool need_write);
/*
 * �᫨ �� �஢�થ ����� �� �� �㤥� �����㦥�� �訡��, �� � ��
 * ��᫥���騥 ����� ���� ���襭�.
 */
extern bool log_truncate(struct log_handle *hlog, uint32_t index, uint32_t tail);
/* ����⨥ ����஫쭮� ����� */
extern bool log_open(struct log_handle *hlog, bool can_create);
/* �����⨥ ����஫쭮� ����� */
extern void log_close(struct log_handle *hlog);
/* ���������� ���� �����᪮�� ����� �ਭ�� */
extern void fill_prn_number(uint8_t *dst, const char *src, size_t len);

/* ���� ���� ����஫쭮� ����� */
extern uint8_t log_prn_buf[PRN_BUF_LEN];
/* ����� ������ � ���� ���� */
extern uint32_t log_prn_data_len;

/* ������ ᨬ���� � ���� ���� ��� ��४���஢�� */
extern bool prn_write_char_raw(uint8_t c);
/* ������ ᨬ���� � ���� ���� � ��४���஢��� */
extern bool prn_write_char(uint8_t c);
/* ������ ��㯯� ᨬ����� ��� ��४���஢�� */
extern bool prn_write_chars_raw(const char *s, int n);
/* ������ � ���� ��ப� � ��४���஢��� */
extern bool prn_write_str(const char *str);
/* ������ � ���� ��ப� �� �ଠ�� � ��४���஢��� */
extern bool prn_write_str_fmt(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
/* ������ � ���� ���� ᨬ���� ���� ��ப� */
extern bool prn_write_eol(void);
/* ������ � ���� ���� ⥪��� ���� � �६��� */
extern bool prn_write_cur_date_time(void);
/* ������ � ���� ���� �������� ���� � �६��� */
extern bool prn_write_date_time(struct date_time *dt);
/* ���� ���� ���� � ��砫쭮� ���ﭨ� */
extern bool log_reset_prn_buf(void);
/* �뢮� �� ����� ����-���� */
extern bool log_print_bcode(const uint8_t *log_data, uint32_t log_data_len,
	uint32_t *log_data_index);

#if defined __cplusplus
}
#endif

#endif		/* LOG_GENERIC_H */
