/* ���祢�� ���ଠ�� �ନ����. (c) gsr 2001, 2004, 2020 */

#if !defined TKI_H
#define TKI_H

#include "ds1990a.h"
#include "gd.h"
#include "md5.h"

/* ��� 䠩�� �����᪮�� ����� �ନ���� */
#define TERM_NR_FILE		"/sdata/term-number.txt"
/* ��� 䠩�� �ਢ離� USB-��᪠ */
#define USB_BIND_FILE		"/sdata/disk.dat"
/* ��� 䠩�� ���祢��� ����ਡ�⨢� VipNet */
#define IPLIR_DST		"/sdata/iplir.dst"
/* ��� 䠩�� �ਢ離� ���祢��� ����ਡ�⨢� VipNet */
#define IPLIR_BIND_FILE		"/sdata/iplir.dat"
/* ��� 䠩�� ��஫� ���祢�� ��� VipNet */
#define IPLIR_PSW_DATA		"/sdata/iplirpsw.dat"

/* ���ଠ�� � �ନ���� (�� ��������� �� ���������� �ନ���� */
struct term_key_info{
	struct md5_hash check_sum;	/* ����஫쭠� �㬬� ��⠫��� ������ */
	struct md5_hash srv_keys[NR_SRV_KEYS];	/* ����஥�� ���� */
	struct md5_hash dbg_keys[NR_DBG_KEYS];	/* �⫠���� ���� */
	term_number tn;				/* �����᪮� ����� �ନ���� */
};

/* �����䨪���� ����� � tki */
enum {
	TKI_CHECK_SUM,
	TKI_SRV_KEYS,
	TKI_DBG_KEYS,
	TKI_NUMBER,
};

/* ���� ���祢�� ���ଠ樨 (�࠭���� � ����஢����� ����) */
extern struct term_key_info tki;
/* ��⠭�� 䠩� �ਢ離� ���祢�� ��� */
extern struct md5_hash iplir_bind;
/* ��⠭�� 䠩� ������᪮� ��業��� */
extern struct md5_hash bank_license;

/* ����� �஢�ન */
extern bool tki_ok;	/* 䠩� .tki ��⠭ � �஢�७ */
extern bool usb_ok;	/* �����㦥� USB-��� ��� ������� �ନ���� */
extern bool iplir_ok;	/* �����㦥� dst-䠩� ��� �⮣� �ନ���� */
extern bool bank_ok;	/* ������ �ࠢ���� 䠩� ������᪮� ��業��� */

/* ����⠭���� ���襣� � ����襣� ���㡠�⮢ */
static inline uint8_t swap_nibbles(uint8_t b)
{
	return (b << 4) | (b >> 4);
}

extern void encrypt_data(uint8_t *p, int l);
extern void decrypt_data(uint8_t *p, int l);

extern bool read_tki(const char *path, bool create);
extern bool write_tki(const char *path);
extern bool get_tki_field(const struct term_key_info *info, int name, uint8_t *val);
extern bool set_tki_field(struct term_key_info *info, int name, const uint8_t *val);
extern void check_tki(void);
extern void check_usb_bind(void);
extern void check_iplir_bind(void);
extern void check_bank_license(void);

#endif		/* TKI_H */
