/*
 * Аналоги функций из base/licsig.c, которые в данном случае ничего не делают,
 * а нужны просто для корректной сборки некоторых утилит.
 * (c) gsr 2011, 2018.
 */

#include <stdlib.h>
#include "licsig.h"

/* Проверка присутствия признака удаления лицензии */
bool check_lic_signature(off_t offs __attribute__((unused)),
	uint16_t sig __attribute__((unused)))
{
	return false;
}

/* Запись признака установленной лицензии */
bool write_lic_signature(off_t offs __attribute__((unused)),
	uint16_t sig __attribute__((unused)))
{
	return true;
}
