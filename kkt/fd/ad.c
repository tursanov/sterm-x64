#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "sysdefs.h"
#include "serialize.h"
#if defined WIN32 || defined __APPLE__
#include "ad.h"
#include "express.h"
void cashier_set_name(const char *name) {
}
#define E_KKT_ILL_ATTR	0x95
#define E_KKT_NO_ATTR	0x96
#else
#include "express.h"
#include "kkt/fd/ad.h"

#endif

#ifdef WIN32
#define strcasecmp _stricmp
#define strdup _strdup
#endif

extern void cart_init();

static int dump_file_pos(const char *prefix, int fd)
{
	int position = lseek(fd, 0, SEEK_CUR);
	printf("%s: file pos: %d\n", prefix, position);

	return 0;
}

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

static char *strcopy(char **old, char *_new)
{
	if (*old)
		free(*old);
	*old = _new != NULL ? strdup(_new) : NULL;
	return *old;
}

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


static struct bank_info_ex *bank_info_clone(const struct bank_info_ex *v)
{
	if (v == NULL)
		return NULL;

	struct bank_info_ex *result = (struct bank_info_ex *)malloc(sizeof(struct bank_info_ex));
	*result = *v;
	return result;
}

static int bank_info_save(int fd, struct bank_info_ex *v)
{
	size_t size = v != NULL ? sizeof(struct bank_info_ex) : 0;
	return save_data(fd, v, size);
}

static int bank_info_load(int fd, struct bank_info_ex **v)
{
	dump_file_pos("bank_info_load", fd);
	size_t size;
	int ret = load_data(fd, (void **)v, &size);

	printf("bank_info_load: ret: %d, size: %d\n", ret, size);

    return ret;
}

static bool bank_info_compare(struct bank_info_ex *x, struct bank_info_ex *y)
{
	if (x == NULL && y == NULL)
	{
		return true;
	}
	
	if (x != NULL && y != NULL)
	{
		return
			x->id == y->id
			&& x->term_id == y->term_id
			&& x->op == y->op
			&& x->repayment == y->repayment
			&& x->ticket == y->ticket
			&& x->blank_nr == y->blank_nr
			&& x->prev_blank_nr == y->prev_blank_nr
			&& x->amount == y->amount;
	}

	return false;
}


L *L_create(void) {
    L *l = (L *)malloc(sizeof(L));
    memset(l, 0, sizeof(L));

    return l;
}

L* L_clone(L *l) {
	L *l1 = (L *)malloc(sizeof(L));
	memcpy(l1, l, sizeof(L));

	return l1;
}

void L_destroy(L *l) {
    if (l->s)
        free(l->s);
    if (l->h)
        free(l->h);
    if (l->z)
        free(l->z);
    free(l);
}

int L_save(int fd, L *l) {
    if (save_string(fd, l->s) < 0
			|| SAVE_INT(fd, l->p) < 0
			|| SAVE_INT(fd, l->r) < 0
			|| SAVE_INT(fd, l->t) < 0
			|| SAVE_INT(fd, l->n) < 0
			|| SAVE_INT(fd, l->c) < 0
			// V2
			|| SAVE_INT(fd, l->i) < 0
			|| save_string(fd, l->h) < 0
			|| save_string(fd, l->z) < 0)
        return -1;
    return 0;
}

L *L_load_v1(int fd) {
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

L *L_load_v2(int fd) {
    L *l = L_create();
    if (load_string(fd, &l->s) < 0
			|| LOAD_INT(fd, l->p) < 0
			|| LOAD_INT(fd, l->r) < 0
			|| LOAD_INT(fd, l->t) < 0
			|| LOAD_INT(fd, l->n) < 0
			|| LOAD_INT(fd, l->c) < 0
			// V2
			|| LOAD_INT(fd, l->i) < 0
			|| load_string(fd, &l->h) < 0
			|| load_string(fd, &l->z) < 0)
	{
        L_destroy(l);
        return NULL;
    }

	dump_file_pos("L_load_v2", fd);

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
    if (k->z)
        free(k->z);

	doc_no_free(&k->d);
	doc_no_free(&k->r);
	doc_no_free(&k->i1);
	doc_no_free(&k->i2);
	doc_no_free(&k->i21);
	doc_no_free(&k->u);
	doc_no_free(&k->b);
	if (k->y)
		free(k->y);

	free(k);
}

bool K_addL(K *k, L* l) {
	if (list_add(&k->llist, l) == 0) {
		return true;
	}
	return false;
}

uint8_t K_lp(K *k)
{
	return k->llist.head != NULL
		? LIST_ITEM(k->llist.head, L)->p
		: 0;
}

K *K_clone(K *k, bool clone_l)
{
	K* k1 = (K*)malloc(sizeof(K));
	memset(k1, 0, sizeof(K));
	k->llist.delete_func = (list_item_delete_func_t)L_destroy;

	k1->o = k->o;			// Операция
	k1->a = k->a;
	k1->p = k->p;			// ИНН перевозчика

	k1->h = COPYSTR(k->h);	// телефон перевозчика
	k1->m = k->m;			// способ оплаты
	k1->t = COPYSTR(k->t);	// номер телефона пассажира
	k1->e = COPYSTR(k->e);	// адрес электронной посты пассажира
	k1->z = COPYSTR(k->z);	// информация о перевозчике

	doc_no_copy(&k1->d, &k->d);
	doc_no_copy(&k1->r, &k->r);
	doc_no_copy(&k1->n, &k->n);
	doc_no_copy(&k1->i1, &k->i1);
	doc_no_copy(&k1->i2, &k->i2);
	doc_no_copy(&k1->i21, &k->i21);
	doc_no_copy(&k1->u, &k->u);
	doc_no_copy(&k1->b, &k->b);
	doc_no_copy(&k1->g, &k->g);

	k1->a_flag = k->a_flag;
	k1->y = bank_info_clone(k->y);

	k1->i = k->i;
	k1->s = k->s;
	k1->c = k->c;
	k1->v = k->v;
	k1->dt = k->dt;

	if (clone_l)
	{
		for (list_item_t *item = k->llist.head; item != NULL; item = item->next) {
			L* l = LIST_ITEM(item, L);
			L* l1 = L_clone(l);
			list_add(&k1->llist, l1);
		}
	}

	return k1;
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
				k1 = K_clone(k, false);
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

static int64_t K_calc_total_sum_by_P(K *k, int p) {
	int64_t sum = 0;
	for (list_item_t *li3 = k->llist.head; li3 != NULL; li3 = li3->next) {
		L *l = LIST_ITEM(li3, L);
		if (l->p == p)
			sum += l->t;
	}
	return sum;
}

static int L_compare(void *arg, L *l1, L *l2) {
	if (strcmp2(l1->s, l2->s) == 0 &&
		l1->r == l2->r &&
		l1->t == l2->t &&
		l1->n == l2->n &&
		l1->c == l2->c &&
		l1->i == l2->i &&
		strcmp2(l1->h, l2->h) == 0 &&
		strcmp2(l1->z, l2->z) == 0)
		return 0;
	return 1;
}

bool K_equalByL(K *k1, K* k2) {
    return list_compare(&k1->llist, &k2->llist, NULL, (list_item_compare_func_t)L_compare) == 0;
}

int K_save(int fd, K *k) {
    if (save_list(fd, &k->llist, (list_item_func_t)L_save) < 0 ||
        SAVE_INT(fd, k->o) < 0 ||
		SAVE_INT(fd, k->a) < 0 ||
        doc_no_save(fd, &k->d) < 0 ||
		doc_no_save(fd, &k->r) < 0 ||
		doc_no_save(fd, &k->n) < 0 ||
		doc_no_save(fd, &k->i1) < 0 ||
		doc_no_save(fd, &k->i2) < 0 ||
		doc_no_save(fd, &k->i21) < 0 ||
		doc_no_save(fd, &k->u) < 0 ||
		doc_no_save(fd, &k->g) < 0 ||
		doc_no_save(fd, &k->b) < 0 ||
		SAVE_INT(fd, k->p) < 0 ||
        save_string(fd, k->h) < 0 ||
        SAVE_INT(fd, k->m) < 0 ||
        save_string(fd, k->t) < 0 ||
        save_string(fd, k->e) < 0 ||
		// v2
        save_string(fd, k->z) < 0 ||
		SAVE_INT(fd, k->a_flag) < 0 ||
		bank_info_save(fd, k->y) < 0 ||
		SAVE_INT(fd, k->s) < 0 ||
		SAVE_INT(fd, k->i) < 0 ||
		SAVE_INT(fd, k->in_processing_state) < 0 ||
		SAVE_INT(fd, k->c) < 0 ||
		SAVE_INT(fd, k->bank_state) < 0 ||
		SAVE_INT(fd, k->print_state) < 0 ||
		SAVE_INT(fd, k->check_state) < 0 ||
		SAVE_INT(fd, k->v) < 0 ||
		SAVE_INT(fd, k->dt) < 0)
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

static void K_preprocess_L(K *k)
{
	for (list_item_t *li3 = k->llist.head; li3 != NULL; li3 = li3->next)
	{
		L *l = LIST_ITEM(li3, L);
		if (l->z && l->z[0] != 0)
		{
			if (!l->h)
				l->h = strdup("+70000000000");
		}
		else 
		{
			l->i = k->p;
			strcopy(&l->h, k->h);
			strcopy(&l->z, k->z);
		}
	}
}

static void K_set_pos_data(K *k)
{
	const struct bank_info_ex *b = get_bi_ex();
	k->y = bank_info_clone(b);
}

K *K_load_v1(int fd) {
    K *k = K_create();
    if (load_list(fd, &k->llist, (load_item_func_t)L_load_v1) < 0 ||
            LOAD_INT(fd, k->o) < 0 ||
			LOAD_INT(fd, k->a) < 0 ||
            doc_no_load(fd, &k->d) < 0 ||
			doc_no_load(fd, &k->r) < 0 ||
			doc_no_load(fd, &k->i1) < 0 ||
			doc_no_load(fd, &k->i2) < 0 ||
			doc_no_load(fd, &k->i21) < 0 ||
			doc_no_load(fd, &k->u) < 0 ||
			doc_no_load(fd, &k->b) < 0 ||
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

K *K_load_v2(int fd) {
    K *k = K_create();

	dump_file_pos("K_load_v2", fd);

	if (load_list(fd, &k->llist, (load_item_func_t)L_load_v2) < 0 ||
		LOAD_INT(fd, k->o) < 0 ||
		LOAD_INT(fd, k->a) < 0 ||
		doc_no_load(fd, &k->d) < 0 ||
		doc_no_load(fd, &k->r) < 0 ||
		doc_no_load(fd, &k->n) < 0 ||
		doc_no_load(fd, &k->i1) < 0 ||
		doc_no_load(fd, &k->i2) < 0 ||
		doc_no_load(fd, &k->i21) < 0 ||
		doc_no_load(fd, &k->u) < 0 ||
		doc_no_load(fd, &k->g) < 0 ||
		doc_no_load(fd, &k->b) < 0 ||
		LOAD_INT(fd, k->p) < 0 ||
		load_string(fd, &k->h) < 0 ||
		LOAD_INT(fd, k->m) < 0 ||
		load_string(fd, &k->t) < 0 ||
		load_string(fd, &k->e) < 0 ||
		// v2
		load_string(fd, &k->z) < 0 ||
		dump_file_pos("K_load_v2: before load y2", fd) < 0 ||
		LOAD_INT(fd, k->a_flag) < 0 ||
		bank_info_load(fd, &k->y) < 0 ||
		LOAD_INT(fd, k->s) < 0 ||
		LOAD_INT(fd, k->i) < 0 ||
		LOAD_INT(fd, k->in_processing_state) < 0 ||
		LOAD_INT(fd, k->c) < 0 ||
		LOAD_INT(fd, k->bank_state) < 0 ||
		LOAD_INT(fd, k->print_state) < 0 ||
		LOAD_INT(fd, k->check_state) < 0 ||
		LOAD_INT(fd, k->v) < 0 ||
		LOAD_INT(fd, k->dt) < 0)
	{
		K_destroy(k);
		return NULL;
	}

    K_after_add(k);
    return k;
}

void set_k_s(char s, K* k, K* k1, K* k2)
{
	k->s = s;

	if (k1 != NULL)
	{
		k1->s = s;
	}

	if (k2 != NULL)
	{
		k2->s = s;
	}
}

void set_k_i(K* k, K* k1, K* k2)
{
	k->i = true;

	if (k1 != NULL)
	{
		k1->i = false;
	}

	if (k2 != NULL)
	{
		k2->i = false;
	}
}

void set_k_g(doc_no_t *doc_no, K* k, K* k1, K* k2)
{
	doc_no_copy(&k->g, doc_no);
	if (k1 != NULL)
	{
		doc_no_copy(&k1->g, doc_no);
	}

	if (k2 != NULL)
	{
		doc_no_copy(&k2->g, doc_no);
	}
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

C * C_load_v1(int fd) {
    C *c = C_create();
    if (load_list(fd, &c->klist, (load_item_func_t)K_load_v1) < 0 ||
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

C * C_load_v2(int fd) {
    C *c = C_create();
    if (load_list(fd, &c->klist, (load_item_func_t)K_load_v2) < 0 ||
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

bool C_is_agent_cheque(C *c, int64_t user_inn, char* phone, bool *is_same_agent)
{
	// проверяем, что есть хотя бы один элемент L
	if (!c->klist.head)
		return false;
	K *k = LIST_ITEM(c->klist.head, K);
	if (!k->llist.head)
		return false;

	bool is_phone_set = false;
	char tmp_phone[32] = { 0 };

	*is_same_agent = false;

	for (list_item_t *li1 = c->klist.head; li1 != NULL; li1 = li1->next) {
		k = LIST_ITEM(li1, K);
		for (list_item_t *li3 = k->llist.head; li3 != NULL; li3 = li3->next) {
			L *l = LIST_ITEM(li3, L);
			const int64_t inn = l->i;

			if (inn == user_inn)
				return false;

			if (l->h) {
				if (tmp_phone[0] == 0) {
					strcpy(tmp_phone, l->h);
					*is_same_agent = true;
				} else if (strcmp(tmp_phone, l->h) != 0) {
					*is_same_agent = false;
				}

				if (!is_phone_set && strcmp(l->h, "+70000000000") != 0)
				{
					strcpy(phone, l->h);
					is_phone_set = true;
				}
			}
		}
	}

	if (!is_phone_set)
		strcpy(phone, "+70000000000");

	return true;
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

P1* P1_load_v1(int fd) {
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

P1* P1_load_v2(int fd) {
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

extern D *D_create(void)
{
	D *d = malloc(sizeof(D));
	memset(d, 0, sizeof(D));

	return d;
}

extern void D_destroy(D *d)
{
	if (d->description)
		free(d->description);
	if (d->name)
		free(d->name);
	list_clear(&d->related);
	list_clear(&d->group);
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
	_ad->archive_items.delete_func = (list_item_delete_func_t)K_destroy;

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
#elif __APPLE__
#define FILE_NAME   "ad.bin"
#define ARCHIVE_FILE_NAME   "archive.bin"
#else
#define FILE_NAME   "/home/sterm/ad.bin"
#define ARCHIVE_FILE_NAME   "/home/sterm/archive.bin"
#endif

int AD_save() {
    int ret = -1;
    int fd = s_open(FILE_NAME, true);

    if (fd == -1) {
        return -1;
    }

    if (SAVE_INT(fd, (uint8_t)AD_VERSION) < 0
			|| SAVE_INT(fd, (uint8_t)(_ad->p1 != 0 ? 1 : 0)) < 0
			|| (_ad->p1 != NULL && P1_save(fd, _ad->p1) < 0)
			|| save_string(fd, _ad->t1086) < 0
			|| save_list(fd, &_ad->clist, (list_item_func_t)C_save) < 0)
        ret = -1;
    else
        ret = 0;

    s_close(fd);

	if (_ad->archive_items.count > 0)
	{
		fd = s_open(ARCHIVE_FILE_NAME, true);

		if (save_list(fd, &_ad->archive_items, (list_item_func_t)K_save) < 0)
			ret = -1;
	}

    return ret;
}

int AD_archive_save()
{
	int ret = 0;

	if (_ad->archive_items.count > 0)
	{
		int fd = s_open(ARCHIVE_FILE_NAME, true);

		if (save_list(fd, &_ad->archive_items, (list_item_func_t)K_save) < 0)
			ret = -1;

		s_close(fd);
	}

	return ret;
}

int AD_load(uint8_t t1055, bool clear) {
    if (_ad != NULL)
        AD_destroy();
    if (AD_create(t1055) < 0)
        return -1;

	cart_init();

	if (clear)
		return 0;

    int fd = s_open(FILE_NAME, false);
    if (fd == -1)
        return -1;
    int ret;
    uint8_t hasP1 = 0;

	if (LOAD_INT(fd, hasP1) < 0)
		ret = -1;
	if (hasP1 < 2) // 1 ????? ?????
	{
		printf("**** AD version 1 ****\n");
		if ((hasP1 && (_ad->p1 = P1_load_v1(fd)) == NULL) ||
			load_string(fd, &_ad->t1086) < 0 ||
			load_list(fd, &_ad->clist, (load_item_func_t)C_load_v1) < 0)
			ret = -1;
		else
			ret = 0;
	} else if (hasP1 == 2) { // 2 ????? ?????
		printf("**** AD version 2 ****\n");
		if (LOAD_INT(fd, hasP1) < 0
				|| (hasP1 && (_ad->p1 = P1_load_v2(fd)) == NULL)
				|| load_string(fd, &_ad->t1086) < 0
				|| load_list(fd, &_ad->clist, (load_item_func_t)C_load_v2) < 0)
			ret = -1;
		else
			ret = 0;
	}

	AD_calc_sum();

    printf("AD_load: %d, ad.C.count: %lu, ad.docs.count: %lu\n", ret, _ad->clist.count,
    	_ad->docs.count);
    printf("ad.t1086: %s\n", _ad->t1086);

    s_close(fd);

	fd = s_open(ARCHIVE_FILE_NAME, false);
	if (fd != -1)
	{
		if (load_list(fd, &_ad->archive_items, (load_item_func_t)K_load_v2) < 0)
		{
			ret = -1;
		}
	}

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

void AD_remove_K(K *k) {
	for (list_item_t *li1 = _ad->clist.head; li1;) {
		C *c = LIST_ITEM(li1, C);
		li1 = li1->next;
		for (list_item_t *li2 = c->klist.head; li2;) {
			K *k1 = LIST_ITEM(li2, K);
			li2 = li2->next;

			if (k1 == k) {
				list_remove_without_delete(&c->klist, k);
				if (c->klist.count == 0)
				{
					list_remove(&_ad->clist, c);
				}
				AD_save();
				AD_calc_sum();
			}
		}
	}
}


//List<K> ZsList = ArchiveItems.FindAll(x => x.C == kX.C && x != kX);

//ArchiveItems.RemoveAll(x => x == kX || ZsList.Contains(x));

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
	k->a = s;
	C* c = AD_getCheque(k, t1054, t1055);

	if (s == 0)
	{
		doc_no_clear(&k->g);
		k->i = false;
	}

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

	list_add(&c->klist, k);
	K_after_add(k);

	printf("AD CH: %lu\n", _ad->clist.count);

	return 0;
}

struct find_annul
{
	K *k;
	K* x;
	K* x2;
	uint8_t t1055;
	bool hasD;
	char zs_s;
	char z1s_s;
};

int find_annul_k(struct find_annul *f, K *x)
{
	if (x->o == 1 && doc_no_compare(&x->i1, &f->k->i1) == 0 && x->y->op == '+' && x->y->repayment == 0)
	{
		f->x = x;
		return 0;
	}
	return 1;
}

int change_lp(void *arg, L* l)
{
	l->p = 3 - l->p;
	return 0;
}

int process_annul_items(struct find_annul *f, K *zs)
{
	if (zs->c != f->x->c)
	{
		return 1;
	}

	K* z1s = K_clone(zs, true);

	zs->bank_state = 0;
	zs->print_state = 0;
	zs->check_state = false;
	zs->o += 2;
	zs->s = f->zs_s;
	list_foreach(&zs->llist, NULL, (list_item_func_t)change_lp);
	AD_makeCheque(zs, &zs->d, K_lp(zs), f->t1055, zs->a);

	z1s->s = f->z1s_s;
	z1s->c = 0;
	AD_makeCheque(z1s, &z1s->d, K_lp(z1s), f->t1055, zs->a);

	return 0;
}

int AD_makeAnnul(K *k, uint8_t o, uint8_t t1054, uint8_t t1055) {
	// для всех
	for (list_item_t *li1 = _ad->clist.head; li1;) {
		C *c = LIST_ITEM(li1, C);
		li1 = li1->next;
		if (c->p == k->p && strcmp2(c->h, k->h) == 0 && c->t1054 == t1054/* && c->t1055 == t1055*/) {
			for (list_item_t *li2 = c->klist.head; li2;) {
				K *k1 = LIST_ITEM(li2, K);
				li2 = li2->next;

				if (k1->in_processing_state)
				{
					continue;
				}

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

	if (k->m == 1)
	{
		set_k_s('B', k, NULL, NULL);
	}
	else if (k->m == 2)
	{
		if (k->y == NULL)
		{
			set_k_s('I', k, NULL, NULL);
		}
		else
		{
			if (k->y->op != '-' || k->y->repayment != 0)
			{
				set_k_s('I', k, NULL, NULL);
			}
			else
			{
				struct find_annul f = {
					.k = k,
					.x = NULL,
					.x2 = NULL,
					.t1055 = t1055,
					.hasD = false,
					.zs_s = 'C',
					.z1s_s = 'D'
				};
				list_remove_if(&_ad->archive_items, &f, (list_item_func_t)find_annul_k);

				if (f.x == NULL)
				{
					set_k_s('H', k, NULL, NULL);
				}
				else
				{
					set_k_s('C', k, NULL, NULL);
					k->c = f.x->c;
					list_remove_if(&_ad->archive_items, &f, (list_item_func_t)process_annul_items);
					AD_archive_save();
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
	AD_makeCheque(k, &k->r, 2, t1055, 0);
	return 0;
}

int find_annul_return_items(struct find_annul *f, K *x)
{
	if (x->o == 2
		&& (f->hasD
			? (doc_no_compare(&x->i2, &f->k->i2) == 0 && doc_no_compare(&x->i21, &f->k->i21) == 0)
			: (doc_no_compare(&x->i2, &f->k->i2) == 0 || doc_no_compare(&x->i21, &f->k->i21) == 0))
		&& x->y != NULL
		&& x->y->op == '-'
		&& x->y->repayment == 0)
	{
		if (K_lp(x) == 2)
		{
			f->x = x;
		}
		else if (K_lp(x) == 1)
		{
			f->x2 = x;
		}
		return 0;
	}
	return 1;
}

int find_annul_return_items1(struct find_annul *f, K *x)
{
	if (x->o == 2
		&& doc_no_compare(&x->i2, &f->k->i2) == 0
		&& doc_no_compare(&x->i21, &f->k->i21) == 0
		&& x->y != NULL
		&& x->y->op == '+'
		&& x->y->repayment == 0)
	{
		if (K_lp(x) == 2)
		{
			f->x = x;
		}
		else if (K_lp(x) == 1)
		{
			f->x2 = x;
		}
		return 0;
	}
	return 1;
}

int AD_makeAnnulReturn(K *k, K *k2, uint8_t t1055, int64_t sB, int64_t tB1, int64_t tB2, bool hasD) {
	C *f = NULL;
	K *f_k = NULL;

	C *s = NULL;
	K *s_k = NULL;

	for (list_item_t *li11 = _ad->clist.head; li11; li11 = li11->next) {
		f = LIST_ITEM(li11, C);
		if (f->p == k->p && strcmp2(f->h, k->h) == 0 && f->t1054 == 2 && f->t1055 == t1055) {
			for (list_item_t *li12 = f->klist.head; li12; li12 = li12->next) {
				f_k = LIST_ITEM(li12, K);

				if (f_k->in_processing_state)
				{
					continue;
				}

				if (((!doc_no_is_empty(&f_k->d) &&
					  doc_no_compare(&f_k->i2, &k->r) == 0 &&
					  doc_no_compare(&f_k->i21, &k->d) == 0) ||
					 doc_no_compare(&f_k->i2, &k->r) == 0 ||
					 doc_no_compare(&f_k->i21, &k->r) == 0) &&
					f_k->m == k->m && f_k->o == 2) {
					if (K_equalByL(k, f_k)) {
						goto L1;
					}
				}
			}
		}
	}

	f = NULL;

L1:
	if (f_k && k2 && k2->llist.head) {
		for (list_item_t *li21 = _ad->clist.head; li21; li21 = li21->next) {
			s = LIST_ITEM(li21, C);
			if (s->p == k2->p && strcmp2(s->h, k2->h) == 0 && s->t1054 == 1 && s->t1055 == t1055) {
				for (list_item_t *li22 = s->klist.head; li22; li22 = li22->next) {
					s_k = LIST_ITEM(li22, K);

					if (s_k->in_processing_state)
					{
						continue;
					}

					if (((!doc_no_is_empty(&s_k->d) &&
						  doc_no_compare(&s_k->i2, &k2->r) == 0 &&
						  doc_no_compare(&s_k->i21, &k2->d) == 0) ||
						 doc_no_compare(&s_k->i2, &k2->r) == 0 ||
						 doc_no_compare(&s_k->i21, &k2->r) == 0) &&
						s_k->m == k2->m && s_k->o == 2) {
						if (K_equalByL(k2, s_k))
							goto L2;
					}
				}
			}
		}
		f = NULL;
		f_k = NULL;
	}

	s = NULL;
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
		if (k->m == 1)
		{
			if (tB1 > tB2)
			{
				set_k_s('A', k, k2, NULL);
			}
			else
			{
				set_k_s('B', k, k2, NULL);
			}
		}
		else if (k->m == 2)
		{
			if (k->y == NULL)
			{
				if (tB1 > tB2)
				{
					set_k_s('A', k, k2, NULL);
				}
				else
				{
					set_k_s('B', k, k2, NULL);
				}
			}
			else
			{
				if (k->y->op == '+' && k->y->repayment == 0)
				{
					struct find_annul fa = {
						.k = k,
						.x = NULL,
						.x2 = NULL,
						.t1055 = t1055,
						.hasD = hasD,
						.zs_s = 'G',
						.z1s_s = 'F'
					};

					list_remove_if(&_ad->archive_items, &f, (list_item_func_t)find_annul_return_items);
					bool success = fa.x != NULL
						&& (k2->llist.count > 0
							? fa.x2 != NULL && fa.x2->c == fa.x->c
							: fa.x2 == NULL);
					if (!success)
					{
						printf("far.x: %p, far.x2: %p\n", fa.x, fa.x2);
						set_k_s('H', k, k2, NULL);
					}
					else
					{
						set_k_s('G', k, k2, NULL);
						k->c = k2->c = fa.x->c;
						list_remove_if(&_ad->archive_items, &f, (list_item_func_t)process_annul_items);
						AD_archive_save();
					}
				}
				else if (k->y->op == '-' && k->y->repayment == 0)
				{
					struct find_annul fa = {
						.k = k,
						.x = NULL,
						.x2 = NULL,
						.t1055 = t1055,
						.hasD = hasD,
						.zs_s = 'C',
						.z1s_s = 'D'
					};
					list_remove_if(&_ad->archive_items, &f, (list_item_func_t)find_annul_return_items1);
					bool success = fa.x != NULL
								   && (k2->llist.count > 0
									   ? fa.x2 != NULL && fa.x2->c == fa.x->c
									   : fa.x2 == NULL);

					if (!success)
					{
						printf("far.x: %p, far.x2: %p\n", fa.x, fa.x2);
						set_k_s('H', k, k2, NULL);
					}
					else
					{
						set_k_s('C', k, k2, NULL);
						k->c = k2->c = fa.x->c;
						list_remove_if(&_ad->archive_items, &f, (list_item_func_t)process_annul_items);
						AD_archive_save();
					}
				}
				else
				{
					set_k_s('I', k, k2, NULL);
				}
			}
		}

		AD_makeCheque(k, &k->d, 1, t1055, sB);

		if (k2 && k2->llist.count > 0) {
			AD_makeCheque(k2, &k->r, 2, t1055, sB);
		}
	}
	return 0;
}

int AD_makeReissue(K *k, K *k1, uint8_t t1055, int64_t tB1, int64_t tB2, int64_t *sB1, int64_t *sB2) {
	C *g = NULL;
	K *g_x = NULL;

	for (list_item_t *li11 = _ad->clist.head; li11; li11 = li11->next) {
		g = LIST_ITEM(li11, C);
		if (g->p == k->p && strcmp2(g->h, k->h) == 0 && g->t1054 == 1 && g->t1055 == t1055) {
			for (list_item_t *li12 = g->klist.head; li12; li12 = li12->next) {
				g_x = LIST_ITEM(li12, K);

				if (g_x->in_processing_state)
				{
					continue;
				}

				if (g_x->o == 1 && doc_no_special_compare(&k->n, &g_x->d) == 0) {
					if (g_x->m == k->m && (k->m != 2 || !k->a_flag)) {
						doc_no_copy(&g_x->g, &k->g);
						int64_t tB = K_calc_total_sum(g_x);
						int64_t sB = 0;
						if (tB + tB1 <= tB2) {
							sB = tB;
							*sB1 = tB1;
							*sB2 = tB + tB1;

							if (k->m == 1)
							{
								set_k_s('B', k, k1, g_x);
							}
							else if (k->m == 2 && k->y == NULL && g_x->y == NULL)
							{
								set_k_s('B', k, k1, g_x);
							}
							else if (k->m == 2
									 && k->y != NULL
									 && k->y->op == '-'
									 && k->y->repayment == '1'
									 && g_x->y != NULL
									 && (g_x->y->op == '+' || g_x->y->op == '*')
									 && g_x->y->repayment == 0)
							{
								set_k_s('B', k, k1, g_x);
							}
							else
							{
								set_k_s('I', k, k1, g_x);
							}

							set_k_i(k, k1, g_x);
							set_k_g(&k->r, k, k1, g_x);
						} else {
							if (k->m == 1)
							{
								set_k_s('A', k, k1, g_x);
							}
							else if (k->m == 2)
							{
								if (k->y != NULL
									&& g_x->y != NULL
									&& bank_info_compare(k->y, g_x->y))
								{
									if (g_x->y->op == '+' && g_x->y->repayment == 0)
									{
										set_k_s('D', k, k1, g_x);
									}
									else if (g_x->y->op == '*' && g_x->y->repayment == 0)
									{
										set_k_s('E', k, k1, g_x);
									}
									else
									{
										set_k_s('I', k, k1, g_x);
									}
								}
								else if (k->y == NULL ||
										 (k->y != NULL && !(k->y->op == '-' && k->y->repayment == '1')))
								{
									set_k_s('I', k, k1, g_x);
								}
								else if (g_x->y == NULL ||
										 (g_x->y != NULL && !((g_x->y->op == '+' || g_x->y->op == '*') && g_x->y->repayment == 0)))
								{
									set_k_s('I', k, k1, g_x);
								}
								else if (k->y != NULL && k->y->op == '-' && k->y->repayment == '1' && g_x->y != NULL)
								{
									if (g_x->y->op == '+' && g_x->y->repayment == 0)
									{
										set_k_s('D', k, k1, g_x);
									}
									else if (g_x->y->op == '*' && g_x->y->repayment == 0)
									{
										set_k_s('E', k, k1, g_x);
									}
									else
									{
										set_k_s('I', k, k1, g_x);
									}
								}
							}

							set_k_g(&g_x->d, k, k1, g_x);
							set_k_i(g_x, k, k1);

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
	int64_t sB, sB1, sB2;
	bool has_d = false;

	printf("process new K. K->o = %d\n", k->o);

	K_preprocess_L(k);
	K_set_pos_data(k);

	switch (k->o) {
	case 1:
		doc_no_copy(&k->i1, &k->d);
		if (doc_no_is_not_empty(&k->r)) {
			doc_no_copy(&k->u, &k->r);
			doc_no_copy(&k->b, &k->d);
		}
		else
		{
			doc_no_copy(&k->b, &k->d);
		}

		if (k->m == 1)
		{
			set_k_s('A', k, NULL, NULL);
		}
		else if (k->m == 2)
		{
			if (k->y == NULL)
			{
				if (!doc_no_is_empty(&k->r))
				{
					printf("переоформление с выплатой");
					set_k_s('I', k, NULL, NULL);
				}
			}
			else
			{
				if (check_k_y(k, '+', 0))
				{
					set_k_s('D', k, NULL, NULL);
				}
				else if (check_k_y(k, '*', 0))
				{
					set_k_s('E', k, NULL, NULL);
				}
			}
		}

		AD_makeCheque(k, &k->d, 1, _ad->t1055, 0);
		break;
	case 2:
		if (doc_no_is_not_empty(&k->d))
		{
			doc_no_copy(&k->i2, &k->d);
			doc_no_copy(&k->i21, &k->r);
		}

		k1 = K_divide(k, 1, &tB1);

		if (doc_no_is_not_empty(&k1->d))
		{
			doc_no_copy(&k1->b, &k1->d);
		}
		else
		{
			doc_no_copy(&k1->d, &k1->r);
			doc_no_copy(&k1->b, &k1->r);
		}

		doc_no_copy(&k->b, &k->r);

		tB2 = K_calc_total_sum(k);
		if (tB1 > tB2)
		{
			sB1 = sB2 = tB2;
			doc_no_copy(&k->g, &k->d);
			doc_no_copy(&k1->g, &k1->d);
			set_k_i(k1, k, NULL);
		}
		else
		{
			sB1 = sB2 = tB1;
			doc_no_copy(&k->g, &k->r);
			doc_no_copy(&k1->g, &k1->r);
			set_k_i(k, k1, NULL);
		}

		if (k->m == 1)
		{
			if (tB1 > tB2)
			{
				set_k_s('A', k, k1, NULL);
			}
			else
			{
				set_k_s('B', k, k1, NULL);
			}
		}
		else if (k->m == 2)
		{
			if (k->y == NULL)
			{
				if (tB1 < tB2)
				{
					set_k_s('B', k, k1, NULL);
				}
				else
				{
					set_k_s('I', k, k1, NULL);
				}
			}
			else
			{
				if (tB1 > tB2)
				{
					if (check_k_y(k, '+', 0))
					{
						set_k_s('D', k, k1, NULL);
					}
					else if (check_k_y(k, '*', 0))
					{
						set_k_s('E', k, k1, NULL);
					}
				}
				else if (tB1 < tB2)
				{
					if (check_k_y(k, '-', '0'))
					{
						set_k_s('F', k, k1, NULL);
					}
					else
					{
						set_k_s('I', k, k1, NULL);
					}
				}
				else
				{
					// TODO Потенциальная ошибка (tB1 == tB2)
					printf("ошибка: tB1 == tB2");
				}
			}
		}

		if (doc_no_is_not_empty(&k->n)) {
			tB2 = K_calc_total_sum(k);
			AD_makeReissue(k, k1, _ad->t1055, tB1, tB2, &sB1, &sB2);
		}

		AD_makeCheque(k, &k->r, 2, _ad->t1055, sB2);
		if (k1 != NULL && k1->llist.count > 0)
		{
			AD_makeCheque(k1, &k1->d, 1, _ad->t1055, sB1);
		}
		break;
	case 3:
		doc_no_copy(&k->b, &k->r);
		doc_no_copy(&k->i1, &k->r);
		AD_makeAnnul(k, 1, 1, _ad->t1055);
		break;
	case 4:
		has_d = doc_no_is_not_empty(&k->d);

		if (has_d)
		{
			doc_no_copy(&k->i2, &k->r);
			doc_no_copy(&k->i21, &k->d);
		}
		else
		{
			doc_no_copy(&k->i2, &k->r);
			doc_no_copy(&k->i21, &k->r);
		}

		tB2 = 0;
		k2 = K_divide(k, 2, &tB2);
		tB1 = K_calc_total_sum_by_P(k, 1);

		if (has_d)
		{
			doc_no_copy(&k->b, &k->d);
		}
		else
		{
			doc_no_copy(&k->d, &k->r);
			doc_no_copy(&k->b, &k->r);
		}

		doc_no_copy(&k2->b, &k->r);

		if (tB1 > tB2)
		{
			sB = tB2;
			doc_no_copy(&k->g, &k->d);
			doc_no_copy(&k2->g, &k->d);
			set_k_i(k, k2, NULL);
		}
		else
		{
			sB = tB1;
			doc_no_copy(&k->g, &k->r);
			doc_no_copy(&k2->g, &k->r);
			set_k_i(k2, k, NULL);
		}

		AD_makeAnnulReturn(k, k2, _ad->t1055, sB, tB1, tB2, has_d);
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
	FILE *f = fopen(buf, "w");
	AD_print(f);
	fclose(f);
#endif
    return 0;
}

#define REQUIRED_K_MASK (0x71 + 0x400)
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
int kkt_xml_callback(bool check, int evt, const char *name, const char *val)
{
	int64_t v64;
	int ret;

	switch (evt) {
	case 0:
		break;
	case 1:
		if (strcmp(name, "K") == 0) {
			if (_l != NULL) {
				L_destroy(_l);
				_l = NULL;
			}
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
				const char tags[] = "ODRNPHMTEAZV";
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
			if (strcmp(name, "S") == 0) {
				if ((ret = process_string_value("L", name, val, 128, &_lMask, 0x1,
					&_l->s)) != 0)
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
			} else if (strcmp(name, "I") == 0) {
				if ((ret = check_str_len("L", name, val, 10, 12)) != 0 ||
					(ret = process_int_value("L", name, val, 0, INT64_MAX,
						&_lMask, 0x40, &v64)) != 0)
					return ret;
				_l->i = v64;
			} else if (strcmp(name, "H") == 0) {
				if ((ret = process_phone_value("L", name, val, &_lMask, 0x80,
					&_l->h)) != 0)
					return ret;
			} else if (strcmp(name, "Z") == 0) {
				if ((ret = process_string_value("L", name, val, 256, &_lMask, 0x100,
					&_l->z)) != 0)
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
			} else if (strcmp(name, "H") == 0) {
				if ((ret = process_phone_value("K", name, val,
					&_kMask, 0x20, &_k->h)) != 0)
					return ret;
			} else if (strcmp(name, "M") == 0) {
				if ((ret = process_int_value("K", name, val, 0, 3,
					&_kMask, 0x40, &v64)) != 0)
					return ret;
				_k->m = (uint8_t)v64;
			} else if (strcmp(name, "T") == 0) {
				if ((ret = process_phone_value("K", name, val,
					&_kMask, 0x80, &_k->t)) != 0)
					return ret;
			}  else if (strcmp(name, "E") == 0) {
				if ((ret = process_email_value("K", name, val,
					&_kMask, 0x100, &_k->e)) != 0)
					return ret;
			}  else if (strcmp(name, "A") == 0) {
				_k->a_flag = true;
				_kMask |= 0x200;
				return 0;
			} else if (strcmp(name, "Z") == 0) {
				if ((ret = process_string_value("K", name, val, 256, &_kMask, 0x400,
					&_k->z)) != 0)
					return ret;
			} else if (strcmp(name, "V") == 0) {
				if ((ret = process_int_value("V", name, val, 1, 2,
					&_kMask, 0x800, &v64)) != 0)
					return ret;
				_k->v = (uint8_t)v64;
			} else {
				printf("Unknown tag %s=\"%s\"\n", name, val);
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
		uint32_t order_id = 0;
		int64_t cashless_sum = 0;

		for (list_item_t *li2 = c->klist.head; li2 != NULL; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			if (doc_no_is_empty(&k->u)) {
				n++;
				if (k->m == 2) {
					int64_t sum = K_calc_total_sum(k);
					sum -= k->a;

					cashless_sum +=	(c->t1054 == 1 || c->t1054 == 4)
						? sum
						: -sum;
				}

				if (k->y != NULL && k->y->id > order_id)
					order_id = k->y->id;
			}
		}
		
		if (n > 0)
			s->actual_cheque_count++;

		if (cashless_sum != 0)
		{
			s->cashless_total_sum += cashless_sum;
			s->cashless_cheque_count++;
			s->order_id = order_id;
		}
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

void bank_info_print(FILE *fd, struct bank_info_ex *b)
{
	fprintf(fd, "%.7d;%s;%c;%s;",
		b->id,
		b->term_id,
		b->op,
		b->ticket ? "П" : "У");

	if (b->repayment == 0)
	{
		fprintf(fd, ";");
	}
	else if (b->repayment == '0')
	{
		fprintf(fd, "0;");
	}
	else
	{
		fprintf(fd, "1(%s);", b->prev_blank_nr);
	}

	fprintf(fd, "%s/%llu.%llu", b->blank_nr, b->amount/100, b->amount % 100);
}

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
			fprintf(fd, "T=%s, E=%s\n     ", CN(k->t), CN(k->e));
			fprintf(fd, "S=%c\n     ", k->s);
			fprintf(fd, "Y=");
			if (k->y)
			{
				bank_info_print(fd, k->y);
			}
			fprintf(fd, ")\n");
			for (list_item_t *li3 = k->llist.head; li3; li3 = li3->next) {
				L *l = LIST_ITEM(li3, L);
				fprintf(fd, "       L (S=%s, P=%d, R=%d, ", l->s, l->p, l->r);
				fprintf(fd, "T=%lld, N=%d, C=%lld)\n", l->t, l->n, l->c);
			}
		}
	}
}

void K_print(FILE *fd, K *k, int indent)
{
	fprintf(fd, "%*s (P=%lld, H=%s, M=%d, ", indent + 1, "K", k->p, k->h, k->m);
	fprintf(fd, "D=%s, R=%s, N=%s, ", CN(k->d.s), CN(k->r.s), CN(k->n.s));
	fprintf(fd, "I1=%s, I2=%s, I21=%s, ", CN(k->i1.s), CN(k->i2.s), CN(k->i21.s));
	fprintf(fd, "U=%s, A=%lld,", CN(k->u.s), k->a);
	fprintf(fd, "B=%s, ", CN(k->b.s));
	fprintf(fd, "T=%s, E=%s, ", CN(k->t), CN(k->e));
	fprintf(fd, "S=%c, ", k->s);
	fprintf(fd, "Y=");
	if (k->y)
	{
		bank_info_print(fd, k->y);
	}
	fprintf(fd, ")\n");
	for (list_item_t *li3 = k->llist.head; li3; li3 = li3->next) {
		L *l = LIST_ITEM(li3, L);
		fprintf(fd, "%*s (S=%s, P=%d, R=%d, T=%lld, N=%d, C=%lld)\n", indent + 3, "L", l->s, l->p, l->r, l->t, l->n, l->c);
	}
}

const char * get_cart_description(SubCart *sc)
{
	switch (sc->type)
	{
		case 'A':
			return "A-НАЛ";
		case 'B':
			return "B-ВОЗВР НАЛ";
		case 'C':
			return "C-ОТМ БЕЗНАЛ";
		case 'D':
			return "D-БЕЗНАЛ";
		case 'E':
			return "E-СБП";
		case 'F':
			return "F-ВОЗВР БЕЗНАЛ";
		case 'G':
			return "G-ОТМ ВОЗВР ПО КАРТЕ";
		case 'H':
			return "H-ДРУГИЕ";
		default:
			return "I-ОШИБКА";
	}
}

void cart_print(FILE *fd)
{
	fprintf(fd, "------------------SC BEGIN------------------\n");
	for (int i = 0; i < MAX_SUB_CART; i++)
	{
		SubCart *sc = &cart.sc[i];
		if (sc->documents.count > 0)
		{
			fprintf(fd, "[%s]\n", get_cart_description(sc));

			for (list_item_t* li1 = sc->documents.head; li1; li1 = li1->next)
			{
				D *d = LIST_ITEM(li1, D);
				for (list_item_t* li2 = d->related.head; li2; li2 = li2->next)
				{
					K *k = LIST_ITEM(li2, K);
					K_print(fd, k, k->i ? 2 : 4);
				}
			}
		}
	}
	fprintf(fd, "------------------SC END------------------\n");
}

#endif

Cart cart;

void sub_cart_init(SubCart *sc, char type)
{
	sc->type = type;
	sc->documents.delete_func = (list_item_delete_func_t)D_destroy;
}

void sub_cart_clear(SubCart *sc)
{
	list_clear(&sc->documents);
}

void cart_init()
{
	for (int i = 0; i < MAX_SUB_CART; i++)
	{
		sub_cart_init(&cart.sc[i], (char)('A' + i));
	}
}

void cart_clear()
{
	for (int i = 0; i < MAX_SUB_CART; i++)
	{
		sub_cart_clear(&cart.sc[i]);
	}
}

D *find_d_for_g(SubCart *sc, K* k)
{
	for (list_item_t* li1 = sc->documents.head; li1; li1 = li1->next)
	{
		D* d = LIST_ITEM(li1, D);
		K* k1 = LIST_ITEM(d->related.head, K);

		if (doc_no_compare(&k->g, &k1->g) == 0)
		{
			return d;
		}
	}

	return NULL;
}

D *add_k_to_sub_cart(K* k)
{
	if (k->s == 0)
	{
		printf("ОШИБКА: у k не установлена подкорзина!!!!!!\n");
	}

	int index = SUB_CART_INDEX(k->s);
	SubCart *sc = &cart.sc[index];
	D* d;

	// если нет g, то сразу добавляем в подкорзину
	if (doc_no_is_empty(&k->g))
	{
		d = D_create();
		d->k = k;
		list_add(&d->related, k);
		list_add(&sc->documents, d);
	}
	else // если g есть, то тут возможны варианты - первым попался подчинённый документ или основной
	{
		d = find_d_for_g(sc, k);
		if (d == NULL)
		{
			d = D_create();
			if (k->i)
			{
				d->k = k;
			}

			list_add(&d->related, k);
			list_add(&sc->documents, d);
		}
		else
		{
			if (k->i)
			{
				d->k = k;
				list_add_head(&d->related, k);
			}
			else
			{
				list_add(&d->related, k);
			}
		}
	}

	return d;
}

void cart_build()
{
	cart_clear();
	for (list_item_t *li1 = _ad->clist.head; li1; li1 = li1->next) {
		C *c = LIST_ITEM(li1, C);
		for (list_item_t *li2 = c->klist.head; li2; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			SubCart *sc = &cart.sc[SUB_CART_INDEX(k->s)];

			add_k_to_sub_cart(k);
		}
	}
}

void get_subcart_documents(char type, list_t *documents, const char *doc_no)
{
	int index = SUB_CART_INDEX(type);
	for (list_item_t *li1 = cart.sc[index].documents.head; li1; li1 = li1->next) {
		D *d = LIST_ITEM(li1, D);
		if (strcmp2(d->k->d.s, doc_no) == 0)
		{
			list_add(documents, d);
		}
	}
}

void process_non_cash_documents(list_t *documents, int invoice)
{
	for (list_item_t *li1 = documents->head; li1; li1 = li1->next) {
		D *d = LIST_ITEM(li1, D);
		for (list_item_t *li2 = d->related.head; li2; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			k->c = invoice;
			list_add(&_ad->archive_items, k);
			AD_remove_K(k);
		}
	}
	AD_archive_save();
	AD_save();
}