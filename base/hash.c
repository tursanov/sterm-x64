/* Работа с ассоциативными массивами. (c) gsr 2000, 2003, 2009 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "genfunc.h"
#include "hash.h"

/* Создание ассоциативного массива */
struct hash *create_hash(int l)
{
	struct hash *h = __new(struct hash);
	if (h != NULL){
		h->data = malloc(l);
		if (h->data != NULL){
			h->hash_len = l;
			memset(h->data, 0, l);
			h->data_len = 0;
			return h;
		}else
			free(h);
	}
	return NULL;
}

/* Освобождение памяти ассоциативного массива */
void release_hash(struct hash *h)
{
	if (h != NULL){
		__delete(h->data);
		free(h);
	}
}

/* Очистка данных ассоциативного массива */
void clear_hash(struct hash *h)
{
	if (h != NULL){
		h->data_len = 0;
		memset(h->data, 0, h->hash_len);
	}
}

/* Установка значений по умолчанию в ОЗУ констант */
void set_hash_defaults(struct hash *h)
{
	static struct hash_entry {
		uint8_t id;
		char *txt;
	} defs[] = {
		{'X', "  БИЛЕТ "},
		{'Y', "  ДОПЛАТА "},
		{'Z', "  ПОСАДОЧНЫЙ "},
		{'M', "МЕСТА "},
		{'B', "ВЫПОЛНЕНО "},
		{'R', "ПРОДОЛЖАЙТЕ "},
		{'G', "ЖДИТЕ "},
		{'T', "СПРАВКА "},
		{'S', "ПОВТОРИТЕ "},
	};
	bool set_def_entry(struct hash *h, struct hash_entry *p)
	{
		void set_byte(struct hash *h, uint8_t b)
		{
			h->data[h->data_len++] = recode(b);
		}
		int i = 0, l;
		enum {st_id, st_data, st_stop} st = st_id;
		if ((p == NULL) || (p->txt == NULL))
			return false;
		l = strlen(p->txt);
		while ((st != st_stop) && (h->data_len < h->hash_len)){
			switch (st){
				case st_id:
					set_byte(h, p->id);
					st = st_data;
					break;
				case st_data:
					if (i < l)
						set_byte(h, p->txt[i++]);
					else{
						set_byte(h, HASH_IDELIM);
						st = st_stop;
					}
					break;
			}
		}
		return (st != st_id) && (i == l);
	}
	int i, j;
	if (h == NULL)
		return;
	clear_hash(h);
	for (i = j = 0; i < ASIZE(defs); i++){
		if (!set_def_entry(h, defs + i)){
			clear_hash(h);
			break;
		}
	}
}

/*
 * Поиск записи в ассоциативном массиве по ее идентификатору.
 * Возвращает индекс записи или -1 если запись не найдена.
 */
int find_hash_item(struct hash *h, uint8_t id)
{
	enum { st_id, st_data, st_found } st = st_id;
	int i;
	uint8_t b;
	if ((h == NULL) || (h->data == NULL))
		return -1;
	for (i = 0; i < h->data_len; i++){
		b = h->data[i];
		switch (st){
			case st_id:
				if (b == id)
					st = st_found;
				else
					st = st_data;
				break;
			case st_data:
				if (b == HASH_IDELIM)
					st = st_id;
				break;
			case st_found:
				return i;
		}
	}
	return -1;
}

/* Присутствует ли запись с данным идентификатором в ассоциативном массиве */
bool in_hash(struct hash *h, uint8_t id)
{
	return find_hash_item(h, id) != -1;
}

/*
 * Возвращает длину заданной записи ассоциативного массива или 0, если запись
 * не найдена.
 */
int hash_get_len(struct hash *h, uint8_t id)
{
	int i, len;
	if (h == NULL)
		return 0;
	i = find_hash_item(h, id);
	if (i == -1)
		return 0;
	for (len = 0; i < h->data_len; i++, len++){
		if (h->data[i] == HASH_IDELIM)
			break;
	}
	return len;
}

/*
 * Получить запись из ассоциативного массива. Возвращает длину полученной
 * записи или 0 если запись не найдена.
 * Проверка размера buf не производится.
 */
int hash_get(struct hash *h, uint8_t id, uint8_t *buf)
{
	int i, j;
	if ((h == NULL) || (buf == NULL))
		return 0;
	i = find_hash_item(h, id);
	if (i == -1)
		return 0;
	for (j = 0; i < h->data_len; i++, j++){
		if (h->data[i] == HASH_IDELIM)
			break;
		else
			buf[j] = h->data[i];
	}
/*	buf[j] = 0;*/
	return j;
}

/*
 * Добавляет новую запись к ассоциативному массиву. Возвращает true
 * в случае успеха и false, если превышен размер массива или в массиве
 * уже есть запись с таким идентификатором.
 */
bool hash_set(struct hash *h, uint8_t id, uint8_t *txt)
{
	enum {
		st_id,
		st_data,
		st_dle,
		st_stop
	};
	int i, st = st_id;
	uint8_t esc_char = 0;
	if ((h == NULL) || (txt == NULL))
		return false;
	if (find_hash_item(h, id) != -1)
		return false;
	if (h->hash_len - h->data_len < 2)
		return false;
	for (i = h->data_len; st != st_stop; txt++){
		if (i == h->hash_len)
			break;
		switch (st){
			case st_id:
				h->data[i++] = id;
				st = st_data;
				txt--;	/* здесь инкремент не нужен */
				break;
			case st_data:
				if (*txt == 0)
					st = st_stop;
				else if (is_escape(*txt)){
					esc_char = *txt;
					st = st_dle;
				}else
					h->data[i++] = *txt;
				break;
			case st_dle:
				if (*txt == X_PROM_END)
					st = st_stop;
				else{
					h->data[i++] = esc_char;
					txt--;
					st = st_data;
				}
				break;
		}
	}
	if (st == st_stop){
		if (i < h->hash_len)
			h->data[i++] = HASH_IDELIM;
		h->data_len = i;
		return true;
	}else
		return false;
}

/* Установка ассоциативного массива из буфера "Экспресс" */
int hash_set_all(struct hash *h, uint8_t *str)
{
	int i, l;
	uint8_t b;
	if ((h == NULL) || (h->data == NULL) || (str == NULL))
		return -1;
	l = strlen((char *)str);
	if (l > h->hash_len)
		return -1;
	for (i = 0; i < h->hash_len; i++){
		b = (i < l) ? str[i] : 0;
		if (b == HASH_DELIM)
			b = HASH_IDELIM;
		h->data[i] = b;
	}
	return l;
}
