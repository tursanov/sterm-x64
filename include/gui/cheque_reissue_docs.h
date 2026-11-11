#if !defined GUI_CHEQUE_REISSUE_DOCS_H
#define GUI_CHEQUE_REISSUE_DOCS_H

#include "kkt/fd/ad.h"

__BEGIN_DECLS

extern int cheque_reissue_docs_init(void);
extern void cheque_reissue_docs_release(void);
extern int cheque_reissue_docs_draw(void);
extern void cheque_reissue_docs_execute(void);

__END_DECLS

#endif		/* GUI_CHEQUE_REISSUE_DOCS_H */
