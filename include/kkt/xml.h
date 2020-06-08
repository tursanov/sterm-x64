/* ������ XML ��� ���. (c) gsr 2018 */

#if !defined KKT_XML_H
#define KKT_XML_H

#include "sysdefs.h"

#if defined __cplusplus
extern "C" {
#endif

/* ���� ᮡ�⨩ �� ࠧ��� XML ��� ��� */
#define KKT_XML_BEGIN		0	/* ��砫� XML */
#define KKT_XML_TAG		1	/* ���� ⥣ */
#define KKT_XML_ETAG		2	/* ����砭�� ⥣� */
#define KKT_XML_ATTR		3	/* ��ਡ�� ⥣� */
#define KKT_XML_END		4	/* ����砭�� XML */

typedef int (*kkt_xml_callback_t)(uint32_t check, int evt, const char *name, const char *val);

extern int kkt_xml_callback(uint32_t check, int evt, const char *name, const char *val);

extern uint8_t *parse_kkt_xml(uint8_t *data, uint32_t check, kkt_xml_callback_t cbk, int *ecode);

#if defined __cplusplus
}
#endif

#endif		/* KKT_XML_H */
