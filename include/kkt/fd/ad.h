#ifndef AD_H
#define AD_H

#include <stdint.h>
#include <stdio.h>
#include <express.h>
#include "sysdefs.h"
#include "list.h"

#define AD_VERSION	2

struct C;
struct L;
struct K;

// сумма
typedef struct S {
    int64_t a; // общая сумма
    int64_t n; // сумма наличных
    int64_t e; // сумма электронных
    int64_t p; // сумма в зачет ранее внесенных средств
    int64_t b; // сумма расчета по всем документам чека встречным предоставлением
} S;

// составляющая
typedef struct L {
    char *s;        // Название составляющей
    uint8_t p;      // Код признака расчёта
    uint8_t r;      // Код признака способа расчёта
    int64_t t;      // Стоимость составляющей, включая НДС
    uint8_t n;      // Стоимость составляющей, включая НДС
    int64_t c;      // Величина НДС за составляющую
	int64_t i;		// ИНН поставщика
	char *h;		// Номер телефона поставщика
	char *z;		// Наименование поставщика
} L;

// создание составляющей
extern L *L_create(void);
// удаление составляющей
extern void L_destroy(L *l);
// запись составляющей в файл
extern int L_save(int fd, L *l);
// загрузка составляющей из файла
extern L *L_load_v1(int fd);
extern L *L_load_v2(int fd);

// номер документа
typedef struct {
	char *s;
} doc_no_t;

// инициализация номера документа из строки
void doc_no_init(doc_no_t *d, const char *s);
// освобождение памяти для номера документа
void doc_no_free(doc_no_t *d);
// сохранение номера документа в файле
int doc_no_save(int fd, doc_no_t *d);
// загрузка номера документа из файла
int doc_no_load(int fd, doc_no_t *d);
// копирование номера документа из одной ячейки в другую
void doc_no_copy(doc_no_t *dst, doc_no_t *src);
// сравнение номеров документа
int doc_no_compare(doc_no_t *d1, doc_no_t *d2);
// преобразование номера документа в 64-разрядное значение
int64_t doc_no_to_i64(doc_no_t *d);
// проверка номера документа на наличие значения
bool doc_no_is_empty(doc_no_t *d);
#define doc_no_is_not_empty(d)	(!doc_no_is_empty(d))

// операция с документами, описывающая связи
typedef struct {
	char *s;
} op_doc_no_t;

// документ
typedef struct K {
	struct list_t llist;    // список составляющих
    uint8_t o;          // Операция
    bool a_flag;		// Наличие флага А
	int64_t a;			// сумма встречного предоставления
	doc_no_t d;         // Номер оформляемого документа или КРС при возврате
	doc_no_t r;         // Номер документа, для которого оформляется дубликат, или возвращаемого документа или гасимого документа или гасимой КРС возврата
	doc_no_t n;         // Номер документа, на который переоформлен возвращаемый документ
	doc_no_t i1;        // индекс
	doc_no_t i2;        // индекс
	doc_no_t i21;       // индекс
	doc_no_t u;			// номер переоформляемого документа
	doc_no_t g;			// признак группировки билетов
	doc_no_t b;			// список документов
	int64_t p;          // ИНН перевозчика
    char *h;            // телефон перевозчика
    uint8_t m;          // способ оплаты
    char *t;            // номер телефона пассажира
    char *e;            // адрес электронной посты пассажира
	char *z;			// наименование договорного перевозчика
	char s;				// подкорзина для элемента
	struct bank_info_ex* y; // информация о банковском абзаце
	bool i;				// признак главного документа
	bool in_processing_state; // состояние в обработке
	int c;				// номер квитанции, по которой документ был оплачен или 0
	int bank_state;		// состояние обработки банковской транзакции
	int print_state;	// состояние печати
	bool check_state;	// состояние проверки
	uint8_t v; 			// вид деятельности (1 - основной, 2 - прочие)
	time_t dt;			// дата и время добавления документа
} K;

// проверка банковского абзаца на соответствие операции и возврата
#define check_k_y(k, o, r) ((k)->y->op == (o) && (k)->y->repayment == (r))

// создание информации о документе
extern K *K_create(void);
// удаление информации о документе
extern void K_destroy(K *k);
// добавление составляющей в документ
extern bool K_addL(K *k, L* l);
// разделение документа на 2 по признаку расчета
extern K *K_divide(K *k, uint8_t p, int64_t* sum);
// проверка на равенство по всем L
extern bool K_equalByL(K *k1, K *k2);
// получить сумму всех узлов L
extern int64_t K_get_sum(K *k);
// посчитать сумму для К
extern void K_calc_sum(K *k, S *s);


// установить код подкорзины
extern void set_k_s(char s, K* k, K* k1, K* k2);

// установить признак главного документа для группы документов
extern void set_k_i(K* k, K* k1, K* k2);

// запись документа в файл
extern int K_save(int fd, K *k);
// загрузка документа из файла
extern K *K_load_v1(int fd);
extern K *K_load_v2(int fd);

// чек
typedef struct C {
    list_t klist;	// список тэгов K
    uint64_t p;     // ИНН переводчика
    char *h;        // телефон перевозчика
    uint8_t t1054;  // признак расчета
    uint8_t t1055;  // применяемая система налогообложения
    S sum;          // сумма
    char *pe;       // Тел. или e-mail покупателя
} C;

extern C* C_create(void);
extern void C_destroy(C *c);
extern bool C_addK(C *c, K *k);

extern int C_save(int fd, C *c);

// загрузка из файла
extern C* C_load_v1(int fd);
extern C* C_load_v2(int fd);

void C_calc_sum(C *c);

// проверка чека, что он является агентским 
bool C_is_agent_cheque(C *c, int64_t user_inn, char *phone, bool *is_same_agent);

// данные кассира
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

// документ
typedef struct D {
	int64_t total_sum;	// общая сумма
	S sum;          	// сумма
	K *k;				// документ
	list_t related; 	// связанные документы
	list_t group; 		// группа документов
	char *description; // описание
	char *name; // наименование
} D;

extern D *D_create(void);
extern void D_destroy(D *d);

// подкорзина
typedef struct SubCart {
	list_t documents; // документы
	char type; // тип подкорзины
} SubCart;

#define MAX_SUB_CART	9
#define SUB_CART_INDEX(s)	((s) - 'A')
typedef struct Cart {
	SubCart sc[MAX_SUB_CART];
} Cart;

extern Cart cart;

extern void cart_build();

// массив 64-битных величин
typedef struct {
	int64_t *values;
	size_t capacity;
	size_t count;
} int64_array_t;
#define INIT_INT64_ARRAY { NULL, 0, 0 }

extern int int64_array_init(int64_array_t *array);
extern int int64_array_clear(int64_array_t *array);
extern int int64_array_free(int64_array_t *array);
// добавить значение (unique - добавить только то, значение, которое отсутствует в списке
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
// добавить значение (unique - добавить только то, значение, которое отсутствует в списке
// cnv - 0 - добавить как есть, 1 - преобразовать в нижний регистр, 
// 2 - преобразовать в верхний регистр)
extern int string_array_add(string_array_t *array, const char *s, bool unique, int cnv);

// Корзина фискального приложения
typedef struct AD {
	uint16_t version;	 // версия корзины
    P1* p1;              // данные кассира
    uint8_t t1055;
    char * t1086;
#define MAX_SUM 4
    S sum[MAX_SUM];     // сумма
    list_t clist;       // список чеков
	list_t archive_items; // архив
	int64_array_t docs;	// список документов
	string_array_t phones; // список телефонов
	string_array_t emails; // список e-mail
} AD;

// ссылка на текущую корзину
extern AD* _ad;

// удаление корзины
extern void AD_destroy(void);
// сохранение корзины на диск
extern int AD_save(void);
// загрузка корзины с диска
extern int AD_load(uint8_t t1055, bool clear);
// установка значения для P1
extern void AD_setP1(P1 *p1);
#define AD_doc_count() (_ad ? _ad->docs.count : 0)

// удаление из корзины документа
extern int AD_delete_doc(int64_t doc);
// расчет суммы по корзине
extern void AD_calc_sum();
// удаление чека из корзины
extern void AD_remove_C(C* c);

typedef struct AD_state {
	// актуальное количество чеков для печати
	int actual_cheque_count;
	// сумма к оплате по банковским операциям
	int64_t cashless_total_sum;
	// кол-во чеков с оплатой по банковской карте
	int cashless_cheque_count;
	// идентификатор заказа
	uint32_t order_id;
} AD_state;

// получение состояния корзины (false - корзина пуста)
extern bool AD_get_state(AD_state *s);
// получение документов для переоформления
extern size_t AD_get_reissue_docs(int64_array_t* docs);
extern void AD_unmark_reissue_doc(int64_t doc);

#ifdef TEST_PRINT
extern void AD_print(FILE *f);
extern void cart_print(FILE *fd);
#endif

// callback для обработки XML
extern int kkt_xml_callback(bool check, int evt, const char *name, const char *val);

extern void get_subcart_documents(char type, list_t *documents, const char *doc_no);
extern void process_non_cash_documents(list_t *documents, int invoice);

#endif // AD_H
