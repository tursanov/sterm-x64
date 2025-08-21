#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "kkt/fd/tags.h"

kkt_tag_t tags[] = {
	{ 1001, "признак автоматического режима", tag_type_byte },
	{ 1002, "признак автономного режима", tag_type_byte },
	{ 1005, "адрес оператора перевода", tag_type_string },
	{ 1008, "телефон или электронный адрес покупателя", tag_type_string },
	{ 1009, "адрес расчетов", tag_type_string },
	{ 1012, "дата, время", tag_type_unixtime },
	{ 1013, "заводской номер ККТ", tag_type_string },
	{ 1016, "ИНН оператора перевода", tag_type_string },
	{ 1017, "ИНН ОФД", tag_type_string },
	{ 1018, "ИНН пользователя", tag_type_string },
	{ 1020, "сумма расчета, указанного в чеке (БСО)", tag_type_vln },
	{ 1021, "кассир", tag_type_string },
	{ 1022, "код ответа ОФД", tag_type_byte },
	{ 1023, "количество предмета расчета", tag_type_fvln },
	{ 1026, "наименование оператора перевода", tag_type_string },
	{ 1030, "наименование", tag_type_string },
	{ 1031, "сумма по чеку (БСО) наличными", tag_type_vln },
	{ 1036, "номер автомата", tag_type_string },
	{ 1037, "регистрационный номер ККТ", tag_type_string },
	{ 1038, "номер смены", tag_type_uint32 },
	{ 1040, "номер ФД", tag_type_uint32 },
	{ 1041, "номер ФН", tag_type_string },
	{ 1042, "номер чека за смену", tag_type_uint32 },
	{ 1043, "стоимость предмета расчета с учетом скидок и наценок", tag_type_vln },
	{ 1044, "операция платежного агента", tag_type_string },
	{ 1046, "наименование ОФД", tag_type_string },
	{ 1048, "наименование пользователя", tag_type_string },
	{ 1050, "признак исчерпания ресурса ФН", tag_type_byte },
	{ 1051, "признак необходимости срочной замены ФН", tag_type_byte },
	{ 1052, "признак переполнения памяти ФН", tag_type_byte },
	{ 1053, "признак превышения времени ожидания ответа ОФД", tag_type_byte },
	{ 1054, "признак расчета", tag_type_byte },
	{ 1055, "применяемая система налогообложения", tag_type_bits },
	{ 1056, "признак шифрования", tag_type_byte },
	{ 1057, "признак агента", tag_type_bits },
	{ 1059, "предмет расчета", tag_type_stlv },
	{ 1060, "адрес сайта ФНС", tag_type_string },
	{ 1062, "системы налогообложения", tag_type_byte },
	{ 1068, "сообщение оператора для ФН", tag_type_stlv },
	{ 1073, "телефон платежного агента", tag_type_string },
	{ 1074, "телефон оператора по приему платежей", tag_type_string },
	{ 1075, "телефон оператора перевода", tag_type_string },
	{ 1077, "ФПД", tag_type_bytes },
	{ 1078, "ФПО", tag_type_bytes },
	{ 1079, "цена за единицу предмета расчета с учетом скидок и наценок", tag_type_vln },
	{ 1081, "сумма по чеку (БСО) безналичными", tag_type_vln },
	{ 1084, "дополнительный реквизит пользователя", tag_type_stlv },
	{ 1085, "наименование дополнительного реквизита пользователя", tag_type_string },
	{ 1086, "значение дополнительного реквизита пользователя", tag_type_string },
	{ 1097, "количество непереданных ФД", tag_type_uint32 },
	{ 1098, "дата первого из непереданных ФД", tag_type_unixtime },
	{ 1101, "код причины перерегистрации", tag_type_byte },
	{ 1102, "сумма НДС чека по ставке 20%", tag_type_vln },
	{ 1103, "сумма НДС чека по ставке 10%", tag_type_vln },
	{ 1104, "сумма расчета по чеку с НДС по ставке 0%", tag_type_vln },
	{ 1105, "сумма расчета по чеку без НДС", tag_type_vln },
	{ 1106, "сумма НДС чека по расч. ставке 20/120", tag_type_vln },
	{ 1107, "сумма НДС чека по расч. ставке 10/110", tag_type_vln },
	{ 1108, "признак ККТ для расчетов только в Интернет", tag_type_byte },
	{ 1109, "признак расчетов за услуги", tag_type_byte },
	{ 1110, "признак АС БСО", tag_type_byte },
	{ 1111, "общее количество ФД за смену", tag_type_uint32 },
	{ 1115, "суммы НДС чека", tag_type_stlv },
	{ 1116, "номер первого непереданного документа", tag_type_uint32 },
	{ 1117, "адрес электронной почты отправителя чека", tag_type_string },
	{ 1118, "количество кассовых чеков (БСО) за смену", tag_type_uint32 },
	{ 1119, "сумма НДС чека", tag_type_stlv },
	{ 1120, "сумма НДС", tag_type_vln },
	{ 1126, "признак проведения лотереи", tag_type_byte },
	{ 1129, "счетчики операций \"приход\"", tag_type_stlv },
	{ 1130, "счетчики операций \"возврат прихода\"", tag_type_stlv },
	{ 1131, "счетчики операций \"расход\"", tag_type_stlv },
	{ 1132, "счетчики операций \"возврат расхода\"", tag_type_stlv },
	{ 1133, "счетчики операций по чекам коррекции", tag_type_stlv },
	{ 1134, "количество чеков (БСО) и чеков коррекции (БСО коррекции) со всеми признаками расчетов", tag_type_uint32 },
	{ 1135, "количество чеков (БСО) по признаку расчетов", tag_type_uint32 },
	{ 1136, "итоговая сумма в чеках (БСО) наличными денежными средствами", tag_type_vln },
	{ 1138, "итоговая сумма в чеках (БСО) безналичными", tag_type_vln },
	{ 1139, "сумма НДС по ставке 20%", tag_type_vln },
	{ 1140, "сумма НДС по ставке 10%", tag_type_vln },
	{ 1141, "сумма НДС по расч. ставке 20/120", tag_type_vln },
	{ 1142, "сумма НДС по расч. ставке 10/110", tag_type_vln },
	{ 1143, "сумма расчетов с НДС по ставке 0%", tag_type_vln },
	{ 1144, "количество чеков коррекции (БСО коррекции) или непереданных чеков (БСО) и чеков коррекции (БСО коррекции)", tag_type_uint32 },
	{ 1145, "счетчики по признаку \"приход\"", tag_type_stlv },
	{ 1146, "счетчики по признаку \"расход\"", tag_type_stlv },
	{ 1148, "количество самостоятельных корректировок", tag_type_uint32 },
	{ 1149, "количество корректировок по предписанию", tag_type_uint32 },
	{ 1151, "сумма коррекций НДС по ставке 20%", tag_type_vln },
	{ 1152, "сумма коррекций НДС по ставке 10%", tag_type_vln },
	{ 1153, "сумма коррекций НДС по расч. ставке 20/120", tag_type_vln },
	{ 1154, "сумма коррекций НДС расч. ставке 10/110", tag_type_vln },
	{ 1155, "сумма коррекций с НДС по ставке 0%", tag_type_vln },
	{ 1157, "счетчики итогов ФН", tag_type_stlv },
	{ 1158, "счетчики итогов непереданных ФД", tag_type_stlv },
	{ 1162, "код товара", tag_type_bytes },
	{ 1171, "телефон поставщика", tag_type_string },
	{ 1173, "тип коррекции", tag_type_byte },
	{ 1174, "основание для коррекции", tag_type_stlv },
	{ 1177, "описание коррекции", tag_type_string },
	{ 1178, "дата совершения корректируемого расчета", tag_type_unixtime },
	{ 1179, "номер предписания налогового органа", tag_type_string },
	{ 1183, "сумма расчетов без НДС", tag_type_vln },
	{ 1184, "сумма коррекций без НДС", tag_type_vln },
	{ 1187, "место расчетов", tag_type_string },
	{ 1188, "версия ККТ", tag_type_string },
	{ 1189, "версия ФФД ККТ", tag_type_byte },
	{ 1190, "версия ФФД ФН", tag_type_byte },
	{ 1191, "дополнительный реквизит предмета расчета", tag_type_string },
	{ 1192, "дополнительный реквизит чека (БСО)", tag_type_string },
	{ 1193, "признак проведения азартных игр", tag_type_byte },
	{ 1194, "счетчики итогов смены", tag_type_stlv },
	{ 1197, "единица измерения предмета расчета", tag_type_string },
	{ 1198, "размер НДС за единицу предмета расчета", tag_type_vln },
	{ 1199, "ставка НДС", tag_type_byte },
	{ 1200, "сумма НДС за предмет расчета", tag_type_vln },
	{ 1201, "общая итоговая сумма в чеках (БСО)", tag_type_vln },
	{ 1203, "ИНН кассира", tag_type_string },
	{ 1205, "коды причин изменения сведений о ККТ", tag_type_bits },
	{ 1206, "сообщение оператора", tag_type_bits },
	{ 1207, "признак торговли подакцизными товарами", tag_type_byte },
	{ 1208, "сайт чеков", tag_type_string },
	{ 1209, "номер версии ФФД", tag_type_byte },
	{ 1212, "признак предмета расчета", tag_type_byte },
	{ 1213, "ресурс ключей ФП", tag_type_uint16 },
	{ 1214, "признак способа расчета", tag_type_byte },
	{ 1215, "сумма по чеку (БСО) предоплатой ", tag_type_vln },
	{ 1216, "сумма по чеку (БСО) постоплатой", tag_type_vln },
	{ 1217, "сумма по чеку (БСО) встречным предоставлением", tag_type_vln },
	{ 1218, "итоговая сумма в чеках (БСО) предоплатами (авансами)", tag_type_vln },
	{ 1219, "итоговая сумма в чеках (БСО) постоплатами (кредитами)", tag_type_vln },
	{ 1220, "итоговая сумма в чеках (БСО) встречными предоставлениями", tag_type_vln },
	{ 1221, "признак установки принтера в автомате", tag_type_byte },
	{ 1222, "признак агента по предмету расчета", tag_type_bits },
	{ 1223, "данные агента", tag_type_stlv },
	{ 1224, "данные поставщика", tag_type_stlv },
	{ 1225, "наименование поставщика", tag_type_string },
	{ 1226, "ИНН поставщика", tag_type_string },
	{ 1227, "покупатель (клиент)", tag_type_string },
	{ 1228, "ИНН покупателя (клиента)", tag_type_string },
	{ 1229, "акциз", tag_type_vln },
	{ 1230, "код страны происхождения товара", tag_type_string },
	{ 1231, "номер таможенной декларации", tag_type_string },
	{ 1232, "счетчики по признаку \"возврат прихода\"", tag_type_stlv },
	{ 1233, "счетчики по признаку \"возврат расхода\"", tag_type_stlv },
};
static kkt_tag_t unknown = { 0, "неизвестный тэг", tag_type_bytes };

const char *tags_get_text(uint16_t tag) {
	kkt_tag_t *t = tags;
	for (int i = 0; i < ASIZE(tags); i++, t++) {
		if (t->tag == tag)
			return t->desc;
	}
	return "неизвестный тэг";
}

tag_type_t tags_get_tlv_text(ffd_tlv_t *tlv, char *text, size_t text_size) {
	kkt_tag_t *t = tags;
	uint8_t *data = (uint8_t *)(tlv + 1);
	size_t i;
	for (i = 0; i < ASIZE(tags); i++, t++) {
		if (t->tag == tlv->tag)
			break;
	}
	if (i == ASIZE(tags))
		t = &unknown;
	size_t l = 0;
	char *s = text;
	l += snprintf(s, text_size - l, "[%.4d] %s: ", tlv->tag, t->desc);
	s += l;

	if (t->tag == 1077) {
		uint32_t v = 
			((uint32_t)data[5] << 0) |
			((uint32_t)data[4] << 8) |
			((uint32_t)data[3] << 16) |
			((uint32_t)data[2] << 24);
		snprintf(s, text_size - l, "%u", v);
		return tag_type_bytes;
	}


	switch (t->type) {
		case tag_type_byte:
			snprintf(s, text_size - l, "%u", *(uint8_t *)data);
			break;
		case tag_type_uint16:
			snprintf(s, text_size - l, "%u", *(uint16_t *)data);
			break;
		case tag_type_uint32:
			snprintf(s, text_size - l, "%u", *(uint32_t *)data);
			break;
		case tag_type_unixtime:
			{ 
				time_t t = (time_t)*(int32_t *)data;
				struct tm tm = *gmtime(&t);
				strftime(s, text_size - l, "%d.%m.%Y %H:%M", &tm);
			}
			break;
		case tag_type_string:
			snprintf(s, text_size - l, "%.*s", tlv->length, (const char *)data);
			break;
		case tag_type_vln:
			{
				uint64_t v = 0;
				int shift = 0;
				uint8_t *p = data;
				for (i = 0; i < tlv->length; i++, p++, shift += 8)
					v |= (uint64_t)*p << shift;
				snprintf(s, text_size - l, "%.1llu.%.2llu", v / 100LLU, v % 100LLU);
			}
			break;
		case tag_type_bits:
			{
				uint32_t v;
				if (tlv->length == 1)
					v = *data;
				else
					v = *(uint32_t *)data;
				snprintf(s, text_size - l, "%d", v);
			}
			break;
		case tag_type_fvln:
			{
				double f;
				uint64_t v = 0;
				int shift = 0;
				uint8_t *p = data + 1;
				for (i = 0; i < tlv->length - 1; i++, p++, shift += 8)
					v |= (uint64_t)*p << shift;

				f = (double)v;
				for (i = 0; i < *data; i++)
					f /= 10;
				snprintf(s, text_size - l, "%1.3f", f);
			}
		case tag_type_stlv:
			break;
		case tag_type_bytes:
		default:
			{
				char tmp[tlv->length*3 + 1];
				char *ss = tmp;
				uint8_t *p = data;
				for (i = 0; i < tlv->length; i++, p++, ss += 3)
					sprintf(ss, "%.2X ", *p);
				*ss = 0;
				snprintf(s, text_size - l, "%s", tmp);
			}
			break;
	}

	return t->type;
}

void dump_stlv(uint8_t *p, size_t size, int level) {
	char text[2048];
	char tmp[8] = {0};

	memset(tmp, ' ', level);
	tmp[level] = 0;

	size_t i = 0;
	while (i < size) {
		ffd_tlv_t *t = (ffd_tlv_t *)p;

		tag_type_t type = tags_get_tlv_text(t, text, sizeof(text));
		printf("%s%s\n", tmp, text);

		if (type == tag_type_stlv)
			dump_stlv(p + 4, t->length, level + 1);

		size_t l = t->length + sizeof(*t);
		i += l;
		p += l;
	}
}

void dump_current_tlv()
{
    uint8_t *tlv = ffd_tlv_data();
    size_t tlv_size = ffd_tlv_size();
    
    dump_stlv(tlv, tlv_size, 0);
}

