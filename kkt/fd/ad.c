#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "express.h"
#include "sysdefs.h"
#include "serialize.h"
#if defined WIN32 || defined __APPLE__
#include "ad.h"
void cashier_set_name(const char *name) {
}
#else
#include "kkt/fd/ad.h"
#include "gui/fa.h"
#endif

#ifdef WIN32
#define strcasecmp _stricmp
#define strdup _strdup
#endif

static int strcmp2(const char *s1, const char *s2) {
    if (s1 == NULL && s2 == NULL)
        return 0;
    else if (s1 == NULL && s2 != NULL)
        return -1;
    else if (s1 != NULL && s2 == NULL)
        return 1;
    else 
        return strcmp(s1, s2);
}

#define COPYSTR(s) ((s) != NULL ? strdup(s) : NULL)

void doc_no_init(doc_no_t *d, const char *s) {
	d->s = COPYSTR(s);
}

void doc_no_free(doc_no_t *d) {
	if (d->s) {
		free(d->s);
		d->s = NULL;
	}
}

int doc_no_save(int fd, doc_no_t *d) {
	return save_string(fd, d->s);
}

int doc_no_load(int fd, doc_no_t *d) {
	int ret = load_string(fd, &d->s);
	return ret;
}

void doc_no_copy(doc_no_t *dst, doc_no_t *src) {
	if (dst == src)
		return;
	if (dst->s)
		free(dst->s);
	dst->s = COPYSTR(src->s);
}

int doc_no_compare(doc_no_t *d1, doc_no_t *d2) {
	return strcmp2(d1->s, d2->s);
}

int doc_no_special_compare(doc_no_t *n, doc_no_t *d) {
	if (n == NULL && d == NULL)
		return 0;
	else if (n == NULL && d != NULL)
		return -1;
	else if (n != NULL && d == NULL)
		return 1;
	else {
		size_t i = 0;
		for (const char *s1 = n->s, *s2 = d->s; 1; s1++, s2++, i++) {
			char ch1 = *s1;
			char ch2 = *s2;
			if (!ch1 && ch2)
				return 1;
			else if (ch1 && !ch2)
				return -1;
			else if (!ch1 && !ch2)
				return 0;

			//((i >= 1 && i <= 3 && ch1 == '*') || ch1 == ch2
			if ((i < 1 || i > 3 || ch1 != '*') && ch1 != ch2)
				return -1;
		}
	}
}

void doc_no_clear(doc_no_t *d) {
	if (d && d->s) {
		free(d->s);
		d->s = NULL;
	}
}

int64_t doc_no_to_i64(doc_no_t *d) {
	char *endp;
	return d->s ? strtoll(d->s, &endp, 10) : 0;
}

bool doc_no_is_empty(doc_no_t *d) {
	return d->s == NULL;
}


void op_doc_no_init(op_doc_no_t *d) {
	d->s = NULL;
}

void op_doc_no_free(op_doc_no_t *d) {
	if (d->s) {
		free(d->s);
		d->s = NULL;
	}
}

int op_doc_no_save(int fd, op_doc_no_t *d) {
	return save_string(fd, d->s);
}

int op_doc_no_load(int fd, op_doc_no_t *d) {
	int ret = load_string(fd, &d->s);
	return ret;
}

void op_doc_no_copy(op_doc_no_t *dst, op_doc_no_t *src) {
	if (dst->s)
		free(dst->s);
	dst->s = COPYSTR(src->s);
}

void op_doc_no_set(op_doc_no_t *dst, doc_no_t *d1, const char *op, doc_no_t *d2) {
	if (dst->s) {
		free(dst->s);
		dst->s = NULL;
	}
	if (d1 != NULL)
		dst->s = COPYSTR(d1->s);
#if 0		
	const char *n = "\xfc";
	size_t n_len = strlen(n);
	size_t len1 = (d1 && d1->s != NULL) ? strlen(d1->s) : 0;
	size_t len2 = op ? strlen(op) : 0;
	size_t len3 = (d2 && d2->s != NULL) ? strlen(d2->s) : 0;
	size_t len = len1 + len2 + len3 + (len2 || len3 ? 4 + n_len : 0);
	size_t ofs = 0;
	dst->s = (char *)malloc(len + 1);
	char *s = dst->s;

	if (len1 > 0) {
		memcpy(s, d1->s, len1);
		ofs += len1;
	}
	if (len2 > 0 || len3 > 0) {
		s[ofs++] = ' ';
		s[ofs++] = '(';
	}
	if (len2 > 0) {
		memcpy(s + ofs, op, len2);
		ofs += len2;
	}
	if (len3 > 0) {
		s[ofs++] = ' ';
		memcpy(s + ofs, n, n_len);
		ofs += n_len;
		memcpy(s + ofs, d2->s, len3);
		ofs += len3;
	}
	if (len2 > 0 || len3 > 0)
		s[ofs++] = ')';
	s[ofs] = 0;

	printf("B = \"%s\"\n", s);
#endif
}

L *L_create(void) {
    L *l = (L *)malloc(sizeof(L));
    memset(l, 0, sizeof(L));

    return l;
}

void L_destroy(L *l) {
    if (l->s)
        free(l->s);
    free(l);
}

int L_save(int fd, L *l) {
    if (save_string(fd, l->s) < 0 ||
        SAVE_INT(fd, l->p) < 0 ||
        SAVE_INT(fd, l->r) < 0 ||
        SAVE_INT(fd, l->t) < 0 ||
        SAVE_INT(fd, l->n) < 0 ||
        SAVE_INT(fd, l->c) < 0)
        return -1;
    return 0;
}

L *L_load(int fd) {
    L *l = L_create();
    if (load_string(fd, &l->s) < 0 ||
        LOAD_INT(fd, l->p) < 0 ||
        LOAD_INT(fd, l->r) < 0 ||
        LOAD_INT(fd, l->t) < 0 ||
        LOAD_INT(fd, l->n) < 0 ||
        LOAD_INT(fd, l->c) < 0) {
        L_destroy(l);
        return NULL;
    }
    return l;
}

K *K_create(void) {
    K *k = (K *)malloc(sizeof(K));
    memset(k, 0, sizeof(K));
    k->llist.delete_func = (list_item_delete_func_t)L_destroy;
    return k;
}

void K_destroy(K *k) {
    list_clear(&k->llist);
    if (k->h)
        free(k->h);
    if (k->t)
        free(k->t);
    if (k->e)
        free(k->e);

    doc_no_free(&k->d);
    doc_no_free(&k->r);
    doc_no_free(&k->i1);
    doc_no_free(&k->i2);
    doc_no_free(&k->i21);
    doc_no_free(&k->u);
    op_doc_no_free(&k->b);

    free(k);
}

bool K_addL(K *k, L* l) {
	if (list_add(&k->llist, l) == 0) {
		return true;
	}
	return false;
}

K *K_divide(K *k, uint8_t p, int64_t *sum) {
	int64_t s = 0;
	K *k1 = NULL;
	int count = 0;
	for (list_item_t *item = k->llist.head, *prev = NULL; item != NULL;) {
		L *l = LIST_ITEM(item, L);
		list_item_t *tmp = item;
		item = item->next;
		if (l->p == p) {
			if (prev != NULL)
				prev->next = item;
			if (item == NULL)
				k->llist.tail = prev;
			if (tmp == k->llist.head)
				k->llist.head = item;
			count++;

			s += l->t;

			if (k1 == NULL) {
				k1 = (K*)malloc(sizeof(K));
				memset(k1, 0, sizeof(K));
				//memset(&k1->llist, 0, sizeof(k1->llist));
				k->llist.delete_func = (list_item_delete_func_t)L_destroy;

				k1->o = k->o;          // Операция
				k1->a = k->a;
				k1->p = k->p;          // ИНН перевозчика

				k1->h = COPYSTR(k->h); // телефон перевозчика
				k1->m = k->m;          // способ оплаты
				k1->t = COPYSTR(k->t);            // номер телефона пассажира
				k1->e = COPYSTR(k->e);            // адрес электронной посты пассажира

				doc_no_copy(&k1->d, &k->d);
				doc_no_copy(&k1->r, &k->r);
				doc_no_copy(&k1->n, &k->n);
				doc_no_copy(&k1->i1, &k->i1);
				doc_no_copy(&k1->i2, &k->i2);
				doc_no_copy(&k1->i21, &k->i21);
				doc_no_copy(&k1->u, &k->u);
				op_doc_no_copy(&k1->b, &k->b);
			}

			list_add_item(&k1->llist, tmp);
		} else
			prev = tmp;
	}
	k->llist.count -= count;

	if (sum)
		*sum = s;

    return k1;
}

static void K_calc_sum(K *k, S *s) {
	memset(s, 0, sizeof(*s));
	for (list_item_t *li3 = k->llist.head; li3 != NULL; li3 = li3->next) {
		L *l = LIST_ITEM(li3, L);
		switch (k->m) {
		case 1:
			s->n += l->t;
			break;
		case 2:
			s->e += l->t;
			break;
		case 3:
			s->p += l->t;
			break;
		}
		s->a += l->t;
	}
}

static int64_t K_calc_total_sum(K *k) {
	int64_t sum = 0;
	for (list_item_t *li3 = k->llist.head; li3 != NULL; li3 = li3->next) {
		L *l = LIST_ITEM(li3, L);
		sum += l->t;
	}
	return sum;
}

static int L_compare(void *arg, L *l1, L *l2) {
	if ((strcmp2(l1->s, l2->s) == 0 &&
		l1->r == l2->r &&
		l1->t == l2->t &&
		l1->n == l2->n &&
		l1->c == l2->c))
		return 0;
	return 1;
}

bool K_equalByL(K *k1, K* k2) {
    return list_compare(&k1->llist, &k2->llist, NULL, (list_item_compare_func_t)L_compare) == 0;
}

int64_t K_get_sum(K *k) {
	int64_t sum = 0;
	for (list_item_t *item = k->llist.head; item != NULL; item = item->next) {
		L *l = LIST_ITEM(item, L);
		sum += l->t;
	}
	return sum;
}

int K_save(int fd, K *k) {
    if (save_list(fd, &k->llist, (list_item_func_t)L_save) < 0 ||
        SAVE_INT(fd, k->o) < 0 ||
		SAVE_INT(fd, k->a) < 0 ||
        doc_no_save(fd, &k->d) < 0 ||
		doc_no_save(fd, &k->r) < 0 ||
		doc_no_save(fd, &k->i1) < 0 ||
		doc_no_save(fd, &k->i2) < 0 ||
		doc_no_save(fd, &k->i21) < 0 ||
		doc_no_save(fd, &k->u) < 0 ||
		op_doc_no_save(fd, &k->b) < 0 ||
		SAVE_INT(fd, k->p) < 0 ||
        save_string(fd, k->h) < 0 ||
        SAVE_INT(fd, k->m) < 0 ||
        save_string(fd, k->t) < 0 ||
        save_string(fd, k->e) < 0)
        return -1;
    return 0;
}

static void K_after_add(K *k) {
    /*int64_array_add(&_ad->docs, k->d, true);*/
    if (k->t != NULL) {
	    string_array_add(&_ad->phones, k->t, true, 0);
	}
    if (k->e != NULL) {
	    string_array_add(&_ad->emails, k->e, true, 1);
	}
}

K *K_load(int fd) {
    K *k = K_create();
    if (load_list(fd, &k->llist, (load_item_func_t)L_load) < 0 ||
            LOAD_INT(fd, k->o) < 0 ||
			LOAD_INT(fd, k->a) < 0 ||
            doc_no_load(fd, &k->d) < 0 ||
			doc_no_load(fd, &k->r) < 0 ||
			doc_no_load(fd, &k->i1) < 0 ||
			doc_no_load(fd, &k->i2) < 0 ||
			doc_no_load(fd, &k->i21) < 0 ||
			doc_no_load(fd, &k->u) < 0 ||
			op_doc_no_load(fd, &k->b) < 0 ||
			LOAD_INT(fd, k->p) < 0 ||
            load_string(fd, &k->h) < 0 ||
            LOAD_INT(fd, k->m) < 0 ||
            load_string(fd, &k->t) < 0 ||
            load_string(fd, &k->e) < 0) {
        K_destroy(k);
        return NULL;
    }
    K_after_add(k);
    return k;
}


C* C_create(void) {
    C *c = (C *)malloc(sizeof(C));
    memset(c, 0, sizeof(C));
    c->klist.delete_func = (list_item_delete_func_t)K_destroy;
    
    return c;
}

void C_destroy(C *c) {
    list_clear(&c->klist);
    if (c->h != NULL)
        free(c->h);
    if (c->pe != NULL)
        free(c->pe);

    int64_array_clear(&_ad->docs);
    string_array_clear(&_ad->phones);
    string_array_clear(&_ad->emails);

    for (list_item_t *i = _ad->clist.head; i != NULL; i = i->next) {
		C *c1 = LIST_ITEM(i, C);
		if (c1 != c) {
		    for (list_item_t *j = c1->klist.head; j != NULL; j = j->next) {
		    	K *k = LIST_ITEM(j, K);
		    	K_after_add(k);
		    }
		}
    }
}

bool C_addK(C *c, K *k) {
	if (list_add(&c->klist, k) == 0) {
		return true;
	}
	return false;
}

int C_save(int fd, C *c) {
    if (save_list(fd, &c->klist, (list_item_func_t)K_save) < 0 ||
            SAVE_INT(fd, c->p) < 0 ||
            save_string(fd, c->h) < 0 ||
            SAVE_INT(fd, c->t1054) < 0 ||
            SAVE_INT(fd, c->t1055) < 0 ||
            save_string(fd, c->pe) < 0)
        return -1;
    return 0;
}

C * C_load(int fd) {
    C *c = C_create();
    if (load_list(fd, &c->klist, (load_item_func_t)K_load) < 0 ||
            LOAD_INT(fd, c->p) < 0 ||
            load_string(fd, &c->h) < 0 ||
            LOAD_INT(fd, c->t1054) < 0 ||
            LOAD_INT(fd, c->t1055) < 0 ||
            load_string(fd, &c->pe) < 0) {
        C_destroy(c);
        return NULL;
    }
    return c;
}

P1 *P1_create(void) {
    P1* p1 = (P1 *)malloc(sizeof(P1));
    memset(p1, 0, sizeof(P1));
    
    return p1;
}

void P1_destroy(P1 *p1) {
    if (p1->i != NULL)
        free(p1->i);
    if (p1->p != NULL)
        free(p1->p);
    if (p1->t != NULL)
        free(p1->t);
    if (p1->c != NULL)
        free(p1->c);
    free(p1);
}

int P1_save(int fd, P1 *p1) {
    if (save_string(fd, p1->i) < 0 ||
        save_string(fd, p1->p) < 0 ||
        save_string(fd, p1->t) < 0 ||
        save_string(fd, p1->c) < 0)
        return -1;
    return 0;
}

P1* P1_load(int fd) {
    P1 *p1 = P1_create();
    if (load_string(fd, &p1->i) < 0 ||
        load_string(fd, &p1->p) < 0 ||
        load_string(fd, &p1->t) < 0 ||
        load_string(fd, &p1->c) < 0)
        return NULL;
    printf("P1->i = %s\n, P1->p = %s, P1->t = %s, P1->c = %s\n",
            p1->i, p1->p, p1->t, p1->c);
    return p1;
}

int int64_array_init(int64_array_t *array) {
#define DEFAULT_CAPACITY	32
	array->capacity = DEFAULT_CAPACITY;
	array->values = (int64_t *)malloc(array->capacity * sizeof(int64_t));
	array->count = 0;

	return array->values != NULL ? 0 : -1;
}

int int64_array_clear(int64_array_t *array) {
	array->count = 0;
	return 0;
}

int int64_array_free(int64_array_t *array) {
	if (array->values)
		free(array->values);
	array->values = NULL;
	array->capacity = array->count = 0;
	return 0;
}

int int64_array_add(int64_array_t *array, int64_t v, bool unique) {
	if (unique) {
		int64_t *p = array->values;
		for (size_t i = 0; i < array->count; i++, p++)
			if (*p == v)
				return 0;
	}
	if (array->count == array->capacity) {
		array->capacity *= 2;
		array->values = (int64_t *)realloc(array->values, array->capacity * sizeof(int64_t));
		if (array->values == NULL)
			return -1;
	}
	array->values[array->count] = v;
	array->count++;
	return 1;
}

int string_array_init(string_array_t *array) {
#define DEFAULT_CAPACITY	32
	array->capacity = DEFAULT_CAPACITY;
	array->values = (char **)malloc(array->capacity * sizeof(char *));
	array->count = 0;

	return array->values != NULL ? 0 : -1;
}

int string_array_clear(string_array_t *array) {
	for (size_t i = 0; i < array->count; i++)
		if (array->values[i])
			free(array->values[i]);
	memset(array->values, 0, sizeof(char *) * array->count);
	array->count = 0;
	return 0;
}

int string_array_free(string_array_t *array) {
	for (size_t i = 0; i < array->count; i++)
		if (array->values[i])
			free(array->values[i]);

	if (array->values)
		free(array->values);
	array->values = NULL;
	array->capacity = array->count = 0;
	return 0;
}

int string_array_add(string_array_t *array, const char *v, bool unique, int cnv) {
	if (unique) {
		char **p = array->values;
		for (size_t i = 0; i < array->count; i++, p++) {
			if (strcasecmp(*p, v) == 0)
				return 0;
		}
	}
	if (array->count == array->capacity) {
		array->capacity *= 2;
		array->values = (char **)realloc(array->values, array->capacity * sizeof(char *));
		if (array->values == NULL)
			return -1;
	}
	char *str = strdup(v);

	if (cnv != 0) {
		for (char *s = str; *s; s++) {
			if (cnv == 1)
				s[0] = tolower(s[0]);
			else
				s[0] = toupper(s[0]);
		}
	}
	array->values[array->count] = str;

	array->count++;
	return 1;
}

AD* _ad = NULL;

int AD_create(uint8_t t1055) {
    _ad = (AD *)malloc(sizeof(AD));
    if (_ad == NULL)
        return -1;
    memset(_ad, 0, sizeof(AD));
    
	if (int64_array_init(&_ad->docs) != 0)
		return -1;
	if (string_array_init(&_ad->phones) != 0)
		return -1;
	if (string_array_init(&_ad->emails) != 0)
		return -1;

	_ad->t1055 = t1055;
    _ad->clist.delete_func = (list_item_delete_func_t)C_destroy;

    return 0;
}

void AD_destroy() {
    list_clear(&_ad->clist);
    if (_ad->p1 != NULL)
        P1_destroy(_ad->p1);
    if (_ad->t1086 != NULL)
        free(_ad->t1086);
    int64_array_free(&_ad->docs);
    string_array_free(&_ad->phones);
    string_array_free(&_ad->emails);
    free(_ad);
    _ad = NULL;
}

#ifdef WIN32
#define FILE_NAME   "ad.bin"
#else
#define FILE_NAME   "/home/sterm/ad.bin"
#endif

int AD_save() {
    int ret = -1;
    int fd = s_open(FILE_NAME, true);
    
    if (fd == -1) {
        return -1;
    }
    
    if (SAVE_INT(fd, (uint8_t)(_ad->p1 != 0 ? 1 : 0)) < 0 ||
        (_ad->p1 != NULL && P1_save(fd, _ad->p1) < 0) ||
        save_string(fd, _ad->t1086) < 0 ||
        save_list(fd, &_ad->clist, (list_item_func_t)C_save) < 0)
        ret = -1;
    else
        ret = 0;

    s_close(fd);
    return ret;
}

int AD_load(uint8_t t1055, bool clear) {
    if (_ad != NULL)
        AD_destroy();
    if (AD_create(t1055) < 0)
        return -1;

    if (clear)
	return 0;

    int fd = s_open(FILE_NAME, false);
    if (fd == -1)
        return -1;
    int ret;
    uint8_t hasP1 = 0;
    
    if (LOAD_INT(fd, hasP1) < 0 ||
        (hasP1 && (_ad->p1 = P1_load(fd)) == NULL) ||
        load_string(fd, &_ad->t1086) < 0 ||
        load_list(fd, &_ad->clist, (load_item_func_t)C_load) < 0)
        ret = -1;
    else
        ret = 0;

    AD_calc_sum();
        
    printf("AD_load: %d, ad.C.count: %zu, ad.docs.count: %zu\n", ret, _ad->clist.count,
    	_ad->docs.count);
    printf("ad.t1086: %s\n", _ad->t1086);
    
    s_close(fd);
    return ret;
}

void AD_setP1(P1 *p1) {
	if (_ad->p1 != NULL)
		P1_destroy(_ad->p1);
	_ad->p1 = p1;

    if (_ad->p1 != NULL) {
        size_t l = 0;
        if (_ad->p1->i)
            l += strlen(_ad->p1->i);
        if (_ad->p1->p)
            l += strlen(_ad->p1->p);
        if (_ad->p1->t)
            l += strlen(_ad->p1->t);

		if (_ad->t1086)
			free(_ad->t1086);
        _ad->t1086 = (char *)malloc(l + 1);
        sprintf(_ad->t1086, "%s%s%s",
                _ad->p1->i ? _ad->p1->i : "",
                _ad->p1->p ? _ad->p1->p : "",
                _ad->p1->t ? _ad->p1->t : "");
    }

	AD_save();
}

int AD_delete_doc(int64_t doc) {
	int count = 0;
	for (list_item_t *li1 = _ad->clist.head; li1;) {
		C *c = LIST_ITEM(li1, C);
		li1 = li1->next;
		for (list_item_t *li2 = c->klist.head; li2;) {
			K *k = LIST_ITEM(li2, K);
			li2 = li2->next;
			if (doc_no_to_i64(&k->d) == doc) {
				list_remove(&c->klist, k);
				count++;
			}
		}
		if (c->klist.count == 0)
			list_remove(&_ad->clist, c);
	}

 //   for (list_it_t i1 = LIST_IT(&_ad->clist); !LIST_IT_END(i1); list_it_next(&i1)) {
 //       C *c = LIST_IT_OBJ(i1, C);
	//	for (list_it_t i2 = LIST_IT(&c->klist); !LIST_IT_END(i2); list_it_next(&i2)) {
	//		K *k = LIST_IT_OBJ(i2, K);
	//		if (doc_no_to_i64(&k->d) == doc) {
	//			list_it_remove(&i2);
	//			count++;
 //           }
 //       }
	//	if (c->klist.count == 0)
	//		list_it_remove(&i1);
	//}

	AD_calc_sum();

    if (count)
    	AD_save();
    return count;
}

void AD_unmark_reissue_doc(int64_t doc) {
	for (list_item_t *li1 = _ad->clist.head; li1;) {
		C *c = LIST_ITEM(li1, C);
		li1 = li1->next;
		for (list_item_t *li2 = c->klist.head; li2;) {
			K *k = LIST_ITEM(li2, K);
			li2 = li2->next;
			if (doc_no_to_i64(&k->d) == doc) {
				doc_no_clear(&k->u);
				AD_save();
				AD_calc_sum();
				return;
			}
		}
	}
}

void AD_remove_C(C *c) {
	list_item_t *li = c->klist.head;
	while (li) {
		K *k = LIST_ITEM(li, K);
		li = li->next;
		if (doc_no_is_empty(&k->u))
			list_remove(&c->klist, k);
	}
	if (c->klist.count == 0)
		list_remove(&_ad->clist, c);
	else
		C_calc_sum(c);
	AD_save();
}

C* AD_getCheque(K *k, uint8_t t1054, uint8_t t1055) {
    for (list_item_t *i = _ad->clist.head; i != NULL; i = i->next) {
        C *c = (C *)i->obj;
        if (c->p == k->p && strcmp2(c->h, k->h) == 0 && c->t1054 == t1054 && c->t1055 == t1055)
            return c;
    }
    return NULL;
}

int AD_makeCheque(K *k, doc_no_t *d, uint8_t t1054, uint8_t t1055, int64_t s) {
    if (k->llist.count == 0)
        return 0;
    
    doc_no_copy(&k->d, d);
    C* c = AD_getCheque(k, t1054, t1055);
    
    if (c == NULL) {
    	printf("create new cheque\n");

        c = C_create();
        c->p = k->p;
        c->h = strdup(k->h);
        c->t1054 = t1054;
        c->t1055 = t1055;

        list_add(&_ad->clist, c);
    } else
    	printf("cheque found\n");

    k->a = s;
    list_add(&c->klist, k);
    K_after_add(k);

    printf("AD CH: %zu\n", _ad->clist.count);

    return 0;
}

int AD_makeAnnul(K *k, uint8_t o, uint8_t t1054, uint8_t t1055) {
	// для всех

	for (list_item_t *li1 = _ad->clist.head; li1;) {
		C *c = LIST_ITEM(li1, C);
		li1 = li1->next;
		if (c->p == k->p && strcmp2(c->h, k->h) == 0 && c->t1054 == t1054 && c->t1055 == t1055) {
			for (list_item_t *li2 = c->klist.head; li2;) {
				K *k1 = LIST_ITEM(li2, K);
				li2 = li2->next;

				if (doc_no_compare(&k1->i1, &k->r) == 0 && k1->m == k->m && k1->o == o) {
					if (K_equalByL(k, k1)) {
						list_remove(&c->klist, k1);
						if (c->klist.count == 0)
							list_remove(&_ad->clist, c);
						printf("Удаляем K из корзины\n");
						K_destroy(k);
						return 0;
					}
				}
			}
		}
	}

    //for (list_it_t i1 = LIST_IT(&_ad->clist); !LIST_IT_END(i1); list_it_next(&i1)) {
    //    C *c = LIST_IT_OBJ(i1, C);
    //    if (c->p == k->p && strcmp2(c->h, k->h) == 0 && c->t1054 == t1054 && c->t1055 == t1055) {
    //        for (list_it_t i2 = LIST_IT(&c->klist); !LIST_IT_END(i2); list_it_next(&i2)) {
    //            K *k1 = LIST_IT_OBJ(i2, K);
    //            if (doc_no_compare(&k1->i1, &k->r) == 0 &&
				//	k1->m == k->m && k1->o == o) {
    //                if (K_equalByL(k, k1)) {
    //                    list_it_remove(&i2);
    //                    if (c->klist.count == 0) {
    //                        list_it_remove(&i1);
    //                    }
    //                    printf("Удаляем K из корзины\n");
				//		K_destroy(k);
				//		return 0;
				//	}
				//}
    //        }
    //    }
    //}
	op_doc_no_set(&k->b, &k->r, "гашение", NULL);
	AD_makeCheque(k, &k->r, 2, t1055, 0);
    return 0;
}

int AD_makeAnnulReturn(K *k, K *k2, uint8_t t1055, int64_t sB) {
	C *f = NULL;
	K *f_k = NULL;

	C *s = NULL;
	K *s_k = NULL;

	for (list_item_t *li11 = _ad->clist.head; li11; li11 = li11->next) {
		f = LIST_ITEM(li11, C);
		if (f->p == k->p && strcmp2(f->h, k->h) == 0 && f->t1054 == 2 && f->t1055 == t1055) {
			for (list_item_t *li12 = f->klist.head; li12; li12 = li12->next) {
				f_k = LIST_ITEM(li12, K);
				if ((doc_no_is_empty(&f_k->i2) || doc_no_compare(&f_k->i2, &k->r) == 0) &&
					doc_no_compare(&f_k->i21, &k->r) == 0 && f_k->m == k->m && f_k->o == 2) {
					if (K_equalByL(k, f_k)) {
						goto L1;
					}
				}
			}
		}
	}

L1:
	if (f_k && k2 && k2->llist.head) {
		for (list_item_t *li21 = _ad->clist.head; li21; li21 = li21->next) {
			s = LIST_ITEM(li21, C);
			if (s->p == k2->p && strcmp2(s->h, k2->h) == 0 && s->t1054 == 1 && s->t1055 == t1055) {
				for (list_item_t *li22 = s->klist.head; li22; li22 = li22->next) {
					s_k = LIST_ITEM(li22, K);
					if ((doc_no_is_empty(&s_k->i2) || doc_no_compare(&s_k->i2, &k2->r) == 0) &&
						doc_no_compare(&s_k->i21, &k2->r) == 0 && s_k->m == k2->m && s_k->o == 2) {
						if (K_equalByL(k2, s_k))
							goto L2;
					}
				}
			}
		}
		f = NULL;
		f_k = NULL;
	}

L2:
	if (f) {
		list_remove(&f->klist, f_k);
		if (!f->klist.count)
			list_remove(&_ad->clist, f);
		if (s) {
			list_remove(&s->klist, s_k);
			if (!s->klist.count)
				list_remove(&_ad->clist, s);
		}
	} else {
		op_doc_no_set(&k->b, &k->r, "гашение возврата", NULL);
		AD_makeCheque(k, &k->r, 1, t1055, sB);

		if (k2 && k2->llist.count > 0) {
			op_doc_no_set(&k2->b, &k->r, "гашение возврата", NULL);
			AD_makeCheque(k2, &k->r, 2, t1055, sB);
		}
	}
	return 0;
}

int AD_makeReissue(K *k, uint8_t t1055, int64_t tB1, int64_t tB2, int64_t *sB1, int64_t *sB2) {
	C *g = NULL;
	K *g_x = NULL;

	for (list_item_t *li11 = _ad->clist.head; li11; li11 = li11->next) {
		g = LIST_ITEM(li11, C);
		if (g->p == k->p && strcmp2(g->h, k->h) == 0 && g->t1054 == 1 && g->t1055 == t1055) {
			for (list_item_t *li12 = g->klist.head; li12; li12 = li12->next) {
				g_x = LIST_ITEM(li12, K);
				if (g_x->o == 1 && doc_no_special_compare(&k->n, &g_x->d) == 0) {
					if (g_x->m == k->m) {
						int64_t tB = K_calc_total_sum(g_x);
						int64_t sB = 0;
						if (tB + tB1 <= tB2) {
							sB = tB;
							*sB1 = tB1;
							*sB2 = tB + tB1;
						} else {
							*sB2 = tB2;
							if (tB <= tB1 && tB1 <= tB2) {
								sB = tB;
								*sB1 = tB2 - tB;
							} else if (tB <= tB2 && tB2 <= tB1) {
								sB = tB;
								*sB1 = tB2 - tB;
							} else if (tB2 <= tB && tB <= tB1) {
								sB = tB2;
								*sB1 = 0;
							} else if (tB1 <= tB && tB <= tB2) {
								sB = tB2 - tB1;
								*sB1 = tB1;
							} else if (tB1 <= tB2 && tB2 <= tB) {
								sB = tB2 - tB1;
								*sB1 = tB1;							
							} else if (tB2 <= tB1 && tB1 <= tB) {
								sB = 0;
								*sB1 = tB2;
							}
						}
						g_x->a = sB;
					}
					doc_no_clear(&g_x->u);
				}
			}
		}
	}
	return 0;
}

int AD_processO(K *k) {
    K *k1, *k2;
	int64_t tB1 = 0;
	int64_t tB2 = 0;
	int64_t sB1, sB2;

    printf("process new K. K->o = %d\n", k->o);

    switch (k->o) {
        case 1:
			doc_no_copy(&k->i1, &k->d);
			if (doc_no_is_not_empty(&k->r)) {
				doc_no_copy(&k->u, &k->r);
				op_doc_no_set(&k->b, &k->d, "переоформление", &k->r);
			} else
				op_doc_no_set(&k->b, &k->d, NULL, NULL);
			AD_makeCheque(k, &k->d, 1, _ad->t1055, 0);
            break;
		case 2:
			k1 = K_divide(k, 1, &tB1);
			tB2 = K_calc_total_sum(k);
			if (tB1 > tB2)
				sB1 = sB2 = tB2;
			else
				sB1 = sB2 = tB1;
			if (doc_no_is_not_empty(&k->n)) {
				tB2 = K_calc_total_sum(k);
				AD_makeReissue(k, _ad->t1055, tB1, tB2, &sB1, &sB2);
			}
			doc_no_copy(&k->i2, &k->d);
			doc_no_copy(&k->i21, &k->r);
			op_doc_no_set(&k->b, &k->r, "возврат", NULL);
			AD_makeCheque(k, &k->r, 2, _ad->t1055, sB2);
			if (k1) {
				if (doc_no_is_not_empty(&k1->d)) {
					doc_no_copy(&k1->i2, &k1->d);
					op_doc_no_set(&k1->b, &k1->d, "возврат", &k1->r);
				} else
					op_doc_no_set(&k1->b, NULL, "возврат", &k1->r);
				doc_no_copy(&k1->i21, &k1->r);
				AD_makeCheque(k1, &k1->d, 1, _ad->t1055, sB1);
			}
			break;
        case 3:
            AD_makeAnnul(k, 1, 1, _ad->t1055);
            break;
        case 4:
			tB1 = 0;
            k2 = K_divide(k, 2, &tB1);
            AD_makeAnnulReturn(k, k2, _ad->t1055, tB1);
            break;
    }
    return 0;
}

int AD_process(K* k) {
    AD_processO(k);
    AD_save();
#ifdef TEST_PRINT
static int stage = 0;
	AD_calc_sum();
	char buf[1024];
	sprintf(buf, "stage%d.txt", ++stage);
	int fd = fopen(buf, "w");
	AD_print(f);
	fclose(f);
#endif
    return 0;
}

#define REQUIRED_K_MASK 0x71
#define REQUIRED_L_MASK 0x1F
#define ERR_INVALID_VALUE   E_KKT_ILL_ATTR
#define ERR_NO_REQ_ATTR   	E_KKT_NO_ATTR

static K* _k = NULL;
static L* _l = NULL;
static P1* _p1 = NULL;
static uint32_t _kMask;
static uint32_t _lMask;
static uint32_t _p1Mask;

static int process_string_value(const char *tag, const char *name, const char *val,
                                size_t max_size,
                                uint32_t *mask,
                                uint32_t mask_value,
                                char **out) {
    size_t len = val != 0 ? strlen(val) : 0;
    if (len == 0) {
        printf("%s/@%s: Пустое значение\n", tag, name);
        return ERR_INVALID_VALUE;
    } else if (len > max_size) {
        printf("%s/@%s: Превышение длины\n", tag, name);
        return ERR_INVALID_VALUE;
    }
    *out = strdup(val);
    *mask |= mask_value;
    
    return 0;
}

static int process_int_value(const char *tag, const char *name, const char *val,
                                int64_t min, int64_t max,
                                uint32_t *mask,
                                uint32_t mask_value,
                                int64_t *out) {
    char *endp;
    int64_t v = strtoll(val, &endp, 10);
    
    if (endp == NULL || *endp != 0) {
        printf("%s/@%s: Ошибка преобразования к целому типу\n", tag, name);
        return ERR_INVALID_VALUE;
    }
    
    if (v < min || v > max) {
        printf("%s/@%s: Недопустимое значение\n", tag, name);
        return ERR_INVALID_VALUE;
    }

    *out = v;
    *mask |= mask_value;
    
    return 0;
}

static int process_doc_no_value(const char *tag, const char *name, const char *val,
	uint32_t *mask,
	uint32_t mask_value,
	bool can_stars,
	doc_no_t *out) {
	const char *s = val;
	int i;

//	printf("%s/@%s: %s\n", tag, name, val);
	for (i = 0; *s && i < 14; i++, s++) {
		if (!isdigit(*s) && (!can_stars || *s != '*' || i == 0 || i >= 4)) {
			printf("%s/@%s: Неправильный символ в номере документа\n", tag, name);
			return ERR_INVALID_VALUE;
		}
	}

//	printf("  i = %d\n", i);
	if (i < 13) {
		printf("%s/@%s: Неправильная длина номера документа\n", tag, name);
		return ERR_INVALID_VALUE;
	}

	doc_no_init(out, val);
	*mask |= mask_value;

	return 0;
}

static int process_currency_value(const char *tag, const char *name,
                                  const char *val,
                                  uint32_t *mask,
                                  uint32_t mask_value,
                                  int64_t *out)
{
    char *endp;
    double v = strtod(val, &endp);

    if (endp == NULL || *endp != 0) {
        printf("%s/@%s: Ошибка преобразования к вещественному типу\n", tag, name);
        return ERR_INVALID_VALUE;
    }

    *out = (int64_t)((v + 0.009) * 100);
    *mask |= mask_value;

    return 0;
}

static int process_phone_value(const char *tag, const char *name,
                                  const char *val,
                                  uint32_t *mask,
                                  uint32_t mask_value,
                                  char **out)
{
    char *s;
    size_t len = strlen(val);
    if (val == 0) {
        printf("%s/@%s: неправильная длина", tag, name);
        return ERR_INVALID_VALUE;
    }
    char f = val[0];
    if ((f == '+' && len > 15) || (f == '8' && len > 11)) {
        printf("%s/@%s: неправильная длина", tag, name);
        return ERR_INVALID_VALUE;
    }

    if (f == '8') {
        s = (char *)malloc(len + 2);
        s[0] = '+';
        s[1] = '7';
        memcpy(s + 2, val + 1, len - 1);
        s[len + 1] = 0;
    } else
        s = strdup(val);
    *out = s;
    *mask |= mask_value;

    return 0;
}

static int process_email_value(const char *tag, const char *name,
                               const char *val,
                               uint32_t *mask,
                               uint32_t mask_value,
                               char **out)
{
    int len = strlen(val);
    if (len < 3) {
        printf("%s/@%s: неправильная длина", tag, name);
        return ERR_INVALID_VALUE;
    }

    int at = -1;
    for (int i = 0; i < len; i++) {
        if (val[i] == '@') {
            if (i == 0 || at >= 0) {
                printf("%s/@%s: Неправильное значение", tag, name);
                return ERR_INVALID_VALUE;
            }
        }
    }
    if (at == len - 1)
        printf("%s/@%s: Неправильное значение", tag, name);

    *out = strdup(val);
    *mask |= mask_value;

    return 0;
}

int check_str_len(const char *tag, const char *name, const char *val,
                  size_t len1, size_t len2)
{
    size_t len = strlen(val);
    if (len != len1 && len != len2) {
        printf("%s/@%s: неправильная длина\n", tag, name);
        return ERR_INVALID_VALUE;
    }
    return 0;
}

// обработка XML
int kkt_xml_callback(uint32_t check, int evt, const char *name, const char *val)
{
    int64_t v64;
    int ret;
    switch (evt) {
        case 0:
            break;
        case 1:
            if (strcmp(name, "K") == 0) {
            	if (_k != NULL)
            		K_destroy(_k);
                _k = K_create();
                _kMask = 0;
            } else if (strcmp(name, "L") == 0) {
            	if (_l != NULL)
            		L_destroy(_l);
                _l = L_create();
                _lMask = 0;
            } else if (strcmp(name, "P1") == 0) {
            	if (_p1 != NULL)
            		P1_destroy(_p1);
                _p1 = P1_create();
                _p1Mask = 0;
            }
            break;
        case 2:
            if (strcmp(name, "K") == 0) {
                if (!check) {
                    AD_process(_k);
                } else {
                    const char tags[] = "ODRNPHMTE";
                    for (int i = 0, mask = 1; i < ASIZE(tags); i++, mask <<= 1) {
                        bool required = (REQUIRED_K_MASK & mask) != 0;
                        bool present = (_kMask & mask) != 0;

                        if (i == 1)
                            required = _k->o == 1;

                        if (required && !present) {
                            printf("Обязательный атрибут K/@%c отсутствует\n",
                                   tags[i]);
                            return ERR_NO_REQ_ATTR;
                        }
                    }
                    K_destroy(_k);
                }
                _k = NULL;
            } else if (strcmp(name, "L") == 0) {
                if (!check) {
                    K_addL(_k, _l);
                } else {
                    const char tags[] = "SPRTNC";
                    for (int i = 0, mask = 1; i < ASIZE(tags); i++, mask <<= 1) {
                        bool required = (REQUIRED_L_MASK & mask) != 0;
                        bool present = (_lMask & mask) != 0;

                        if (i == 5 && _l->n > 0 && _l->n <= 4)
                            required = true;

                        if (required && !present) {
                            printf("Обязательный атрибут L/@%c отсутствует\n",
                                   tags[i]);
							return ERR_NO_REQ_ATTR;
                        }
                    }
                    L_destroy(_l);
                }
                _l = NULL;
            } else if (strcmp(name, "P1") == 0) {
                if (!check) {
                	AD_setP1(_p1);
                	if (_p1->c)
	                	cashier_set_name(_p1->c);
                } else
                	P1_destroy(_p1);
               	_p1 = NULL;
            }
            break;
        case 3:
            if (_l != NULL) {
                if (strcmp(name, "S") == 0 &&
                    (ret = process_string_value("L", name, val, 128, &_lMask, 0x1,
                                                &_l->s)) != 0) {
                    return ret;
                } else if (strcmp(name, "P") == 0) {
                    if ((ret = process_int_value("L", name, val, 1, 4,
                                                     &_lMask, 0x02, &v64)) != 0)
                        return ret;
                    _l->p = (uint8_t)v64;
                } else if (strcmp(name, "R") == 0) {
                    if ((ret = process_int_value("L", name, val, 1, 4,
                                                 &_lMask, 0x04, &v64)) != 0)
                        return ret;
                    _l->r = (uint8_t)v64;
                } else if (strcmp(name, "T") == 0) {
                    if ((ret = process_currency_value("L", name, val,
                                                 &_lMask, 0x08, &_l->t)) != 0)
                        return ret;
                } else if (strcmp(name, "N") == 0) {
                    if ((ret = process_int_value("L", name, val, 0, 5,
                                                 &_lMask, 0x10, &v64)) != 0)
                        return ret;
					_l->n = (uint8_t)v64;
                } else if (strcmp(name, "C") == 0) {
                    if ((ret = process_currency_value("L", name, val,
                                                      &_lMask, 0x20, &_l->c)) != 0)
                        return ret;
                }
            } else if (_k != NULL) {
                if (strcmp(name, "O") == 0) {
                    if ((ret = process_int_value("K", name, val, 0, 4,
                                                 &_kMask, 0x1, &v64)) != 0)
                        return ret;
					_k->o = (uint8_t)v64;
                } else if (strcmp(name, "D") == 0) {
                    if ((ret = process_doc_no_value("K", name, val, &_kMask, 0x2, false, &_k->d)) != 0)
                        return ret;
				} else if (strcmp(name, "R") == 0) {
					if ((ret = process_doc_no_value("K", name, val, &_kMask, 0x4, true, &_k->r)) != 0)
						return ret;
				} else if (strcmp(name, "N") == 0) {
					if ((ret = process_doc_no_value("K", name, val, &_kMask, 0x8, true, &_k->n)) != 0)
						return ret;
				} else if (strcmp(name, "P") == 0) {
                    if ((ret = check_str_len("K", name, val, 10, 12)) != 0 ||
                        (ret = process_int_value("K", name, val, 0, INT64_MAX,
                                                 &_kMask, 0x10, &v64)) != 0)
                        return ret;
                    _k->p = v64;
                } else if (strcmp(name, "H") == 0 &&
                           (ret = process_phone_value("K", name, val,
                                                      &_kMask, 0x20, &_k->h)) != 0) {
                    return ret;
                } else if (strcmp(name, "M") == 0) {
                    if ((ret = process_int_value("K", name, val, 0, 3,
                                                 &_kMask, 0x40, &v64)) != 0)
                        return ret;
					_k->m = (uint8_t)v64;
                } else if (strcmp(name, "T") == 0 &&
                          (ret = process_phone_value("K", name, val,
                                                     &_kMask, 0x80, &_k->t)) != 0) {
                    return ret;
                }  else if (strcmp(name, "E") == 0 &&
                            (ret = process_email_value("K", name, val,
                                                       &_kMask, 0x100, &_k->e)) != 0) {
                    return ret;
                }
            } else if (_p1 != NULL) {
                if (strcmp(name, "I") == 0) {
                    _p1->i = strdup(val);
                    _p1Mask |= 0x01;
                } else if (strcmp(name, "P") == 0) {
                    _p1->p = strdup(val);
                    _p1Mask |= 0x02;
                } else if (strcmp(name, "T") == 0) {
                    _p1->t = strdup(val);
                    _p1Mask |= 0x04;
                } else if (strcmp(name, "C") == 0) {
                    _p1->c = strdup(val);
                    _p1Mask |= 0x08;
                }
            }
            break;
        case 4:
			if (!check)
				AD_calc_sum();
            break;
    }
    return 0;
}

static void AD_sum_add(S *dst, S *src) {
	dst->n += src->n;
	dst->e += src->e;
	dst->p += src->p;
	dst->b += src->b;
	dst->a += src->a;
}

void C_calc_sum(C *c) {
	memset(&c->sum, 0, sizeof(c->sum));
	for (list_item_t *li2 = c->klist.head; li2 != NULL; li2 = li2->next) {
		K *k = LIST_ITEM(li2, K);
		int64_t d = doc_no_to_i64(&k->d);
		int64_array_add(&_ad->docs, d, true);

		if (k->llist.count == 0)
			continue;

		struct S ks = { 0, 0, 0, 0, 0 };
		K_calc_sum(k, &ks);

		if (k->a > 0) {
			ks.b = k->a;
			switch (k->m) {
			case 1:
				ks.n -= k->a;
				break;
			case 2:
				ks.e -= k->a;
				break;
			case 3:
				ks.p -= k->a;
				break;
			}
		}

		AD_sum_add(&c->sum, &ks);
	}
}

void AD_calc_sum() {
	memset(_ad->sum, 0, sizeof(_ad->sum));
	_ad->docs.count = 0;

	for (list_item_t *li1 = _ad->clist.head; li1 != NULL; li1 = li1->next) {
		C *c = LIST_ITEM(li1, C);
		memset(&c->sum, 0, sizeof(c->sum));
		for (list_item_t *li2 = c->klist.head; li2 != NULL; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			int64_t d = doc_no_to_i64(&k->d);
			int64_array_add(&_ad->docs, d, true);

			if (k->llist.count == 0 || doc_no_is_not_empty(&k->u))
				continue;

			struct S ks = { 0, 0, 0, 0, 0 };
			S *s = &_ad->sum[c->t1054 - 1];

			K_calc_sum(k, &ks);

			if (k->a > 0) {
				ks.b = k->a;
				switch (k->m) {
				case 1:
					ks.n -= k->a;
					break;
				case 2:
					ks.e -= k->a;
					break;
				case 3:
					ks.p -= k->a;
					break;
				}
			}

			AD_sum_add(&c->sum, &ks);
			AD_sum_add(s, &ks);
		}
	}
}

bool AD_get_state(AD_state *s) {
	memset(s, 0, sizeof(*s));
	for (list_item_t *li1 = _ad->clist.head; li1 != NULL; li1 = li1->next) {
		C *c = LIST_ITEM(li1, C);
		size_t n = 0;
		for (list_item_t *li2 = c->klist.head; li2 != NULL; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			if (doc_no_is_empty(&k->u)) {
				n++;
				if (k->m == 2) {
					s->has_cashless_payments = true;
					int64_t sum = 0;

					for (list_item_t *li3 = k->llist.head; li3 != NULL; li3 = li3->next) {
						L *l = LIST_ITEM(li3, L);
						sum += l->t;
					}
					sum -= k->a;
					if (c->t1054 == 1 || c->t1054 == 4)
						s->cashless_total_sum += sum;
					else
						s->cashless_total_sum -= sum;
				}
			}
		}
		if (n > 0)
			s->actual_cheque_count++;
	}
	return s->actual_cheque_count > 0;
}

// получение документов для переоформления
size_t AD_get_reissue_docs(int64_array_t* docs) {
	int64_array_clear(docs);
	for (list_item_t *li1 = _ad->clist.head; li1 != NULL; li1 = li1->next) {
		C *c = LIST_ITEM(li1, C);
		for (list_item_t *li2 = c->klist.head; li2 != NULL; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			if (!doc_no_is_empty(&k->u)) {
				int64_t d = doc_no_to_i64(&k->d);
				int64_array_add(docs, d, true);
			}
		}
	}
	return docs->count;
}

#ifdef TEST_PRINT

#define CN(s) ((s) ? (s) : "")

void AD_print(FILE *fd) {
	fprintf(fd, "AD\n");
	S *s = _ad->sum;
	const char *s_title[] = { "Приход", "Возврат прихода", "Расход", "Возврат расхода" };
	for(size_t i = 0; i < 4; i++, s++) {
		fprintf(fd, "  SUM[%s]\n", s_title[i]);
		fprintf(fd, "    N = %lld\n", s->n);
		fprintf(fd, "    E = %lld\n", s->e);
		fprintf(fd, "    P = %lld\n", s->p);
		fprintf(fd, "    B = %lld\n", s->b);
		fprintf(fd, "    A = %lld\n", s->a);
	}

	for (list_item_t *li1 = _ad->clist.head; li1; li1 = li1->next) {
		C *c = LIST_ITEM(li1, C);
		fprintf(fd, "C (P=%llu, H=%s, T1054=%d, T1055=%d, PE=%s)\n", c->p, c->h, c->t1054, c->t1055, CN(c->pe));
		fprintf(fd, "  SUM\n");
		fprintf(fd, "    N = %lld\n", c->sum.n);
		fprintf(fd, "    E = %lld\n", c->sum.e);
		fprintf(fd, "    P = %lld\n", c->sum.p);
		fprintf(fd, "    B = %lld\n", c->sum.b);
		fprintf(fd, "    A = %lld\n", c->sum.a);
		for (list_item_t *li2 = c->klist.head; li2; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			fprintf(fd, "  K (P=%lld, H=%s, M=%d, ", k->p, k->h, k->m);
			fprintf(fd, "D=%s, R=%s, \n     N=%s, ", CN(k->d.s), CN(k->r.s), CN(k->n.s));
			fprintf(fd, "I1=%s, I2=%s, I21=%s, ", CN(k->i1.s), CN(k->i2.s), CN(k->i21.s)); 
			fprintf(fd, "U=%s, A=%lld,\n     ", CN(k->u.s), k->a);
			fprintf(fd, "B=%s, ", CN(k->b.s));
			fprintf(fd, "T=%s, E=%s)\n", CN(k->t), CN(k->e));
			for (list_item_t *li3 = k->llist.head; li3; li3 = li3->next) {
				L *l = LIST_ITEM(li3, L);
				fprintf(fd, "       L (S=%s, P=%d, R=%d, ", l->s, l->p, l->r);
				fprintf(fd, "T=%lld, N=%d, C=%lld)\n", l->t, l->n, l->c);
			}
		}
	}
}

#endif
