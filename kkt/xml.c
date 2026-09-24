/* Разбор XML для ККТ. (c) gsr 2024, 2026 */

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <unistd.h>
#include <string.h>
#include "kkt/xml.h"
#include "express.h"
#include "genfunc.h"
#include "termlog.h"

bool enable_log_kkt_xml = false;

static bool log_kkt_xml(const char *text_buf);


static inline int call_xml_cbk(kkt_xml_callback_t cbk, bool check, int evt,
	const char *name, const char *val)
{
	return (cbk == NULL) ? E_OK : cbk(check, evt, name, val);
}

static bool handle_xml_node(xmlNode *first, bool check, kkt_xml_callback_t cbk, int *ecode)
{
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
	
    if (!check && enable_log_kkt_xml)
      log_kkt_xml(data);
	
	
	xmlDoc *xml = xmlReadMemory(data, strlen(data), NULL, NULL, XML_PARSE_COMPACT);
	if (xml == NULL)
		ret = false;
	else{
		ret = handle_xml(xml, check, cbk, ecode);
		if (check && !ret)
			log_err("Ошибка %d.", *ecode);
	}
	return ret;
}


static bool log_kkt_xml(const char *text_buf)
{
    /* Получаем текущее время (секунды + наносекунды) */
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        log_sys_err("clock_gettime:");
        return false;
    }

    struct tm tm_info;
    if (localtime_r(&ts.tv_sec, &tm_info) == NULL) {
        log_sys_err("localtime_r:");
        return false;
    }

    /* Миллисекунды (3 цифры) */
    int msec = (int)(ts.tv_nsec / 1000000);

    /* Формируем имя файла: request-YYYY-MM-DD-HH-SS.mmm.xml */
    char filename[64];
    int n = snprintf(filename, sizeof(filename),
                     "/home/sterm/request-%04d-%02d-%02d-%02d-%02d.%03d.xml",
                     tm_info.tm_year + 1900,
                     tm_info.tm_mon + 1,
                     tm_info.tm_mday,
                     tm_info.tm_hour,
                     tm_info.tm_sec,
                     msec);
    if (n < 0 || n >= (int)sizeof(filename)) {
        log_err("filename too long.");
        return false;
    }

    /* Открываем файл на запись (текстовый режим) */
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("fopen");
        return false;
    }

    /* Пишем содержимое text_buf */
    if (fputs(text_buf, fp) == EOF) {
        perror("fputs");
        fclose(fp);
        return false;
    }

    /* Закрываем файл */
    if (fclose(fp) != 0) {
        perror("fclose");
        return false;
    }
    
    sync();

    return true;
}
