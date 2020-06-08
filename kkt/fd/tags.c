#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "kkt/fd/tags.h"

kkt_tag_t tags[] = {
	{ 1001, "�ਧ��� ��⮬���᪮�� ०���", tag_type_byte },
	{ 1002, "�ਧ��� ��⮭������ ०���", tag_type_byte },
	{ 1005, "���� ������ ��ॢ���", tag_type_string },
	{ 1008, "⥫�䮭 ��� ���஭�� ���� ���㯠⥫�", tag_type_string },
	{ 1009, "���� ���⮢", tag_type_string },
	{ 1012, "���, �६�", tag_type_unixtime },
	{ 1013, "�����᪮� ����� ���", tag_type_string },
	{ 1016, "��� ������ ��ॢ���", tag_type_string },
	{ 1017, "��� ���", tag_type_string },
	{ 1018, "��� ���짮��⥫�", tag_type_string },
	{ 1020, "�㬬� ����, 㪠������� � 祪� (���)", tag_type_vln },
	{ 1021, "�����", tag_type_string },
	{ 1022, "��� �⢥� ���", tag_type_byte },
	{ 1023, "������⢮ �।��� ����", tag_type_fvln },
	{ 1026, "������������ ������ ��ॢ���", tag_type_string },
	{ 1030, "������������", tag_type_string },
	{ 1031, "�㬬� �� 祪� (���) �����묨", tag_type_vln },
	{ 1036, "����� ��⮬��", tag_type_string },
	{ 1037, "ॣ����樮��� ����� ���", tag_type_string },
	{ 1038, "����� ᬥ��", tag_type_uint32 },
	{ 1040, "����� ��", tag_type_uint32 },
	{ 1041, "����� ��", tag_type_string },
	{ 1042, "����� 祪� �� ᬥ��", tag_type_uint32 },
	{ 1043, "�⮨����� �।��� ���� � ��⮬ ᪨��� � ��業��", tag_type_vln },
	{ 1044, "������ ���⥦���� �����", tag_type_string },
	{ 1046, "������������ ���", tag_type_string },
	{ 1048, "������������ ���짮��⥫�", tag_type_string },
	{ 1050, "�ਧ��� ���௠��� ����� ��", tag_type_byte },
	{ 1051, "�ਧ��� ����室����� ��筮� ������ ��", tag_type_byte },
	{ 1052, "�ਧ��� ��९������� ����� ��", tag_type_byte },
	{ 1053, "�ਧ��� �ॢ�襭�� �६��� �������� �⢥� ���", tag_type_byte },
	{ 1054, "�ਧ��� ����", tag_type_byte },
	{ 1055, "�ਬ��塞�� ��⥬� ���������������", tag_type_bits },
	{ 1056, "�ਧ��� ��஢����", tag_type_byte },
	{ 1057, "�ਧ��� �����", tag_type_bits },
	{ 1059, "�।��� ����", tag_type_stlv },
	{ 1060, "���� ᠩ� ���", tag_type_string },
	{ 1062, "��⥬� ���������������", tag_type_byte },
	{ 1068, "ᮮ�饭�� ������ ��� ��", tag_type_stlv },
	{ 1073, "⥫�䮭 ���⥦���� �����", tag_type_string },
	{ 1074, "⥫�䮭 ������ �� �ਥ�� ���⥦��", tag_type_string },
	{ 1075, "⥫�䮭 ������ ��ॢ���", tag_type_string },
	{ 1077, "���", tag_type_bytes },
	{ 1078, "���", tag_type_bytes },
	{ 1079, "業� �� ������� �।��� ���� � ��⮬ ᪨��� � ��業��", tag_type_vln },
	{ 1081, "�㬬� �� 祪� (���) ��������묨", tag_type_vln },
	{ 1084, "�������⥫�� ४����� ���짮��⥫�", tag_type_stlv },
	{ 1085, "������������ �������⥫쭮�� ४����� ���짮��⥫�", tag_type_string },
	{ 1086, "���祭�� �������⥫쭮�� ४����� ���짮��⥫�", tag_type_string },
	{ 1097, "������⢮ ����।����� ��", tag_type_uint32 },
	{ 1098, "��� ��ࢮ�� �� ����।����� ��", tag_type_unixtime },
	{ 1101, "��� ��稭� ���ॣ����樨", tag_type_byte },
	{ 1102, "�㬬� ��� 祪� �� �⠢�� 20%", tag_type_vln },
	{ 1103, "�㬬� ��� 祪� �� �⠢�� 10%", tag_type_vln },
	{ 1104, "�㬬� ���� �� 祪� � ��� �� �⠢�� 0%", tag_type_vln },
	{ 1105, "�㬬� ���� �� 祪� ��� ���", tag_type_vln },
	{ 1106, "�㬬� ��� 祪� �� ���. �⠢�� 20/120", tag_type_vln },
	{ 1107, "�㬬� ��� 祪� �� ���. �⠢�� 10/110", tag_type_vln },
	{ 1108, "�ਧ��� ��� ��� ���⮢ ⮫쪮 � ���୥�", tag_type_byte },
	{ 1109, "�ਧ��� ���⮢ �� ��㣨", tag_type_byte },
	{ 1110, "�ਧ��� �� ���", tag_type_byte },
	{ 1111, "��饥 ������⢮ �� �� ᬥ��", tag_type_uint32 },
	{ 1116, "����� ��ࢮ�� ����।������ ���㬥��", tag_type_uint32 },
	{ 1117, "���� ���஭��� ����� ��ࠢ�⥫� 祪�", tag_type_string },
	{ 1118, "������⢮ ���ᮢ�� 祪�� (���) �� ᬥ��", tag_type_uint32 },
	{ 1126, "�ਧ��� �஢������ ���२", tag_type_byte },
	{ 1129, "���稪� ����権 \"��室\"", tag_type_stlv },
	{ 1130, "���稪� ����権 \"������ ��室�\"", tag_type_stlv },
	{ 1131, "���稪� ����権 \"��室\"", tag_type_stlv },
	{ 1132, "���稪� ����権 \"������ ��室�\"", tag_type_stlv },
	{ 1133, "���稪� ����権 �� 祪�� ���४樨", tag_type_stlv },
	{ 1134, "������⢮ 祪�� (���) � 祪�� ���४樨 (��� ���४樨) � �ᥬ� �ਧ������ ���⮢", tag_type_uint32 },
	{ 1135, "������⢮ 祪�� (���) �� �ਧ���� ���⮢", tag_type_uint32 },
	{ 1136, "�⮣���� �㬬� � 祪�� (���) �����묨 ������묨 �।�⢠��", tag_type_vln },
	{ 1138, "�⮣���� �㬬� � 祪�� (���) ��������묨", tag_type_vln },
	{ 1139, "�㬬� ��� �� �⠢�� 20%", tag_type_vln },
	{ 1140, "�㬬� ��� �� �⠢�� 10%", tag_type_vln },
	{ 1141, "�㬬� ��� �� ���. �⠢�� 20/120", tag_type_vln },
	{ 1142, "�㬬� ��� �� ���. �⠢�� 10/110", tag_type_vln },
	{ 1143, "�㬬� ���⮢ � ��� �� �⠢�� 0%", tag_type_vln },
	{ 1144, "������⢮ 祪�� ���४樨 (��� ���४樨) ��� ����।����� 祪�� (���) � 祪�� ���४樨 (��� ���४樨)", tag_type_uint32 },
	{ 1145, "���稪� �� �ਧ���� \"��室\"", tag_type_stlv },
	{ 1146, "���稪� �� �ਧ���� \"��室\"", tag_type_stlv },
	{ 1148, "������⢮ ᠬ����⥫��� ���४�஢��", tag_type_uint32 },
	{ 1149, "������⢮ ���४�஢�� �� �।��ᠭ��", tag_type_uint32 },
	{ 1151, "�㬬� ���४権 ��� �� �⠢�� 20%", tag_type_vln },
	{ 1152, "�㬬� ���४権 ��� �� �⠢�� 10%", tag_type_vln },
	{ 1153, "�㬬� ���४権 ��� �� ���. �⠢�� 20/120", tag_type_vln },
	{ 1154, "�㬬� ���४権 ��� ���. �⠢�� 10/110", tag_type_vln },
	{ 1155, "�㬬� ���४権 � ��� �� �⠢�� 0%", tag_type_vln },
	{ 1157, "���稪� �⮣�� ��", tag_type_stlv },
	{ 1158, "���稪� �⮣�� ����।����� ��", tag_type_stlv },
	{ 1162, "��� ⮢��", tag_type_bytes },
	{ 1171, "⥫�䮭 ���⠢騪�", tag_type_string },
	{ 1173, "⨯ ���४樨", tag_type_byte },
	{ 1174, "�᭮����� ��� ���४樨", tag_type_stlv },
	{ 1177, "���ᠭ�� ���४樨", tag_type_string },
	{ 1178, "��� ᮢ��襭�� ���४��㥬��� ����", tag_type_unixtime },
	{ 1179, "����� �।��ᠭ�� ���������� �࣠��", tag_type_string },
	{ 1183, "�㬬� ���⮢ ��� ���", tag_type_vln },
	{ 1184, "�㬬� ���४権 ��� ���", tag_type_vln },
	{ 1187, "���� ���⮢", tag_type_string },
	{ 1188, "����� ���", tag_type_string },
	{ 1189, "����� ��� ���", tag_type_byte },
	{ 1190, "����� ��� ��", tag_type_byte },
	{ 1191, "�������⥫�� ४����� �।��� ����", tag_type_string },
	{ 1192, "�������⥫�� ४����� 祪� (���)", tag_type_string },
	{ 1193, "�ਧ��� �஢������ ������� ���", tag_type_byte },
	{ 1194, "���稪� �⮣�� ᬥ��", tag_type_stlv },
	{ 1197, "������ ����७�� �।��� ����", tag_type_string },
	{ 1198, "ࠧ��� ��� �� ������� �।��� ����", tag_type_vln },
	{ 1199, "�⠢�� ���", tag_type_byte },
	{ 1200, "�㬬� ��� �� �।��� ����", tag_type_vln },
	{ 1201, "���� �⮣���� �㬬� � 祪�� (���)", tag_type_vln },
	{ 1203, "��� �����", tag_type_string },
	{ 1205, "���� ��稭 ��������� ᢥ����� � ���", tag_type_bits },
	{ 1206, "ᮮ�饭�� ������", tag_type_bits },
	{ 1207, "�ਧ��� �࣮��� �����樧�묨 ⮢�ࠬ�", tag_type_byte },
	{ 1208, "ᠩ� 祪��", tag_type_string },
	{ 1209, "����� ���ᨨ ���", tag_type_byte },
	{ 1212, "�ਧ��� �।��� ����", tag_type_byte },
	{ 1213, "����� ���祩 ��", tag_type_uint16 },
	{ 1214, "�ਧ��� ᯮᮡ� ����", tag_type_byte },
	{ 1215, "�㬬� �� 祪� (���) �।����⮩ ", tag_type_vln },
	{ 1216, "�㬬� �� 祪� (���) ���⮯��⮩", tag_type_vln },
	{ 1217, "�㬬� �� 祪� (���) ������ �।��⠢������", tag_type_vln },
	{ 1218, "�⮣���� �㬬� � 祪�� (���) �।����⠬� (����ᠬ�)", tag_type_vln },
	{ 1219, "�⮣���� �㬬� � 祪�� (���) ���⮯��⠬� (�।�⠬�)", tag_type_vln },
	{ 1220, "�⮣���� �㬬� � 祪�� (���) �����묨 �।��⠢����ﬨ", tag_type_vln },
	{ 1221, "�ਧ��� ��⠭���� �ਭ�� � ��⮬��", tag_type_byte },
	{ 1222, "�ਧ��� ����� �� �।���� ����", tag_type_bits },
	{ 1223, "����� �����", tag_type_stlv },
	{ 1224, "����� ���⠢騪�", tag_type_stlv },
	{ 1225, "������������ ���⠢騪�", tag_type_string },
	{ 1226, "��� ���⠢騪�", tag_type_string },
	{ 1227, "���㯠⥫� (������)", tag_type_string },
	{ 1228, "��� ���㯠⥫� (������)", tag_type_string },
	{ 1229, "��樧", tag_type_vln },
	{ 1230, "��� ��࠭� �ந�宦����� ⮢��", tag_type_string },
	{ 1231, "����� ⠬������� ������樨", tag_type_string },
	{ 1232, "���稪� �� �ਧ���� \"������ ��室�\"", tag_type_stlv },
	{ 1233, "���稪� �� �ਧ���� \"������ ��室�\"", tag_type_stlv },
};
static kkt_tag_t unknown = { 0, "��������� ��", tag_type_bytes };

const char *tags_get_text(uint16_t tag) {
	kkt_tag_t *t = tags;
	for (int i = 0; i < ASIZE(tags); i++, t++) {
		if (t->tag == tag)
			return t->desc;
	}
	return "��������� ��";
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
				time_t t = *(time_t *)data;
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
