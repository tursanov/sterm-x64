/* ���� � ����⠭�� ��� ࠡ��� � �� ���. (c) gsr 2018-2019 */

#if !defined KKT_FS_H
#define KKT_FS_H

#include <stdint.h>
#include "sysdefs.h"

/* ���/�६�, �ᯮ��㥬� � �� */
struct kkt_fs_date {
	uint32_t year;		/* ��� (���ਬ��, 2018) */
	uint32_t month;		/* ����� (1 -- 12) */
	uint32_t day;		/* ���� (1 -- 31) */
};

#define KKT_FS_DATE_LEN		3

struct kkt_fs_time {
	uint32_t hour;		/* �� (0 -- 23) */
	uint32_t minute;	/* ����� (0 -- 59) */
};

#define KKT_FS_TIME_LEN		2

struct kkt_fs_date_time {
	struct kkt_fs_date date;
	struct kkt_fs_time time;
};

#define KKT_FS_DATE_TIME_LEN	(KKT_FS_DATE_LEN + KKT_FS_TIME_LEN)

/* ���� ����� �� */
#define FS_LIFESTATE_NONE	0	/* ����ன�� */
#define FS_LIFESTATE_FREADY	1	/* ��⮢ � �᪠����樨 */
#define FS_LIFESTATE_FMODE	3	/* �᪠��� ०�� */
#define FS_LIFESTATE_POST_FMODE	7	/* �����᪠��� ०��, ���� ��।�� �� � ��� */
#define FS_LIFESTATE_ARCH_READ	15	/* �⥭�� ������ �� ��娢� �� */

/* ����騩 ���㬥�� */
/* ��� ����⮣� ���㬥�� */
#define FS_CURDOC_NOT_OPEN		0x00
/* ����� � ॣ����樨 ��� */
#define FS_CURDOC_REG_REPORT		0x01
/* ����� �� ����⨨ ᬥ�� */
#define FS_CURDOC_SHIFT_OPEN_REPORT	0x02
/* ���ᮢ� 祪 */
#define FS_CURDOC_CHEQUE		0x04
/* ����� � �����⨨ ᬥ�� */
#define FS_CURDOC_SHIFT_CLOSE_REPORT	0x08
/* ����� � �����⨨ �᪠�쭮�� ०��� */
#define FS_CURDOC_FMODE_CLOSE_REPORT	0x10
/* ����� ��ண�� ���⭮�� */
#define FS_CURDOC_BSO			0x11
/* ���� �� ��������� ��ࠬ��஢ ॣ����樨 ��� � �裡 � ������� �� */
#define FS_CURDOC_REG_PARAMS_REPORT_ON_FS_CHANGE	0x12
/* ���� �� ��������� ��ࠬ��஢ ॣ����樨 ��� */
#define FS_CURDOC_REG_PARAMS_REPORT	0x13
/* ���ᮢ� 祪 ���४樨 */
#define FS_CURDOC_CORRECTION_CHEQUE	0x14
/* ��� ���४樨 */
#define FS_CURDOC_CORRECTION_BSO	0x15
/* ���� � ⥪�饬 ���ﭨ� ���⮢ */
#define FS_CURDOC_CURRENT_PAY_REPORT	0x17

/* ����� ���㬥�� */
#define FS_NO_DOC_DATA			0	/* ��� ������ ���㬥�� */
#define FS_HAS_DOC_DATA			1	/* ����祭� ����� ���㬥�� */

/* ����ﭨ� ᬥ�� */
#define FS_SHIFT_CLOSED			0	/* ᬥ�� ������ */
#define FS_SHIFT_OPENED			1	/* ᬥ�� ����� */

/* �।�०����� �� ���௠��� ����ᮢ �� */

/* ��筠� ������ �� */
#define FS_ALERT_CC_REPLACE_URGENT	0x01
/* ���௠��� ����� �� */
#define FS_ALERT_CC_EXHAUST		0x02
/* ��९������� ����� (90% ���������) */
#define FS_ALERT_MEMORY_FULL		0x04
/* �ॢ�襭� �६� �������� �⢥� �� ��� */
#define FS_ALERT_RESP_TIMEOUT		0x08
/* �⪠� �� ����� �ଠ⭮-�����᪮�� ����஫� (�ਧ��� ��।����� � ���⢥ত���� �� ���) */
#define FS_ALERT_FLC			0x10
/* �ॡ���� ����ன�� ��� (�ਧ��� ��।����� � ���⢥ত���� �� ���) */
#define FS_ALERT_SETUP_REQUIRED		0x20
/* ��� ���㫨஢��(�ਧ��� ��।����� � ���⢥ত���� �� ���) */
#define FS_ALERT_OFD_REVOKED		0x40
/* ����᪠� �訡�� �� */
#define FS_ALERT_CRITRICAL		0x80

/* ����ﭨ� �� */
struct kkt_fs_status {
	uint8_t life_state;		/* ���ﭨ� 䠧� ����� */
	uint8_t current_doc;		/* ⥪�騩 ���㬥�� */
	uint8_t doc_data;		/* ����� ���㬥�� */
	uint8_t shift_state;		/* ���ﭨ� ᬥ�� */
	uint8_t alert_flags;		/* 䫠�� �।�०����� */
	struct kkt_fs_date_time dt;	/* ��� � �६� ��᫥����� ���㬥�� */
#define KKT_FS_NR_LEN	16
	char nr[KKT_FS_NR_LEN + 1];	/* ����� �� */
	uint32_t last_doc_nr;		/* ����� ��᫥����� �� */
};

/* �ப ����⢨� �� */
struct kkt_fs_lifetime {
	struct kkt_fs_date_time dt;
	uint32_t reg_complete;
	uint32_t reg_remain;
};

/* ����� �� */
struct kkt_fs_version {
#define KKT_FS_VERSION_LEN		16
	char version[KKT_FS_VERSION_LEN + 1];
	uint32_t type;
};

/* ���ଠ�� � ᬥ�� */
struct kkt_fs_shift_state {
/* ����ﭨ� ᬥ�� (0 -- ������, 1 -- �����) */
	bool opened;
/* ����� ᬥ��. �᫨ ᬥ�� ������, �  ����� ��᫥���� �����⮩ ᬥ��,
 * �᫨ �����, � ����� ⥪�饩 ᬥ��.
 */
	uint32_t shift_nr;
/* ����� 祪�. �᫨ ᬥ�� ������, � �᫮ ���㬥�⮢ � �।��饩 �����⮩ ᬥ��
 * (0, �᫨ �� ��ࢠ� ᬥ��). �᫨ ᬥ�� �����, �� ��� �� ������ 祪�, � 0. 
 * � ��⠫��� �����  ����� ��᫥����� ��ନ஢������ 祪�
 */
	uint32_t cheque_nr;
};

/* ����ﭨ� ������ � ��� */
#define FS_TS_CONNECTED		0x01	/* �࠭ᯮ�⭮� ᮥ������� ��⠭������ */
#define FS_TS_HAS_REQ		0x02	/* ���� ᮮ�饭�� ��� ��।�� � ��� */
#define FS_TS_WAIT_ACK		0x04	/* �������� ���⠭樨 �� ��� */
#define FS_TS_HAS_CMD		0x08	/* ���� ������� �� ��� */
#define FS_TS_SETTINGS_CHANGED	0x10	/* ���������� ����ன�� ᮥ������� � ��� */
#define FS_TS_WAIT_RESP		0x20	/* �������� �⢥� */

/* ����� ���ଠ樮����� ������ � ��� */
struct kkt_fs_transmission_state {
	uint8_t state;		/* ����� ���ଠ樮����� ������ */
	bool read_msg_started;	/* ��砫��� �⥭�� ᮮ�饭�� ��� ��� */
	size_t sndq_len;	/* ����� ��।� ᮮ�饭�� ��� ��।�� */
	uint32_t first_doc_nr;	/* ����� ��ࢮ�� ���㬥�� ��� ��।�� */
	struct kkt_fs_date_time first_doc_dt;	/* ���/�६� ᮧ����� ��ࢮ�� ���㬥�� */
};

/* ���� �᪠���� ���㬥�⮢ */
#define NONE			0	/* ��� */
#define REGISTRATION		1	/* ���� � ॣ����樨 */
#define RE_REGISTRATION		11	/* ���� � ���ॣ����樨 */
#define OPEN_SHIFT		2	/* ����⨥ ᬥ�� */
#define CALC_REPORT		21	/* ���� � ���ﭨ� ���⮢ */
#define CHEQUE			3	/* 祪 */
#define CHEQUE_CORR		31	/* 祪 ����樨 */
#define BSO			4	/* ��� */
#define BSO_CORR		41	/* ��� ���४樨 */
#define CLOSE_SHIFT		5	/* �����⨥ ᬥ�� */
#define CLOSE_FS		6	/* �����⨥ �� */
#define OP_COMMIT		7	/* ���⢥ত���� ������ */

/* ���ଠ�� � ࠧ����� ⨯�� ���㬥�⮢ */

/* ����� � ॣ����樨 (FrDocType::Registration) */
struct kkt_register_report {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	char inn[12];
	char reg_number[20];
	uint8_t tax_system;
	uint8_t mode;
} __attribute__((__packed__));

/* ����� � ���ॣ����樨 (FrDocType::ReRegistration) */
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

/* ���/祪 ���४樨 (FrDocType::Cheque/FrDocType::ChequeCorr/FrDocType::BSO/FrDocType::BSOCorr) */
struct kkt_cheque_report {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	uint8_t type;
	uint8_t sum[5];
} __attribute__((__packed__));

/* ����⨥/�����⨥ ᬥ�� (FrDocType::OpenShift/FrDocType::CloseShift) */
struct kkt_shift_report {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	uint16_t shift_nr;
} __attribute__((__packed__));

/* �����⨥ �᪠�쭮�� ०��� (FrDocType::CloseFS) */
struct kkt_close_fs {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	char inn[12];
	char reg_number[20];
} __attribute__((__packed__));

/* ����� � ���ﭨ� ����⮢ (FrDocType::CalcReport) */
struct kkt_calc_report {
	uint8_t dt[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
	uint32_t nr_uncommited;
	uint8_t first_uncommited_dt[3];
} __attribute__((__packed__));

/* ���⠭�� ��� */
struct kkt_fdo_ack {
	uint8_t dt[5];
	uint8_t fiscal_sign[18];
	uint32_t doc_nr;
} __attribute__((__packed__));


/* ���ଠ�� � ��������� ���㬥�� */
struct kkt_fs_find_doc_info {
	uint32_t doc_type;		/* ⨯ ���㬥�� */
	bool fdo_ack;			/* �ਧ��� ���⢥ত���� ��� */
};

/* ���ଠ�� � ���⠭樨 ��� */
struct kkt_fs_fdo_ack {
	struct kkt_fs_date_time dt;
	uint8_t fiscal_sign[18];
	uint32_t doc_nr;
};

/* ���ଠ�� �� STLV �᪠�쭮�� ���㬥�� */
struct kkt_fs_doc_stlv_info {
	uint32_t doc_type;		/* ⨯ ���㬥�� */
	size_t len;			/* ����� ���㬥�� */
};

#endif		/* KKT_FS_H */
