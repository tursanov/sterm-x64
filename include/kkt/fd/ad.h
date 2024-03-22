#ifndef AD_H
#define AD_H

#include <stdint.h>
#include <stdio.h>
#include "sysdefs.h"
#include "list.h"

#define AD_VERSION	2

struct C;
struct L;
struct K;

// ��⠢�����
typedef struct L {
    char *s;        // �������� ��⠢���饩
    uint8_t p;      // ��� �ਧ���� �����
    uint8_t r;      // ��� �ਧ���� ᯮᮡ� �����
    int64_t t;      // �⮨����� ��⠢���饩, ������ ���
    uint8_t n;      // �⮨����� ��⠢���饩, ������ ���
    int64_t c;      // ����稭� ��� �� ��⠢������
	int64_t i;		// ��� ���⠢騪�
	char *h;		// ����� ⥫�䮭� ���⠢騪�
	char *z;		// ������������ ���⠢騪�
} L;

// ᮧ����� ��⠢���饩
extern L *L_create(void);
// 㤠����� ��⠢���饩
extern void L_destroy(L *l);
// ������ ��⠢���饩 � 䠩�
extern int L_save(void *arg, L *l);
// ����㧪� ��⠢���饩 �� 䠩��
extern L *L_load_v1(int fd);
extern L *L_load_v2(int fd);

// ����� ���㬥��
typedef struct {
	char *s;
} doc_no_t;

// ���樠������ ����� ���㬥�� �� ��ப�
void doc_no_init(doc_no_t *d, const char *s);
// �᢮�������� ����� ��� ����� ���㬥��
void doc_no_free(doc_no_t *d);
// ��࠭���� ����� ���㬥�� � 䠩��
int doc_no_save(int fd, doc_no_t *d);
// ����㧪� ����� ���㬥�� �� 䠩��
int doc_no_load(int fd, doc_no_t *d);
// ����஢���� ����� ���㬥�� �� ����� �祩�� � �����
void doc_no_copy(doc_no_t *dst, doc_no_t *src);
// �ࠢ����� ����஢ ���㬥��
int doc_no_compare(doc_no_t *d1, doc_no_t *d2);
// �८�ࠧ������ ����� ���㬥�� � 64-ࠧ�來�� ���祭��
int64_t doc_no_to_i64(doc_no_t *d);
// �஢�ઠ ����� ���㬥�� �� ����稥 ���祭��
bool doc_no_is_empty(doc_no_t *d);
#define doc_no_is_not_empty(d)	(!doc_no_is_empty(d))

// ������ � ���㬥�⠬�, ����뢠��� �裡
typedef struct {
	char *s;
} op_doc_no_t;

// ���樠������
void op_doc_no_init(op_doc_no_t *d);
// �᢮��������
void op_doc_no_free(op_doc_no_t *d);
// ��࠭���� �� 䠩��
int op_doc_no_save(int fd, op_doc_no_t *d);
// ����㧪� �� 䠩��
int op_doc_no_load(int fd, op_doc_no_t *d);
// ����஢����
void op_doc_no_copy(op_doc_no_t *dst, op_doc_no_t *src);
// ��⠭���� ���祭��
void op_doc_no_set(op_doc_no_t *dst, doc_no_t *d1, const char *op, doc_no_t *d2);

// ���㬥��
typedef struct K {
	struct list_t llist;    // ᯨ᮪ ��⠢�����
    uint8_t o;          // ������
    bool a_flag;		// ����稥 䫠�� �
	int64_t a;			// �㬬� ����筮�� �।��⠢�����
	doc_no_t d;         // ����� ��ଫ塞��� ���㬥�� ��� ��� �� ������
	doc_no_t r;         // ����� ���㬥��, ��� ���ண� ��ଫ���� �㡫����, ��� �����頥���� ���㬥�� ��� ��ᨬ��� ���㬥�� ��� ��ᨬ�� ��� ������
	doc_no_t n;         // ����� ���㬥��, �� ����� ��८�ଫ�� �����頥�� ���㬥��
	doc_no_t i1;        // ������
	doc_no_t i2;        // ������
	doc_no_t i21;       // ������
	doc_no_t u;			// ����� ��८�ଫ塞��� ���㬥��
	op_doc_no_t b;		// ᯨ᮪ ���㬥�⮢
	int64_t p;          // ��� ��ॢ��稪�
    char *h;            // ⥫�䮭 ��ॢ��稪�
    uint8_t m;          // ᯮᮡ ������
    char *t;            // ����� ⥫�䮭� ���ᠦ��
    char *e;            // ���� ���஭��� ����� ���ᠦ��
	char *z;			// ������������ ������୮�� ��ॢ��稪�
	int32_t y;			// orderid ������᪮�� �����, ��� -1, �᫨ ��� ��� ��� K
} K;

// ᮧ����� ���ଠ樨 � ���㬥��
extern K *K_create(void);
// 㤠����� ���ଠ樨 � ���㬥��
extern void K_destroy(K *k);
// ���������� ��⠢���饩 � ���㬥��
extern bool K_addL(K *k, L* l);
// ࠧ������� ���㬥�� �� 2 �� �ਧ���� ����
extern K *K_divide(K *k, uint8_t p, int64_t* sum);
// �஢�ઠ �� ࠢ���⢮ �� �ᥬ L
extern bool K_equalByL(K *k1, K *k2);
// ������� �㬬� ��� 㧫�� L
extern int64_t K_get_sum(K *k);

// ������ ���㬥�� � 䠩�
extern int K_save(void *arg, K *k);
// ����㧪� ���㬥�� �� 䠩��
extern K *K_load_v1(int fd);
extern K *K_load_v2(int fd);

// �㬬�
typedef struct S {
    int64_t a; // ���� �㬬�
    int64_t n; // �㬬� �������
    int64_t e; // �㬬� ���஭���
    int64_t p; // �㬬� � ���� ࠭�� ���ᥭ��� �।��
    int64_t b; // �㬬� ���� �� �ᥬ ���㬥�⠬ 祪� ������ �।��⠢������
} S;

// 祪
typedef struct C {
    list_t klist;	// ᯨ᮪ ��� K
    uint64_t p;     // ��� ��ॢ��稪�
    char *h;        // ⥫�䮭 ��ॢ��稪�
    uint8_t t1054;  // �ਧ��� ����
    uint8_t t1055;  // �ਬ��塞�� ��⥬� ���������������
    S sum;          // �㬬�
    char *pe;       // ���. ��� e-mail ���㯠⥫�
} C;

extern C* C_create(void);
extern void C_destroy(C *c);
extern bool C_addK(C *c, K *k);

extern int C_save(void *arg, C *c);

// ����㧪� �� 䠩��
extern C* C_load_v1(int fd);
extern C* C_load_v2(int fd);

void C_calc_sum(C *c);

// �஢�ઠ 祪�, �� �� ���� �����᪨� 
bool C_is_agent_cheque(C *c, int64_t user_inn, char *phone, bool *is_same_agent);

// ����� �����
typedef struct P1 {
    char *i;
    char *p;
    char *t;
    char *c;
} P1;

extern P1 *P1_create(void);
extern void P1_destroy(P1 *p1);
extern int P1_save(int fd, P1 *p1);
extern P1* P1_load_v1(int fd);
extern P1* P1_load_v2(int fd);

// ���ᨢ 64-����� ����稭
typedef struct {
	int64_t *values;
	size_t capacity;
	size_t count;
} int64_array_t;
#define INIT_INT64_ARRAY { NULL, 0, 0 }

extern int int64_array_init(int64_array_t *array);
extern int int64_array_clear(int64_array_t *array);
extern int int64_array_free(int64_array_t *array);
// �������� ���祭�� (unique - �������� ⮫쪮 �, ���祭��, ���஥ ��������� � ᯨ᪥
extern int int64_array_add(int64_array_t *array, int64_t v, bool unique);

typedef struct string_array_t {
	char **values;
	size_t capacity;
	size_t count;
} string_array_t;
#define INIT_STRING_ARRAY { NULL, 0, 0 }

extern int string_array_init(string_array_t *array);
extern int string_array_clear(string_array_t *array);
extern int string_array_free(string_array_t *array);
// �������� ���祭�� (unique - �������� ⮫쪮 �, ���祭��, ���஥ ��������� � ᯨ᪥
// cnv - 0 - �������� ��� ����, 1 - �८�ࠧ����� � ������ ॣ����, 
// 2 - �८�ࠧ����� � ���孨� ॣ����)
extern int string_array_add(string_array_t *array, const char *s, bool unique, int cnv);

// ��২�� �᪠�쭮�� �ਫ������
typedef struct AD {
	uint16_t version;	 // ����� ��২��
    P1* p1;              // ����� �����
    uint8_t t1055;
    char * t1086;
#define MAX_SUM 4
    S sum[MAX_SUM];     // �㬬�
    list_t clist;       // ᯨ᮪ 祪��
	int64_array_t docs;	// ᯨ᮪ ���㬥�⮢
	string_array_t phones; // ᯨ᮪ ⥫�䮭��
	string_array_t emails; // ᯨ᮪ e-mail
} AD;

// ��뫪� �� ⥪���� ��২��
extern AD* _ad;

// 㤠����� ��২��
extern void AD_destroy(void);
// ��࠭���� ��২�� �� ���
extern int AD_save(void);
// ����㧪� ��২�� � ��᪠
extern int AD_load(uint8_t t1055, bool clear);
// ��⠭���� ���祭�� ��� P1
extern void AD_setP1(P1 *p1);
#define AD_doc_count() (_ad ? _ad->docs.count : 0)

// 㤠����� �� ��২�� ���㬥��
extern int AD_delete_doc(int64_t doc);
// ���� �㬬� �� ��২��
extern void AD_calc_sum();
// 㤠����� 祪� �� ��২��
extern void AD_remove_C(C* c);

typedef struct AD_state {
	// ���㠫쭮� ������⢮ 祪�� ��� ����
	int actual_cheque_count;
	// �㬬� � ����� �� ������᪨� ������
	int64_t cashless_total_sum;
	// ���-�� 祪�� � ����⮩ �� ������᪮� ����
	int cashless_cheque_count;
	// �����䨪��� ������
	int order_id;
} AD_state;

// ����祭�� ���ﭨ� ��২�� (false - ��২�� ����)
extern bool AD_get_state(AD_state *s);
// ����祭�� ���㬥�⮢ ��� ��८�ଫ����
extern size_t AD_get_reissue_docs(int64_array_t* docs);
extern void AD_unmark_reissue_doc(int64_t doc);

#ifdef TEST_PRINT
extern void AD_print(FILE *f);
#endif

#endif // AD_H
