/* ��������� ����. (c) gsr, Alex Popov 2000-2004 */

#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"

/* ������ � ���� ������� */
enum {
	dlg_none = 0,
	dlg_yes,
	dlg_yes_no,
	dlg_custom,
};

/* ��ࠢ������� ⥪�� � ���� ������� */
enum {
	al_left,
	al_right,
	al_center,
};

/* �������, �����頥�� �� ����砭�� �����쭮�� ���ﭨ� ���� */
enum {
	DLG_BTN_YES = 0,
	DLG_BTN_NO,
	DLG_BTN_CANCEL,
	DLG_BTN_RETRY,
};

extern int message_box(const char *title, const char *message,
	intptr_t buttons, int active_btn, int align);
extern bool begin_message_box(const char *title, const char *message,
	int align);
extern bool end_message_box(void);

/*
 * message_box � ����⢥ ��ࠬ��� buttons ����� �ਭ����� 㪠��⥫� �� ���ᨢ
 * �������, ����뢠��� ᯥ樠��� ������.
 */
struct custom_btn {
	const char *text;	/* ⥪�� ������; ��� ��᫥����� ����� ���ᨢ� -- NULL */
	int cmd;		/* �������, �����頥��� �� ����⨨ �� ������ */
};

/*
 * ������� ��������� ����ᥩ ����஫쭮� �����.
 * min, max - �������� ��������, �������� ���짮��⥫��;
 * min_val, max_val - �࠭��� ���祭�� ���������.
 * �㭪�� �����頥� DLG_BTN_YES ��� DLG_BTN_NO.
 */
extern int log_range_dlg(uint32_t *min, uint32_t *max, uint32_t min_val, uint32_t max_val);
/* ���� ����� ����஫쭮� ����� �� ��� �� ᮧ����� */
extern int find_log_date_dlg(int *hour, int *day, int *mon, int *year);
/* ���� ����� ����஫쭮� ����� �� ������ */
extern int find_log_number_dlg(uint32_t *number, uint32_t min_number,
		uint32_t max_number);

#if defined __cplusplus
}
#endif

#endif		/* GUI_DIALOG_H */
