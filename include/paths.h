/* Размещение файлов терминала. (c) gsr 2018-2019 */

#if !defined PATHS_H
#define PATHS_H

#include "config.h"

/* Для удобства указания пути */
#define _(s)			STERM_HOME "/" s
/* Имя файла ключевой информации */
#define STERM_TKI_NAME		_("sterm.dat")
/* Имя файла банковской лицензии */
#define BANK_LICENSE		_("bank.dat")
/* Имя файла лицензии ППУ */
#define LPRN_LICENSE		_("lprn.dat")
/* Имя файла ЦКЛ */
#define STERM_XLOG_NAME		_("express.log")
/* Имя файла ЦКЛ в предыдущих версиях */
#define STERM_XLOG_OLD_NAME	_("sterm.log")
/* Имя файла БКЛ */
#define STERM_PLOG_NAME		_("pos.log")
/* Имя файла ПКЛ */
#define STERM_LLOG_NAME		_("local.log")
/* Имя файла ККЛ */
#define STERM_KLOG_NAME		_("kkt.log")
/* Имя файла конфигурации терминала */
#define STERM_CFG_NAME		_("sterm.conf")
/* Имя файла запросов */
#define STERM_REQ_LIST		_("sterm.req")
/* Имя файла сигнатуры дополнительных данных */
#define SIG_FILE_NAME		_("auxdata.sig")
/* Имя файла признака работы в пригородном режиме */
#define LOCAL_FLAG_NAME		_(".wm_local")

#endif		/* PATHS_H */
