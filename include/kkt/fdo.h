/* ����� ����묨 � ���. (c) gsr 2018 */

#if !defined KKT_FDO_H
#define KKT_FDO_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

extern bool fdo_init(void);
extern void fdo_release(void);

extern bool fdo_lock(void);
extern bool fdo_unlock(void);

extern bool fdo_suspend(void);
extern bool fdo_resume(void);

#if defined __cplusplus
}
#endif

#endif		/* KKT_FDO_H */
