/* Работа с VipNet Client (Инфотекс). (c) gsr 2003-2020 */

#if !defined IPLIR_H
#define IPLIR_H

#if defined __cplusplus
extern "C" {
#endif

#include <netinet/in.h>
#include "sysdefs.h"

#define IPLIR_CTL	"/usr/bin/vipnetclient"

extern bool iplir_init(void);
extern void iplir_release(void);
extern bool iplir_install_keys(void);
extern bool iplir_delete_keys(void);
extern bool iplir_start(void);
extern bool iplir_stop(void);
extern bool iplir_is_active(void);

extern bool iplir_disabled;

extern bool iplir_test(void);

#if defined __cplusplus
}
#endif

#endif		/* IPLIR_H */
