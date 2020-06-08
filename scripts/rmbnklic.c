/*
 * �ணࠬ�� 㤠����� ��業��� ��� � �ନ����.
 * �� 㤠����� � MBR �����뢠���� ᯥ樠��� 䫠�, � ���짮��⥫�
 * �뤠���� 32-���筮� ��⭠����筮� �᫮. ��᫮ �ନ�����
 * �� �᭮�� ⥪�饩 ����, ⠡���� ��砩��� �ᥫ (rndtbl.c) �
 * ��� �����᪮�� ����� �ନ����.
 * (c) gsr 2006, 2011, 2019.
 */

#include <stdio.h>
#include <stdlib.h>
#include "licsig.h"
#include "paths.h"
#include "rmlic.h"

#define LIC_NAME	"���"

int main()
{
	int ret = 1;
	uint8_t buf[16];
	if (ask_rmlic_confirmation(LIC_NAME) &&
			mk_del_hash(buf, rnd_tbl_bnk, ASIZE(rnd_tbl_bnk)) &&
			rm_lic_file(BANK_LICENSE, LIC_NAME) &&
			write_lic_signature(BANK_LIC_SIGNATURE_OFFSET,
				BANK_LIC_SIGNATURE)){
		print_del_hash(buf, LIC_NAME);
		ret = 0;
	}
	return ret;
}
