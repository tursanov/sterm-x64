/*
 * Работа с предыдущими вариантами ключевой информации терминала (см. oldtki.h).
 * (c) gsr 2004.
 */

#include <stdlib.h>
#include "oldtki.h"

/* Ранний вариант ключевой информации */
void early_decrypt_data(byte *p, int l)
{
	int i;
	for (i = l; i > 0; i--)
		p[i % l] ^= p[i - 1];
}
