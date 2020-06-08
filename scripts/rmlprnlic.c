/*
 * Программа удаления лицензии ППУ с терминала.
 * При удалении в MBR записывается специальный флаг, а пользователю
 * выдается 32-значное шестнадцатеричное число. Число формируется
 * на основе текущей даты, таблицы случайных чисел (rndtbl.c) и
 * хэша заводского номера терминала.
 * (c) gsr 2011, 2019.
 */

#include <stdio.h>
#include <stdlib.h>
#include "prn/local.h"
#include "licsig.h"
#include "paths.h"
#include "rmlic.h"

#define LIC_NAME	"ППУ"

int main()
{
	int ret = 1;
	uint8_t buf[16];
	if (ask_rmlic_confirmation(LIC_NAME) &&
			mk_del_hash(buf, rnd_tbl_lprn, ASIZE(rnd_tbl_lprn)) &&
			rm_lic_file(LPRN_LICENSE, LIC_NAME) &&
			write_lic_signature(LPRN_LIC_SIGNATURE_OFFSET,
				LPRN_LIC_SIGNATURE)){
		print_del_hash(buf, LIC_NAME);
		ret = 0;
	}
	return ret;
}
