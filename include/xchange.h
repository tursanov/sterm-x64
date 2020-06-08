/* Журнал обмена данными. (c) gsr 2004 */

#if !defined XCHANGE_H
#define XCHANGE_H

#include "genfunc.h"

/* Максимальная длина блока данных в журнале */
#define XLOG_MAX_DATA_LEN	65536
/* Максимальное число элементов в журнале */
#define XLOG_NR_ITEMS		64
/* Направление передачи данных (относительно терминала) */
enum {
	xlog_in,
	xlog_out,
};
/* Элемент данных в журнале */
struct xlog_item {
	struct date_time dt;	/* дата/время создания */
	int dir;		/* направление */
	int len;		/* длина данных */
	uint8_t *data;		/* данные */
	uint16_t Ntz;		/* ZNтз/ONтз */
	uint16_t Npo;		/* ZNпо/ONпо */
	uint16_t Bp;		/* Zбп/Oбп */
};

extern bool xlog_add_item(uint8_t *data, int len, int dir);
extern int xlog_count_items(void);
extern struct xlog_item *xlog_get_item(int index);

#endif		/* XCHANGE_H */
