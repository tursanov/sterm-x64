/*
 * ������� �㭪権 �� base/licsig.c, ����� � ������ ��砥 ��祣� �� ������,
 * � �㦭� ���� ��� ���४⭮� ᡮન �������� �⨫��.
 * (c) gsr 2011, 2018.
 */

#include <stdlib.h>
#include "licsig.h"

/* �஢�ઠ ������⢨� �ਧ���� 㤠����� ��業��� */
bool check_lic_signature(off_t offs __attribute__((unused)),
	uint16_t sig __attribute__((unused)))
{
	return false;
}

/* ������ �ਧ���� ��⠭�������� ��業��� */
bool write_lic_signature(off_t offs __attribute__((unused)),
	uint16_t sig __attribute__((unused)))
{
	return true;
}
