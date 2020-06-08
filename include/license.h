/* ����� � ��業��ﬨ ��� � ���. (c) gsr 2005-2006, 2011, 2019 */

#if !defined LICENSE_H
#define LICENSE_H

#include "md5.h"

/* ���ଠ�� � ��業��� ��� */
struct bank_license_info {
	struct md5_hash number;		/* �� �����᪮�� ����� */
	struct md5_hash license;	/* ��業��� */
};

/* ���ᨬ��쭮� ������⢮ ��業��� ��� � 䠩�� */
#define MAX_BANK_LICENSES		10000

/* ���ଠ�� � ��業��� ��� */
struct lprn_license_info {
	struct md5_hash number;		/* �� �����᪮�� ����� */
	struct md5_hash license;	/* ��業��� */
};

/* ���ᨬ��쭮� ������⢮ ��業��� ��� � 䠩�� */
#define MAX_LPRN_LICENSES		MAX_BANK_LICENSES

/* ��� 䠩�� �� �����᪮�� ����� �ନ���� (��� �஢�ન ��業���) */
#define TERM_NUMBER_FILE		"/sdata/disk.dat"

#endif		/* LICENSE_H */
