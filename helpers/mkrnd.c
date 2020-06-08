/*
 * Создание таблицы случайных чисел для программы снятия лицензий ИПТ.
 * Каждая запись в таблице состоит из 3-х 32-разрядных случайных чисел.
 * (c) gsr 2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NR_RAND		100

unsigned int do_rand(void)
{
	unsigned int ret = rand();
	if (ret & 1)
		ret |= 0x80000000;
	return ret;
}

int main()
{
	int i;
	srand(time(NULL));
	for (i = 0; i < NR_RAND; i++)
		printf("\t{0x%.8x,\t0x%.8x,\t0x%.8x},\n",
			do_rand(), do_rand(), do_rand());
	return 0;
}
