#if !defined GUI_CHEQUE_DOCS_H
#define GUI_CHEQUE_DOCS_H

#include "kkt/fd/ad.h"

__BEGIN_DECLS

extern int cheque_docs_init(void);
extern void cheque_docs_release(void);
extern int cheque_docs_draw(void);
extern void cheque_docs_execute(void);

__END_DECLS

#endif		/* GUI_CHEQUE_DOCS_H */
