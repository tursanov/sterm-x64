/* Просмотр обмена с "Экспресс" по TCP/IP. (c) gsr 2004 */

#ifndef GUI_XCHANGE_H
#define GUI_XCHANGE_H

/* Число строк на экране */
#define SCR_LINES		20
/* Число столбцов на экране */
#define SCR_COLS		80
/* Число элементов в одной строке на экране */
#define SCR_LINE_ELEMS		16
#define SCR_DASH_POS		8
/* Число строк в обрамлении записи */
#define NR_HEADER_LINES		1
#define NR_FOOTER_LINES		1
/* Цветовая схема */
#define XLOG_HEADER_COLOR	clLime
#define XLOG_DATA_COLOR		clYellow
#define XLOG_LAT_COLOR		clAqua
#define XLOG_RUS_COLOR		clYellow
#define XLOG_BG_COLOR		clNavy

#define XLOG_FONT		font80x20

extern void init_xchg_view(void);
extern void release_xchg_view(void);
extern bool draw_xchange(void);
extern void on_new_xchg_item(void);
extern int process_xchange(struct kbd_event *e);

#endif		/* GUI_XCHANGE_H */
