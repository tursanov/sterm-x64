#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include "list.h"
#include "sysdefs.h"
#include "kbd.h"
#include "kkt/fd/tlv.h"
#include "kkt/fd/ad.h"
#include "kkt/fd/fd.h"
#include "kkt/fd/tags.h"
#include "kkt/kkt.h"
#include "paths.h"
#include "serialize.h"
#include "gui/gdi.h"
#include "gui/controls/controls.h"
#include "gui/controls/window.h"
#include "gui/dialog.h"
#include "gui/archivefn.h"
#include "gui/fa.h"

#define GAP 10
#define YGAP	2
#define TEXT_START		GAP
#define	CONTROLS_START	300 
#define CONTROLS_TOP	30
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

static void text_free(char *text) {
	free(text);
}

static int op_kind;
static int res_out;
static uint32_t doc_no;
static list_t output = { NULL, NULL, 0, (list_item_delete_func_t)text_free };

static void archivefn_show_error_ex(const char *where) {
	char error_text[512];
	const char *error;
	fd_get_last_error(&error);
	snprintf(error_text, sizeof(error_text) - 1, "%s:\n%s", where, error_text);
	message_box("Ошибка", error_text, dlg_yes, 0, al_center);
}

static void archivefn_show_error(uint8_t status, const char *where) {
	fd_set_error(0, status, NULL, 0);
	archivefn_show_error_ex(where);
}

static void add_text_to_listbox(const char *text) {
	int count = 0;
	const char *start = text;
	size_t size = 0;
	int shift = 0;

	for (const char *s = text; *s && *s == ' '; s++, shift++);
	shift += 2;


#define MAX_OUT_CHARS	75
	for (const char *s = text; true; s++, size++) {
		if (size >= MAX_OUT_CHARS || !*s) {
			size_t l = size + (count ? shift : 0);
			char *txt = malloc(l + 1);
			char *p = txt;
			if (count) {
				for (int i = 0; i < shift; i++)
					*p++ = ' ';
			}

			memcpy(p, start, size);
			txt[l] = 0;
			list_add(&output, txt);

			if (!*s)
				break;

			size = 0;
			start = s;
			count++;
		}
	}
}

static void out_printf(const char *fmt, ...) {
	va_list args;
#define MAX_LEN	512
	char text[MAX_LEN];

	va_start(args, fmt);
	vsnprintf(text, MAX_LEN, fmt, args);
	va_end(args);

	add_text_to_listbox(text);
}

static const char *get_doc_name(uint32_t type) {
	switch (type) {
		case REGISTRATION:
			return "Отчет о регистрации";
		case RE_REGISTRATION:
			return "Отчет о перерегистрации";
			break;
		case OPEN_SHIFT:
			return "Отчет об открытии смены";
		case CLOSE_SHIFT:
			return "Отчет о закрытии смены";
		case CALC_REPORT:
			return "Отчет о текущем состоянии расчетов";
		case CHEQUE:
			return "Чек";
		case CHEQUE_CORR:
			return "Чек коррекции";
		case BSO:
			return "БСО";
		case BSO_CORR:
			return "БСО коррекции";
		case CLOSE_FS:
			return "Отчет о закрытии ФН";
		default:
			return NULL;
	}
}

struct kkt_report_hdr {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
} __attribute__((__packed__));

static void print_hdr(struct kkt_report_hdr *hdr) {
	out_printf(" Дата/время: %.2d.%.2d.%.4d %.2d:%.2d", 
			hdr->date_time[2], hdr->date_time[1], (int)hdr->date_time[0] + 2000,
			hdr->date_time[3], hdr->date_time[4]);
	out_printf(" Номер документа: %u", hdr->doc_nr);
	out_printf(" ФП: %u", hdr->fiscal_sign);
}

static const char *get_rereg_code(uint8_t code) {
	switch (code) {
		case 1:
			return "Замена ФН";
		case 2:
			return "Замена ОФД";
		case 3:
			return "Изменение реквизитов";
		case 4:
			return "Изменение настроек ККТ";
		default:
			return "Неизвестная причина";
	}
}

static const char *get_pay_type(uint8_t type) {
	switch (type) {
		case 1:
			return "Приход";
		case 2:
			return "Возврат прихода";
		case 3:
			return "Расход";
		case 4:
			return "Возврат расхода";
		default:
			return "Незвестный тип";
	}
}

const char *str_kkt_modes_ex[] = { 
	"ШФД", "АВТОН. Р.", "АВТОМАТ. Р.",
	"УСЛУГИ", "БСО", "ИНТЕРНЕТ",
	"ПЛ. АГ.", "БАНК. АГ."
};

static void print_reg(struct kkt_reregister_report *p, bool reg) {
	char tax_systems[256] = {0};
	char kkt_modes[256] = {0};

	out_printf(" ИНН: %.12s", p->inn);
	out_printf(" РНМ: %.20s", p->reg_number);

	char *s = tax_systems;
	for (int i = 0, n = 0; i < str_tax_system_count; i++) {
		if (p->tax_system & (1 << i))
			s += sprintf(s, "%s%s", n++ > 0 ? "," : "", str_tax_systems[i]);
	}
	out_printf(" Системы налогообложения: %s", tax_systems);

	s = kkt_modes;
	for (int i = 0, n = 0; i < ASIZE(str_kkt_modes_ex); i++) {
		if (p->mode & (1 << i))
			s += sprintf(s, "%s%s", n++ > 0 ? "," : "", str_kkt_modes_ex[i]);
	}
	out_printf(" Режимы работы: %s", kkt_modes);

	if (!reg)
		out_printf(" Код причины перерегистрации: %s", get_rereg_code(p->rereg_code));
}

static void print_shift(struct kkt_shift_report *p) {
	out_printf(" Номер смены: %d", p->shift_nr);
}

static void print_calc_report(struct kkt_calc_report *p) {
	out_printf(" Непереданных ФД: %u", p->nr_uncommited);
	out_printf(" ФД не переданы с: %.2d.%.2d.%4d",
			p->first_uncommited_dt[2],
			p->first_uncommited_dt[1],
			(int)p->first_uncommited_dt[0] + 2000);
}

static void print_cheque(struct kkt_cheque_report *p) {
	uint64_t v = 
		(uint64_t)(p->sum[0] << 0) |
		((uint64_t)p->sum[1] << 8) |
		((uint64_t)p->sum[2] << 16) |
		((uint64_t)p->sum[3] << 24) |
		((uint64_t)p->sum[4] << 32);
	out_printf(" Признак расчета: %s", get_pay_type(p->type));
	out_printf(" Цена: %.1lld.%.2lld", v / 100, v % 100);
}

static void print_close_fs(struct kkt_close_fs *p) {
	out_printf(" ИНН: %.12s", p->inn);
	out_printf(" Регистрационный номер: %.20s", p->reg_number);
}

static void print_fdo_ack(struct kkt_fs_fdo_ack *fdo_ack) {
	char str_fs[18*2 + 1];
	char *s;

	out_printf("Подтверждение оператора (Квитанция ОФД)");
	out_printf(" Номер док-та: %u\n", fdo_ack->doc_nr);
	out_printf(" Дата/время: %.2d.%.2d.%.2d %.2d:%.2d", 
			fdo_ack->dt.date.day,
			fdo_ack->dt.date.month,
			fdo_ack->dt.date.year,
			fdo_ack->dt.time.hour,
			fdo_ack->dt.time.minute);

	s = str_fs;
	for (int i = 0; i < 18; i++)
		s += sprintf(s, "%.2X", fdo_ack->fiscal_sign[i]);
	out_printf(" ФПД ОФД: %s", str_fs);
}

static bool archivefn_get_archive_doc() {
	struct kkt_fs_find_doc_info fdi;
	struct kkt_fs_fdo_ack fdo_ack;
	uint8_t data[512];
	size_t data_len = sizeof(data);
	uint8_t status;

   	if ((status = kkt_find_fs_doc(doc_no, res_out != 0, &fdi, data, &data_len)) != 0) {
		archivefn_show_error(status, "Ошибка при получении документа из архива ФН");
		return false;
	}

	if (fdi.fdo_ack) {
		if ((status = kkt_find_fdo_ack(doc_no, false, &fdo_ack)) != 0) {
			archivefn_show_error(status, "Ошибка при получении квитанции ОФД из архива ФН");
			return false;
		}
	}

	const char *doc_name = get_doc_name(fdi.doc_type);
	out_printf("%s", doc_name);
	print_hdr((struct kkt_report_hdr *)data);

	switch (fdi.doc_type) {
		case REGISTRATION:
		case RE_REGISTRATION:
			print_reg((struct kkt_reregister_report *)data, fdi.doc_type == REGISTRATION);
			break;
		case OPEN_SHIFT:
		case CLOSE_SHIFT:
			print_shift((struct kkt_shift_report *)data);
			break;
		case CALC_REPORT:
			print_calc_report((struct kkt_calc_report *)data);
			break;
		case CHEQUE:
		case CHEQUE_CORR:
		case BSO:
		case BSO_CORR:
			print_cheque((struct kkt_cheque_report *)data);
			break;
		case CLOSE_FS:
			print_close_fs((struct kkt_close_fs *)data);
			break;
	}

	if (fdi.fdo_ack)
		print_fdo_ack(&fdo_ack);
	else {
		out_printf("Подтверждение оператора (Квитанция ОФД)");
		out_printf(" Нет");
	}

	return true;
}

static void print_stlv(uint8_t *p, size_t size, int level) {
	char text[2048];
	char tmp[8] = {0};

	memset(tmp, ' ', level);
	tmp[level] = 0;

	size_t i = 0;
	while (i < size) {
		ffd_tlv_t *t = (ffd_tlv_t *)p;

		tag_type_t type = tags_get_tlv_text(t, text, sizeof(text));
		out_printf("%s%s", tmp, text);

		if (type == tag_type_stlv)
			print_stlv(p + 4, t->length, 1);

		size_t l = t->length + sizeof(*t);
		i += l;
		p += l;
	}
}

static bool archivefn_get_doc() {
	uint8_t status;
	uint16_t doc_type;
	size_t tlv_size;
	uint8_t *tlv;

	if ((status = kkt_get_doc_stlv(doc_no, &doc_type, &tlv_size)) != 0) {
		if (status == 0x08) {
			char text[128];
			sprintf(text, "Документ N%d отсутствует в ФН", doc_no);
			message_box("Ошибка", text, dlg_yes, 0, al_center);
		} else
			archivefn_show_error(status, "Ошибка при получении информации о документе");
		return false;
	}

	out_printf("Тип документа: %s, длина: %d", get_doc_name(doc_type), tlv_size);

	tlv = (uint8_t *)malloc(tlv_size);
	if (!tlv) {
		message_box("Ошибка", "Ошибка при выделении памяти", dlg_yes, 0, al_center);
		return false;
	}

	if ((status = kkt_read_doc_tlv(tlv, &tlv_size)) != 0) {
		archivefn_show_error(status, "Ошибка при чтении TLV из ФН");
	} else
		print_stlv(tlv, tlv_size, 0);

	free(tlv);


	if (res_out != 0) {
		if ((status = fd_print_doc(doc_type, doc_no) != 0)) {
			archivefn_show_error_ex("Ошибка при печати документа");
		}
	}

	return status == 0;
}

static bool archivefn_get_data() {
	list_clear(&output);
	if (op_kind == 0 || op_kind == 1)
		return archivefn_get_archive_doc();
	return archivefn_get_doc();
}

static void button_action(control_t *c, int cmd) {
	window_set_dialog_result(((window_t *)c->parent.parent), cmd);
}

static void listbox_get_item_text(void *obj, char *text, size_t text_size) {
	snprintf(text, text_size, "%s", (char *)obj);
}

int archivefn_execute() {
	window_t *win = window_create(NULL, 
			"Просмотр/печать документов из ФН (архива ФН) (Esc - выход)", NULL);
	GCPtr screen = window_get_gc(win);

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);
	int h = th + 8;

	const char *str_op_kind[] = {
		"Документ из архива ФН",
		"Подтверждение оператора (Квитанция ОФД)",	
		"Документ из ФН",
	};
	const char *str_res_out[] = {
		"На экран",
		"На экран и на печать"
	};
	char str_doc_no[16];

	if (doc_no != 0)
		sprintf(str_doc_no, "%d", doc_no);

	window_add_label(win, TEXT_START, y, align_left, "Вид операции:");
	window_add_control(win, 
			simple_combobox_create(9999, screen, x, y, w, h, 
				str_op_kind, ASIZE(str_op_kind), op_kind));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "Вывод результата:");
	window_add_control(win, 
			simple_combobox_create(9998, screen, x, y, w, h, 
				str_res_out, ASIZE(str_res_out), res_out));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "Номер документа:");
	window_add_control(win, 
			edit_create(1040, screen, x, y, w, h, doc_no == 0 ? NULL : str_doc_no,
				EDIT_INPUT_TYPE_TEXT, 10));
	y += h + YGAP + th + 12;

	window_add_label(win, TEXT_START, y - th - 4, align_left, "Данные документа:");

	window_add_control(win,
			listbox_create(9997, screen, GAP, y, DISCX - GAP*2, DISCY - y - BUTTON_HEIGHT - GAP*2,
				&output, listbox_get_item_text, 0));

	window_add_control(win, 
			button_create(1, screen, DISCX - (BUTTON_WIDTH + GAP)*2, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Выполнить", button_action));
	window_add_control(win, 
			button_create(0, screen, DISCX - (BUTTON_WIDTH + GAP)*1, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "Закрыть", button_action));

	int result;
	while (true) {
		result = window_show_dialog(win, -1);
		data_t data;

		op_kind = window_get_int_data(win, 9999, 0, 0);
		res_out = window_get_int_data(win, 9998, 0, 0);

		if (result == 0)
			break;

		window_get_data(win, 1040, 0, &data);

		if (data.size == 0) {
			window_show_error(win, 1040, "Поле \"Номер документа\" не заполнено");
			continue;
		}
		doc_no = atoi(data.data);
		if (doc_no == 0) {
			window_show_error(win, 1040, "Данное значение поля \"Номер документа\" недопустимо");
			continue;
		}

		window_set_data(win, 9997, 0, (const void *)-1, 0);

		if (!archivefn_get_data())
			continue;
		window_set_focus(win, 9997);
	}

	window_destroy(win);

	return result;
}
