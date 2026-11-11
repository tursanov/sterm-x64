#if !defined GUI_REFERENCES_ARTICLE_H
#define GUI_REFERENCES_ARTICLE_H

#include "list.h"
#include "agent.h"

__BEGIN_DECLS

typedef struct {
	int n;
	char *name;
	uint8_t pay_method;
	uint64_t price_per_unit;
	uint8_t vat_rate;
	int32_t pay_agent;
} article_t;

extern list_t articles;

extern int articles_destroy(void);

__END_DECLS

#endif		/* GUI_REFERENCES_ARTICLE_H */
