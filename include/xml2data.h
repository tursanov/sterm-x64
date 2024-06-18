/* Обработка XML внутри команды Ар2 R. (c) gsr 2024 */

#if !defined XML2DATA_H
#define XML2DATA_H

#include "blimits.h"

#if defined __cplusplus
extern "C" {
#endif

/* Преобразованные данные XML */
struct xml_data {
	uint8_t *scr_data;
	size_t scr_data_len;
	uint8_t *prn_data;
	size_t prn_data_len;
};

extern struct xml_data *get_xml_data(int n_para);
extern bool is_xml_data_valid(int n_para);
extern void clr_xml_data(void);
extern void shr_xml_data(void);

extern uint8_t *check_xml(uint8_t *p, size_t l, int dst, int *ecode, struct xml_data *xml_data);

#if defined __cplusplus
}
#endif

#endif		/* XML2DATA_H */
