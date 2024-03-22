/* Разбор XML для ККТ. (c) gsr 2024 */

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include "kkt/xml.h"
#include "express.h"
#include "genfunc.h"

static inline int call_xml_cbk(kkt_xml_callback_t cbk, bool check, int evt,
	const char *name, const char *val)
{
	printf("%s: check = %d; evt = %d; name = %s; val = %s\n", __func__,
		check, evt, name ? name : "NULL", val ? val : "NULL");
	return (cbk == NULL) ? E_OK : cbk(check, evt, name, val);
}

static bool handle_xml_node(xmlNode *first, bool check, kkt_xml_callback_t cbk, int *ecode)
{
	printf("%s (%p): first = %p; check = %d; cbk = %p; ecode = %p\n", __func__,
		handle_xml_node, first, check, cbk, ecode);
	bool ret = true;
	*ecode = E_OK;
	for (xmlNode *node = first; node != NULL; node = node->next){
		if (node->type != XML_ELEMENT_NODE)
			continue;
		int rc = call_xml_cbk(cbk, check, KKT_XML_TAG, (const char *)node->name, NULL);
		if (check && (rc != E_OK)){
			*ecode = rc;
			break;
		}
		for (xmlAttr *attr = node->properties; attr != NULL; attr = attr->next){
			if ((attr->children == NULL) || (attr->children->type != XML_TEXT_NODE) ||
					(attr->children->content == NULL))
				continue;
			char buf[256];
			snprintf(buf, ASIZE(buf), "%s", attr->children->content);
			recode_str(buf, -1);
			rc = call_xml_cbk(cbk, check, KKT_XML_ATTR, (const char *)attr->name, buf);
			if (check && (rc != E_OK)){
				*ecode = rc;
				ret = false;
				break;
			}
		}
		if (check && (*ecode != E_OK))
			break;
		if (node->children != NULL)
			ret = handle_xml_node(node->children, check, cbk, ecode);
		if (check && !ret)
			break;
		rc = call_xml_cbk(cbk, check, KKT_XML_ETAG, (const char *)node->name, NULL);
		if (check && (rc != E_OK)){
			*ecode = rc;
			break;
		}
	}
	return ret;
}

static bool handle_xml(xmlDoc *xml, bool check, kkt_xml_callback_t cbk, int *ecode)
{
	printf("%s (%p): xml = %p; check = %d; cbk = %p; ecode = %p\n", __func__,
		handle_xml, xml, check, cbk, ecode);
	*ecode = E_OK;
	int rc = call_xml_cbk(cbk, check, KKT_XML_BEGIN, NULL, NULL);
	if (check && (rc != E_OK)){
		*ecode = rc;
		return false;
	}
	bool ret = handle_xml_node(xmlDocGetRootElement(xml), check, cbk, ecode);
	if (check && !ret)
		return ret;
	rc = call_xml_cbk(cbk, check, KKT_XML_END, NULL, NULL);
	if (check && (rc != E_OK)){
		*ecode = rc;
		ret = false;
	}
	return ret;
}

bool parse_kkt_xml(const char *data, bool check, kkt_xml_callback_t cbk, int *ecode)
{
	bool ret = true;
	*ecode = E_OK;
	xmlDoc *xml = xmlReadMemory(data, strlen(data), NULL, NULL, XML_PARSE_COMPACT);
	if (xml == NULL)
		ret = false;
	else{
		ret = handle_xml(xml, check, cbk, ecode);
		if (check && !ret)
			fprintf(stderr, "%s: ошибка %d\n", __func__, *ecode);
	}
	return ret;
}

