/*
 * �ணࠬ�� 㤠����� ��業��� ��� � �ନ���� ��� �ᯮ�짮�����
 * ����� ������᭮��. �� 㤠����� � MBR �����뢠���� ᯥ樠��� 䫠�,
 * � ���짮��⥫� �뤠���� 32-���筮� ��⭠����筮� �᫮. ��᫮
 * �ନ����� �� �᭮�� ⥪�饩 ����, ⠡���� ��砩��� �ᥫ (rndtbl.c)
 * � ��� �����᪮�� ����� �ନ����. (c) gsr 2006, 2011, 2019.
 */

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "base64.h"
#include "colortty.h"
#include "licsig.h"
#include "md5.h"
#include "paths.h"
#include "rmlic.h"
#include "tki.h"

#define LIC_NAME	"���"

/* �������� �� �����᪮�� ����� �ନ���� */
static bool read_term_number(struct md5_hash *number)
{
	bool ret = false;
	uint8_t nr[TERM_NUMBER_LEN], buf[32];
	int l;
	if (read_tki(STERM_TKI_NAME, false)){
		get_tki_field(&tki, TKI_NUMBER, nr);
		l = base64_encode(nr, sizeof(nr), buf);
		encrypt_data(buf, l);
		get_md5(buf, l, number);
		ret = true;
	}
	return ret;
}

int main()
{
	int ret = 1;
	uint8_t buf[16];
	if (ask_rmlic_confirmation(LIC_NAME)){
		struct md5_hash tnumber;
		if (read_term_number(&tnumber) &&
				mk_del_hash_for_tnumber(buf, &tnumber,
					rnd_tbl_bnk, ASIZE(rnd_tbl_bnk)) &&
				rm_lic_file(BANK_LICENSE, LIC_NAME) &&
				write_lic_signature(BANK_LIC_SIGNATURE_OFFSET,
					BANK_LIC_SIGNATURE)){
			print_del_hash(buf, LIC_NAME);
			ret = 0;
		}
	}
	return ret;
}
