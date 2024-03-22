/* Разбор XML для ККТ. (c) gsr 2018 */

#if !defined KKT_XML_H
#define KKT_XML_H

#include "sysdefs.h"

/* Типы событий при разборе XML для ККТ */
#define KKT_XML_BEGIN		0	/* начало XML */
#define KKT_XML_TAG		1	/* новый тег */
#define KKT_XML_ETAG		2	/* окончание тега */
#define KKT_XML_ATTR		3	/* атрибут тега */
#define KKT_XML_END		4	/* окончание XML */

typedef int (*kkt_xml_callback_t)(bool check, int evt, const char *name, const char *val);
extern int kkt_xml_callback(bool check, int evt, const char *name, const char *val);

extern bool parse_kkt_xml(const char *data, bool check, kkt_xml_callback_t cbk, int *ecode);

#endif		/* KKT_XML_H */
