#include <stdint.h>
#include <unistd.h>
#include "gui/references/article.h"

static int article_max_id = 0;

/*static int get_bit(uint8_t v) {
	switch (v) {
		case 0x1:
			return 0;
		case 0x2:
			return 1;
		case 0x4:
			return 2;
		case 0x8:
			return 3;
		case 0x10:
			return 4;
		case 0x20:
			return 5;
		case 0x40:
			return 6;
		case 0x80:
			return 7;
		default:
			return -1;
	}
}*/

static const char * str_pay_methods[] = 
{
	"ПРЕДОПЛАТА 100%",
	"ПРЕДОПЛАТА",
	"АВАНС",
	"ПОЛНЫЙ РАСЧЕТ",
	"ЧАСТИЧНЫЙ РАСЧЕТ И КРЕДИТ",
	"ПЕРЕДАЧА В КРЕДИТ",
	"ОПЛАТА КРЕДИТА",

};

static const char * str_vats[] = 
{
	"НДС 20%",
	"НДС 10%",
	"НДС 20/120",
	"НДС 10/110",
	"НДС 0%",
	"НЕ ПОДЛЕЖИТ НАЛОГООБЛОЖЕНИЮ НДС",
	"НДС НЕ ОБЛАГАЕТСЯ",
	"НДС 5%",
	"НДС 7%",
	"НДС 5/105",
	"НДС 7/107",
};

int article_get_text(void *obj, int index, char *text, size_t text_size) {
	article_t *a = (article_t *)obj;
	//int v;

	switch (index) {
		case 0:
			snprintf(text, text_size, "%d", a->n);
			break;
		case 1:
			snprintf(text, text_size, "%s", a->name);
			break;
/*		case 2: {
			const char * str_tax_system;
			if ((v = get_bit(a->tax_system)) == -1)
				str_tax_system = "<НЕ УКАЗАНА>";
			else
				str_tax_system = str_tax_systems[v];
			snprintf(text, text_size, "%s", str_tax_system);
			break;
		}*/
		case 2:
			snprintf(text, text_size, "%s", str_pay_methods[a->pay_method - 1]);
			break;			
		case 3:
			snprintf(text, text_size, "%.1lld.%.2lld",
				a->price_per_unit / 100LLU, a->price_per_unit % 100LLU);
			break;
		case 4:
			snprintf(text, text_size, "%s", str_vats[a->vat_rate - 1]);
			break;
		case 5:
			for (list_item_t *li = agents.head; li; li = li->next) { 
				agent_t *agent = LIST_ITEM(li, agent_t);
				if (a->pay_agent == agent->n) {
					snprintf(text, text_size, "%s", agent->name);
					return 0;
				}
			}
			snprintf(text, text_size, "%s", "<Не выбран>");
			break;
		default:
			return -1;
	}		
	return 0;
}

static article_t *article_new() {
	article_t *a = (article_t *)malloc(sizeof(article_t));
   	memset(a, 0, sizeof(article_t));
	return a;
}

static void article_free(article_t *a) {
	if (a->name)
		free(a->name);
	free(a);
}

int article_save(void *arg, article_t *a) {
	int fd = (int)(intptr_t)arg;
	uint8_t tax_system = 1;
	if (SAVE_INT(fd, a->n) < 0 ||
		save_string(fd, a->name) < 0 ||
		SAVE_INT(fd, tax_system) < 0 ||
		SAVE_INT(fd, a->pay_method) < 0 ||
		SAVE_INT(fd, a->price_per_unit) < 0 ||
		SAVE_INT(fd, a->vat_rate) < 0 ||
		SAVE_INT(fd, a->pay_agent) < 0)
		return -1;
	return 0;
}

article_t* article_load(int fd) {
	uint8_t tax_system;
	article_t *a = article_new();
	if (LOAD_INT(fd, a->n) < 0 ||
		load_string(fd, &a->name) < 0 ||
		LOAD_INT(fd, tax_system) < 0 ||
		LOAD_INT(fd, a->pay_method) < 0 ||
		LOAD_INT(fd, a->price_per_unit) < 0 ||
		LOAD_INT(fd, a->vat_rate) < 0 ||
		LOAD_INT(fd, a->pay_agent) < 0) {
		article_free(a);
		return NULL;
	}
	if (a->n > article_max_id)
		article_max_id = a->n;
	return a;
}

static bool process_article_edit(form_t *form, article_t *a) {
	while (form_execute(form) == 1) {
		form_data_t name;
		form_data_t price_per_unit;
		//int tax_system;
		int pay_method;
		int vat_rate;
		int pay_agent;
		char *endp;

		form_get_data(form, 1030, 1, &name);
		//tax_system = form_get_int_data(form, 1055, 0, -1);
		pay_method = form_get_int_data(form, 1214, 0, -1);
		vat_rate = form_get_int_data(form, 1199, 0, -1);
		pay_agent = form_get_int_data(form, 1054, 0, 0);
		form_get_data(form, 1079, 1, &price_per_unit);

/*		printf("name: %s\n", (const char *)name.data);
		printf("tax_system: %d\n", tax_system);
		printf("pay_method: %d\n", pay_method);
		printf("vat_rate: %d\n", vat_rate);
		printf("price_per_unit: %s\n", (const char *)price_per_unit.data);*/

		if (name.size == 0)
			fa_show_error(form, 100, "Наименование не заполнено");
        /*else if (tax_system == -1)
			fa_show_error(form, 1055, "Выберите систему налогообложения");*/
		else if (pay_method == -1)
			fa_show_error(form, 1214, "Выберите признак способа расчета");
		else if (vat_rate == -1)
			fa_show_error(form, 1199, "Выберите ставку НДС");
		else if (price_per_unit.size == 0)
			fa_show_error(form, 1079, "Цена за ед. предмета расчета не заполнена");
		else {
			double v = strtod((const char *)price_per_unit.data, &endp);
			if (*endp != 0)
				fa_show_error(form, 1079, "Цена за ед. предмета расчета введена неверно");
			else {
				a->price_per_unit = (uint64_t)((v + 0.009) * 100.0);
				if (a->name)
					free(a->name);
				a->name = strdup((char *)name.data);
				a->name[name.size] = 0;
				//a->tax_system = (1 << tax_system);
				a->pay_method = pay_method + 1;
				a->vat_rate = vat_rate + 1;
				a->pay_agent = get_agent_id_by_index(pay_agent);

				return true;
			}
		}
	}

	return false;
}

char **create_agent_info_list() {
	char **agent_list = malloc(sizeof(char *)*agents.count + 1);
	char **al = agent_list;

	al[0] = malloc(64+1);
	strcpy(al[0], "<Не выбран>");
	al++;

	for (list_item_t *li = agents.head; li; li = li->next, al++) {
		agent_t *agent = LIST_ITEM(li, agent_t);
		*al = malloc(64+1);
		sprintf(*al, "%s", agent->name);
	}
	return agent_list;
}

void destroy_agent_info_list(char **l) {
	for (int i = 0; i < agents.count; i++)
		free(l[i]);
	free(l);
}

void* create_new_article(data_source_t *ds __attribute__((unused))) {
	article_t *a = NULL;
	form_t *form = NULL;
	char **agent_info_list = create_agent_info_list();

	BEGIN_FORM(form, "Новый товар/работа/услуга")
		FORM_ITEM_EDIT_TEXT(1030, "Наименование:", NULL, FORM_INPUT_TYPE_TEXT, 128)
//		FORM_ITEM_COMBOBOX(1055, "Система налогообложения:", str_tax_systems, str_tax_system_count, -1)
		FORM_ITEM_COMBOBOX(1214, "Признак способа расчета:", str_pay_methods, ASIZE(str_pay_methods), -1)
		FORM_ITEM_EDIT_TEXT(1079, "Цена за ед. предмета расчета:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_COMBOBOX(1199, "Ставка НДС:", str_vats, ASIZE(str_vats), -1)
		FORM_ITEM_COMBOBOX(1054, "Поставщик:", (const char **)agent_info_list, agents.count + 1, 0)

		FORM_ITEM_BUTTON(1, "ОК")
		FORM_ITEM_BUTTON(2, "Отмена")
	END_FORM()

	a = article_new();
   	a->n = ++article_max_id;

	if (!process_article_edit(form, a)) {
		article_free(a);
		a = NULL;
		article_max_id--;
	}

	destroy_agent_info_list(agent_info_list);

	form_destroy(form);

	return a;
}

int edit_article(data_source_t *ds __attribute__((unused)), void *obj) {
	int ret = -1;
	article_t *a = (article_t *)obj;
	form_t *form = NULL;
//	int tax_system = get_bit(a->tax_system);
	char price_per_unit[32];
	char **agent_info_list = create_agent_info_list();
	int pay_agent_index = 0;

	for (list_item_t *li = agents.head; li; li = li->next, pay_agent_index++) { 
		agent_t *agent = LIST_ITEM(li, agent_t);
		if (a->pay_agent == agent->n)
			break;
	}
	if (pay_agent_index >= agents.count)
		pay_agent_index = 0;
	else
		pay_agent_index++;

	sprintf(price_per_unit, "%.1lld.%.2lld",
		a->price_per_unit / 100LLU, a->price_per_unit % 100LLU);

	BEGIN_FORM(form, "Изменить данные товара/работы/услуги")
		FORM_ITEM_EDIT_TEXT(1030, "Наименование:", a->name, FORM_INPUT_TYPE_TEXT, 128)
//		FORM_ITEM_COMBOBOX(1055, "Система налогообложения:", str_tax_systems, str_tax_system_count, tax_system)
		FORM_ITEM_COMBOBOX(1214, "Признак способа расчета:", str_pay_methods, ASIZE(str_pay_methods), a->pay_method - 1)
		FORM_ITEM_EDIT_TEXT(1079, "Цена за ед. предмета расчета:", price_per_unit, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_COMBOBOX(1199, "Ставка НДС:", str_vats, ASIZE(str_vats), a->vat_rate - 1)
		FORM_ITEM_COMBOBOX(1054, "Поставщик:", (const char **)agent_info_list, agents.count + 1, pay_agent_index)

		FORM_ITEM_BUTTON(1, "ОК")
		FORM_ITEM_BUTTON(2, "Отмена")
	END_FORM()

	if (process_article_edit(form, a)) {
		ret = 0;
	}

	destroy_agent_info_list(agent_info_list);
	form_destroy(form);

	return ret;
}


extern void on_newcheque_article_removed(article_t *a);

int remove_article(data_source_t *ds __attribute__((unused)), void *obj) {
	on_newcheque_article_removed((article_t *)obj);
	return 0;
}


list_t articles = { NULL, NULL, 0, (list_item_delete_func_t)article_free };

#define ARTICLES_FILE_NAME	"/home/sterm/articles.bin"

int articles_load() {
    int fd = s_open(ARTICLES_FILE_NAME, false);
	int ret;

    if (fd == -1) {
        return -1;
    }

    if (load_list(fd, &articles, (load_item_func_t)article_load) < 0)
		ret = -1;
	else
		ret = 0;

	s_close(fd);

	return ret;
}

int articles_save() {
    int fd = s_open(ARTICLES_FILE_NAME, true);
	int ret;

    if (fd == -1) {
        return -1;
    }

    if (save_list(fd, &articles, (list_item_func_t)article_save) < 0)
		ret = -1;
	else
		ret = 0;

	s_close(fd);

	return ret;
}

int articles_destroy() {
	return list_clear(&articles);
}


void fa_articles() {
	lvform_column_t columns[] = {
		{ "\xfc", 30 },
		{ "Наименование", 200 },
		//{ "СНО", 50 },
		{ "Способ расчета", 150 },
		{ "Цена за ед.", 120 },
		{ "Ставка НДС", 120 },
		{ "Поставщик", 158 },
	};
	data_source_t ds = {
		&articles,
		article_get_text,
		create_new_article,
		edit_article,
		remove_article
	};

	lvform_t *lv = lvform_create("Справочник товаров/работ/услуг", columns, ASIZE(columns),	&ds);
	lvform_execute(lv);
	lvform_destroy(lv);

	articles_save();

	ClearScreen(clBlack);
	draw_menu(fa_sales_menu);
}
