/* Работа с VipNet Client (Инфотекс). (c) gsr 2003-2020 */

#if !defined IPLIR_H
#define IPLIR_H

#include <netinet/in.h>
#include "sysdefs.h"

#define IPLIR_CTL	"/usr/bin/vipnetclient"

extern bool iplir_disabled;
extern bool has_iplir;

extern bool start_iplir(void);
extern bool stop_iplir(void);
extern bool is_iplir_loaded(void);

#endif		/* IPLIR_H */
