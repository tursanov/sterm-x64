/*
 * �ணࠬ�� ࠧ�����஢���� �ନ���� ��᫥ 㤠����� � ���� ��業��� ���.
 * ��᫥ ����᪠ �⮩ �ணࠬ�� �� �ନ��� ᭮�� ����� �㤥� ��⠭�����
 * ��業��� ���. (c) gsr 2011.
 */

#include <stdio.h>
#include "colortty.h"
#include "licsig.h"
#include "md5.h"
#include "tki.h"
#include "unlock.h"

/* ���᮪ �����᪨� ����஢ ��� ����⠭������� ��業��� */
static struct fuzzy_md5 known_numbers[] = {
	/* F001370016397 (24.10.11, ���) */
	{
		.a = 0x49315f14, .aa = 0x949fb1f2,
		.b = 0x4c2e6ab7, .bb = 0x2fb33794,
		.c = 0x99c4f7be, .cc = 0x5c94b1fb,
		.d = 0xbec86715, .dd = 0x541745e5,
	},
};

int main()
{
	int ret = 1;
	struct md5_hash number;
	if (read_term_number(USB_BIND_FILE, &number)){
		if (check_term_number(&number, known_numbers, ASIZE(known_numbers))){
			if (clear_lic_signature(LPRN_LIC_SIGNATURE_OFFSET,
					LPRN_LIC_SIGNATURE)){
				print_unlock_msg("���");
				ret = 0;
			}else
				fprintf(stderr, tRED "�訡�� ࠧ�����஢�� �ନ����\n" ANSI_DEFAULT);
		}else
			fprintf(stderr, tRED "����� �ନ��� �� ����� ���� ࠧ�����஢�� �⮩ �ணࠬ���\n" ANSI_DEFAULT);
	}
	return ret;
}
