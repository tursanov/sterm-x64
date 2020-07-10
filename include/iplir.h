/* ����� � VipNet Client (���⥪�). (c) gsr 2003-2020 */

#if !defined IPLIR_H
#define IPLIR_H

#include <netinet/in.h>
#include "sysdefs.h"

#define IPLIR_CTL	"/usr/bin/vipnetclient"

extern bool iplir_init(void);
extern void iplir_relase(void);
extern bool iplir_install_keys(void);
extern bool iplir_start(void);
extern bool iplir_stop(void);
extern bool iplir_is_active(void);

extern bool iplir_disabled;
extern bool has_iplir;

extern bool test_vipnet(void);

#endif		/* IPLIR_H */
