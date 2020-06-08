#ifndef ARTICLE_H
#define ARTICLE_H


#include "list.h"

typedef struct {
	int n;
	char *name;
	uint8_t pay_method;
	uint64_t price_per_unit;
	uint8_t vat_rate;
	int32_t pay_agent;
} article_t;

extern list_t articles;

extern int articles_destroy();

#endif // ARTICLE_H
