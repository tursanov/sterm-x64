/* ����� � ��⮪�� POS-����� ��� �࠭�. (c) A.Popov, gsr 2004 */

#if !defined POS_SCREEN_H
#define POS_SCREEN_H

#include "gui/gdi.h"
#include "pos/pos.h"
#include "kbd.h"

typedef struct pos_node_t_ pos_node_t;
typedef struct pos_screen_t_ pos_screen_t;

/* ��� ����᪮�� ����� */
enum
{
	POS_TYPE_TEXT,	/* ����� */
	POS_TYPE_EDIT, 	/* ��ப� ����� */
	POS_TYPE_MENU 	/* ���� */
};

/* ���� */

/* �㭪�� ��� 㧫� */
typedef void (*pos_node_func_t)(pos_node_t *node);
/* �㭪�� ��� ��ࠡ�⪨ ᮡ�⨩ */
typedef void (*pos_node_process_func_t)(pos_node_t *node, struct kbd_event *e);

struct pos_node_t_
{
	pos_node_t *next; 	/* ������騩 ����� */
	int type;		/* ��� ����� */
	int x, y;		/* ���न���� �뢮�� (� ����������) */
	bool can_active;	/* ���� ��⨢���樨 ����� */
	bool update; 		/* ���� ����ᮢ�� */
	pos_node_func_t draw;	/* ���� ����ᮢ�� */
	pos_node_func_t activate;	/* ���� ��⨢���樨 */
	pos_node_func_t deactivate;	/* ���� ����⨢���樨 */
	pos_node_func_t free; 	/* �㭪�� 㤠����� ���. ������ */
	pos_node_process_func_t process;	/* ���� ��ࠡ�⪨ ᮡ�⨩ */
};

/* ����� */
typedef struct
{
	pos_node_t root;	/* ����祭�� pos_node */
	char *str;			/* ����� ��� �⮡ࠦ���� */
	uint8_t fg;			/* ���� ᨬ����� */
	uint8_t bg;			/* ���� 䮭� */
	uint8_t attr;			/* ���ਡ��� */
} pos_text_t;

/* ��ப� ����� */
typedef struct
{
	pos_node_t root;	/* ����祭�� pos_node */
	char *name;			/* ��� ��ப� ����� */
	char *text;			/* ����� */
	int count;			/* ������⢮ �����⨬�� ᨬ����� ��� ����� */
	int pos;			/* ��������� ����� */
	int view_start;		/* ��砫� �⮡ࠦ���� �� �஫����� */
	int view_width;		/* ��ਭ� �� �࠭� � ᨬ����� */
} pos_edit_t;

/* ���� */
typedef struct
{
	pos_node_t root;	/* ����祭�� pos_node */
	char *name;			/* ��� ���� */
	char **items;		/* �������� ���� */
	int count;			/* ������⢮ ����⮢ */
	int selected;		/* ��࠭�� ����� */
	int width;			/* ��ਭ� ���� */
	int height;			/* ���� ���� */
	int view_top;		/* ��砫� �⮡ࠦ���� */
	int view_width;		/* �⮡ࠦ����� �ਭ� */
	int view_height;	/* �⮡ࠦ����� ���� */
} pos_menu_t;

/* ��࠭ */
struct pos_screen_t_
{
	GCPtr gc;			/* ���⥪�� ��� �뢮�� */
	int cols;			/* ��ਭ� � ���������� */
	int rows;			/* ���� � ���������� */
	pos_node_t *head;		/* ��砫� ᯨ᪠ ����⮢ */
	pos_node_t *tail;		/* ����� ᯨ᪠ ����⮢ */
	pos_node_t *active;	 	/* ��⨢�� ����� */
	int x, y;			/* ����騥 ���न���� */
	uint8_t fg, bg;			/* ����騥 梥� �㪢 � 䮭� */
	bool update; 			/* ���� ����ᮢ�� */
	uint32_t blink_time;		/* �६� ��᫥����� ������� */
	bool blink_hide;		/* �����騥 ᨬ���� ������ ���� ����襭� */
	bool show_cursor;		/* �������� / ������ ����� */
	uint32_t cursor_time;		/* �६� ��᫥����� ������� ����� */
	bool cursor_on_screen;		/* ����� �� �࠭� */
	int cursor_x;			/* ���न��� �� x ����� */
	int cursor_y;			/* ���न��� �� y ����� */
	bool outside;			/* ���� ������������ ���������� ����� ����⮢ */
	BitmapPtr bmp_up;		/* ��५�� ����� */
	BitmapPtr bmp_down;		/* ��५�� ���� */
	int x_off, y_off;		/* ����� ��� ࠬ�� */
	int saved_x, saved_y;		/* ������ ����� ��� ����������� */
	bool saved_outside;		/* ���������� outside */
};

/* �������� �࠭� (x, y - ���न���� ���ᥫ��, cols, rows - � ����������,
 * x_off, y_off - ����� � ���ᥫ�� ��� ࠬ��, font - ����,
 * bmp_up, bmp_down - ��� ������ �����/���� � ���� */
extern bool pos_screen_create(int x, int y, int cols, int rows,
		int x_off, int y_off,
		FontPtr font, BitmapPtr bmp_up, BitmapPtr bmp_down);

/* ���࠭��� ⥪���� ������ ����� */
extern void pos_screen_save_pos(void);
/* ����⠭����� ��࠭����� ������ ����� */
extern void pos_screen_restore_pos(void);
/* �������� �࠭� */
extern bool pos_screen_destroy(void);
/* ����ᮢ�� �࠭� */
extern bool pos_screen_draw(void);
/* ��ࠡ�⪠ ᮡ�⨩ */
extern bool pos_screen_process(struct kbd_event *e);

/* ������� ��������� ����㠫쭮�� ����� */
extern bool pos_screen_cur(int x, int y);
/* ��������� 梥� �㪢 � 䮭� */
extern bool pos_screen_color(uint8_t fg, uint8_t bg);
/* ���⪠ �࠭� */
extern bool pos_screen_cls(void);
/* ��⨢�஢��� ᫥���騩 ����� */
extern bool pos_screen_activate_next(void);

/* ���������� ⥪�� */
extern bool pos_screen_insert_text(const char *text, uint8_t attr);

/* ���������� ��ப� ����� */
extern bool pos_screen_insert_edit(const char *name, const char *value, int width);

/* ���������� ���� */
extern bool pos_screen_insert_menu(const char *name, char **items,
		int item_count,	int width, int height);

/* ���� ������ ��� �࠭� */
#define POS_SCREEN_CUR			0x01
#define POS_SCREEN_CLS			0x02
#define POS_SCREEN_EDIT			0x03
#define POS_SCREEN_MENU			0x04
#define POS_SCREEN_PRINT		0x05
#define POS_SCREEN_COLOR		0x08

/* ���樠����஢�� �� �࠭ */
extern bool pos_screen_initialized(void);

/* ������ �⢥� �� POS-����� � �뢮��� �� �࠭ */
extern bool pos_parse_screen_stream(struct pos_data_buf *buf, bool check_only);

/* ��।������ ࠧ��� ��⮪� �� ���������� */
extern int pos_req_get_keyboard_len(void);
/* ������ ��⮪� �� ���������� */
extern bool pos_req_save_keyboard_stream(struct pos_data_buf *buf);
/* ������� �� �� �࠭� ��� �� ���� ���� */
extern bool pos_screen_has_menu(void);

/* �뢮� �� �࠭ ᮮ�饭�� � ������묨 ��ਡ�⠬� */
extern bool pos_write_scr(struct pos_data_buf *buf, char *msg,
		uint8_t fg, uint8_t bg);
/* ������ � ��⮪ ��� �࠭� ᮮ�饭�� �� �訡�� */
extern bool pos_save_err_msg(struct pos_data_buf *buf, char *msg);

#endif		/* POS_SCREEN_H */
