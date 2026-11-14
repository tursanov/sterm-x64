#if !defined GUI_CHEQUE_H
#define GUI_CHEQUE_H

#include "kkt/fd/ad.h"

__BEGIN_DECLS

extern int cheque_init(void);
extern void cheque_release(void);
extern int cheque_draw(void);
extern bool cheque_execute(void);
extern void cheque_sync_first(void);
extern void cheque_begin_op(const char *title);
extern void cheque_end_op();

__END_DECLS

#endif		/* GUI_CHEQUE_H */
