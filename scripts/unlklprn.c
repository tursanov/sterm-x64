/*
 * Программа разблокирования терминала после удаления с него лицензии ППУ.
 * После запуска этой программы на терминал снова можно будет установить
 * лицензию ППУ. (c) gsr 2011.
 */

#include <stdio.h>
#include "colortty.h"
#include "licsig.h"
#include "md5.h"
#include "tki.h"
#include "unlock.h"

/* Список заводских номеров для восстановления лицензии */
static struct fuzzy_md5 known_numbers[] = {
	/* F001370016397 (24.10.11, тест) */
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
				print_unlock_msg("ППУ");
				ret = 0;
			}else
				fprintf(stderr, tRED "ошибка разблокировки терминала\n" ANSI_DEFAULT);
		}else
			fprintf(stderr, tRED "данный терминал не может быть разблокирован этой программой\n" ANSI_DEFAULT);
	}
	return ret;
}
