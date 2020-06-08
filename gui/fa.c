#include <sys/socket.h>
#include <sys/param.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include "sysdefs.h"
#include "gui/dialog.h"
#include "gui/exgdi.h"
#include "gui/fa.h"
#include "gui/menu.h"
#include "gui/scr.h"
#include "gui/forms.h"
#include "gui/cheque.h"
#include "gui/cheque_docs.h"
#include "gui/cheque_reissue_docs.h"
#include "gui/lvform.h"
#include "gui/newcheque.h"
#include "gui/archivefn.h"
#include "kkt/fd/fd.h"
#include "kkt/kkt.h"
#include "kkt/fdo.h"
#include "ds1990a.h"
#include "kkt/fd/tlv.h"

static int fa_active_group = -1;
static int fa_active_item = -1;
//static int fa_active_control = -1;
int fa_cm = cmd_none;
static struct menu *fa_menu = NULL;
static struct menu *fa_sales_menu = NULL;
int fa_arg = cmd_fa;
static bool fs_debug = false;

static bool process_fa_cmd(int cmd);

static char cashier_name[64+1] = {0};
static char cashier_post[64+1] = {0};
static char cashier_inn[12+1] = {0};
static char cashier_cashier[64+1] = {0};

static void make_cashier() {
	size_t cashier_name_size = strlen(cashier_name);
	size_t cashier_post_size = strlen(cashier_post);
	size_t cashier_cashier_size = cashier_name_size;

	memcpy(cashier_cashier, cashier_name, cashier_name_size);
	if (cashier_post_size > 0 && cashier_name_size < 63) {
		size_t l = 64 - cashier_name_size - 1;
		l = MIN(l, cashier_post_size);

		cashier_cashier[cashier_name_size] = ' ';
		memcpy(cashier_cashier + cashier_name_size + 1, cashier_post, l);
		cashier_cashier_size += l + 1;
	}
	cashier_cashier[cashier_cashier_size] = 0;
}

static void cashier_data_read(FILE *f, char *s, size_t max_size) {
	char *end = s + max_size - 1;
	while (true) {
		int ch = fgetc(f);
		if (ch == -1 || ch == '\n')
			break;
		if (s < end)
			*s++ = (char)ch;
	}
	*s = 0;
}

bool cashier_load() {
	FILE *f = fopen("/home/sterm/cashier.txt", "r");
	if (f != NULL) {
		for (int i = 0; i < 3; i++) {
			switch (i) {
				case 0:
					cashier_data_read(f, cashier_name, sizeof(cashier_name));
					break;
				case 1:
					cashier_data_read(f, cashier_post, sizeof(cashier_post));
					break;
				case 2:
					cashier_data_read(f, cashier_inn, sizeof(cashier_inn));
					break;
			}
		}

		fclose(f);

		make_cashier();

		printf("cashier_name: \"%s\"\n", cashier_name);
		printf("cashier_post: \"%s\"\n", cashier_post);
		printf("cashier_inn: \"%s\"\n", cashier_inn);
		printf("cashier_cashier: \"%s\"\n", cashier_cashier);

		return 0;
	}
	return -1;
}

bool cashier_save() {
	FILE *f = fopen("/home/sterm/cashier.txt", "w");
	if (f != NULL) {

		printf("cashier_save\n");
		printf(" cashier_name: \"%s\"\n", cashier_name);
		printf(" cashier_post: \"%s\"\n", cashier_post);
		printf(" cashier_inn: \"%s\"\n", cashier_inn);
		printf(" cashier_cashier: \"%s\"\n", cashier_cashier);

		fprintf(f, "%s\n%s\n%s\n%s\n",
			(const char *)cashier_name,
			(const char *)cashier_post,
			(const char *)cashier_inn,
			(const char *)cashier_cashier);
		fclose(f);
		return true;
	}
	return false;
}


bool cashier_set(const char *name, const char *post, const char *inn) {
	strncpy(cashier_name, name, sizeof(cashier_name) - 1);
	cashier_name[sizeof(cashier_name) - 1] = 0;

	strncpy(cashier_post, post, sizeof(cashier_post) - 1);
	cashier_post[sizeof(cashier_post) - 1] = 0;

	strncpy(cashier_inn, inn, sizeof(cashier_inn) - 1);
	cashier_inn[sizeof(cashier_inn) - 1] = 0;

	make_cashier();
	return cashier_save();
}

bool cashier_set_name(const char *name) {
	if (strcmp(name, cashier_name) != 0) {
		strncpy(cashier_name, name, sizeof(cashier_name) - 1);
		cashier_name[sizeof(cashier_name) - 1] = 0;
		cashier_post[0] = 0;
		cashier_inn[0] = 0;
		strcpy(cashier_cashier, cashier_name);
		return cashier_save();
	}
	return false;
}

const char *cashier_get_name() {
	return cashier_name;
}

const char *cashier_get_post() {
	return cashier_post;
}

const char *cashier_get_inn() {
	return cashier_inn;
}

const char *cashier_get_cashier() {
	return cashier_cashier;
}

/* ��㯯� ��ࠬ��஢ ����ன�� */
enum {
	FAPP_GROUP_MENU = -1,
	FAPP_GROUP_SALES_MENU = 0,
};

/* ����� � ���� ����஥� */
static bool fa_create_menu(void)
{
	fa_menu = new_menu(false, false);
	if (kt != key_reg) {
		add_menu_item(fa_menu, new_menu_item("���������", cmd_reg_fa, true));
		add_menu_item(fa_menu, new_menu_item("���ॣ������", cmd_rereg_fa, true));
	}
	add_menu_item(fa_menu, new_menu_item("����⨥ ᬥ��", cmd_open_shift_fa, true));
	add_menu_item(fa_menu, new_menu_item("�����⨥ ᬥ��", cmd_close_shift_fa, true));
	add_menu_item(fa_menu, new_menu_item("���", cmd_cheque_fa, true));
	add_menu_item(fa_menu, new_menu_item("��� ���४樨", cmd_cheque_corr_fa, true));
	add_menu_item(fa_menu, new_menu_item("���� � ⥪��e� ���ﭨ� ���⮢", cmd_calc_report_fa, true));
	if (kt != key_reg) {
		add_menu_item(fa_menu, new_menu_item("�����⨥ ��", cmd_close_fs_fa, true));
		add_menu_item(fa_menu, new_menu_item("�������� ���㬥�� �� 祪�", cmd_del_doc_fa, true));
		add_menu_item(fa_menu, new_menu_item("���⨥ � ���㬥�� �⬥⪨ ��८�ଫ����", cmd_unmark_reissue_fa, true));
	}
	add_menu_item(fa_menu, new_menu_item("����� ��� ���饭�� � ��� \"������\"", cmd_sales_fa, true));
	add_menu_item(fa_menu, new_menu_item("����� ��᫥����� ��ନ஢������ ���㬥��", cmd_print_last_doc_fa, true));

	add_menu_item(fa_menu, new_menu_item("����� ���㬥�⮢ �� ��", cmd_archive_fa, true));
	if (kt != key_reg) {
		if (fs_debug)
			add_menu_item(fa_menu, new_menu_item("���� ��", cmd_reset_fs_fa, true));
	}
	add_menu_item(fa_menu, new_menu_item("��室", cmd_exit, true));


	fa_sales_menu = new_menu(false, false);
	add_menu_item(fa_sales_menu, new_menu_item("���(�)           ", cmd_newcheque_fa, true));
	if (kt != key_reg) {
		add_menu_item(fa_sales_menu, new_menu_item("��ࠢ�筨� ���⠢騪��", cmd_agents_fa, true));
		add_menu_item(fa_sales_menu, new_menu_item("��ࠢ�筨� ⮢�஢/ࠡ��/���", cmd_articles_fa, true));
	}

	return true;
}

static bool fa_set_group(int n)
{
	if (fa_arg != cmd_fa)
		return false;
	bool ret = false;
	fa_cm = cmd_none;
	fa_active_group = n;
	ClearScreen(clBlack);
	if (n == FAPP_GROUP_MENU){
		draw_menu(fa_menu);
		ret = true;
	} else if (n == FAPP_GROUP_SALES_MENU) {
		fa_sales_menu->selected = 0;
		draw_menu(fa_sales_menu);
		ret = true;
	}
	return ret;
}


static uint8_t rereg_data[2048];
static size_t rereg_data_len = sizeof(rereg_data);
static int64_t user_inn = 0;
uint8_t reg_tax_systems = 0;

static int fa_get_reregistration_data() {
	int ret;
	rereg_data_len = sizeof(rereg_data);
	if ((ret = kkt_get_last_reg_data(rereg_data, &rereg_data_len)) == 0 && rereg_data_len > 0) {
		for (const ffd_tlv_t *tlv = (ffd_tlv_t *)rereg_data,
				*end = (ffd_tlv_t *)(rereg_data + rereg_data_len);
				tlv < end;
				tlv = FFD_TLV_NEXT(tlv)) {
			switch (tlv->tag) {
				case 1018:
					user_inn = atoll(FFD_TLV_DATA_AS_STRING(tlv));
					break;
				case 1062:
					reg_tax_systems = FFD_TLV_DATA_AS_UINT8(tlv);
					break;
			}
		}
	}

	return ret;
}

static void fa_check_fn() {
	struct kkt_fs_version ver;
	fs_debug = kkt_get_fs_version(&ver) == 0 && ver.type == 0;
}

bool init_fa(int arg)
{
	fa_arg = arg;
	fa_active = true;
	hide_cursor();
	scr_visible = false;
	scr_visible = false;
	set_scr_mode(m80x20, true, false);
	set_term_busy(true);

	fdo_suspend();
	fa_check_fn();
	fa_get_reregistration_data();
	fdo_resume();

	if (arg == cmd_fa) {
		ClearScreen(clBlack);
		fa_create_menu();
		if (_ad->clist.count > 0) {
			fa_active_item =  kt != key_reg ? 4 : 2;
			fa_menu->selected = fa_active_item;
			printf("fa_active_item = %d\n", fa_active_item);
		}
		fa_set_group(FAPP_GROUP_MENU);
	} else {
		fa_menu = NULL;
		process_fa_cmd(arg);
	}

	return true;
}

form_t *reg_form = NULL;
form_t *rereg_form = NULL;
form_t *open_shift_form = NULL;
form_t *close_shift_form = NULL;
form_t *cheque_corr_form = NULL;
form_t *close_fs_form = NULL;

void release_fa(void)
{
	if (reg_form) {
		form_destroy(reg_form);
		reg_form = NULL;
	}
	if (rereg_form) {
		form_destroy(rereg_form);
		rereg_form = NULL;
	}
	if (open_shift_form) {
		form_destroy(open_shift_form);
		open_shift_form = NULL;
	}
	if (close_shift_form) {
		form_destroy(close_shift_form);
		close_shift_form = NULL;
	}
	if (cheque_corr_form) {
		form_destroy(cheque_corr_form);
		cheque_corr_form = NULL;
	}
	if (close_fs_form) {
		form_destroy(close_fs_form);
		close_fs_form = NULL;
	}
	cheque_release();
	cheque_docs_release();
	if (fa_menu) {
		release_menu(fa_menu,false);
		fa_menu = NULL;
	}
	if (fa_sales_menu) {
		release_menu(fa_sales_menu,false);
		fa_sales_menu = NULL;
	}
	online = true;
	pop_term_info();
	ClearScreen(clBtnFace);
	fa_active = false;
}

bool draw_fa(void)
{
	if (fa_active_group == FAPP_GROUP_MENU)
		return draw_menu(fa_menu);
	else if (fa_active_group == FAPP_GROUP_SALES_MENU)
		return draw_menu(fa_sales_menu);
    return true;
}

static void fa_show_error(form_t *form, uint16_t tag, const char *text) {
	message_box("�訡��", text, dlg_yes, 0, al_center);
	form_focus(form, tag);
	form_draw(form);
}

static bool fa_check_field(form_t *form, form_data_t *data, uint16_t tag, const char *error_text) {
	if (data->size == 0) {
		fa_show_error(form, tag, error_text);
		return false;
	}
	return true;
}

static bool fa_check_inn_field(form_t *form, form_data_t *data, uint16_t tag, const char *error_text) {
	if (data->size != 10 && data->size != 12) {
		fa_show_error(form, tag, error_text);
		return false;
	}
	return true;
}

static int fa_get_string(form_t *form, form_data_t *data, uint16_t tag, bool required) {
	if (!form_get_data(form, tag, 1, data)) {
		fa_show_error(form, tag, "������ ������ ��");
		return -1;
	}

	if (data->size == 0) {
		if (required) {
			fa_show_error(form, tag, "��易⥫쭮� ���� �� ���������");
			return -2;
		}
	}
	return 0;
}

static int fa_tlv_add_string(form_t *form, uint16_t tag, bool required) {
	int ret;
	form_data_t data;

	if ((ret = fa_get_string(form, &data, tag, required)) != 0)
		return ret;

	if (data.size == 0)
		return 0;

	printf("tlv_add_string(%.4d, %s)\n", tag, (const char *)data.data);

	if ((ret = ffd_tlv_add_string(tag, (const char *)data.data)) != 0) {
		fa_show_error(form, tag, "�訡�� �� ���������� TLV. ������� � ࠧࠡ��稪��");
		return ret;
	}
	return 0;
}

static int fa_tlv_add_fixed_string(form_t *form, uint16_t tag, size_t fixed_length, bool required) {
	int ret;
	form_data_t data;

	if ((ret = fa_get_string(form, &data, tag, required)) != 0)
		return ret;
		
	if (data.size == 0)
		return 0;
		
	printf("tlv_add_fixed_string(%.4d, %s, %zd)\n", tag, (const char *)data.data, fixed_length);

	if ((ret = ffd_tlv_add_fixed_string(tag, (const char *)data.data, fixed_length)) != 0) {
		fa_show_error(form, tag, "�訡�� �� ���������� TLV. ������� � ࠧࠡ��稪��");
		return ret;
	}
	return 0;
}

static uint64_t form_data_to_vln(form_data_t *data) {
	double v = atof(data->data);
	uint64_t value = (uint64_t)((v + 0.009) * 100.0);
/*	
	uint64_t d = 1;
	
	const char *text = (const char *)data->data + data->size - 1;
	for (int i = 0; i < data->size; i++, text--, d = d * 10) {
		if (isdigit(*text))
			value += (*text - '0') * d;
		else if (*text == '.' || *text == ',') {
		}
	}*/
	return value;
}

static int fa_tlv_add_vln_ex(form_t *form, uint16_t tag, bool required,
		uint32_t *v, uint8_t bit) {
	form_data_t data;

	if (!form_get_data(form, tag, 1, &data)) {
		fa_show_error(form, tag, "������ ������ ��");
		return -1;
	}

	if (data.size == 0) {
		if (required) {
			fa_show_error(form, tag, "��易⥫쭮� ���� �� ���������");
			return -2;
		}
		return 0;
	}

	uint64_t value = form_data_to_vln(&data);

	int ret;
	if ((ret = ffd_tlv_add_vln(tag, value)) != 0) {
		fa_show_error(form, tag, "���ࠢ��쭮� ���祭��");
		return ret;
	}

	if (v != NULL)
		*v |= 1 << bit;

	return 0;
}

static int fa_tlv_add_vln(form_t *form, uint16_t tag, bool required) {
	return fa_tlv_add_vln_ex(form, tag, required, NULL, 0);
}


static int fa_tlv_add_unixtime(form_t *form, uint16_t tag, bool required) {
	form_data_t data;

	if (!form_get_data(form, tag, 1, &data)) {
		fa_show_error(form, tag, "������ ������ ��");
		return -1;
	}

	if (data.size == 0) {
		if (required) {
			fa_show_error(form, tag, "��易⥫쭮� ���� �� ���������");
			return -2;
		}
		return 0;
	}

	printf("data.data: %s\n", (const char *)data.data);

	struct tm tm;

	memset(&tm, 0, sizeof(tm));

	char *s;
	if ((s = strptime((const char *)data.data, "%d.%m.%Y", &tm)) == NULL || *s != 0) {
		fa_show_error(form, tag, "���ࠢ��쭮� ���祭��");
		return -1;
	}

	time_t value = timegm(&tm);
	if (value == 0 || value == -1) {
		fa_show_error(form, tag, "���ࠢ��쭮� ���祭��. ��ଠ� �����: ��.��.����");
		return -1;
	}

	printf("time: %ld\n", value);

	int ret;
	if ((ret = ffd_tlv_add_unix_time(tag, value)) != 0) {
		fa_show_error(form, tag, "�訡�� ���������� TLV");
		return ret;
	}
	return 0;
}

static int fa_tlv_add_cashier(form_t *form) {
	form_data_t cashier;
	form_data_t post;
	form_data_t inn;

	form_get_data(form, 1021, 1, &cashier);
	form_get_data(form, 9999, 1, &post);
	form_get_data(form, 1203, 1, &inn);

	if (cashier.size == 0) {
		fa_show_error(form, 1021, "��易⥫쭮� ���� \"�����\" �� ���������");
		return -1;
	}

	printf("fa_tlv_add_cashier\n");
		printf(" cashier_name: \"%s\"\n", (char *)cashier.data);
		printf(" cashier_post: \"%s\"\n", (char *)post.data);
		printf(" cashier_inn: \"%s\"\n", (char *)inn.data);

	if (!cashier_set(cashier.data, post.data, inn.data)) {
		fa_show_error(form, 1021, "�訡�� ����� � 䠩� ������ � �����");
		return -1;
	}

	if (inn.size > 0) {
		int ret;
		if ((ret = ffd_tlv_add_fixed_string(1203, (const char *)cashier_inn, 12)) != 0) {
			fa_show_error(form, 1203, "�訡�� �� ���������� TLV. ������� � ࠧࠡ��稪��");
			return ret;
		}
	}
	return ffd_tlv_add_string(1021, cashier_cashier);
}

bool fa_create_doc(uint16_t doc_type, const uint8_t *pattern_footer,
		size_t pattern_footer_size, 
		update_screen_func_t update_func, void *update_func_arg) 
{
	uint8_t status;

	fdo_suspend();
	if ((status = fd_create_doc(doc_type, pattern_footer, pattern_footer_size)) != 0) {
		if (status == 0x46) {
			struct kkt_last_doc_info ldi;
			uint8_t err_info[32];
			size_t err_info_len;

			//printf("#1 %d\n", doc_type);
LCheckLastDocNo:
			err_info_len = sizeof(err_info);
			status = kkt_get_last_doc_info(&ldi, err_info, &err_info_len);
			if (status != 0) {
				//printf("#2: %d\n", status);
				fd_set_error(doc_type, status, err_info, err_info_len);
			} else {
				//printf("#3: %d, %d\n", ldi.last_nr, ldi.last_printed_nr);
				if (ldi.last_nr != ldi.last_printed_nr) {
					message_box("�訡��", "��᫥���� ��ନ஢���� ���㬥�� �� �� �����⠭.\n"
							"��� ��� ���� ��⠢�� �㬠�� � ��� � ������ Enter",
							dlg_yes, 0, al_center);
					if (update_func)
						update_func(update_func_arg);
					status = fd_print_last_doc(ldi.last_type);

					//printf("LD: status = %d\n", status);

					if (status != 0)
						fd_set_error(doc_type, status, err_info, err_info_len);
				}
			}
		}

		fdo_resume();

		if (status != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("�訡��", error, dlg_yes, 0, al_center);
			if (update_func)
				update_func(update_func_arg);
		} else
			return true;

		printf("status: %.2X\n", status);

		if (status == 0x41 || status == 0x42 || status == 0x44)
			goto LCheckLastDocNo;

		return false;
	}
	return true;
}

static void update_form(void *form) {
	if (form)
		form_draw((form_t *)form);
}

static void update_cheque(void *arg) {
	kbd_flush_queue();
	cheque_draw();
}

static void fa_set_cashier_info(form_t * form) {
	if (form != NULL) {
		form_set_data(form, 1021, 0, cashier_name, strlen(cashier_name));
		form_set_data(form, 9999, 0, cashier_post, strlen(cashier_post));
		form_set_data(form, 1203, 0, cashier_inn, strlen(cashier_inn));
	}
}

void fa_open_shift() {
	fa_set_cashier_info(open_shift_form);
	BEGIN_FORM(open_shift_form, "����⨥ ᬥ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", cashier_name, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", cashier_post, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", cashier_inn, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����")
		FORM_ITEM_BUTTON(0, "�⬥��")
	END_FORM()
	form_t *form = open_shift_form;

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fa_tlv_add_cashier(form) != 0)
			continue;

		if (fa_create_doc(OPEN_SHIFT, NULL, 0, update_form, form))
			break;
	}

	fa_set_group(FAPP_GROUP_MENU);
}

void fa_close_shift() {
	fa_set_cashier_info(close_shift_form);
	BEGIN_FORM(close_shift_form, "�����⨥ ᬥ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", cashier_name, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", cashier_post, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", cashier_inn, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����")
		FORM_ITEM_BUTTON(0, "�⬥��")
	END_FORM()
	form_t *form = close_shift_form;

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fa_tlv_add_cashier(form) != 0)
			continue;

		if (fa_create_doc(CLOSE_SHIFT, NULL, 0, update_form, form))
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_calc_report() {
	form_t *form = NULL;
	BEGIN_FORM(form, "���� � ⥪�饬 ���ﭨ� ���⮢")
		FORM_ITEM_BUTTON(1, "�����")
		FORM_ITEM_BUTTON(0, "�⬥��")
	END_FORM()

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fa_create_doc(CALC_REPORT, NULL, 0, update_form, form))
			break;
	}
	form_destroy(form);
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_close_fs() {
	fa_set_cashier_info(close_fs_form);
	BEGIN_FORM(close_fs_form, "�����⨥ ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", cashier_name, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", cashier_post, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", cashier_inn, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����")
		FORM_ITEM_BUTTON(0, "�⬥��")
	END_FORM()
	form_t *form = close_fs_form;

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fa_tlv_add_cashier(form) != 0)
			continue;
		if (fa_create_doc(CLOSE_FS, NULL, 0, update_form, form))
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

#include "references/agent.c"
#include "references/article.c"

const char *str_tax_systems[] = { "���", "��� �����", "��� �����-������", "����", "����", "������" };
size_t str_tax_system_count = ASIZE(str_tax_systems);
const char *str_short_kkt_modes[] = { "���", "�����.�.", "�������.�.",
		"������", "���", "��������" };
const char *str_kkt_modes[] = { "���஢����", "��⮭���� ०��", "��⮬���᪨� ०��",
		"�ਬ������ � ��� ���", "����� ���", "�ਬ������ � ���୥�" };
size_t str_kkt_mode_count = ASIZE(str_kkt_modes);

static int fa_fill_registration_tlv(form_t *form) {
	ffd_tlv_reset();

	int tax_systems = form_get_int_data(form, 1062, 0, 0);
	int reg_kkt_modes = form_get_int_data(form, 9998, 0, 0);

	if (fa_tlv_add_cashier(form) != 0 ||
		fa_tlv_add_string(form, 1048, true) != 0 ||
		fa_tlv_add_fixed_string(form, 1018, 12, true) != 0 ||
		fa_tlv_add_string(form, 1009, false) != 0 ||
		fa_tlv_add_string(form, 1187, false) != 0 ||
		ffd_tlv_add_uint8(1062, (uint8_t)tax_systems) != 0 ||
		fa_tlv_add_fixed_string(form, 1037, 20, true) != 0 ||
		fa_tlv_add_string(form, 1036, (reg_kkt_modes & REG_MODE_AUTOMAT) != 0) != 0 ||
//		fa_tlv_add_string(form, 1021, true) != 0 ||
//		fa_tlv_add_fixed_string(form, 1203, 12, false) != 0 ||
		fa_tlv_add_string(form, 1060, (reg_kkt_modes & REG_MODE_OFFLINE) == 0) != 0 ||
		fa_tlv_add_string(form, 1117, (reg_kkt_modes & REG_MODE_OFFLINE) == 0) != 0 ||
		fa_tlv_add_string(form, 1046, (reg_kkt_modes & REG_MODE_OFFLINE) == 0) != 0 ||
		fa_tlv_add_fixed_string(form, 1017, 12, (reg_kkt_modes & REG_MODE_OFFLINE) == 0) != 0) {
		return -1;
	}

	uint16_t reg_mode_tags[] = { 1056, 1002, 1001, 1109, 1110, 1108 };
	for (int i = 0; i < ASIZE(reg_mode_tags); i++)
		if ((reg_kkt_modes & (1 << i)) != 0)
			ffd_tlv_add_uint8(reg_mode_tags[i], 1);
	return 0;
}

void fa_registration() {
	fa_set_cashier_info(reg_form);
	BEGIN_FORM(reg_form, "���������")
		FORM_ITEM_EDIT_TEXT(1048, "������������ ���짮��⥫�:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1018, "��� ���짮��⥫�:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1009, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1187, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_BITSET(1062, "���⥬� ���������������:", str_tax_systems, str_tax_systems, 
				str_tax_system_count, 0)
		FORM_ITEM_EDIT_TEXT(1037, "�������樮��� ����� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 16)
		FORM_ITEM_BITSET(9998, "������ ࠡ���:", str_short_kkt_modes, str_kkt_modes, 6, 0)
		FORM_ITEM_EDIT_TEXT(1036, "����� ��⮬��:", NULL, FORM_INPUT_TYPE_TEXT, 20)

		FORM_ITEM_EDIT_TEXT(1021, "�����:", cashier_name, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", cashier_post, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", cashier_inn, FORM_INPUT_TYPE_NUMBER, 12)

		FORM_ITEM_EDIT_TEXT(1060, "���� ᠩ� ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1117, "���� �. ����� ���. 祪�:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1046, "������������ ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1017, "��� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����")
		FORM_ITEM_BUTTON(0, "�⬥��")
	END_FORM()
	form_t *form = reg_form;

	while (form_execute(form) == 1) {
		if (fa_fill_registration_tlv(form) != 0)
			continue;

		if (fa_create_doc(REGISTRATION, NULL, 0, update_form, form))
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

static const char *short_rereg_reason[8] = { "������ ��", "������ ���", "���.����.",
	"���.�����.���", NULL,  NULL, NULL, NULL };
static const char *rereg_reason[8] = { "������ ��", "������ ���", "��������� ४����⮢",
	"��������� ����஥� ���", NULL,  NULL, NULL, NULL };

static size_t get_trim_string_size(const ffd_tlv_t *tlv) {
	size_t size = tlv->length;
	const char *s = FFD_TLV_DATA_AS_STRING(tlv) + size - 1;
	while (size != 0 && *s-- == ' ')
		size--;
	return size;
}

static int fa_fill_reregistration_form(form_t *form) {
	uint8_t modes = 0;

	if (rereg_data_len > 0) {
		for (const ffd_tlv_t *tlv = (ffd_tlv_t *)rereg_data,
				*end = (ffd_tlv_t *)(rereg_data + rereg_data_len);
				tlv < end;
				tlv = FFD_TLV_NEXT(tlv)) {
			switch (tlv->tag) {
			case 1048:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
				break;
			case 1018:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv),
					get_trim_string_size(tlv));
				break;
			case 1009:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
				break;
			case 1187:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
				break;
			case 1062:
				FORM_BITSET_SET_VALUE(form, tlv->tag, FFD_TLV_DATA_AS_UINT8(tlv));
				break;
			case 1037:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv),
					get_trim_string_size(tlv));
				break;
			case 1056:
				if (FFD_TLV_DATA_AS_UINT8(tlv))
					modes |= 0x01;
				break;
			case 1002:
				if (FFD_TLV_DATA_AS_UINT8(tlv))
					modes |= 0x02;
				break;
			case 1001:
				if (FFD_TLV_DATA_AS_UINT8(tlv))
					modes |= 0x04;
				break;
			case 1109:
				if (FFD_TLV_DATA_AS_UINT8(tlv))
					modes |= 0x08;
				break;
			case 1110:
				if (FFD_TLV_DATA_AS_UINT8(tlv))
					modes |= 0x10;
				break;
			case 1108:
				if (FFD_TLV_DATA_AS_UINT8(tlv))
					modes |= 0x20;
				break;
			case 1036:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
				break;
			case 1060:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
				break;
			case 1117:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
				break;
			case 1046:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
				break;
			case 1017:
				FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv),
					get_trim_string_size(tlv));
				break;
			}
		}
		FORM_BITSET_SET_VALUE(form, 9998, modes);
	}
	return 0;
}

void fa_reregistration() {
	bool empty = rereg_form == NULL;
	fa_set_cashier_info(rereg_form);
	BEGIN_FORM(rereg_form, "���ॣ������")
		FORM_ITEM_EDIT_TEXT(1048, "������������ ���짮��⥫�:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1018, "��� ���짮��⥫�:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1009, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1187, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_BITSET(1062, "���⥬� ���������������:", str_tax_systems, str_tax_systems, 
				str_tax_system_count, 0)
		FORM_ITEM_EDIT_TEXT(1037, "�������樮��� ����� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 16)
		FORM_ITEM_BITSET(9998, "������ ࠡ���:", str_short_kkt_modes, str_kkt_modes, 6, 0)
		FORM_ITEM_EDIT_TEXT(1036, "����� ��⮬��:", NULL, FORM_INPUT_TYPE_TEXT, 20)
		
		FORM_ITEM_EDIT_TEXT(1021, "�����:", cashier_name, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", cashier_post, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", cashier_inn, FORM_INPUT_TYPE_NUMBER, 12)
		
//		FORM_ITEM_EDIT_TEXT(1021, "�����:", _ad->p1 ? _ad->p1->c : NULL, FORM_INPUT_TYPE_TEXT, 64)
//		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1060, "���� ᠩ� ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1117, "���� �. ����� ���. 祪�:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1046, "������������ ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1017, "��� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BITSET(9997, "��稭� ���ॣ����樨", short_rereg_reason, rereg_reason, 4, 0)
		FORM_ITEM_BUTTON(1, "�����")
		FORM_ITEM_BUTTON(0, "�⬥��")
	END_FORM()
	form_t *form = rereg_form;

	if (empty)
		fa_fill_reregistration_form(form);

	while (form_execute(form) == 1) {
		if (fa_fill_registration_tlv(form) != 0) {
			printf("���ࠢ��쭮� ���������� �����\n");
			continue;
		}

		int rereg_reason = form_get_int_data(form, 9997, 0, 0);
		printf("rereg_reason = %d\n", rereg_reason);
		if (rereg_reason == 0) {
			message_box("�訡��", "�� 㪠���� �� ���� ��稭� ���ॣ����樨", dlg_yes, 0, al_center);
			form_focus(form, 9997);
			form_draw(form);
			continue;
		} else {
			for (int i = 0; i < 4; i++) {
				if ((rereg_reason & (1 << i)) != 0)
					ffd_tlv_add_uint8(1101, (uint8_t)(i + 1));
			}
		}

		if (fa_create_doc(RE_REGISTRATION, NULL, 0, update_form, form))
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

static int fa_get_int_field(form_t *form, uint16_t tag) {
	int ret = form_get_int_data(form, tag, 0, -1);
	if (ret == -1) {
		message_box("�訡��", "���祭�� ��易⥫쭮�� ���� �� 㪠����", dlg_yes, 0, al_center);
		form_focus(form, tag);
		form_draw(form);
		return -1;
	}
	return ret;
}

void fa_cheque_corr() {
	const char *str_pay_type[] = { "���४�� ��室�", "���४�� ��室�" };
	const char *str_tax_mode[] = { "���", "��� �����", "��� �����-������", "����", "����", "������" };
	const char *str_corr_type[] = { "�������⥫쭮", "�� �।��ᠭ��" };

	fa_set_cashier_info(cheque_corr_form);
	BEGIN_FORM(cheque_corr_form, "��� ���४樨")
		FORM_ITEM_COMBOBOX(1054, "�ਧ��� ����:", str_pay_type, ASIZE(str_pay_type), -1)
		FORM_ITEM_COMBOBOX(1055, "���⥬� ���������������:", str_tax_mode, ASIZE(str_tax_mode), -1)
		FORM_ITEM_COMBOBOX(1173, "��� ���४樨:", str_corr_type, ASIZE(str_corr_type), -1)

		FORM_ITEM_EDIT_TEXT(1021, "�����:", cashier_name, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", cashier_post, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", cashier_inn, FORM_INPUT_TYPE_NUMBER, 12)

		FORM_ITEM_EDIT_TEXT(1177, "���ᠭ�� ���४樨:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1178, "��� ���४樨:", NULL, FORM_INPUT_TYPE_DATE, 10)
		FORM_ITEM_EDIT_TEXT(1179, "����� �।��ᠭ��:", NULL, FORM_INPUT_TYPE_TEXT, 32)

		FORM_ITEM_EDIT_TEXT(1031, "�����묨:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1081, "��������묨:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1215, "�।����⮩:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1216, "���⮯��⮩:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1217, "������ �।��⠢������:", "0", FORM_INPUT_TYPE_MONEY, 16)

		FORM_ITEM_EDIT_TEXT(1102, "��� 20%:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1103, "��� 10%:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1104, "��� 0%:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1105, "��� ���:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1106, "��� 20/120:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1107, "��� 10/110:", NULL, FORM_INPUT_TYPE_MONEY, 16)

		FORM_ITEM_BUTTON(1, "�����")
		FORM_ITEM_BUTTON(0, "�⬥��")
	END_FORM()
	form_t *form = cheque_corr_form;

	while (form_execute(form) == 1) {
		ffd_tlv_reset();

		int pay_type;
		int tax_system;
		int corr_type;

		if ((pay_type = fa_get_int_field(form, 1054)) < 0 ||
			(tax_system = fa_get_int_field(form, 1055)) < 0 ||
			(corr_type = fa_get_int_field(form, 1173)) < 0)
			continue;

		if (ffd_tlv_add_uint8(1054, (pay_type == 0) ? 1 : 3) != 0 ||
			ffd_tlv_add_uint8(1055, (1 << tax_system)) != 0 ||
			ffd_tlv_add_uint8(1173, corr_type) != 0) 
			continue;
		if (ffd_tlv_stlv_begin(1174, 292) != 0 ||
			fa_tlv_add_string(form, 1177, false) != 0 ||
			fa_tlv_add_unixtime(form, 1178, true) != 0 ||
			fa_tlv_add_string(form, 1179, true) != 0 ||
			ffd_tlv_stlv_end() != 0)
			continue;

		uint32_t vat_flags = 0;
		if (fa_tlv_add_vln(form, 1031, true) != 0 ||
			fa_tlv_add_vln(form, 1081, true) != 0 ||
			fa_tlv_add_vln(form, 1215, true) != 0 ||
			fa_tlv_add_vln(form, 1216, true) != 0 ||
			fa_tlv_add_vln(form, 1217, true) != 0 ||

			fa_tlv_add_vln_ex(form, 1102, false, &vat_flags, 0) != 0 ||
			fa_tlv_add_vln_ex(form, 1103, false, &vat_flags, 1) != 0 ||
			fa_tlv_add_vln_ex(form, 1104, false, &vat_flags, 2) != 0 ||
			fa_tlv_add_vln_ex(form, 1105, false, &vat_flags, 3) != 0 ||
			fa_tlv_add_vln_ex(form, 1106, false, &vat_flags, 4) != 0 ||
			fa_tlv_add_vln_ex(form, 1107, false, &vat_flags, 5) != 0)
			continue;

		if (vat_flags == 0) {
			fa_show_error(form, 1102, "��� ������� ���㬥�� ������ ���� ��������� ��� �� ���� ���� � ���");
			continue;
		}

		if (fa_tlv_add_cashier(form) != 0)
			continue;

		if (fa_create_doc(CHEQUE_CORR, NULL, 0, update_form, form))
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

static size_t get_phone(char *src, char *dst) {
	if (src == NULL)
		return 0;
	char ch = *src;
	size_t len = 0;
	if (ch == '8') {
		*dst++ = '+';
		*dst++ = '7';
		len += 2;
		src++;
	} else if (isdigit(ch)) {
		*dst++ = '+';
		len++;
	}

	while (*src) {
		*dst++ = *src++;
		len++;
	}
	*dst = 0;

	return len;
}

void fa_cheque() {
//	bool changed = false;
	bool have_unformed_docs = false;
	bool result;

	cheque_init();

	while ((result = cheque_execute())) {
		bool has_errors = false;
		list_item_t *li = _ad->clist.head;
		have_unformed_docs = false;

		while (li) {
			C *c = LIST_ITEM(li, C);
			li = li->next;

//			bool have_u = false;
			size_t doc_count = 0;

			ffd_tlv_reset();

			ffd_tlv_add_string(1021, cashier_cashier);
			if (cashier_inn[0])
				ffd_tlv_add_fixed_string(1203, cashier_inn, 12);
			ffd_tlv_add_uint8(1054, c->t1054);
			ffd_tlv_add_uint8(1055, c->t1055);
			if (c->pe)
				ffd_tlv_add_string(1008, c->pe);
			ffd_tlv_add_vln(1031, (uint64_t)c->sum.n);
			ffd_tlv_add_vln(1081, (uint64_t)c->sum.e);
			ffd_tlv_add_vln(1215, (uint64_t)c->sum.p);
			ffd_tlv_add_vln(1216, 0);
			ffd_tlv_add_vln(1217, (uint64_t)c->sum.b);

			if (c->p != user_inn) {
				ffd_tlv_add_uint8(1057, 1 << 6);
				char phone[19+1];
				/*size_t size =*/ get_phone(c->h, phone);
				ffd_tlv_add_string(1171, phone);
			}

			if (_ad->t1086 != NULL) {
				ffd_tlv_stlv_begin(1084, 320);
				ffd_tlv_add_string(1085, "��������");
				ffd_tlv_add_string(1086, _ad->t1086);
				ffd_tlv_stlv_end();
			}

			for (list_item_t *i1 = c->klist.head; i1; i1 = i1->next) {
				K *k = LIST_ITEM(i1, K);
				if (doc_no_is_not_empty(&k->u)) {
//					have_u = true;
					have_unformed_docs = true;
				} else {
					for (list_item_t *i2 = k->llist.head; i2; i2 = i2->next) {
						L *l = LIST_ITEM(i2, L);
						ffd_tlv_stlv_begin(1059, 1024);
						ffd_tlv_add_uint8(1214, l->r);
						char s1030[256];
						sprintf(s1030, "%s\n\r���㬥�� \xfc%s", l->s, k->b.s ? k->b.s : "");
						ffd_tlv_add_string(1030, s1030);
						ffd_tlv_add_vln(1079, l->t);
						ffd_tlv_add_fvln(1023, 1, 0);
						if (l->n > 0)
							ffd_tlv_add_uint8(1199, l->n);
						if (l->n >= 1 && l->n <= 4) {
							printf("ADD 1198, %lld\n", (long long)l->c);
							ffd_tlv_add_vln(1198, l->c);
							printf("ADD 1200, %lld\n", (long long)l->c);
							ffd_tlv_add_vln(1200, l->c);
						}
						if (c->p != user_inn) {
							char inn[12+1];
							if (c->p > 9999999999ll)
								sprintf(inn, "%.12lld", (long long)c->p);
							else
								sprintf(inn, "%.10lld", (long long)c->p);
							ffd_tlv_add_fixed_string(1226, inn, 12);
						}
						ffd_tlv_stlv_end();
					}
					doc_count++;
				}
			}

			if (doc_count > 0) {
				uint8_t* pattern_footer = NULL;
				size_t pattern_footer_size = 0;

				cheque_begin_op("���� ����� 祪�...");
				if (fa_create_doc(CHEQUE, pattern_footer, pattern_footer_size, update_cheque, NULL)) {
					AD_remove_C(c);
					cheque_sync_first();
					cheque_draw();
//					changed = true;
				} else {
					has_errors = true;
					break;
				}
			}
		}

		if (!has_errors)
			break;
	}

	AD_calc_sum();

	if (result && have_unformed_docs)
		message_box("�����������", "��������! ������� ���㬥���, ��� ������\n"
				"�� �����襭� ��८�ଫ����", dlg_yes, 0, al_center);

	fa_set_group(FAPP_GROUP_MENU);

	kbd_flush_queue();
}

void fa_del_doc() {
	cheque_docs_init();
	cheque_docs_execute();

	fa_set_group(FAPP_GROUP_MENU);
}

void fa_unmark_reissue_doc() {
	cheque_reissue_docs_init();
	cheque_reissue_docs_execute();

	fa_set_group(FAPP_GROUP_MENU);
}

void fa_reset_fs() {
	if (message_box("�����������", "���⮢� �� �㤥� २��樠����஢��. �த������?",
				dlg_yes_no, 0, al_center) == DLG_BTN_YES) {
		ClearScreen(clBlack);
		draw_menu(fa_menu);
				
		kbd_flush_queue();
		uint8_t ret = kkt_reset_fs(0x16);
		
		char buffer[512];
		
		if (ret == 0)
			sprintf(buffer, "������ �ᯥ譮 �����襭�. �஢���� ��楤��� ���ॣ����樨");
		else
			sprintf(buffer, "������ �����襭� � �訡��� (%.2x). ��१���㧨� ��� � ���஡�� �� ࠧ.",
				ret);
		
		message_box("�����������", buffer, dlg_yes, 0, al_center);
		ClearScreen(clBlack);
		draw_menu(fa_menu);
	}
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_print_last_doc() {
	struct kkt_last_doc_info ldi;
	uint8_t err_info[32];
	size_t err_info_len = sizeof(err_info);

	fdo_suspend();
	uint8_t status = kkt_get_last_doc_info(&ldi, err_info, &err_info_len);
	fdo_resume();
	printf("status = 0x%x\n", status);
	if (status != 0) {
		fd_set_error(0, status, err_info, err_info_len);
	} else if (ldi.last_type != 0) {
		if (message_box("�����������",
			"�㤥� �����⠭ ��᫥���� ��ନ஢���� ���㬥��.\n"
			"�த������?",
					dlg_yes_no, 0, al_center) == DLG_BTN_YES) {

			ClearScreen(clBlack);
			draw_menu(fa_menu);

			status = fd_print_last_doc(ldi.last_type);

			if (status != 0) {
				fd_set_error(ldi.last_type, status, err_info, err_info_len);
			}
		}
	} else
		message_box("�����������", "��� ��ନ஢����� ���㬥�⮢ ��� ����",
				dlg_yes, 0, al_center);

	if (status != 0) {
		const char *error;
		fd_get_last_error(&error);
		message_box("�訡��", error, dlg_yes, 0, al_center);
	}

	fa_set_group(FAPP_GROUP_MENU);
}

static void fa_newcheque() {
	newcheque_execute();

	ClearScreen(clBlack);
	draw_menu(fa_sales_menu);
}

static void fa_archive() {
	archivefn_execute();

	fa_set_group(FAPP_GROUP_MENU);
}

static bool process_fa_sales_cmd(int cmd) {
	bool ret = true;
	switch (cmd){
		case cmd_newcheque_fa:
			fa_newcheque();
			break;
		case cmd_agents_fa:
			fa_agents();
			break;
		case cmd_articles_fa:
			fa_articles();
			break;
		default:
			ret = false;
			break;
	}
	return ret;
}

static bool process_fa_cmd(int cmd) {
	bool ret = true;
	switch (cmd){
		case cmd_reg_fa:
			fa_registration();
			break;
		case cmd_rereg_fa:
			fa_reregistration();
			break;
		case cmd_open_shift_fa:
			fa_open_shift();
			break;
		case cmd_close_shift_fa:
			fa_close_shift();
			break;
		case cmd_calc_report_fa:
			fa_calc_report();
			break;
		case cmd_close_fs_fa:
			fa_close_fs();
			break;
		case cmd_cheque_corr_fa:
			fa_cheque_corr();
			break;
		case cmd_cheque_fa:
			fa_cheque();
			break;
		case cmd_del_doc_fa:
			fa_del_doc();
			break;
		case cmd_unmark_reissue_fa:
			fa_unmark_reissue_doc();
			break;
		case cmd_reset_fs_fa:
			fa_reset_fs();
			break;
		case cmd_print_last_doc_fa:
			fa_print_last_doc();
			break;
		case cmd_sales_fa:
			fa_set_group(FAPP_GROUP_SALES_MENU);
			break;
		case cmd_archive_fa:
			fa_archive();
			break;
		default:
			ret = false;
			break;
	}
	return ret;
}


bool process_fa(const struct kbd_event *e)
{
	if (fa_arg != cmd_fa)
		return false;
	if (!e->pressed)
		return true;
	if (fa_active_group == FAPP_GROUP_MENU) {
		if (process_menu(fa_menu, (struct kbd_event *)e) != cmd_none)
			return process_fa_cmd(get_menu_command(fa_menu));
    } else if (fa_active_group == FAPP_GROUP_SALES_MENU) {
		if (process_menu(fa_sales_menu, (struct kbd_event *)e) != cmd_none)
			if (!process_fa_sales_cmd(get_menu_command(fa_sales_menu)))
				fa_set_group(FAPP_GROUP_MENU);
	}
	return true;
}
