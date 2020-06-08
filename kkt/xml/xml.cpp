/* Разбор XML для ККТ. (c) gsr 2018 */

#include <stdio.h>
#include <string.h>
#include "kkt/xml.h"
#include "express.h"
#include "genfunc.h"
#include "rapidxml.hpp"

using namespace rapidxml;

static xml_document<uint8_t> xml;

static inline int call_xml_cbk(kkt_xml_callback_t cbk, bool check, int evt,
	const uint8_t *name = NULL, const uint8_t *val = NULL)
{
	return (cbk == NULL) ? E_OK : cbk(check, evt,
		reinterpret_cast<const char *>(name), reinterpret_cast<const char *>(val));
}

static uint8_t *handle_xml_node(xml_node<uint8_t> *node, bool check,
	kkt_xml_callback_t cbk, int *ecode)
{
	uint8_t *ret = node->name();
	*ecode = E_OK;
	int rc = call_xml_cbk(cbk, check, KKT_XML_TAG, node->name());
	if (check && (rc != E_OK)){
		*ecode = rc;
		return ret;
	}
	for (xml_attribute<uint8_t> *attr = node->first_attribute(); attr != NULL; attr = attr->next_attribute()){
		recode_str(reinterpret_cast<char *>(attr->value()), -1);
		rc = call_xml_cbk(cbk, check, KKT_XML_ATTR, attr->name(), attr->value());
		if (check && (rc != E_OK)){
			*ecode = rc;
			ret = attr->name();
			break;
		}
	}
	if (check && (*ecode != E_OK))
		return ret;
	for (xml_node<uint8_t> *inode = node->first_node(); inode != NULL; inode = inode->next_sibling()){
		ret = handle_xml_node(inode, check, cbk, ecode);
		if (*ecode != E_OK)
			break;
	}
	if (check && (*ecode != E_OK))
		return ret;
	rc = call_xml_cbk(cbk, check, KKT_XML_ETAG, node->name());
	if (check && (rc != E_OK))
		*ecode = rc;
	return ret;
}

static uint8_t *handle_xml(const xml_document<uint8_t> &xml, bool check,
	kkt_xml_callback_t cbk, int *ecode)
{
	uint8_t *ret = NULL;
	int rc = call_xml_cbk(cbk, check, KKT_XML_BEGIN);
	if (check && (rc != E_OK)){
		*ecode = rc;
		return ret;
	}
	for (xml_node<uint8_t> *node = xml.first_node(); node != NULL; node = node->next_sibling()){
		if (node->type() == node_element){
			ret = handle_xml_node(node, check, cbk, ecode);
			if (check && (*ecode != E_OK))
				break;
		}
	}
	if (check && (*ecode != E_OK))
		return ret;
	rc = call_xml_cbk(cbk, check, KKT_XML_END);
	if (check && (rc != E_OK))
		*ecode = rc;
	return ret;
}

uint8_t *parse_kkt_xml(uint8_t *data, uint32_t check, kkt_xml_callback_t cbk, int *ecode)
{
	uint8_t *ret = data;
	*ecode = E_OK;
	try {
		xml.parse<parse_validate_closing_tags | parse_trim_whitespace>(data);
		ret = handle_xml(xml, check, cbk, ecode);
		if (ret == NULL)
			ret = data;
		if (*ecode != E_OK){
			fprintf(stderr, "%s: ошибка %d\n", __func__, *ecode);
			goto exit;
		}
	} catch (const parse_error &pe){
		*ecode = E_KKT_XML;
		ret = pe.where<uint8_t>();
		fprintf(stderr, "%s: %s %s\n", __func__, pe.what(), ret);
	}
exit:
	return ret;
}

