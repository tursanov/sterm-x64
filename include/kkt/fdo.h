/* Ž¡¬¥­ ¤ ­­ë¬¨ á Ž”„. (c) gsr 2018 */

#if !defined KKT_FDO_H
#define KKT_FDO_H

#include "sysdefs.h"

__BEGIN_DECLS

extern bool fdo_init(void);
extern void fdo_release(void);

extern bool fdo_lock(void);
extern bool fdo_unlock(void);

extern bool fdo_suspend(void);
extern bool fdo_resume(void);

__END_DECLS

#endif		/* KKT_FDO_H */
