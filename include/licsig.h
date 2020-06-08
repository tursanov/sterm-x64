/* ����� � �ਧ������ 㤠����� ��業���. (c) gsr 2011, 2019 */

#if !defined LICSIG_H
#define LICSIG_H

#include <stdlib.h>
#include "sysdefs.h"

/* ������ ᥪ�� ���⪮�� ��᪠ */
#define SECTOR_SIZE			512

/*
 * NB: �᫨ � MBR �����㦥� �ਧ��� 㤠������ ��業���, � �� �ᯮ�짮�����
 * ��� ����ୠ� ��⠭���� �� ����᪠����.
 */

/* �ਧ��� 㤠������ ��業��� ��� */
#define BANK_LIC_SIGNATURE		0x8680
/* ���饭�� �ਧ���� 㤠������ ��業��� ��� � MBR */
#define BANK_LIC_SIGNATURE_OFFSET	0x1fa

/* �ਧ��� 㤠������ ��業��� ��� */
#define LPRN_LIC_SIGNATURE		0x8080
/* ���饭�� �ਧ���� 㤠������ ��業��� ��� � MBR */
#define LPRN_LIC_SIGNATURE_OFFSET	0x1da

/* �஢�ઠ ������⢨� �ਧ���� 㤠����� ��業��� */
extern bool check_lic_signature(off_t offs, uint16_t sig);
/* ������ �ਧ���� 㤠����� ��業��� */
extern bool write_lic_signature(off_t offs, uint16_t sig);
/* ���⪠ �ਧ���� 㤠����� ��業��� */
extern bool clear_lic_signature(off_t offs, uint16_t sig);

#endif		/* LICSIG_H */
