#ifndef CHEQUE_REISSUE_DOCS_H
#define CHEQUE_REISSUE_DOCS_H

#if defined __cplusplus
extern "C" {
#endif

#include "kkt/fd/ad.h"

int cheque_reissue_docs_init(void);
void cheque_reissue_docs_release(void);
int cheque_reissue_docs_draw(void);
void cheque_reissue_docs_execute(void);

#endif // CHEQUE_REISSUE_DOCS_H
