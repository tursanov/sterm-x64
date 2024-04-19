/* ��騥 �㭪樨 ��� ࠡ��� � ����஫�묨 ���⠬�. (c) gsr 2009 */

#if !defined GUI_LOG_GENERIC_H
#define GUI_LOG_GENERIC_H

#if defined __cplusplus
extern "C" {
#endif

#include "gui/gdi.h"
#include "log/generic.h"
#include "blimits.h"
#include "genfunc.h"
#include "kbd.h"

/* ������� ��� �⮡ࠦ���� �� �� �࠭� */
struct log_gui_context {
	struct log_handle *hlog;
	bool *active;		/* 䫠� ��⨢���� ���� ��ᬮ�� */
	const char *title;	/* ��������� ���� �� */
	uint32_t cur_rec_index;	/* ����� (�� �����䨪���) ⥪�饩 ����� �� */
	uint32_t nr_head_lines;	/* �᫮ ��ப � ��������� ����� */
	uint32_t nr_hint_lines;	/* ������⢮ ��ப ���᪠��� */
/*
 * ���� ��� �뢮�� ����� �� �� �࠭. ����� �࣠�������� � ���� ��ப �
 * �������騬 �㫥�. � ��砫� ������ ��ப� ���� ���� �⮫�� ��ࢮ�� ᨬ����.
 * �᫨ ���訩 ��� ᨬ���� ��⠭����� � 1, ᨬ��� �뢮����� � �����᭮� ����.
 */
	uint8_t *scr_data;
	uint32_t scr_data_size;	/* ࠧ��� �࠭���� ���� */
	uint32_t scr_data_len;	/* ����� ������ � �࠭��� ���� */
	uint32_t scr_data_lines;/* �᫮ ��ப � �࠭��� ���� */
	uint32_t first_line;	/* ����� ��ࢮ� �뢮����� ��ப� */
	GCPtr gc;
	GCPtr mem_gc;
	bool modal;
	bool asis;		/* �����뢠�� ᨬ���� ��� ��४���஢�� */
/* ������� ���樠������ �������� */
	void (*init)(struct log_gui_context *ctx);
/* ����� �� �����뢠�� ������ */
	bool (*filter)(struct log_handle *hlog, uint32_t index);
/* �⥭�� ����� �� ������� */
	bool (*read_rec)(struct log_handle *hlog, uint32_t index);
/* ����祭�� �������� ��ப� ��������� */
	const char *(*get_head_line)(uint32_t n);
/* ����祭�� ��ப� ���᪠��� */
	const char *(*get_hint_line)(uint32_t n);
/* ���������� �࠭���� ���� */
	void (*fill_scr_data)(struct log_gui_context *ctx);
/* ��ࠡ��稪 ᮡ�⨩ ���������� */
	int  (*handle_kbd)(struct log_gui_context *ctx, const struct kbd_event *e);
};

/*
 * ������ ��㯯� ����ᥩ ����஫쭮� �����.
 * �ᯮ������ ��� ����ண� ��६�饭��.
 */
#define LOG_GROUP_LEN		10
#define LOG_BGROUP_LEN		100

/* �ᯮ������ ��� �ᮢ���� */
#define LOG_SCREEN_LINES	22
#define LOG_SCREEN_COLS		80

/* ���⪠ �࠭���� ���� */
extern void log_clear_ctx(struct log_gui_context *ctx);
/* ��।������ ����� ������� ��� ���� ��2 */
extern int  log_get_cmd_len(const uint8_t *data, uint32_t len, int index);
/* ������ � �࠭�� ���� �������� ��ப� */
extern void log_fill_scr_str(struct log_gui_context *ctx,
	const char *format, ...) __attribute__((format (printf, 2, 3)));
/*
 * ���������� � �࠭�� ���� ��ப�. �����頥� false � ��砥 ��९�������
 * �࠭���� ����.
 */
extern bool log_add_scr_str(struct log_gui_context *ctx, bool need_recode,
	const char *format, ...) __attribute__((format (printf, 3, 4)));
/* ��ᮢ���� ����஫쭮� ����� */
extern bool log_draw(struct log_gui_context *ctx);
/* ����ᮢ�� �ᥣ� �࠭� */
extern void log_redraw(struct log_gui_context *ctx);
/* ���� ����� ����஫쭮� ����� �� �� ������ */
extern bool log_show_rec_by_number(struct log_gui_context *ctx, uint32_t number);
/* ���� ����� ����஫쭮� ����� �� ��� ᮧ����� */
extern bool log_show_rec_by_date(struct log_gui_context *ctx,
		struct date_time *dt);
/* ��ࠡ��稪 ᮡ�⨩ ���������� */
extern int  log_process(struct log_gui_context *ctx, struct kbd_event *e);
/* ���樠������ ����䥩� ���짮��⥫� */
extern bool log_init_view(struct log_gui_context *ctx);
/* �᢮�������� ����䥩� ���짮��⥫� */
extern void log_release_view(struct log_gui_context *ctx);

#if defined __cplusplus
}
#endif

#endif		/* GUI_LOG_GENERIC_H */
