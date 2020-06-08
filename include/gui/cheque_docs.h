#ifndef CHEQUE_DOCS_H
#define CHEQUE_DOCS_H

#include "kkt/fd/ad.h"

int cheque_docs_init(void);
void cheque_docs_release(void);
int cheque_docs_draw(void);
void cheque_docs_execute(void);

#endif // CHEQUE_DOCS_H
