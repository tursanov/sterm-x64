#ifndef list_h
#define list_h

#include "sysdefs.h"

typedef struct list_item_t {
    struct list_item_t *next;
    struct list_item_t *prev;
    void *obj;
} list_item_t;

typedef void (*list_item_delete_func_t)(void *obj);
typedef int (*list_item_func_t)(void *arg, void *obj);
typedef int (*list_item_compare_func_t)(void *arg, void *x, void *y);

typedef struct list_t {
    struct list_item_t *head;
    struct list_item_t *tail;
    size_t count;
    list_item_delete_func_t delete_func;
} list_t;

#define LIST_INIT(name, delete_func) \
	list_t name = { NULL, NULL, 0, (list_item_delete_func_t)(delete_func) }

int list_add(list_t *list, void *obj);
int list_add_item(list_t *list, list_item_t *item);
int list_remove_item(list_t *list, list_item_t *item);
int list_remove(list_t *list, void *obj);
int list_remove_at(list_t *list, int i);
int list_remove_if(list_t *list, void *arg, list_item_func_t func);
int list_clear(list_t* list);
int list_compare(list_t *list1, list_t *list2,
                  void *arg, list_item_compare_func_t func);
int list_foreach(list_t* list, void *arg, list_item_func_t func);
list_item_t *list_item_at(list_t *list, int index);

#define LIST_ITEM(i, type) ((i) ? ((type *)((i)->obj)) : NULL)
#define USE_LIST_ITEM(x, i, type) type * x = LIST_ITEM(i, type)

//typedef struct list_it_t {
//    list_t *list;
//    list_item_t *i;
//	int flags;
//} list_it_t;
//
//#define LIST_IT(list) { (list), (list)->head, NULL, 0 }
//#define LIST_IT_END(it) ((it).i == NULL)
//#define LIST_IT_OBJ(it, type) ((type *)((it).i->obj))
//void list_it_next(list_it_t *it);
//void list_it_remove(list_it_t *it);

#endif /* list_h */
