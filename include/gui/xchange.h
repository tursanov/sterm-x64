/* ��ᬮ�� ������ � "������" �� TCP/IP. (c) gsr 2004 */

#ifndef GUI_XCHANGE_H
#define GUI_XCHANGE_H

/* ��᫮ ��ப �� �࠭� */
#define SCR_LINES		20
/* ��᫮ �⮫�殢 �� �࠭� */
#define SCR_COLS		80
/* ��᫮ ����⮢ � ����� ��ப� �� �࠭� */
#define SCR_LINE_ELEMS		16
#define SCR_DASH_POS		8
/* ��᫮ ��ப � ��ࠬ����� ����� */
#define NR_HEADER_LINES		1
#define NR_FOOTER_LINES		1
/* ���⮢�� �奬� */
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
