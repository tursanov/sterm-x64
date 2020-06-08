#ifndef CHEQUE_H
#define CHEQUE_H

#include "kkt/fd/ad.h"

int cheque_init(void);
void cheque_release(void);
int cheque_draw(void);
bool cheque_execute(void);
void cheque_sync_first(void);
void cheque_begin_op(const char *title);
void cheque_end_op();

#endif // CHEQUE_H
