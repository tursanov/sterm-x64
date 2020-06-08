/*
 * Программа удаления лицензии ППУ с терминала без использования
 * модуля безопасности. При удалении в MBR записывается специальный флаг,
 * а пользователю выдается 32-значное шестнадцатеричное число. Число
 * формируется на основе текущей даты, таблицы случайных чисел (rndtbl.c)
 * и хэша заводского номера терминала. (c) gsr 2011, 2019.
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
#include "prn/local.h"
#include "base64.h"
#include "colortty.h"
#include "licsig.h"
#include "md5.h"
#include "paths.h"
#include "rmlic.h"
#include "tki.h"

#define LIC_NAME	"ППУ"

/* Создание хеша заводского номера терминала */
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
					rnd_tbl_lprn, ASIZE(rnd_tbl_lprn)) &&
				rm_lic_file(LPRN_LICENSE, LIC_NAME) &&
				write_lic_signature(LPRN_LIC_SIGNATURE_OFFSET,
					LPRN_LIC_SIGNATURE)){
			print_del_hash(buf, LIC_NAME);
			ret = 0;
		}
	}
	return ret;
}
