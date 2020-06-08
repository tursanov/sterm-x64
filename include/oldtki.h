/*
 * Предыдущие варианты ключевой информации терминала. (c) gsr 2004.
 * Всего существует 3 варианта ключевой информации:
 * ранний вариант (early_tki) -- использовался до версии 2.2.3 включительно;
 * старый вариант (old_tki) -- начиная с версии 2.2.4 до 2.7 включительно;
 * новый вариант (tki) -- начиная с версии 2.8.
 */
#if !defined OLDTKI_H
#define OLDTKI_H

#include "ds1990a.h"
#include "gd.h"
#include "md5.h"

/* Имя раннего файла ключевой информации */
#define STERM_EARLY_TKI_FILE	_(".key")

/* Ранний вариант файла ключевой информации */
/* Версия терминала */
struct build_version{
	int version;
	int subversion;
	int modification;
};

/* Ключи DS1990A */
/* Тип ключа */
enum {
	key_service = 0,	/* сервисный ключ */
	key_work,		/* рабочий ключ */
};

/* Описание ключа */
struct ds_descr {
	int ds_type;
	ds_number dsn;
};

/* Количество ключей в раннем варианте ключевой информации */
#define EARLY_NR_KEYS		4

struct early_term_key_info{
	struct ds_descr keys[EARLY_NR_KEYS];
	term_number tn;
	byte pad1[3];
	struct build_version term_build;
	word term_check_sum;
	byte pad2[2];
};

/*
 * При формировании раннего варианта файла ключевой информации терминала
 * не учитывались байты-заполнители в конце структуры (pad2).
 */
#define EARLY_TKI_SIZE1		xoffsetof(struct early_term_key_info, pad2)

/* Расшифровка данных для старого варианта */
extern void early_decrypt_data(byte *p, int l);

/* Старый вариант ключевой информации терминала */
/* Количество ключей в старом варианте */
#define OLD_NR_KEYS		EARLY_NR_KEYS

struct old_term_key_info{
	struct md5_hash check_sum;
	struct ds_descr keys[OLD_NR_KEYS];
	term_number tn;
};

#endif		/* OLDTKI_H */
