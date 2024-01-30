#include "serialize.h"
#include "gui/references/agent.h"
#include "gui/references/article.h"

static int agent_max_id = 0;

int agent_get_text(void *obj, int index, char *text, size_t text_size) {
	agent_t *a = (agent_t *)obj;
	switch (index) {
		case 0:
			snprintf(text, text_size, "%d", a->n);
			break;
		case 1:
			snprintf(text, text_size, "%s", a->name);
			break;
		case 2:
			snprintf(text, text_size, "%s", a->inn);
			break;
		case 3:
			snprintf(text, text_size, "%s", a->description ? a->description : "");
			break;
		default:
			return -1;
	}		
	return 0;
}

static agent_t *agent_new() {
	agent_t *a = (agent_t *)malloc(sizeof(agent_t));
   	memset(a, 0, sizeof(agent_t));
	return a;
}

static void agent_free(agent_t *a) {
	if (a->name)
		free(a->name);
	if (a->inn)
		free(a->inn);
	if (a->description)
		free(a->description);
	if (a->transfer_operator_phone)
		free(a->transfer_operator_phone);
	if (a->pay_agent_operation)
		free(a->pay_agent_operation);
	if (a->pay_agent_phone)
		free(a->pay_agent_phone);
	if (a->payment_processor_phone)
		free(a->payment_processor_phone);
	if (a->money_transfer_operator_name)
		free(a->money_transfer_operator_name);
	if (a->money_transfer_operator_address)
		free(a->money_transfer_operator_address);
	if (a->money_transfer_operator_inn)
		free(a->money_transfer_operator_inn);
	if (a->supplier_phone)
		free(a->supplier_phone);
	free(a);
}

int agent_save(void *arg, agent_t *a) {
	int fd = (int)(intptr_t)arg;
	if (SAVE_INT(fd, a->n) < 0 ||
		save_string(fd, a->name) < 0 ||
		save_string(fd, a->inn) < 0 ||
		save_string(fd, a->description) < 0 ||
		SAVE_INT(fd, a->pay_agent) < 0 ||
		save_string(fd, a->transfer_operator_phone) < 0 ||
		save_string(fd, a->pay_agent_operation) < 0 ||
		save_string(fd, a->pay_agent_phone) < 0 ||
		save_string(fd, a->payment_processor_phone) < 0 ||
		save_string(fd, a->money_transfer_operator_name) < 0 ||
		save_string(fd, a->money_transfer_operator_address) < 0 ||
		save_string(fd, a->money_transfer_operator_inn) < 0 ||
		save_string(fd, a->supplier_phone) < 0)
		return -1;
	return 0;
}

agent_t* agent_load(int fd) {
	agent_t *a = agent_new();
	if (LOAD_INT(fd, a->n) < 0 ||
		load_string(fd, &a->name) < 0 ||
		load_string(fd, &a->inn) < 0 ||
		load_string(fd, &a->description) < 0 ||
		LOAD_INT(fd, a->pay_agent) < 0 ||
		load_string(fd, &a->transfer_operator_phone) < 0 ||
		load_string(fd, &a->pay_agent_operation) < 0 ||
		load_string(fd, &a->pay_agent_phone) < 0 ||
		load_string(fd, &a->payment_processor_phone) < 0 ||
		load_string(fd, &a->money_transfer_operator_name) < 0 ||
		load_string(fd, &a->money_transfer_operator_address) < 0 ||
		load_string(fd, &a->money_transfer_operator_inn) < 0 ||
		load_string(fd, &a->supplier_phone) < 0) {
		agent_free(a);
		return NULL;
	}
	if (a->n > agent_max_id)
		agent_max_id = a->n;
	return a;
}

static const char *str_pay_agents[] = 
{
	"БАНК. ПЛ. АГЕНТ",
	"БАНК. ПЛ. СУБАГЕНТ",
	"ПЛ. АГЕНТ",
	"ПЛ. СУБАГЕНТ",
	"ПОВЕРЕННЫЙ",
	"КОМИССИОНЕР",
	"АГЕНТ",
};

static bool process_agent_edit(form_t *form, agent_t *a) {
	bool ok = false;

	while (form_execute(form) == 1) {
		int pay_agent;
		form_data_t name;
		form_data_t inn;
		form_data_t description;
		form_data_t transfer_operator_phone;
		form_data_t pay_agent_operation;
		form_data_t pay_agent_phone;
		form_data_t payment_processor_phone;
		form_data_t money_transfer_operator_name;
		form_data_t money_transfer_operator_address;
		form_data_t money_transfer_operator_inn;
		form_data_t supplier_phone;

		ok = false;
		
		form_get_data(form, 100, 1, &name);
		form_get_data(form, 101, 1, &inn);
		form_get_data(form, 102, 1, &description);
		pay_agent = form_get_int_data(form, 1054, 0, -1);
		form_get_data(form, 1075, 1, &transfer_operator_phone);
		form_get_data(form, 1044, 1, &pay_agent_operation);
		form_get_data(form, 1073, 1, &pay_agent_phone);
		form_get_data(form, 1174, 1, &payment_processor_phone);
		form_get_data(form, 1026, 1, &money_transfer_operator_name);
		form_get_data(form, 1005, 1, &money_transfer_operator_address);
		form_get_data(form, 1016, 1, &money_transfer_operator_inn);
		form_get_data(form, 1171, 1, &supplier_phone);

		if (name.size == 0)
			fa_show_error(form, 100, "Наименование поставщика не заполнено");
		else if (inn.size != 10 && inn.size != 12)
			fa_show_error(form, 101, "ИНН должно содержать 10 или 12 цифр");
        else if (pay_agent == -1)
			fa_show_error(form, 1054, "Выберите тип поставщика");
		else if (pay_agent == 0 || pay_agent == 1) {
			ok = fa_check_field(form, &transfer_operator_phone, 1075, "Поле \"Тлф. оп. перевода\" не заполнено") &&
				fa_check_field(form, &pay_agent_operation, 1044, "Поле \"Оп. агента\" не заполнено") &&
				fa_check_field(form, &pay_agent_phone, 1073, "Поле \"Тлф. платежного агента\" не заполнено") &&
				fa_check_field(form, &money_transfer_operator_name, 1026, "Поле \"Оператор перевода\" не заполнено") &&
				fa_check_field(form, &money_transfer_operator_address, 1005, "Поле \"Адрес оператора перевода\" не заполнено") &&						
				fa_check_inn_field(form, &money_transfer_operator_inn, 1016, "Поле \"ИНН оператора перевода\" заполнено не правильно (д.б. 10 или 12 цифр)") &&
				fa_check_field(form, &supplier_phone, 1171, "Поле \"Телефон поставщика\" не заполнено");
		} else if (pay_agent == 2 || pay_agent == 3) {
			ok = fa_check_field(form, &pay_agent_phone, 1073, "Поле \"Тлф. платежного агента\" не заполнено") &&
				fa_check_field(form, &payment_processor_phone, 1174, "Поле \"Тлф. оп. по приему платежей\" не заполнено") &&
				fa_check_field(form, &supplier_phone, 1171, "Поле \"Телефон поставщика\" не заполнено");
		} else if (pay_agent == 4 || pay_agent == 5 || pay_agent == 6)
			ok = fa_check_field(form, &supplier_phone, 1171, "Поле \"Телефон поставщика\" не заполнено");

		if (ok) {
			if (a->name)
				free(a->name);
			a->name = strdup((char *)name.data);
			if (a->inn)
				free(a->inn);
			a->inn = strdup((char *)inn.data);
			if (a->description)
				free(a->description);
			a->description = strdup((char *)description.data);
			a->pay_agent = pay_agent;
			if (a->transfer_operator_phone)
				free(a->transfer_operator_phone);
			a->transfer_operator_phone = strdup((char *)transfer_operator_phone.data);
			if (a->pay_agent_operation)
				free(a->pay_agent_operation);
			a->pay_agent_operation = strdup((char *)pay_agent_operation.data);
			if (a->pay_agent_phone)
				free(a->pay_agent_phone);
			a->pay_agent_phone = strdup((char *)pay_agent_phone.data);
			if (a->payment_processor_phone)
				free(a->payment_processor_phone);
			a->payment_processor_phone = strdup((char *)payment_processor_phone.data);
			if (a->money_transfer_operator_name)
				free(a->money_transfer_operator_name);
			a->money_transfer_operator_name = strdup((char *)money_transfer_operator_name.data);
			if (a->money_transfer_operator_address)
				free(a->money_transfer_operator_address);
			a->money_transfer_operator_address = strdup((char *)money_transfer_operator_address.data);
			if (a->money_transfer_operator_inn)
				free(a->money_transfer_operator_inn);
			a->money_transfer_operator_inn = strdup((char *)money_transfer_operator_inn.data);
			if (a->supplier_phone)
				free(a->supplier_phone);
			a->supplier_phone = strdup((char *)supplier_phone.data);
			return true;
		}
	}

	return ok;
}

void* create_new_agent(data_source_t *ds __attribute__((unused))) {
	agent_t *a = NULL;
	form_t *form = NULL;
	BEGIN_FORM(form, "Новый поставщик")
		FORM_ITEM_EDIT_TEXT(100, "Наименование поставщика:", NULL, FORM_INPUT_TYPE_TEXT, 32)
		FORM_ITEM_EDIT_TEXT(101, "ИНН поставщика:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(102, "Описание:", NULL, FORM_INPUT_TYPE_TEXT, 64)

		FORM_ITEM_COMBOBOX(1054, "Тип агентских отношений:", str_pay_agents, ASIZE(str_pay_agents), -1)
        FORM_ITEM_EDIT_TEXT(1075, "Тлф. оп. перевода:", NULL, FORM_INPUT_TYPE_TEXT, 64)
        FORM_ITEM_EDIT_TEXT(1044, "Оп. агента:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1073, "Тлф. платежного агента:", NULL, FORM_INPUT_TYPE_TEXT, 19)
		FORM_ITEM_EDIT_TEXT(1174, "Тлф. оп. по приему платежей:", NULL, FORM_INPUT_TYPE_TEXT, 19)
		FORM_ITEM_EDIT_TEXT(1026, "Оператор перевода:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1005, "Адрес оператора перевода:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1016, "ИНН оператора перевода:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1171, "Телефон поставщика:", NULL, FORM_INPUT_TYPE_TEXT, 19)

		FORM_ITEM_BUTTON(1, "ОК")
		FORM_ITEM_BUTTON(2, "Отмена")
	END_FORM()

	a = agent_new();
   	a->n = ++agent_max_id;

	if (!process_agent_edit(form, a)) {
		agent_free(a);
		a = NULL;
		agent_max_id--;
	}

	form_destroy(form);

	return a;
}

int edit_agent(data_source_t *ds __attribute__((unused)), void *obj) {
	int ret = -1;
	agent_t *a = (agent_t *)obj;
	form_t *form = NULL;
	BEGIN_FORM(form, "Изменить данные поставщика")
		FORM_ITEM_EDIT_TEXT(100, "Наименование поставщика:", a->name, FORM_INPUT_TYPE_TEXT, 32)
		FORM_ITEM_EDIT_TEXT(101, "ИНН поставщика:", a->inn, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(102, "Описание:", a->description, FORM_INPUT_TYPE_TEXT, 64)
		
		FORM_ITEM_COMBOBOX(1054, "Тип агентских отношений:", str_pay_agents, ASIZE(str_pay_agents), a->pay_agent)
		FORM_ITEM_EDIT_TEXT(1075, "Тлф. оп. перевода:", a->transfer_operator_phone, FORM_INPUT_TYPE_TEXT, 64)
        FORM_ITEM_EDIT_TEXT(1044, "Оп. агента:", a->pay_agent_operation, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1073, "Тлф. платежного агента:", a->pay_agent_phone, FORM_INPUT_TYPE_TEXT, 19)
		FORM_ITEM_EDIT_TEXT(1174, "Тлф. оп. по приему платежей:", a->payment_processor_phone, FORM_INPUT_TYPE_TEXT, 19)
		FORM_ITEM_EDIT_TEXT(1026, "Оператор перевода:", a->money_transfer_operator_name, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1005, "Адрес оператора перевода:", a->money_transfer_operator_address, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1016, "ИНН оператора перевода:", a->money_transfer_operator_inn, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1171, "Телефон поставщика:", a->supplier_phone, FORM_INPUT_TYPE_TEXT, 19)

		FORM_ITEM_BUTTON(1, "ОК")
		FORM_ITEM_BUTTON(2, "Отмена")
	END_FORM()

	if (process_agent_edit(form, a)) {
		ret = 0;
	}

	form_destroy(form);

	return ret;
}

int remove_agent(data_source_t *ds __attribute__((unused)), void *obj) {
	agent_t *agent = (agent_t *)obj;
	for (list_item_t *li = articles.head; li; li = li->next) {
		article_t *a = LIST_ITEM(li, article_t);
		if (a->pay_agent == agent->n) {
			message_box("Уведомление", 
						"Данный поствщик используется в описании товаров/работ/услуг и не может быть удален."
						"\nСначала необходимо удалить все ссылки на данного поставщика.",
						dlg_yes, 0, al_center);
			return -1;
		}
	}

	return 0;
}

list_t agents = { NULL, NULL, 0, (list_item_delete_func_t)agent_free };

#define AGENTS_FILE_NAME	"/home/sterm/agents.bin"

int agents_load() {
    int fd = s_open(AGENTS_FILE_NAME, false);
	int ret;

    if (fd == -1) {
        return -1;
    }

    if (load_list(fd, &agents, (load_item_func_t)agent_load) < 0)
		ret = -1;
	else
		ret = 0;

	s_close(fd);

	return ret;
}

int agents_destroy() {
	return list_clear(&agents);
}

int agents_save() {
    int fd = s_open(AGENTS_FILE_NAME, true);
	int ret;

    if (fd == -1) {
        return -1;
    }

    if (save_list(fd, &agents, (list_item_func_t)agent_save) < 0)
		ret = -1;
	else
		ret = 0;

	s_close(fd);

	return ret;
}

agent_t* get_agent_by_id(int id) {
	if (id <= 0)
		return NULL;

	for (list_item_t *li = agents.head; li; li = li->next) {
		agent_t *agent = LIST_ITEM(li, agent_t);
		if (agent->n == id)
			return agent;
	}
	return NULL;
}

int get_agent_id_by_index(int index) {
	if (index == 0)
		return -1;

	int n = 1;
	for (list_item_t *li = agents.head; li; li = li->next, n++) {
		agent_t *agent = LIST_ITEM(li, agent_t);
		if (n == index)
			return agent->n;
	}
	return -1;
}

void fa_agents() {

	lvform_column_t columns[] = {
		{ "\xfc", 50 },
		{ "Наименование поставщика", 250 },
		{ "ИНН", 200 },
		{ "Описание", 278 },
	};
	data_source_t ds = {
		&agents,
		agent_get_text,
		create_new_agent,
		edit_agent,
		remove_agent
	};

	lvform_t *lv = lvform_create("Справочник поставщиков", columns, ASIZE(columns),	&ds);
	lvform_execute(lv);
	lvform_destroy(lv);

	agents_save();

	ClearScreen(clBlack);
	draw_menu(fa_sales_menu);
}


