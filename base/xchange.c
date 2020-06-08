/* Журнал обмена данными. (c) gsr 2004 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hex.h"
#include "sterm.h"
#include "xchange.h"

/* Кольцевой буфер для элементов журнала */
static struct xlog_item items[XLOG_NR_ITEMS];
/* Первая запись в буфере */
static int head;
/* Количество записей в буфере */
static int n_items;

/* Проверка индекса */
static bool index_ok(int index)
{
	return (index >= 0) && (index < XLOG_NR_ITEMS);
}

/* Удаление данных из буфера */
static void del_item(int index)
{
	if (index_ok(index) && (items[index].data != NULL)){
		free(items[index].data);
		items[index].data = NULL;
	}
}

/* Инициализация dt для записи на основании времени на хосте */
static bool fill_date_time(struct date_time *dt)
{
	time_t t;
	struct tm *tm;
	if (dt == NULL)
		return false;
	t = time(NULL) + time_delta;
	tm = localtime(&t);
	dt->sec = tm->tm_sec;
	dt->min = tm->tm_min;
	dt->hour = tm->tm_hour;
	dt->day = tm->tm_mday - 1;
	dt->mon = tm->tm_mon;
	dt->year = tm->tm_year - 100;
	return true;
}
	
/* Чтение счетчиков входящего сообщения */
static bool read_in_counters(uint8_t *data, int len,
		uint16_t *ONpo, uint16_t *ONtz, uint16_t *OBp)
{
	if ((data == NULL) || (len < 13) ||
			(ONpo == NULL) || (ONtz == NULL) || (OBp == NULL))
		return false;
	*ONpo = read_hex(data + 6, 3);
	if (hex_error)
		return false;
	*ONtz = read_hex(data + 9, 3);
	if (hex_error)
		return false;
	*OBp = data[12];
	return !hex_error;
}

/* Чтение счетчиков исходящего сообщения */
static bool read_out_counters(uint8_t *data, int len,
		uint16_t *ZNtz, uint16_t *ZNpo, uint16_t *ZBp)
{
	int i;
	if ((data == NULL) || (len <= 0) ||
			(ZNtz == NULL) || (ZNpo == NULL) || (ZBp == NULL))
		return false;
	for (i = 0; i < len - 8; i++){
		if (data[i] == SESSION_INFO_MARK){
			*ZNtz = read_hex(data + i + 1, 3);
			if (hex_error)
				return false;
			*ZNpo = read_hex(data + i + 4, 3);
			if (hex_error)
				return false;
			*ZBp = data[i + 7];
			return !hex_error;
		}
	}
	return false;
}

/* Поместить данные в журнал */
bool xlog_add_item(uint8_t *data, int len, int dir)
{
	int index;
	uint8_t *buf;
	if ((data == NULL) || (len <= 0) || (len > XLOG_MAX_DATA_LEN))
		return false;
	buf = malloc(len);
	if (buf == NULL){
		printf("%s: ошибка выделения памяти\n", __func__);
		return false;
	}
	memcpy(buf, data, len);
	index = (head + n_items) % XLOG_NR_ITEMS;
	del_item(index);
	fill_date_time(&items[index].dt);
	items[index].dir = dir;
	items[index].len = len;
	items[index].data = buf;
	if (dir == xlog_in){
		if (!read_in_counters(data, len, &items[index].Npo,
				&items[index].Ntz, &items[index].Bp))
			items[index].Npo = items[index].Ntz =
				items[index].Bp = 0;
	}else if (!read_out_counters(data, len, &items[index].Ntz,
				&items[index].Npo, &items[index].Bp))
		items[index].Ntz = items[index].Npo = items[index].Bp = 0;
	if (n_items < XLOG_NR_ITEMS)
		n_items++;
	else{
		head++;
		head %= XLOG_NR_ITEMS;
	}
	return true;
}

/* Получить число элементов в журнале */
int xlog_count_items(void)
{
	return n_items;
}

/* Получить запись по ее индексу от начала журнала */
struct xlog_item *xlog_get_item(int index)
{
	if (index < n_items)
		return items + ((head + index) % XLOG_NR_ITEMS);
	else
		return NULL;
}
