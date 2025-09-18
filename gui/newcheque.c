#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include "list.h"
#include "sysdefs.h"
#include "kbd.h"
#include "kkt/fd/fd.h"
#include "kkt/fd/tlv.h"
#include "kkt/fd/ad.h"
#include "kkt/kkt.h"
#include "paths.h"
#include "serialize.h"
#include "gui/gdi.h"
#include "gui/controls/controls.h"
#include "gui/controls/window.h"
#include "gui/dialog.h"
#include "gui/newcheque.h"
#include "gui/fa.h"
#include "gui/references/agent.h"
#include "gui/references/article.h"

#define GAP 10
#define YGAP	2
#define TEXT_START		GAP
#define	CONTROLS_START	300 
#define CONTROLS_TOP	30
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

typedef struct {
	article_t *article;
	agent_t *agent;
	ffd_tlv_t *tlv;
	const ffd_tlv_t *name;
	ffd_fvln_t count;
	uint64_t price_per_unit;
	uint64_t sum;
} cheque_article_t;

typedef struct {
	int tax_system;
	int pay_type;
	int pay_kind;
	char* phone_or_email;
	char* add_info;
	uint64_t sum;
	list_t articles;
	size_t agent_data_size;
	uint8_t *agent_data;
	uint64_t dsum[5];
	char* client;
	char* client_inn;
} newcheque_t;

extern uint8_t reg_tax_systems;
static const char * s_tax_systems[6];
static uint8_t b_tax_systems[6];
static int s_tax_system_count;
static bool support_1222_1224_1225;

static cheque_article_t *cheque_article_new() {
	cheque_article_t *ca =  (cheque_article_t *)malloc(sizeof(cheque_article_t));
	ca->tlv = NULL;
	return ca;
}

static void cheque_article_tlv_init(cheque_article_t *ca) {
	uint8_t t1214 = 0;
	int size = FFD_TLV_SIZE(ca->tlv);

	for (uint8_t *ptr = FFD_TLV_DATA(ca->tlv), *end = ptr + ca->tlv->length; ptr < end;) {
		const ffd_tlv_t *tlv = (ffd_tlv_t *)ptr;

		switch (tlv->tag) {
			case 1214:
				t1214 = FFD_TLV_DATA_AS_UINT8(tlv);
				break;
			case 1023:
				ffd_tlv_data_as_fvln(tlv, &ca->count);
				break;
			case 1030:
				ca->name = tlv;
				break;
			case 1079:
				ca->price_per_unit = ffd_tlv_data_as_vln(tlv);
				break;
			case 1043:
				ca->sum = ffd_tlv_data_as_vln(tlv);
				break;
		}

		ptr += FFD_TLV_SIZE(tlv);
	}

	if (t1214 == 3 && !ca->name) {
		const char *text = "АВАНСОВЫЙ ПРЕДМЕТ РАСЧЕТА";
		int text_len = strlen(text);
		size_t tlv_size = sizeof(ffd_tlv_t) + text_len;

		ca->tlv = (ffd_tlv_t *)realloc(ca->tlv, size + tlv_size);
		ffd_tlv_t *tlv = (ffd_tlv_t *)((uint8_t *)ca->tlv + size);
		
		tlv->tag = 1030;
		tlv->length = text_len;
		memcpy(FFD_TLV_DATA(tlv), text, text_len);
		ca->tlv->length += tlv_size;
		ca->name = tlv;
	}
}

static cheque_article_t *cheque_article_new_from_tlv(ffd_tlv_t *tlv) {
	cheque_article_t *ca =  (cheque_article_t *)malloc(sizeof(cheque_article_t));
	size_t size = FFD_TLV_SIZE(tlv);
	ca->tlv = (ffd_tlv_t *)malloc(size);
	ca->name = NULL;
	ca->article = NULL;
	memcpy(ca->tlv, tlv, size);
	cheque_article_tlv_init(ca);

	return ca;
}


static void cheque_article_free(cheque_article_t *ca) {
	if (ca->tlv) {
		free(ca->tlv);
	}
	free(ca);
}

static newcheque_t newcheque = {
	.pay_type = 1,
	.tax_system = 0,
	.pay_kind = 0,
	.phone_or_email = NULL,
	.add_info = NULL,
	.sum = 0,
	.articles = { NULL, NULL, 0, (list_item_delete_func_t)cheque_article_free },
	.agent_data_size = 0,
	.agent_data = NULL,
	.dsum = { 0, 0, 0, 0, 0 },
	.client = NULL,
	.client_inn = NULL
};

extern bool check_phone_or_email(const char *pe, bool allowNone);


/*static double cheque_article_sum(cheque_article_t *ca) {
	double count = ffd_fvln_to_double(&ca->count);
	double price_per_unit = (ca->article ? (double)ca->article->price_per_unit : 0) / 100.0;
	return (count * price_per_unit);
}*/

static uint64_t cheque_article_vln_sum(cheque_article_t *ca) {
//	printf("price_per_unit: %lld, count->value = %lld, count->dot = %d\n",
//			ca->article->price_per_unit, ca->count.value, ca->count.dot);

	uint64_t sum = ca->price_per_unit * ca->count.value;
	uint64_t rem = 0;
	for (int i = 0; i < ca->count.dot; i++) {
		rem = sum % 10;
		sum /= 10;
	}
	if (rem >= 5)
		sum++;
//	printf("sum: %lld\n", sum);
	return sum;
}

static int cheque_article_save(void *arg, cheque_article_t *a) {
	int fd = (int)(intptr_t)arg;
	int i = 0;
	for (list_item_t *li = articles.head; li; li = li->next, i++) {
		if (li->obj == a->article)
			break;
	}

	if (SAVE_INT(fd, i) ||
			save_data(fd, a->tlv, a->tlv ? FFD_TLV_SIZE(a->tlv) : 0) ||
			SAVE_INT(fd, a->count.value) ||
			SAVE_INT(fd, a->count.dot) ||
			SAVE_INT(fd, a->price_per_unit))
		return -1;
	return 0;
}

static cheque_article_t * cheque_article_load(int fd) {
	cheque_article_t *a = cheque_article_new();
	int article_index = -1;
	size_t tlv_size;

	if (LOAD_INT(fd, article_index) ||
			load_data(fd, (void **)&a->tlv, &tlv_size) ||
			LOAD_INT(fd, a->count.value) ||
			LOAD_INT(fd, a->count.dot) ||
			LOAD_INT(fd, a->price_per_unit)) {
		free(a);
		return NULL;
	}

	if (a->tlv == NULL) {
		a->article = NULL;
		int i = 0;
		for (list_item_t *li = articles.head; li; li = li->next, i++) {
			if (i == article_index) {
				a->article = LIST_ITEM(li, article_t);
				break;
			}
		}

		if (a->article == NULL) {
			free(a);
			return NULL;
		}
		a->sum = cheque_article_vln_sum(a);
	} else
		cheque_article_tlv_init(a);

	newcheque.sum += a->sum;

	return a;
}

#define FILE_NAME	"/home/sterm/newcheque.dat"
static bool newcheque_save() {
	bool ret = false;
	int fd = s_open(FILE_NAME, true);
	if (fd != -1) {
		ret = SAVE_INT(fd, newcheque.pay_type) == 0 &&
			SAVE_INT(fd, newcheque.pay_kind) == 0 &&
			save_string(fd, newcheque.add_info) == 0 &&
			save_list(fd, &newcheque.articles, (list_item_func_t)cheque_article_save) == 0 &&
			save_data(fd, newcheque.agent_data, newcheque.agent_data_size) == 0 &&
			SAVE_INT(fd, newcheque.dsum[0]) &&
			SAVE_INT(fd, newcheque.dsum[1]) &&
			SAVE_INT(fd, newcheque.dsum[2]) &&
			SAVE_INT(fd, newcheque.dsum[3]) &&
			SAVE_INT(fd, newcheque.dsum[4]) &&
			save_string(fd, newcheque.client) == 0 &&
			save_string(fd, newcheque.client_inn) == 0;

		s_close(fd);
	}

	return ret;
}

bool newcheque_load() {
	bool ret = false;
	int fd = s_open(FILE_NAME, false);
	if (fd != -1) {
		ret = LOAD_INT(fd, newcheque.pay_type) == 0 &&
			LOAD_INT(fd, newcheque.pay_kind) == 0 &&
			load_string(fd, &newcheque.add_info) == 0 &&
			load_list(fd, &newcheque.articles, (load_item_func_t)cheque_article_load) == 0 &&
			load_data(fd, (void **)&newcheque.agent_data, &newcheque.agent_data_size) == 0 &&
			LOAD_INT(fd, newcheque.dsum[0]) == 0 &&
			LOAD_INT(fd, newcheque.dsum[1]) == 0 &&
			LOAD_INT(fd, newcheque.dsum[2]) == 0 &&
			LOAD_INT(fd, newcheque.dsum[3]) == 0 &&
			LOAD_INT(fd, newcheque.dsum[4]) == 0 &&
			load_string(fd, &newcheque.client) == 0 &&
			load_string(fd, &newcheque.client_inn) == 0;
		s_close(fd);
	}

	printf("agents.count = %zu\n", agents.count);
	printf("articles.count = %zu\n", articles.count);
	printf("newcheque.articles.count = %zu\n", newcheque.articles.count);

	return ret;
}

int newcheque_destroy() {
	if (newcheque.phone_or_email)
		free(newcheque.phone_or_email);
	if (newcheque.add_info)
		free(newcheque.add_info);
	if (newcheque.agent_data)
		free(newcheque.agent_data);
	if (newcheque.client)
		free(newcheque.client);
	if (newcheque.client_inn)
		free(newcheque.client_inn);
	return list_clear(&newcheque.articles);
}

static void button_action(control_t *c, int cmd) {
	window_set_dialog_result(((window_t *)c->parent.parent), cmd);
}

static int article_get_text(void *obj, int index, char *text, size_t text_size) {
	article_t *a = (article_t *)obj;

	switch (index) {
		case 0:
			snprintf(text, text_size, "%d", a->n);
			break;
		case 1:
			snprintf(text, text_size, "%s", a->name);
			break;
		case 2:
			snprintf(text, text_size, "%.1lld.%.2lld",
				a->price_per_unit / 100LLU, a->price_per_unit % 100LLU);
			break;
		case 3:
			for (list_item_t *li = agents.head; li; li = li->next) { 
				agent_t *agent = LIST_ITEM(li, agent_t);
				if (a->pay_agent == agent->n) {
					snprintf(text, text_size, "%s", agent->name);
					return 0;
				}
			}
			text[0] = 0;
			break;
		default:
			return -1;
	}
	return 0;
}

static void newcheque_update_sum(window_t *w, bool redraw) {
	char text[256];
	sprintf(text, "ИТОГО: %.1lld.%.2lld", newcheque.sum / 100LLU, newcheque.sum % 100LLU);
	window_set_label_text(w, 1, text, redraw);
}

static void article_selected_changed(control_t *c, int index __attribute__((unused)), void *item) {
	if (item) {
		window_t *win = (window_t *)c->parent.parent;
		char text[64] = "";

		if (win) {
			control_t *c = window_get_control(win, 1079);
			printf("article_selected_changed: %d\n", index);

			article_t *a = (article_t *)item;
			if (a->price_per_unit > 0) {
				snprintf(text, sizeof(text), "%.1lld.%.2lld",
					a->price_per_unit / 100LLU, a->price_per_unit % 100LLU);
			}
			c->enabled = a->price_per_unit == 0;

			control_set_data(c, 0, text, strlen(text));
		}
	}
}

static void article_new(window_t *parent) {
	if (newcheque.articles.head &&
			LIST_ITEM(newcheque.articles.head, cheque_article_t)->tlv) {
		window_show_error(parent, 1059,
			"Нельзя добавлять предметы расчета в чек,\nсформированный из ранее отпечатанного.\n"
			"Удалите предметы расчета и повторите операцию");
		return;
	}

	bool added = false;
	window_t *win = window_create(NULL, "Выберите товар/работу/услугу", NULL);
	GCPtr screen = window_get_gc(win);
	listview_column_t columns[] = {
		{ "\xfc", 30 },
		{ "Наименование", 320 },
		{ "Цена за ед.", 120 },
		{ "Поставщик", 158 },
	};

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);
	int h = DISCY - 200;

	window_add_label(win, TEXT_START, y, align_left, "Список товаров/работ/услуг:");
	y += th + 2;

	window_add_control(win, 
			listview_create(1059, screen, TEXT_START, y, DISCX - GAP * 2, h, columns, ASIZE(columns),
				&articles, (listview_get_item_text_func_t)article_get_text,
				article_selected_changed, 0));
	y += h + GAP;

	window_add_label(win, TEXT_START, y + 4, align_left, "Цена за ед.:");
	window_add_control(win,
			edit_create(1079, screen, TEXT_START + 150, y, w, th + 8, NULL, EDIT_INPUT_TYPE_MONEY, 14));
	y += 40;

	window_add_label(win, TEXT_START, y + 4, align_left, "Кол-во:");
	window_add_control(win,
			edit_create(1023, screen, TEXT_START + 150, y, w, th + 8, NULL, EDIT_INPUT_TYPE_DOUBLE, 16));


	x = (DISCX - (BUTTON_WIDTH + GAP)*2 - GAP) / 2;
	window_add_control(win, 
			button_create(1, screen, x, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Добавить", button_action));
	window_add_control(win, 
			button_create(0, screen, x + BUTTON_WIDTH + GAP,
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "Отмена", button_action));

	window_set_data(win, 1059, 1, NULL, 0);

	int result;
	while ((result = window_show_dialog(win, -1)) == 1) {
		article_t *a = window_get_ptr_data(win, 1059, 1);
		if (a == NULL) {
			window_show_error(win, 1059, "Не выбран товар/работа/услуга");
			continue;
		} 
		data_t data;
		window_get_data(win, 1023, 0, &data);
		if (data.size == 0) {
			window_show_error(win, 1023, "Не указано кол-во предмета расчета");
			continue;
		}

		ffd_fvln_t fvln;
		if (!ffd_string_to_fvln(data.data, data.size, &fvln) || fvln.value == 0) {
			window_show_error(win, 1059, "Кол-во предмета расчета указано неверно");
			continue;
		}

		window_get_data(win, 1079, 0, &data);
		if (data.size == 0) {
			window_show_error(win, 1079, "Не указана цена за ед. предмета расчета");
			continue;
		}

		uint64_t vln;
		if (!ffd_string_to_vln(data.data, data.size, &vln)) {
			window_show_error(win, 1079, "Цена за ед. предмета расчета указано неверно");
			continue;
		}

		cheque_article_t *ca;
/*		if (newcheque.articles.head) {
			ca = LIST_ITEM(newcheque.articles.head, cheque_article_t);
			if (ca->article == NULL || ca->article->pay_agent != a->pay_agent) {
				window_show_error(win, 1059, 
					"Нельзя добавлять в один чек товары (услуги) разных поставщиков");
				continue;
			}
		}*/

		ca = cheque_article_new();
		ca->article = a;
		ca->count = fvln;
		ca->price_per_unit = vln;
		ca->sum = cheque_article_vln_sum(ca);
		ca->agent = get_agent_by_id(a->pay_agent);

		newcheque.sum += ca->sum;

		list_add(&newcheque.articles, ca);

		window_set_data(parent, 9997, 0, (void *)(newcheque.articles.count - 1), 0);

		newcheque_update_sum(parent, false);
		added = true;

		break;
	}

	window_destroy(win);
	window_draw(parent);
	if (added)
		window_set_focus(parent, 9997);
}

static void show_error_ex(window_t *parent, const char *where) {
	char error_text[512];
	const char *error;
	fd_get_last_error(&error);
	snprintf(error_text, sizeof(error_text), "%s:\n%s", where, error);

	window_show_error(parent, 9998, error_text);
}


static bool read_doc(window_t *parent, window_t *win, uint32_t doc_no) {
	uint8_t status;
	uint16_t doc_type;
	size_t tlv_size;
	uint8_t *buf;
	bool ret = false;

	if ((status = kkt_get_doc_stlv(doc_no, &doc_type, &tlv_size)) != 0) {
		fd_set_error(0, status, NULL, 0);
		show_error_ex(win, "Ошибка при получении документа из ФН");
		return false;
	}

	if (doc_type != 3) {
		window_show_error(win, 9998, "Данный документ не является чеком");
		return false;
	}

	buf = (uint8_t *)malloc(tlv_size);
	if (!buf) {
		window_show_error(win, 9998, "Ошибка при выделении памяти");
		return false;
	}

    size_t len = tlv_size;

	if ((status = kkt_read_doc_tlv(buf, &tlv_size)) != 0) {
		fd_set_error(0, status, NULL, 0);
		show_error_ex(win, "Ошибка при чтении TLV из ФН");
		goto LOut;
	}
	
	if (tlv_size != len)
	{
		window_show_error(win, 9998, "Ошибка при считывании документа из ФН, повторите операцию.");
		goto LOut;
	}

	if (newcheque.agent_data_size > 0) {
		free(newcheque.agent_data);
		newcheque.agent_data_size = 0;
		newcheque.agent_data = NULL;
	}
	
	if (newcheque.client) {
	    free(newcheque.client);
	    newcheque.client = NULL;
	}
	if (newcheque.client_inn) {
	    free(newcheque.client_inn);
	    newcheque.client_inn = NULL;
	}

	newcheque.dsum[0] = 
	newcheque.dsum[1] = 
	newcheque.dsum[2] = 
	newcheque.dsum[3] = 
	newcheque.dsum[4] = 0;

	int pay_kind = -1;

	for (uint8_t *ptr = buf, *end = buf + tlv_size; ptr < end; ) {
		ffd_tlv_t *tlv = (ffd_tlv_t *)ptr;

		if (tlv->tag == 1059) {
			cheque_article_t *ca = cheque_article_new_from_tlv(tlv);
			newcheque.sum += ca->sum;
			list_add(&newcheque.articles, ca);
			window_set_data(parent, 9997, 0, (void *)(newcheque.articles.count - 1), 0);

			newcheque_update_sum(parent, false);
		} else if (tlv->tag == 1077) {
			uint8_t *data = FFD_TLV_DATA(tlv);
			uint32_t fsign =
				((uint32_t)data[5] << 0) |
				((uint32_t)data[4] << 8) |
				((uint32_t)data[3] << 16) |
				((uint32_t)data[2] << 24);
			char text[32];
			sprintf(text, "%u", fsign);
			window_set_data(parent, 1192, 0, text, strlen(text));
		} else if (tlv->tag == 1057 ||
					tlv->tag == 1171 ||
					tlv->tag == 1075 ||
					tlv->tag == 1044 ||
					tlv->tag == 1073 ||
					tlv->tag == 1026 ||
					tlv->tag == 1005 ||
					tlv->tag == 1016 ||
					tlv->tag == 1174) {
			printf("tlv->tag = %d\n", tlv->tag);
			size_t t_size = FFD_TLV_SIZE(tlv);
			size_t new_size = newcheque.agent_data_size + t_size;
			if (newcheque.agent_data_size == 0)
				newcheque.agent_data = (uint8_t *)malloc(new_size);
			else
				newcheque.agent_data = (uint8_t *)realloc(newcheque.agent_data, new_size);
			if (newcheque.agent_data == NULL) {
				window_show_error(win, 9998, "Ошибка при выделении памяти для данных агента");
				goto LOut;
			}
			memcpy(newcheque.agent_data + newcheque.agent_data_size, tlv, t_size);
			newcheque.agent_data_size = new_size;
		} else if (tlv->tag == 1008) {
			window_set_data(parent, 1008, 0, FFD_TLV_DATA(tlv), tlv->length);
		} else if (tlv->tag == 1054) {
			newcheque.pay_type = FFD_TLV_DATA_AS_UINT8(tlv);
			window_set_data(parent, 1054, 0, (const void *)(intptr_t)(newcheque.pay_type - 1), 0);
		} else if (tlv->tag == 1055) {
			uint8_t tax_system = FFD_TLV_DATA_AS_UINT8(tlv);

			for (int i = 0; i < s_tax_system_count; i++) {
				if (b_tax_systems[i] == tax_system) {
					newcheque.tax_system = i;
					window_set_data(parent, 1055, 0, (const void *)(intptr_t)i, 0);
					break;
				}
			}
		} else if (tlv->tag == 1031) {
			newcheque.dsum[0] = ffd_tlv_data_as_vln(tlv);
			if (newcheque.dsum[0] > 0)
				pay_kind = (pay_kind < 0) ? 0 : 5;
		} else if (tlv->tag == 1081) {
			newcheque.dsum[1] = ffd_tlv_data_as_vln(tlv);
			if (newcheque.dsum[1] > 0)
				pay_kind = (pay_kind < 0) ? 1 : 5;
		} else if (tlv->tag == 1215) {
			newcheque.dsum[2] = ffd_tlv_data_as_vln(tlv);
			if (newcheque.dsum[2] > 0)
				pay_kind = (pay_kind < 0) ? 2 : 5;
		} else if (tlv->tag == 1216) {
			newcheque.dsum[3] = ffd_tlv_data_as_vln(tlv);
			if (newcheque.dsum[3] > 0)
				pay_kind = (pay_kind < 0) ? 3 : 5;
		} else if (tlv->tag == 1217) {
			newcheque.dsum[4] = ffd_tlv_data_as_vln(tlv);
			if (newcheque.dsum[4] > 0)
				pay_kind = (pay_kind < 0) ? 4 : 5;
		} else if (tlv->tag == 1227) {
		    newcheque.client = (char *)malloc(tlv->length + 1);
		    memcpy(newcheque.client, FFD_TLV_DATA(tlv), tlv->length);
		    newcheque.client[tlv->length] = 0;
			window_set_data(parent, 1227, 0, newcheque.client, tlv->length);
		} else if (tlv->tag == 1228) {
		    newcheque.client_inn = (char *)malloc(tlv->length + 1);
		    memcpy(newcheque.client_inn, FFD_TLV_DATA(tlv), tlv->length);
		    newcheque.client_inn[tlv->length] = 0;
			window_set_data(parent, 1228, 0, newcheque.client_inn, tlv->length);
		}

		ptr += FFD_TLV_SIZE(tlv);
	}
	ret = true;

	newcheque.pay_kind = pay_kind < 0 ? 0 : pay_kind;
	window_set_data(parent, 9998, 0, (const void *)(intptr_t)newcheque.pay_kind, 0);

LOut:
	free(buf);
	return ret;
}

static void article_from_cheque_new(window_t *parent) {
	if (newcheque.articles.count > 0) {
		window_show_error(parent, 1059,
			"Нельзя добавить ранее сформированный чек в непустой чек.\n"
			"Удалите предметы расчета и повторите операцию");
		return;
	}

	bool added = false;
	window_t *win = window_create(NULL, "Выберите чек", NULL);
	GCPtr screen = window_get_gc(win);
	int x = TEXT_START + 120;
	int y = CONTROLS_TOP + 20;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);

	window_add_label(win, TEXT_START, y + 4, align_left, "Номер чека:");
	window_add_control(win,
			edit_create(9998, screen, TEXT_START + 120, y, w, th + 8, NULL, 
			EDIT_INPUT_TYPE_NUMBER, 14));
	y += 40;

	x = (DISCX - (BUTTON_WIDTH + GAP)*2 - GAP) / 2;
	window_add_control(win, 
			button_create(1, screen, x,
				y + GAP,
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Добавить", button_action));
	window_add_control(win, 
			button_create(0, screen, x + BUTTON_WIDTH + GAP,
				y + GAP,
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "Отмена", button_action));

	int result;
	while ((result = window_show_dialog(win, -1)) == 1) {
		data_t data;
		uint32_t doc_no;
		if (!window_get_data(win, 9998, 0, &data) || (doc_no = atoi(data.data)) == 0)
			window_show_error(win, 9998, "Неправильный номер документа");
		else if (read_doc(parent, win, doc_no)) {
			added = true;
			break;
		}
	}

	window_destroy(win);
	window_draw(parent);
	if (added)
		window_set_focus(parent, 9997);
}


static void article_delete(window_t *parent) {
	int index = window_get_int_data(parent, 9997, 0, -1);
	list_item_t *li = list_item_at(&newcheque.articles, index);

	if (li) {
		cheque_article_t *ca = LIST_ITEM(li, cheque_article_t);
		uint64_t sum = ca->sum;

		list_remove(&newcheque.articles, ca);

		newcheque.sum -= sum;
		newcheque_update_sum(parent, true);
		window_redraw_control(parent, 9997);

		if (newcheque.articles.count == 0) {
			if (newcheque.agent_data_size > 0) {
				newcheque.agent_data_size = 0;
				free(newcheque.agent_data);
				newcheque.agent_data = NULL;
			}
		}
	}
}

static void listbox_get_item_text(void *obj, char *text, size_t text_size) {
	cheque_article_t *ca = (cheque_article_t *)obj;
	const char *name;
	size_t name_len;

	if (ca->tlv && ca->name) {
		name = FFD_TLV_DATA_AS_STRING(ca->name);
		name_len = ca->name->length;
	} else if (ca->article && ca->article->name) {
		name = ca->article->name;
		name_len = strlen(ca->article->name);
	} else {
		name = "<Товар/работа/услуга не найден(а) в справочнике>";
		name_len = strlen(name);
	}

	double count = ffd_fvln_to_double(&ca->count);
	snprintf(text, text_size, "%.1lld.%.2lld*%.3f=%.1lld.%.2lld %.*s",
			ca->price_per_unit / 100LL, ca->price_per_unit % 100LL,
			count, ca->sum / 100LL, ca->sum % 100LL,
			(int)name_len,
			name);
}

bool newcheque_process(window_t *w, const struct kbd_event *e) {
	control_t *c;
	if (!e->pressed)
		return true;
		
    if (e->key == KEY_NUMMUL || (e->key == KEY_8 && (e->shift_state & SHIFT_CTRL)))
    {
		c = window_get_focus(w);
		if (c && c->id == 9997)
			article_from_cheque_new(w);
        return true;
    }

	switch (e->key) {
		case KEY_NUMPLUS:
		case KEY_PLUS:
			c = window_get_focus(w);
			if (c && c->id == 9997)
				article_new(w);
			break;
		case KEY_MINUS:
		case KEY_NUMMINUS:
			c = window_get_focus(w);
			if (c && c->id == 9997)
				article_delete(w);
			break;
	}
	
	return true;
}


typedef struct {
	int agent_id;
} article_group_params_t;

int remove_articles(article_group_params_t *p, cheque_article_t *a) {
	if ((a->article ? a->article->pay_agent : 0) == p->agent_id)
		return 0;
	return -1;
}

static bool newcheque_distribute_sum(window_t *w, uint64_t sum[5]) {
	window_t *win = window_create(NULL, "Распределите сумму по видам оплаты", NULL);
	GCPtr screen = window_get_gc(win);

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int width = DISCX - x - GAP - 100;
	int th = GetTextHeight(screen);
	int height = th + 8;
	char text[256];
	uint64_t total_sum = newcheque.sum;

	/*for (list_item_t *li = newcheque.articles.head; li; li = li->next) {
		cheque_article_t *ca = LIST_ITEM(li, cheque_article_t);
		total_sum += ca->sum;
	}*/
	sprintf(text, "%.1llu.%.2llu РУБ.", total_sum / 100LLU, total_sum % 100LLU);

	window_add_label(win, TEXT_START, y, align_left, "Общая сумма чека:");
	window_add_label(win, TEXT_START + 350, y, align_left, text);
	y += height + YGAP*2;

	if (sum[0] > 0)
		sprintf(text, "%.1llu.%.2llu", sum[0] / 100LLU, sum[0] % 100LLU);
	else
		text[0] = 0;
	window_add_label(win, TEXT_START, y, align_left, "Наличными:");
	window_add_control(win,
		edit_create(1031, screen, TEXT_START + 350, y, width, th + 8, 
			text, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;


	if (sum[1] > 0)
		sprintf(text, "%.1llu.%.2llu", sum[1] / 100LLU, sum[1] % 100LLU);
	else
		text[0] = 0;
	window_add_label(win, TEXT_START, y, align_left, "Безналичными:");
	window_add_control(win,
		edit_create(1081, screen, TEXT_START + 350, y, width, th + 8, text, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;

	if (sum[2] > 0)
		sprintf(text, "%.1llu.%.2llu", sum[2] / 100LLU, sum[2] % 100LLU);
	else
		text[0] = 0;
	window_add_label(win, TEXT_START, y, align_left, "В зачет ранее внесенных средств:");
	window_add_control(win,
		edit_create(1215, screen, TEXT_START + 350, y, width, th + 8, text, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;

	if (sum[3] > 0)
		sprintf(text, "%.1llu.%.2llu", sum[3] / 100LLU, sum[3] % 100LLU);
	else
		text[0] = 0;
	window_add_label(win, TEXT_START, y, align_left, "Постоплатой (кредит):");
	window_add_control(win,
		edit_create(1216, screen, TEXT_START + 350, y, width, th + 8, text, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;


	if (sum[4] > 0)
		sprintf(text, "%.1llu.%.2llu", sum[4] / 100LLU, sum[4] % 100LLU);
	else
		text[0] = 0;
	window_add_label(win, TEXT_START, y, align_left, "Встречным предоставлением:");
	window_add_control(win,
		edit_create(1217, screen, TEXT_START + 350, y, width, th + 8, text, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;

	x = (DISCX - (BUTTON_WIDTH + GAP)*2 - GAP) / 2;
	window_add_control(win,
			button_create(1, screen, x,
				DISCY - BUTTON_HEIGHT - GAP,
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Печать", button_action));
	window_add_control(win, 
			button_create(0, screen, x + BUTTON_WIDTH + GAP,
				DISCY - BUTTON_HEIGHT - GAP,
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "Отмена", button_action));

	bool ret = false;
	int result;
	while ((result = window_show_dialog(win, -1)) == 1) {
		uint16_t tags[] = { 1031, 1081, 1215, 1216, 1217 };
		uint64_t all_sum = 0;

		for (int i = 0; i < ASIZE(tags); i++) {
			data_t data;
			char *endp;
			window_get_data(win, tags[i], 1, &data);

			if (data.size > 0) {
				double v = strtod((const char *)data.data, &endp);
				if (*endp != 0) {
					window_show_error(win, tags[i], "Ошибка при вводе суммы");
					continue;
				} else {
					sum[i] = (uint64_t)((v + 0.009) * 100.0);
					//printf("%.4d: %llu\n", tags[i], sum[i]);
					all_sum += sum[i];
				}
			}
		}

		//printf("all_sum: %llu, total_sum: %llu\n", all_sum, total_sum);

		if (all_sum == total_sum) {
			ret = true;
			break;
		}
		window_show_error(win, tags[0], "Ошибка: сумма всех полей должна быть равна общей сумме чека");
	}
	window_destroy(win);
	window_draw(w);
	kbd_flush_queue();
	return ret;
}

static void newcheque_show_op(window_t *w, const char *title) {
	GCPtr screen = window_get_gc(w);
	SetGCBounds(screen, 0, 0, DISCX, DISCY);

	SetTextColor(screen, clBlack);
	fill_rect(screen, 100, 240, DISCX - 100*2, DISCY - 240*2, 2, clRopnetDarkBrown, clRopnetBrown);
	DrawText(screen, 100, 240, DISCX - 100*2, DISCY - 240*2, title, DT_CENTER | DT_VCENTER);
}

static agent_t *get_newcheque_agent() {
	agent_t *cheque_agent = NULL;
	bool first = true;

	if (!newcheque.articles.head)
		return NULL;

	for (list_item_t *li = newcheque.articles.head; li != NULL; li = li->next) {
		cheque_article_t *ca = LIST_ITEM(li, cheque_article_t);
		int agent_id = ca->article != NULL ? ca->article->pay_agent : 0;
		agent_t *agent = get_agent_by_id(agent_id);

		if (agent == NULL)
			return NULL;

		if (first) {
			cheque_agent = agent;
			first = false;
		}
	}

	return cheque_agent;
}

bool newcheque_print(window_t *w) {
	data_t cashier;
	data_t post;
	data_t inn;

	window_get_data(w, 1021, 1, &cashier);
	window_get_data(w, 9999, 1, &post);
	window_get_data(w, 1203, 1, &inn);

	if (newcheque.phone_or_email && newcheque.phone_or_email[0] && 
			!check_phone_or_email(newcheque.phone_or_email, true)) {
		window_show_error(w, 1008, "Номер тел. или e-mail имеют недопустимый формат.\n"
			"Пример ввода: +71111111111 или name@mail.ru");
		return false;
	}

	if (cashier.size == 0) {
		window_show_error(w, 1021, "Обязательное поле не заполнено");
		return false;
	}

	if (!cashier_set(cashier.data, post.data, inn.data)) {
		window_show_error(w, 1021, "Ошибка записи в файл данных о кассире");
		return false;
	}


	if (newcheque.pay_type < 1 || newcheque.pay_type > 4) {
		window_show_error(w, 1054, "Выберите признак расчета");
		return false;
	}

	if (newcheque.pay_kind == 5 && !newcheque_distribute_sum(w, newcheque.dsum))
		return false;

	cheque_article_t *ca = LIST_ITEM(newcheque.articles.head, cheque_article_t);
	article_group_params_t p = { ca->article ? ca->article->pay_agent : 0 };
	uint64_t sum = 0;
	agent_t *agent = get_newcheque_agent();

	printf("cashier: %s\n", cashier_get_cashier());

	ffd_tlv_reset();
	ffd_tlv_add_string(1021, cashier_get_cashier());
	const char *cashier_inn = cashier_get_inn();
	if (cashier_inn[0])
		ffd_tlv_add_fixed_string(1203, cashier_inn, 12);
	ffd_tlv_add_uint8(1054, newcheque.pay_type);

	printf("p.tax_system = %d, p.agent = %d\n", newcheque.tax_system, p.agent_id);

	ffd_tlv_add_uint8(1055, b_tax_systems[newcheque.tax_system]);
	if (newcheque.phone_or_email && newcheque.phone_or_email[0])
		ffd_tlv_add_string(1008, newcheque.phone_or_email);

	if (_ad->t1086 != NULL) {
		ffd_tlv_stlv_begin(1084, 320);
		ffd_tlv_add_string(1085, "ТЕРМИНАЛ");
		ffd_tlv_add_string(1086, _ad->t1086);
		ffd_tlv_stlv_end();
	}
	
	const char *supplier_phone = agent != NULL ? agent->supplier_phone : NULL;

	if (newcheque.add_info && newcheque.add_info[0])
		ffd_tlv_add_string(1192, newcheque.add_info);

	for (list_item_t *li = newcheque.articles.head; li; li = li->next) {
		cheque_article_t *ca = LIST_ITEM(li, cheque_article_t);
		ffd_tlv_stlv_begin(1059, 1024);
		if (ca->tlv) {
			for (uint8_t *p = FFD_TLV_DATA(ca->tlv), *e = p + ca->tlv->length; p < e; ) {
				ffd_tlv_t *t = (ffd_tlv_t *)p;
				if (t->tag != 1043)
				{
					if (t->tag == 1030 && FFD_TLV_SIZE(t) == 0)
					{
						ffd_tlv_add_string(1030, "АВАНСОВЫЙ ПРЕДМЕТ РАСЧЕТА");
					}
					else
						ffd_tlv_add(t);
				}
				p += FFD_TLV_SIZE(t);
			}
		} else {
			printf("add new 1059\n");
			ffd_tlv_add_uint8(1214, ca->article->pay_method);
			ffd_tlv_add_string(1030, ca->article->name);
			ffd_tlv_add_vln(1079, ca->price_per_unit);
			ffd_tlv_add_fvln(1023, ca->count.value, ca->count.dot);

			if (ca->article->vat_rate < 6)
				ffd_tlv_add_uint8(1199, ca->article->vat_rate);

            if (ca->article->vat_rate > 6)
                ffd_tlv_add_uint8(1199, ca->article->vat_rate - 1);
                
			if (ca->agent != NULL) {
				printf("ca->agent->inn: %s\n", ca->agent->inn);
				ffd_tlv_add_fixed_string(1226, ca->agent->inn, 12);
				if (support_1222_1224_1225)
				{
				    if (ca->agent->pay_agent != 0)
				    {
            			ffd_tlv_add_uint8(1222, 1 << ca->agent->pay_agent);
            			ffd_tlv_stlv_begin(1224, 512);
        				ffd_tlv_add_string(1225, ca->agent->name);
        				if (ca->agent->supplier_phone != NULL) {
        				    /*if (supplier_phone == NULL
        				        || ca->agent->supplier_phone != supplier_phone
        				        || strcmp(ca->agent->supplier_phone, supplier_phone) != 0)*/
            				ffd_tlv_add_string(1171, ca->agent->supplier_phone);
        				}
            			ffd_tlv_stlv_end();				        
				    }
				}
			}
		}
		ffd_tlv_stlv_end();
		sum += ca->sum;
		printf("sum: %ld\n", ca->sum);
	}

/*	if (agent != NULL) {
		ffd_tlv_add_uint8(1057, 1 << agent->pay_agent);
		switch (agent->pay_agent) {
			case 0:
			case 1:
				ffd_tlv_add_string(1075, agent->transfer_operator_phone);
				ffd_tlv_add_string(1044, agent->pay_agent_operation);
				ffd_tlv_add_string(1073, agent->pay_agent_phone);
				ffd_tlv_add_string(1026, agent->money_transfer_operator_name);
				ffd_tlv_add_string(1005, agent->money_transfer_operator_address);
				ffd_tlv_add_fixed_string(1016, agent->money_transfer_operator_inn, 12);
				ffd_tlv_add_string(1171, agent->supplier_phone);
				break;
			case 2:
			case 3:
				ffd_tlv_add_string(1073, agent->pay_agent_phone);
				ffd_tlv_add_string(1174, agent->payment_processor_phone);
				ffd_tlv_add_string(1171, agent->supplier_phone);
				break;
			case 4:
			case 5:
			case 6:
				ffd_tlv_add_string(1171, agent->supplier_phone);
				break;
		}
	} else*/ if (newcheque.agent_data_size > 0) {
		for (uint8_t *p = newcheque.agent_data,
				*end = p + newcheque.agent_data_size; p < end; ) {
			ffd_tlv_t *tlv = (ffd_tlv_t *)p;
			ffd_tlv_add(tlv);
			p += FFD_TLV_SIZE(tlv);
		}
	}
	
	if (newcheque.client && newcheque.client[0] != 0) {
	    printf("newcheque.client = %s\n", newcheque.client);
	    ffd_tlv_add_string(1227, newcheque.client);
	}
	
	if (newcheque.client_inn && newcheque.client_inn[0] != 0) {
	    int len = strlen(newcheque.client_inn);

	    printf("newcheque.client_inn = %s\n", newcheque.client_inn);
	    
	    if (len != 10 && len != 12) {
    		window_show_error(w, 1228, "Неправильный ИНН клиента");
	    	return false;
		}
	
	    ffd_tlv_add_fixed_string(1228, newcheque.client_inn, 12);
	}

	switch (newcheque.pay_kind) {
	case 0:
		newcheque.dsum[0] = sum;
		newcheque.dsum[1] =
		newcheque.dsum[2] =
		newcheque.dsum[3] =
		newcheque.dsum[4] = 0;
		break;
	case 1:
		newcheque.dsum[1] = sum;
		newcheque.dsum[0] =
		newcheque.dsum[2] =
		newcheque.dsum[3] =
		newcheque.dsum[4] = 0;
		break;
	case 2:
		newcheque.dsum[2] = sum;
		newcheque.dsum[0] =
		newcheque.dsum[1] =
		newcheque.dsum[3] =
		newcheque.dsum[4] = 0;
		break;
	case 3:
		newcheque.dsum[3] = sum;
		newcheque.dsum[0] =
		newcheque.dsum[1] =
		newcheque.dsum[2] =
		newcheque.dsum[4] = 0;
		break;
	case 4:
		newcheque.dsum[4] = sum;
		newcheque.dsum[0] =
		newcheque.dsum[1] =
		newcheque.dsum[2] =
		newcheque.dsum[3] = 0;
		break;
	}

	ffd_tlv_add_vln(1031, newcheque.dsum[0]);
	ffd_tlv_add_vln(1081, newcheque.dsum[1]);
	ffd_tlv_add_vln(1215, newcheque.dsum[2]);
	ffd_tlv_add_vln(1216, newcheque.dsum[3]);
	ffd_tlv_add_vln(1217, newcheque.dsum[4]);

/*		printf("dsum[0] = %llu\n", dsum[0]);
	printf("dsum[1] = %llu\n", dsum[1]);
	printf("dsum[2] = %llu\n", dsum[2]);
	printf("dsum[3] = %llu\n", dsum[3]);
	printf("dsum[4] = %llu\n", dsum[4]);*/

	cashier_save();

	newcheque_show_op(w, "Идет печать чека...");

	if (!fa_create_doc(CHEQUE, NULL, 0, NULL, NULL))
		return false;

	list_clear(&newcheque.articles);
	//list_remove_if(&newcheque.articles, &p, (list_item_func_t)remove_articles);

	newcheque.sum = 0;
	newcheque_update_sum(w, false);

	if (newcheque.agent_data_size > 0) {
		newcheque.agent_data_size = 0;
		free(newcheque.agent_data);
		newcheque.agent_data = NULL;
	}

	if (newcheque.add_info) {
		free(newcheque.add_info);
		newcheque.add_info = NULL;
		window_set_data(w, 1192, 0, "", 0);
	}

	if (newcheque.client) {
		free(newcheque.client);
		newcheque.client = NULL;
		window_set_data(w, 1227, 0, "", 0);
	}

	if (newcheque.client_inn) {
		free(newcheque.client_inn);
		newcheque.client_inn = NULL;
		window_set_data(w, 1228, 0, "", 0);
	}

	newcheque.dsum[0] =
	newcheque.dsum[1] =
	newcheque.dsum[2] =
	newcheque.dsum[3] =
	newcheque.dsum[4];

	newcheque_save();
	window_draw(w);
	kbd_flush_queue();

	return true;
}

int newcheque_execute() {
	int focus_id = 9997;
	
	support_1222_1224_1225 = kkt_has_param("SUPPORT_1222_1224_1225");

	window_t *win = window_create(NULL, "Чек(и) (Esc - выход)", newcheque_process);
	GCPtr screen = window_get_gc(win);

	s_tax_system_count = 0;
	for (int i = 0; i < str_tax_system_count; i++) {
		uint8_t b = tax_systems_bits[i];
		if (reg_tax_systems & b) {
			s_tax_systems[s_tax_system_count] = str_tax_systems[i];
			b_tax_systems[s_tax_system_count] = b;
			s_tax_system_count++;
		}
	}

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);
	int h = th + 8;
#define DY 2

	const char *pay_type[] = {
		"Приход",
		"Возврат прихода",
		"Расход",
		"Возврат расхода"
	};

	const char *pay_kind[] = {
		"Наличные",
		"Безналичные",
		"В зачет ранее внесенных средств",
		"Постоплата (кредит)",
		"Встречным предоставлением",
		"Распределение по видам оплаты"
	};

	if (newcheque.tax_system >= s_tax_system_count)
		newcheque.tax_system = 0;

	window_add_label(win, TEXT_START, y, align_left, "Признак расчета:");
	window_add_control(win, 
			simple_combobox_create(1054, screen, x, y, w, h, pay_type, ASIZE(pay_type), 
				newcheque.pay_type - 1));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "СНО:");
	window_add_control(win, 
				simple_combobox_create(1055, screen, x, y, w, h, s_tax_systems, s_tax_system_count, 
				newcheque.tax_system));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "Вид оплаты:");
	window_add_control(win, 
			simple_combobox_create(9998, screen, x, y, w, h, pay_kind, ASIZE(pay_kind), 
				newcheque.pay_kind));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "Кассир:");
	window_add_control(win, 
			edit_create(1021, screen, x, y, w, h, cashier_get_name(), EDIT_INPUT_TYPE_TEXT, 64));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "Должность:");
	window_add_control(win,
			edit_create(9999, screen, x, y, w, h, cashier_get_post(), EDIT_INPUT_TYPE_TEXT, 64));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "ИНН кассира:");
	window_add_control(win,
			edit_create(1203, screen, x, y, w, h, cashier_get_inn(), EDIT_INPUT_TYPE_TEXT, 12));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "Тел. или e-mail покупателя:");
	window_add_control(win,
			edit_create(1008, screen, x, y, w, h, newcheque.phone_or_email,
			   	EDIT_INPUT_TYPE_TEXT, 64));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "Доп. реквизит чека:");
	window_add_control(win,
			edit_create(1192, screen, x, y, w, h, newcheque.add_info,
			   	EDIT_INPUT_TYPE_TEXT, 16));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "Покупатель (клиент):");
	window_add_control(win,
			edit_create(1227, screen, x, y, w, h, newcheque.client,
			   	EDIT_INPUT_TYPE_TEXT, 255));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "ИНН покупателя (клиента):");
	window_add_control(win,
			edit_create(1228, screen, x, y, w, h, newcheque.client_inn,
			   	EDIT_INPUT_TYPE_TEXT, 12));
	y += h + YGAP + th + 12;

	window_add_label(win, TEXT_START, y - th - 4, align_left,
			"Предметы расчета (\"+\",\"*\" добавить, \"-\" удалить):");


	char text[256];
	sprintf(text, "ИТОГО: %.1lld.%.2lld", newcheque.sum / 100LL, newcheque.sum % 100LL);
	window_add_label_with_id(win, 1, DISCX - GAP, y - th - 4, align_right, text);

	window_add_control(win,
			listbox_create(9997, screen, GAP, y, DISCX - GAP*2, DISCY - y - BUTTON_HEIGHT - GAP*2,
				&newcheque.articles, listbox_get_item_text, 0));

	window_add_control(win, 
			button_create(1, screen, DISCX - (BUTTON_WIDTH + GAP)*2, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Печать", button_action));
	window_add_control(win, 
			button_create(0, screen, DISCX - (BUTTON_WIDTH + GAP)*1, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "Закрыть", button_action));

/*	if (newcheque.articles.count < 25) {
		for (int i = newcheque.articles.count; i < 25; i++) {
			cheque_article_t *ca = cheque_article_new();
			ca->article = LIST_ITEM(articles.head, article_t);
			ca->count.value = i + 1;
			ca->count.dot = 0;

			newcheque.sum += cheque_article_sum(ca);

			list_add(&newcheque.articles, ca);
		}
	}*/

	int result;
	while (true) {
		result = window_show_dialog(win, focus_id);
		focus_id = -1;

		data_t data;
		window_get_data(win, 1008, 1, &data);

		if (newcheque.phone_or_email)
			free(newcheque.phone_or_email);
		newcheque.phone_or_email = strdup(data.data);

		window_get_data(win, 1192, 0, &data);
		if (newcheque.add_info)
			free(newcheque.add_info);
		newcheque.add_info = strdup(data.data);
		
		window_get_data(win, 1227, 0, &data);
		if (newcheque.client)
			free(newcheque.client);
		newcheque.client = strdup(data.data);
		
		window_get_data(win, 1228, 0, &data);
		if (newcheque.client_inn)
			free(newcheque.client_inn);
		newcheque.client_inn = strdup(data.data);
		

		newcheque.pay_type = window_get_int_data(win, 1054, 0, 0) + 1;
		newcheque.pay_kind = window_get_int_data(win, 9998, 0, 0);
		newcheque.tax_system = window_get_int_data(win, 1055, 0, 0);

		newcheque_save();

		if (result == 0)
			break;

		if (newcheque_print(win))
			break;
	}

	window_destroy(win);
	
	return result;
}


void on_newcheque_article_removed(article_t *a) {
	for (list_item_t *li = newcheque.articles.head; li; li = li->next) {
		cheque_article_t *ca = LIST_ITEM(li, cheque_article_t);
		if (ca->article == a) {
			newcheque.sum -= ca->sum;
			list_remove(&newcheque.articles, ca);
			break;
		}
	}
}
