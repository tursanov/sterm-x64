/*
 * Программа разблокирования терминала после удаления с него лицензии ИПТ.
 * После запуска этой программы на терминал снова можно будет установить
 * лицензию ИПТ. (c) gsr 2006, 2011, 2019.
 */

#include <stdio.h>
#include "colortty.h"
#include "licsig.h"
#include "md5.h"
#include "tki.h"
#include "unlock.h"

/* Список заводских номеров для восстановления лицензии */
static struct fuzzy_md5 known_numbers[] = {
	/* D001370015509 (10.09.10) */
	{
		.a = 0xa4d764d7, .aa = 0x312e157f,
		.b = 0xbce002bf, .bb = 0x28a9431e,
		.c = 0xdd8858d8, .cc = 0x84ef3a05,
		.d = 0x8f2f951b, .dd = 0x761aa13d,
	},
	/* F001370011058 (17.08.11) */
	{
		.a = 0xee563350, .aa = 0x357290e4,
		.b = 0x1c322d61, .bb = 0x4509ea1b,
		.c = 0x61a88900, .cc = 0xc4e8791a,
		.d = 0x432678be, .dd = 0x87eed731,
	},
	/* F001370011060 (17.08.11) */
	{
		.a = 0x031d6415, .aa = 0x601e54d2,
		.b = 0x865f4f40, .bb = 0x8b40be6f,
		.c = 0xf05f7d90, .cc = 0x05dd11da,
		.d = 0x596fcb36, .dd = 0x0d546c1c,
	},
	/* F001370011080 (17.08.11) */
	{
		.a = 0x560746d3, .aa = 0x68a5b75c,
		.b = 0x595b5df1, .bb = 0xf26e887a,
		.c = 0x29b3f6e7, .cc = 0x732a0c1d,
		.d = 0x5b8f7407, .dd = 0xbe2dfa2f,
	},
	/* F001370011086 (17.08.11) */
	{
		.a = 0xb954a716, .aa = 0xcc2a4d74,
		.b = 0xb9d4f1ab, .bb = 0x3ba25bf4,
		.c = 0xbaa3f969, .cc = 0xcb881d80,
		.d = 0xde904077, .dd = 0x4825bc6e,
	},
	/* F001370016397 (24.10.11, тест) */
	{
		.a = 0x49315f14, .aa = 0xf98932d2,
		.b = 0x4c2e6ab7, .bb = 0x8f6d13f5,
		.c = 0x99c4f7be, .cc = 0xc9bf821c,
		.d = 0xbec86715, .dd = 0x43d12ec3,
	},
	/* D001370013695 (07.06.19) */
	{
		.a = 0x3617ed7c, .aa = 0xe80b7e55,
		.b = 0x0da5e41d, .bb = 0xa0584f18,
		.c = 0xf490cf0d, .cc = 0x154340dc,
		.d = 0xce916cdf, .dd = 0x4042356e,
	},
	/* F001370012136 (07.06.19) */
	{
		.a = 0xdf624fa6, .aa = 0xdabaf10d,
		.b = 0xb1405379, .bb = 0x933d676e,
		.c = 0xcc8bc9a1, .cc = 0xc249a952,
		.d = 0xba70b2be, .dd = 0xce915e04,
	},
	/* F001370012526 (07.06.19) */
	{
		.a = 0xde07fccf, .aa = 0x2a0ca5ad,
		.b = 0x0c7f80a2, .bb = 0xbcfd2a6a,
		.c = 0xdd143f3f, .cc = 0x2c19bf4d,
		.d = 0x5447b6c2, .dd = 0xda2c2940,
	},
	/* F001370013700 (07.06.19) */
	{
		.a = 0x034469da, .aa = 0x247fa58e,
		.b = 0x44627085, .bb = 0xdb827690,
		.c = 0x4cbe8c10, .cc = 0x3edd0b4b,
		.d = 0xf8b0b50e, .dd = 0xff33e4f8,
	},
	/* F001370013705 (07.06.19) */
	{
		.a = 0x00dea36a, .aa = 0xa89246b8,
		.b = 0x10badd2f, .bb = 0x28cbe619,
		.c = 0xbd817564, .cc = 0x41fc032b,
		.d = 0x9e09ae73, .dd = 0x971287eb,
	},
};

int main()
{
	int ret = 1;
	struct md5_hash number;
	if (read_term_number(USB_BIND_FILE, &number)){
		if (check_term_number(&number, known_numbers, ASIZE(known_numbers))){
			if (clear_lic_signature(BANK_LIC_SIGNATURE_OFFSET,
					BANK_LIC_SIGNATURE)){
				print_unlock_msg("ИПТ");
				ret = 0;
			}else
				fprintf(stderr, tRED "ошибка разблокировки терминала\n" ANSI_DEFAULT);
		}else
			fprintf(stderr, tRED "данный терминал не может быть разблокирован этой программой\n" ANSI_DEFAULT);
	}
	return ret;
}
