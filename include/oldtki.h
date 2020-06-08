/*
 * �।��騥 ��ਠ��� ���祢�� ���ଠ樨 �ନ����. (c) gsr 2004.
 * �ᥣ� ������� 3 ��ਠ�� ���祢�� ���ଠ樨:
 * ࠭��� ��ਠ�� (early_tki) -- �ᯮ�짮����� �� ���ᨨ 2.2.3 �����⥫쭮;
 * ���� ��ਠ�� (old_tki) -- ��稭�� � ���ᨨ 2.2.4 �� 2.7 �����⥫쭮;
 * ���� ��ਠ�� (tki) -- ��稭�� � ���ᨨ 2.8.
 */
#if !defined OLDTKI_H
#define OLDTKI_H

#include "ds1990a.h"
#include "gd.h"
#include "md5.h"

/* ��� ࠭���� 䠩�� ���祢�� ���ଠ樨 */
#define STERM_EARLY_TKI_FILE	_(".key")

/* ������ ��ਠ�� 䠩�� ���祢�� ���ଠ樨 */
/* ����� �ନ���� */
struct build_version{
	int version;
	int subversion;
	int modification;
};

/* ���� DS1990A */
/* ��� ���� */
enum {
	key_service = 0,	/* �ࢨ�� ���� */
	key_work,		/* ࠡ�稩 ���� */
};

/* ���ᠭ�� ���� */
struct ds_descr {
	int ds_type;
	ds_number dsn;
};

/* ������⢮ ���祩 � ࠭��� ��ਠ�� ���祢�� ���ଠ樨 */
#define EARLY_NR_KEYS		4

struct early_term_key_info{
	struct ds_descr keys[EARLY_NR_KEYS];
	term_number tn;
	byte pad1[3];
	struct build_version term_build;
	word term_check_sum;
	byte pad2[2];
};

/*
 * �� �ନ஢���� ࠭���� ��ਠ�� 䠩�� ���祢�� ���ଠ樨 �ନ����
 * �� ���뢠���� �����-�������⥫� � ���� �������� (pad2).
 */
#define EARLY_TKI_SIZE1		xoffsetof(struct early_term_key_info, pad2)

/* �����஢�� ������ ��� ��ண� ��ਠ�� */
extern void early_decrypt_data(byte *p, int l);

/* ���� ��ਠ�� ���祢�� ���ଠ樨 �ନ���� */
/* ������⢮ ���祩 � ��஬ ��ਠ�� */
#define OLD_NR_KEYS		EARLY_NR_KEYS

struct old_term_key_info{
	struct md5_hash check_sum;
	struct ds_descr keys[OLD_NR_KEYS];
	term_number tn;
};

#endif		/* OLDTKI_H */
