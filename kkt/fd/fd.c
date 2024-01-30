#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sysdefs.h"
#include "kkt/fd/tlv.h"
#include "kkt/fd/fd.h"
#include "kkt/kkt.h"
#include "kkt/fd/tags.h"
#include "kkt/fdo.h"
#include <limits.h>

static char last_error[1024] = { 0 };

#define DIR_NAME "/home/sterm/patterns"

uint8_t *load_pattern(uint8_t doc_type, const uint8_t *pattern_footer,
	size_t pattern_footer_size, size_t *pattern_size)
{
#define PATTERN_FORMAT DIR_NAME "/%s"
	char file_name[PATH_MAX];
	FILE *f;
	int ret;
	long file_size;
	uint8_t *pattern = NULL;

	switch (doc_type) {
		case REGISTRATION:
		case RE_REGISTRATION:
			sprintf(file_name, PATTERN_FORMAT, "registration_pattern.dat");
			break;
		case OPEN_SHIFT:
			sprintf(file_name, PATTERN_FORMAT, "open_shift_pattern.dat");
			break;
		case CALC_REPORT:
			sprintf(file_name, PATTERN_FORMAT, "calc_report_pattern.dat");
			break;
		case CHEQUE:
		case BSO:
			sprintf(file_name, PATTERN_FORMAT, "cheque_pattern.dat");
			break;
		case CHEQUE_CORR:
		case BSO_CORR:
			sprintf(file_name, PATTERN_FORMAT, "cheque_corr_pattern.dat");
			break;
		case CLOSE_SHIFT:
			sprintf(file_name, PATTERN_FORMAT, "close_shift_pattern.dat");
			break;
		case CLOSE_FS:
			sprintf(file_name, PATTERN_FORMAT, "close_fs_pattern.dat");
			break;
	}

	printf("file_name: %s\n", file_name);

	if ((f = fopen(file_name, "rb")) == NULL) {
		perror("fopen");
		sprintf(last_error, "�訡�� ������ 䠩�� 蠡���� (%d)", errno);
		return NULL;
	}

	if ((ret = fseek(f, 0, SEEK_END)) != 0) {
		perror("fseek end");
		sprintf(last_error, "�訡�� ����樮��஢���� 䠩�� 蠡���� (%d)", errno);
		goto LOut;
	}
	file_size = ftell(f);
	if (file_size < 0) {
		perror("ftell");
		sprintf(last_error, "�訡�� ����祭�� ����� 䠩�� 蠡���� (%d)", errno);
		goto LOut;
	}
	if ((ret = fseek(f, 0, SEEK_SET)) != 0) {
		sprintf(last_error, "�訡�� ����祭�� ����� 䠩�� 蠡���� (%d)", errno);
		perror("fseek set");
		goto LOut;
	}
	printf("file_size: %ld\n", file_size);
	if ((pattern = (uint8_t *)malloc(file_size + pattern_footer_size + 1)) == NULL) {
		sprintf(last_error, "�訡�� �뤥����� ����� ��� 蠡���� (%d)", errno);
		perror("malloc");
		goto LOut;
	}

	if ((ret = fread(pattern, file_size, 1, f) != 1)) {
		sprintf(last_error, "�訡�� �⥭�� 䠩�� 蠡���� (%d)", errno);
		perror("read");
		free(pattern);
		pattern = NULL;
		goto LOut;
	}
	memcpy(pattern + file_size, pattern_footer, pattern_footer_size);
	*pattern_size = (size_t)file_size + pattern_footer_size;

LOut:
	fclose(f);
	return pattern;
}

static void set_fn_error(char *s, uint8_t *err_info, size_t err_info_len __attribute__((unused)))
{
	s += sprintf(s, "\n������� ��: %.2x, ��� �訡��: %.2x\n", err_info[0], err_info[1]);
    switch (err_info[1]) {
        case 0x00: // FS_RET_SUCCESS
            sprintf(s, "%s", "��� �訡��");
			break;
        case 0x01: // FS_RET_UNKNOWN_CMD_OR_ARGS
            sprintf(s, "%s", "�������⭠� �������, ������ �ଠ� ���뫪� ��� ��������� ��ࠬ����.");
			break;
        case 0x02: // FS_RET_INVAL_STATE
            sprintf(s, "%s", "����୮� ���ﭨ� ��");
			break;
        case 0x03: // FS_RET_FS_ERROR
            sprintf(s, "%s", "�訡�� ��");
			break;
        case 0x04: // FS_RET_CC_ERROR
            sprintf(s, "%s", "�訡�� �� (�ਯ⮣���᪨� ᮯ�����)");
			break;
        case 0x05: // FS_RET_LIFETIME_OVER
            sprintf(s, "%s", "�����祭 �ப �ᯫ��樨 ��");
			break;
        case 0x06: // FS_RET_ARCHIVE_OVERFLOW
            sprintf(s, "%s", "��娢 �� ��९�����");
			break;
        case 0x07: // FS_RET_INVALID_DATETIME
            sprintf(s, "%s", "������ ��� �/��� �६�");
			break;
        case 0x08: // FS_RET_NO_DATA
            sprintf(s, "%s", "����襭�� ����� ���������� � ��娢� ��");
			break;
        case 0x09: // FS_RET_INVAL_ARG_VALUE
            sprintf(s, "%s", "��ࠬ���� ������� ����� �ࠢ���� �ଠ�, �� �� ���祭�� �� ��୮");
			break;
        case 0x10: // FS_RET_TLV_OVERSIZE
            sprintf(s, "%s", "������ ��।������� TLV ������ �ॢ�ᨫ �����⨬�");
			break;
        case 0x0a: // FS_RET_INVALID_CMD
            sprintf(s, "%s", "�����४⭠� �������.");
			break;
        case 0x0b: // FS_RET_ILLEGAL_ATTR
            sprintf(s, "%s", "��ࠧ�襭�� ४������.");
			break;
        case 0x0c: // FS_RET_DUP_ATTR
            sprintf(s, "%s", "�㡫�஢���� ������	��� ��।��� �����, ����� 㦥 �뫨 ��।��� � ��⠢� ������� ���㬥��.");
			break;
        case 0x0d: // FS_RET_MISS_ATTR
            sprintf(s, "%s", "���������� �����, ����室��� ��� ���४⭮�� ��� � ��.");
			break;
        case 0x0e: // FS_RET_POS_OVERFLOW
            sprintf(s, "%s", "������⢮ ����権 � ���㬥��, �ॢ�ᨫ� �����⨬� �।��.");
			break;
        case 0x11: // FS_RET_NO_TRANSPORT
            sprintf(s, "%s", "��� �࠭ᯮ�⭮�� ᮥ�������");
			break;
        case 0x12: // FS_RET_CC_OUT
            sprintf(s, "%s", "���௠� ����� �� (�ਯ⮣���᪮�� ᮯ�����)");
			break;
        case 0x14: // FS_RET_ARCHIVE_OUT
            sprintf(s, "%s", "���௠� ����� �࠭����");
			break;
        case 0x15: // FS_RET_MSG_SEND_TIMEOUT
            sprintf(s, "%s", "���௠� ����� �������� ��।�� ᮮ�饭��");
			break;
        case 0x16: // FS_RET_SHIFT_TIMEOUT
            sprintf(s, "%s", "�த����⥫쭮��� ᬥ�� ����� 24 �ᮢ");
			break;
        case 0x17: // FS_RET_WRONG_PERIOD
            sprintf(s, "%s", "����ୠ� ࠧ��� �� �६��� ����� 2 �����ﬨ");
			break;
        case 0x18: // FS_RET_INVALID_ATTR
            sprintf(s, "%s", "�����४�� ४�����, ��।���� ��� � ��	��������, ��।���� ��� � ��, �� ᮮ⢥����� ��⠭������� �ॡ������.");
			break;
        case 0x19: // FS_RET_INVALID_ATTR_EXISABLE
            sprintf(s, "%s", "�����४�� ४����� � �ਧ����� �த��� �����樧���� ⮢��.");
			break;
        case 0x20: // FS_RET_MSG_NOT_ACCEPTED
            sprintf(s, "%s", "����饭�� �� ��� �� ����� ���� �ਭ��");
			break;
        case 0x21: // FS_RET_INVALID_RESP_DATA_SIZE
            sprintf(s, "%s", "����� ������ � �⢥� ����� ���������");
			break;
        case 0x22: // FS_RET_INVAL
            sprintf(s, "%s", "���ࠢ��쭮� ���祭�� ������ � �������");
			break;
        case 0x23: // FS_RET_TRANSPORT_ERROR
            sprintf(s, "%s", "�訡�� �� ��।�� ������ � ��");
			break;
        case 0x24: // FS_RET_SEND_TIMEOUT
            sprintf(s, "%s", "������� ��।�� � ��");
			break;
        case 0x25: // FS_RET_RECV_TIMEOUT
            sprintf(s, "%s", "������� �ਥ�� �� ��");
			break;
        case 0x26: // FS_RET_CRC_ERROR
            sprintf(s, "%s", "�訡�� CRC");
			break;
        case 0x27: // FS_RET_WRITE_ERROR
            sprintf(s, "%s", "�訡�� ����� � ��");
			break;
        case 0x29: // FS_RET_READ_ERROR
            sprintf(s, "%s", "�訡�� �ਥ�� �� ��");
			break;
        default:
            sprintf(s, "%s (0x%.2x)", "�������⭠� �訡��", err_info[1]);
			break;
    }
}

static void set_tag_error(uint8_t doc_type, char *s, uint8_t *err_info,
		size_t err_info_len __attribute__((unused))) {
	uint16_t tag = *(uint16_t *)err_info;
	if ((doc_type == 31 || doc_type == 41) && tag == 1102 && err_info[2] == 0x0c) {
		s += sprintf(s, "\n��� ������� ���㬥�� ����室��� ��������� ��� �� ���� �� ���祭�� ���");
		return;
	} else
	    s += sprintf(s, "\n�� %.4d (%s)\n", tag, tags_get_text(tag));
    switch (err_info[2]) {
        case 0x01: // ERR_TAG_UNKNOWN
            sprintf(s, "%s", "��������� ��� ������� ���㬥�� TLV");
            break;
        case 0x02: // ERR_TAG_NOT_FOR_INPUT
            sprintf(s, "%s", "����� TLV ���������� ��� � �� ������ ��।�������");
            break;
        case 0x03: // ERR_TAG_STLV_FIELDS_NOT_FILLED
            sprintf(s, "%s", "�� �� ���� � STLV ���������");
            break;
        case 0x04: // ERR_TAG_EMPTY
            sprintf(s, "%s", "TLV �� �������� (�㫥��� ����� ������)");
            break;
        case 0x05: // ERR_TAG_TOO_LONG
            sprintf(s, "%s", "����� TLV �ॢ�蠥� �����⨬��");
            break;
        case 0x06: // ERR_TAG_LENGTH
            sprintf(s, "%s", "���ࠢ��쭠� ����� TLV (��� ������� TLV ����� 䨪�஢���)");
            break;
        case 0x07: // ERR_TAG_CONTENT
            sprintf(s, "%s", "���ࠢ��쭮� ᮤ�ন��� TLV");
            break;
        case 0x08: // ERR_TAG_METADATA
            sprintf(s, "%s", "���ࠢ��쭮� ᮤ�ন��� � ��⠤�����");
            break;
        case 0x09: // ERR_TAG_NOT_IN_RANGE
            sprintf(s, "%s", "���祭�� TLV ��室�� �� ࠧ�襭�� ��������");
            break;
        case 0x0a: // ERR_TAG_SAVE
            sprintf(s, "%s", "�訡�� �� ����� TLV � ���� ��� ��ࠢ�� � ��");
            break;
        case 0x0b: // ERR_TAG_EXTRA
            sprintf(s, "%s", "�������⨬� ��� ������� ���㬥�� TLV");
            break;
        case 0x0c: // ERR_TAG_MISS
	        sprintf(s, "%s", "����室��� ��� ������� ���㬥�� TLV �� �� ��।��");
            break;
        case 0x0d: // ERR_TAG_INVALID_VALUE
            sprintf(s, "%s", "���ࠢ��쭮� ���祭�� TLV");
            break;
        case 0x0e: // ERR_TAG_DUPLICATE
            sprintf(s, "%s", "�㡫�஢���� ��, ����� �� ����� ���� �������騬��");
            break;
        case 0x20: // ERR_TAG_BEGIN_FB
            sprintf(s, "%s", "�訡�� �� ��ࠡ�⪥ ��砫� �᪠�쭮�� ����� � 蠡���� ��� ����");
            break;
        case 0x21: // ERR_TAG_END_FB
            sprintf(s, "%s", "�訡�� �� ��ࠡ�⪥ ����砭�� �᪠�쭮�� ����� � 蠡���� ��� ����");
            break;
        case 0x22: // ERR_TAG_TLV_HDR
            sprintf(s, "%s", "�訡�� �� ��ࠡ�⪥ ��������� TLV � 蠡���� ��� ����");
            break;
        case 0x23: // ERR_TAG_TLV_VALUE
            sprintf(s, "%s", "�訡�� �� ��ࠡ�⪥ ���祭�� TLV � 蠡���� ��� ����");
            break;
    }
}

void fd_set_error(uint8_t doc_type, uint8_t status, uint8_t *err_info, size_t err_info_len) 
{
	char *s = last_error;


	s += sprintf(s, "��� �訡��: �:%.3d", status);
	if (err_info_len > 0) {
		s += sprintf(s, " (");

		for (int i = 0; i < err_info_len; i++) {
			if (i > 0)
				s += sprintf(s, "%s", ", ");
			s += sprintf(s, "%d", err_info[i]);
		}

		s += sprintf(s, ")");
	}


	s += sprintf(s, "\n");


	switch (status) {
		case 0x00: // STATUS_OK
		case 0x30:
			sprintf(s, "%s", "��� �訡��");
			break;
		case 0x01: // FS_RET_UNKNOWN_CMD_OR_ARGS
            sprintf(s, "%s", "�������⭠� �������, ������ �ଠ� ���뫪� ��� ��������� ��ࠬ����.");
			break;
        case 0x02: // FS_RET_INVAL_STATE
            sprintf(s, "%s", "����୮� ���ﭨ� ��");
			break;
        case 0x03: // FS_RET_FS_ERROR
            sprintf(s, "%s", "�訡�� ��");
			break;
        case 0x04: // FS_RET_CC_ERROR
            sprintf(s, "%s", "�訡�� �� (�ਯ⮣���᪨� ᮯ�����)");
			break;
        case 0x05: // FS_RET_LIFETIME_OVER
            sprintf(s, "%s", "�����祭 �ப �ᯫ��樨 ��");
			break;
        case 0x06: // FS_RET_ARCHIVE_OVERFLOW
            sprintf(s, "%s", "��娢 �� ��९�����");
			break;
        case 0x07: // FS_RET_INVALID_DATETIME
            sprintf(s, "%s", "������ ��� �/��� �६�");
			break;
        case 0x08: // FS_RET_NO_DATA
            sprintf(s, "%s", "����襭�� ����� ���������� � ��娢� ��");
			break;
        case 0x09: // FS_RET_INVAL_ARG_VALUE
            sprintf(s, "%s", "��ࠬ���� ������� ����� �ࠢ���� �ଠ�, �� �� ���祭�� �� ��୮");
			break;
        case 0x10: // FS_RET_TLV_OVERSIZE
            sprintf(s, "%s", "������ ��।������� TLV ������ �ॢ�ᨫ �����⨬�");
			break;
        case 0x0a: // FS_RET_INVALID_CMD
            sprintf(s, "%s", "�����४⭠� �������.");
			break;
        case 0x0b: // FS_RET_ILLEGAL_ATTR
            sprintf(s, "%s", "��ࠧ�襭�� ४������.");
			break;
        case 0x0c: // FS_RET_DUP_ATTR
            sprintf(s, "%s", "�㡫�஢���� ������	��� ��।��� �����, ����� 㦥 �뫨 ��।��� � ��⠢� ������� ���㬥��.");
			break;
        case 0x0d: // FS_RET_MISS_ATTR
            sprintf(s, "%s", "���������� �����, ����室��� ��� ���४⭮�� ��� � ��.");
			break;
        case 0x0e: // FS_RET_POS_OVERFLOW
            sprintf(s, "%s", "������⢮ ����権 � ���㬥��, �ॢ�ᨫ� �����⨬� �।��.");
			break;
        case 0x11: // FS_RET_NO_TRANSPORT
            sprintf(s, "%s", "��� �࠭ᯮ�⭮�� ᮥ�������");
			break;
        case 0x12: // FS_RET_CC_OUT
            sprintf(s, "%s", "���௠� ����� �� (�ਯ⮣���᪮�� ᮯ�����)");
			break;
        case 0x14: // FS_RET_ARCHIVE_OUT
            sprintf(s, "%s", "���௠� ����� �࠭����");
			break;
        case 0x15: // FS_RET_MSG_SEND_TIMEOUT
            sprintf(s, "%s", "���௠� ����� �������� ��।�� ᮮ�饭��");
			break;
        case 0x16: // FS_RET_SHIFT_TIMEOUT
            sprintf(s, "%s", "�த����⥫쭮��� ᬥ�� ����� 24 �ᮢ");
			break;
        case 0x17: // FS_RET_WRONG_PERIOD
            sprintf(s, "%s", "����ୠ� ࠧ��� �� �६��� ����� 2 �����ﬨ");
			break;
        case 0x18: // FS_RET_INVALID_ATTR
            sprintf(s, "%s", "�����४�� ४�����, ��।���� ��� � ��	��������, ��।���� ��� � ��, �� ᮮ⢥����� ��⠭������� �ॡ������.");
			break;
        case 0x19: // FS_RET_INVALID_ATTR_EXISABLE
            sprintf(s, "%s", "�����४�� ४����� � �ਧ����� �த��� �����樧���� ⮢��.");
			break;
        case 0x20: // FS_RET_MSG_NOT_ACCEPTED
            sprintf(s, "%s", "����饭�� �� ��� �� ����� ���� �ਭ��");
			break;
        case 0x21: // FS_RET_INVALID_RESP_DATA_SIZE
            sprintf(s, "%s", "����� ������ � �⢥� ����� ���������");
			break;
        case 0x22: // FS_RET_INVAL
            sprintf(s, "%s", "���ࠢ��쭮� ���祭�� ������ � �������");
			break;
        case 0x23: // FS_RET_TRANSPORT_ERROR
            sprintf(s, "%s", "�訡�� �� ��।�� ������ � ��");
			break;
        case 0x24: // FS_RET_SEND_TIMEOUT
            sprintf(s, "%s", "������� ��।�� � ��");
			break;
        case 0x25: // FS_RET_RECV_TIMEOUT
            sprintf(s, "%s", "������� �ਥ�� �� ��");
			break;
        case 0x26: // FS_RET_CRC_ERROR
            sprintf(s, "%s", "�訡�� CRC");
			break;
        case 0x27: // FS_RET_WRITE_ERROR
            sprintf(s, "%s", "�訡�� ����� � ��");
			break;
        case 0x29: // FS_RET_READ_ERROR
            sprintf(s, "%s", "�訡�� �ਥ�� �� ��");
			break;		
		case 0x41: // STATUS_PAPER_END
			sprintf(s, "%s", "����� �㬠��");
			break;
		case 0x42: // STATUS_COVER_OPEN
			sprintf(s, "%s", "���誠 �����");
			break;
		case 0x43: // STATUS_PAPER_LOCK
			sprintf(s, "%s", "�㬠�� �����﫠 �� ��室�");
			break;
		case 0x44: // STATUS_PAPER_WRACK
			sprintf(s, "%s", "�㬠�� ���﫠��");
			break;
		case 0x45: // STATUS_FS_ERR
			s += sprintf(s, "%s", "���� �����⭠� �訡�� ��");
			if (err_info_len > 0)
				set_fn_error(s, err_info, err_info_len);
			break;
		case 0x46: // STATUS_LAST_UNPRINTED
			sprintf(s, "%s", "��᫥���� ��ନ஢���� ���㬥�� �� �⯥�⠭");
			break;
		case 0x48: // STATUS_CUT_ERR
			sprintf(s, "%s", "�訡�� ��१�� �㬠��");
			break;
		case 0x49: // STATUS_INIT
			sprintf(s, "%s", "�� ��室���� � ���ﭨ� ���樠����樨");
			break;
		case 0x4a: // STATUS_THERMHEAD_ERR
			sprintf(s, "%s", "��������� �ମ�������");
			break;
		case 0x4d: // STATUS_PREV_INCOMPLETE
			sprintf(s, "%s", "�।���� ������� �뫠 �ਭ�� �� ���������");
			break;
		case 0x4e: // STATUS_CRC_ERR
			sprintf(s, "%s", "�।���� ������� �뫠 �ਭ�� � �訡��� ����஫쭮� �㬬�");
			break;
		case 0x4f: // STATUS_HW_ERR
			sprintf(s, "%s", "���� �����⭠� �訡�� ��");
			break;
		case 0x50: // STATUS_NO_FFEED
			sprintf(s, "%s", "��� ������� ��१�� ������");
			break;
		case 0x51: // STATUS_VPOS_OVER
			sprintf(s, "%s", "�ॢ�襭�� ���� ⥪�� �� ���⨪���� ������");
			break;
		case 0x52: // STATUS_HPOS_OVER
			sprintf(s, "%s", "�ॢ�襭�� ���� ⥪�� �� ��ਧ��⠫�� ������");
			break;
		case 0x53: // STATUS_LOG_ERR
			sprintf(s, "%s", "����襭�� �������� ���ଠ樨 �� ���� ��");
			break;
		case 0x55: // STATUS_GRID_ERROR
			sprintf(s, "%s", "����襭�� ��ࠬ��஢ ����ᥭ�� ����⮢");
			break;
		case 0x70: // STATUS_BCODE_PARAM
			sprintf(s, "%s", "����襭�� ��ࠬ��஢ ����ᥭ�� ����-����");
			break;
		case 0x71: // STATUS_NO_ICON
			sprintf(s, "%s", "���⮣ࠬ�� �� �������");
			break;
		case 0x72: // STATUS_GRID_WIDTH
			sprintf(s, "%s", "��ਭ� �⪨ ����� �ਭ� ������ � ��⠭�����");
			break;
		case 0x73: // STATUS_GRID_HEIGHT
			sprintf(s, "%s", "���� �⪨ ����� ����� ������ � ��⠭�����");
			break;
		case 0x74: // STATUS_GRID_NM_FMT
			sprintf(s, "%s", "���ࠢ���� �ଠ� ����� �⪨");
			break;
		case 0x75: // STATUS_GRID_NM_LEN
			sprintf(s, "%s", "����� ����� �⪨ ����� �����⨬��");
			break;
		case 0x76: // STATUS_GRID_NR
			sprintf(s, "%s", "������ �ଠ� ����� �⪨");
			break;
		case 0x77: // STATUS_INVALID_ARG
			sprintf(s, "%s", "������ ��ࠬ���");
			break;
		case 0x80: // STATUS_INVALID_TAG
			s += sprintf(s, "%s", "�訡�� � TLV ");
            if (err_info_len > 0)
                set_tag_error(doc_type, s, err_info, err_info_len);
			break;
		case 0x81: // STATUS_GET_TIME
			sprintf(s, "%s", "�訡�� �� ����祭�� ����/�६���");
			break;
		case 0x82: // STATUS_SEND_DOC
			sprintf(s, "%s", "�訡�� �� ��।�� ������ � ��");
			break;
		case 0x83: // STATUS_TLV_OVERSIZE
			sprintf(s, "%s", "��室 �� �࠭��� ��饩 ����� TLV-������");
			break;
		case 0x84: // STATUS_INVALID_STATE
			sprintf(s, "%s", "���ࠢ��쭮� ���ﭨ�");
			break;
		case 0x85: // STATUS_FS_REPLACED
			sprintf(s, "%s", "����� �ନ஢��� �� �� ��㣮� �� "
				"(����᪠���� �ନ஢���� �� ⮫쪮 �� ⮬ ��, "
				"� ����� �뫠 �஢����� ��楤�� ॣ����樨 "
				"(���ॣ����樨 � �裡 � ������� ��)");
			break;
		case 0x86: // STATUS_SHIFT_NOT_CLOSED
			sprintf(s, "%s", "����� �� ������");
			break;
		case 0x87: // STATUS_SHIFT_NOT_OPENED
			sprintf(s, "%s", "����� �� �����");
			break;
		case 0x88: // STATUS_NOMEM
			sprintf(s, "%s", "�� 墠⠥� ����� ��� �����襭�� ����樨");
			break;
		case 0x89: // STATUS_SAVE_REG_DATA
			sprintf(s, "%s", "�訡�� ����� ॣ����樮���� ������");
			break;
		case 0x8a: // STATUS_READ_REG_DATA
			sprintf(s, "%s", "�訡�� �⥭�� ॣ����樮���� ������");
			break;
		case 0x8b: // STATUS_INVALID_INPUT
			sprintf(s, "%s", "���ࠢ���� ᨬ��� � 蠡����");
			break;
		case 0x8c: // STATUS_INVALID_TAG_IN_PATTERN
			s += sprintf(s, "%s", "���ࠢ���� �� � 蠡���� ");
            if (err_info_len > 0)
                set_tag_error(doc_type, s, err_info, err_info_len);
			break;
		case 0x8d: // STATUS_SHIFT_ALREADY_CLOSED
			sprintf(s, "%s", "����� 㦥 ������");
			break;
		case 0x8e: // STATUS_SHIFT_ALREADY_OPENED
			sprintf(s, "%s", "����� 㦥 �����");
			break;
		case 0x8f: // STATUS_NOT_ALL_DATA_SENDED
			sprintf(s, "%s", "�� �� ����� ��।��� � ���");
			break;
		case 0x90: // STATUS_SHIFT_NEED_CLOSED
			sprintf(s, "%s", "��� ���������� ����室��� ������� ᬥ��");
			break;
		case 0xf0:
			sprintf(s, "%s", "��९������� ���� ��� ��।��");
			break;
		case 0xf1: // STATUS_TIMEOUT
			sprintf(s, "%s", "������� �ਥ�� �⢥� �� ��");
			break;
		case 0xf2:
			sprintf(s, "%s", "��� �� �����㦥��. �������� ���");
			break;
		case 0xf3:
			sprintf(s, "%s", "������ �ଠ� �⢥�");
			break;
		default:
			sprintf(s, "%s (0x%.2x)", "�������⭠� �訡��", status);
			break;
	}
}

int fd_create_doc(uint8_t doc_type, const uint8_t *pattern_footer, size_t pattern_footer_size)
{
	uint8_t ret;
	_Alignas(_Alignof(uint16_t)) uint8_t err_info[32];
	size_t err_info_len;
	uint8_t *pattern;
	size_t pattern_size;
	struct kkt_doc_info di;

	fdo_suspend();

	err_info_len = sizeof(err_info);
	if ((ret = kkt_begin_doc(doc_type, err_info, &err_info_len)) != 0) {
		fd_set_error(doc_type, ret, err_info, err_info_len);
		printf("kkt_begin_doc->ret = %.2x\n", ret);
		fdo_resume();
		return ret;
	}
	
	uint32_t timeout_factor = 50;

	size_t tlv_size = ffd_tlv_size();
	printf("tlv_size = %zu\n", tlv_size);
	if (tlv_size > 0) {
		uint8_t *tlv_data = ffd_tlv_data();
		const ffd_tlv_t *tlv = (const ffd_tlv_t *)tlv_data;
		const ffd_tlv_t *end = (const ffd_tlv_t *)(tlv_data + tlv_size);
		size_t tlv_buf_size = 0;
		uint8_t *tlv_buf = tlv_data;
	#define MAX_SEND_SIZE	4096

		int i = 0;
		while (tlv <= end) {
		
			if (tlv == end || tlv_buf_size + FFD_TLV_SIZE(tlv) > MAX_SEND_SIZE) {
				err_info_len = sizeof(err_info);

				printf("tlv_buf_size = %zu\n", tlv_buf_size);

				if ((ret = kkt_send_doc_data(tlv_buf, tlv_buf_size, err_info, &err_info_len)) != 0) {
					fd_set_error(doc_type, ret, err_info, err_info_len);
					printf("i = %d, kkt_send_doc_data->ret = %.2x\n", i, ret);

					if (ret == 0x80 || ret == 0x8c) {
						if (err_info_len > 0) {
#if defined __GNUC__ && (__GNUC__ < 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
							uint16_t tag = *(uint16_t *)err_info;
#if defined __GNUC__ && (__GNUC__ < 8)
#pragma GCC diagnostic pop
#endif
							uint8_t ex_err = err_info[2];

							printf("error in tag %.4d -> %.2x\n", tag, ex_err);
						}
					}

					fdo_resume();
					return ret;
				}
				i++;

				if (tlv == end)
					break;

				tlv_buf_size = 0;
				tlv_buf = (uint8_t *)tlv;
			}

/*			printf("tlv->tag: %d\n", tlv->tag);
			printf("tlv->length: %d\n", tlv->length);
			printf("  asString: %.*s\n", tlv->length, FFD_TLV_DATA_AS_STRING(tlv));
			printf("  as8: %d\n", FFD_TLV_DATA_AS_UINT8(tlv));
			if (tlv->length > 1)
				printf("  as16: %d\n", FFD_TLV_DATA_AS_UINT16(tlv));
			if (tlv->length > 3)
				printf("  as32: %d\n", FFD_TLV_DATA_AS_UINT32(tlv));*/

			if (tlv->tag == 1059)
				timeout_factor += 3;

			tlv_buf_size += FFD_TLV_SIZE(tlv);
			tlv = FFD_TLV_NEXT(tlv);
		}

	}
	
	printf("timeout_factor: %u\n", timeout_factor);
	
	printf("------------------------------------\n");

   	if ((pattern = load_pattern(doc_type, pattern_footer,
				   	pattern_footer_size, &pattern_size)) == NULL) {
		printf("load_pattern fail\n");
		fdo_resume();
		return -1;
	}

	err_info_len = sizeof(err_info);
	if ((ret = kkt_end_doc(doc_type, pattern, pattern_size, timeout_factor,
			&di, err_info, &err_info_len)) != 0) {
		fd_set_error(doc_type, ret, err_info, err_info_len);
		printf("kkt_end_doc->ret = %.2x, err_info_len = %zu\n", ret, err_info_len);

		if (ret == 0x80 || ret == 0x8c) {
			if (err_info_len > 0) {
#if defined __GNUC__ && (__GNUC__ < 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
				uint16_t tag = *(uint16_t *)err_info;
#if defined __GNUC__ && (__GNUC__ < 8)
#pragma GCC diagnostic pop
#endif
				uint8_t ex_err = err_info[2];

				printf("error in tag %.4d -> %.2x\n", tag, ex_err);
			}
		}

		goto LOut;
	}

LOut:
	free(pattern);
	fdo_resume();

	return ret;
}

int fd_get_last_error(const char **error) {
	*error = last_error;
	return 0;
}

int fd_print_last_doc(uint8_t doc_type) {
	uint8_t ret;
	_Alignas(_Alignof(uint16_t)) uint8_t err_info[32];
	size_t err_info_len;
	uint8_t *pattern;
	size_t pattern_size;
	struct kkt_last_printed_info lpi;
	uint8_t *pattern_footer = NULL;
	size_t pattern_footer_size = 0;

   	if ((pattern = load_pattern(doc_type, pattern_footer,
				   	pattern_footer_size, &pattern_size)) == NULL) {
		printf("load_pattern fail\n");
		return -1;
	}

	err_info_len = sizeof(err_info);
	
	fdo_suspend();

	if ((ret = kkt_print_last_doc(doc_type, pattern, pattern_size, &lpi,
					err_info, &err_info_len)) != 0) {
		fd_set_error(doc_type, ret, err_info, err_info_len);

		if (ret == 0x80 || ret == 0x8c) {
			if (err_info_len > 0) {
#if defined __GNUC__ && (__GNUC__ < 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
				uint16_t tag = *(uint16_t *)err_info;
#if defined __GNUC__ && (__GNUC__ < 8)
#pragma GCC diagnostic pop
#endif
				uint8_t ex_err = err_info[2];

				printf("error in tag %.4d -> %.2x\n", tag, ex_err);
			}
		}

		goto LOut;
	}
	

LOut:
	fdo_resume();
	free(pattern);

	return ret;
}

// ����� �᪠�쭮�� ���㬥�� �� ������
int fd_print_doc(uint8_t doc_type, uint32_t doc_no) {
	uint8_t ret;
	_Alignas(_Alignof(uint16_t)) uint8_t err_info[32];
	size_t err_info_len;
	uint8_t *pattern;
	size_t pattern_size;
	struct kkt_last_printed_info lpi;
	uint8_t *pattern_footer = NULL;
	size_t pattern_footer_size = 0;

   	if ((pattern = load_pattern(doc_type, pattern_footer,
				   	pattern_footer_size, &pattern_size)) == NULL) {
		printf("load_pattern fail\n");
		return -1;
	}

	err_info_len = sizeof(err_info);

	if ((ret = kkt_print_doc(doc_no, pattern, pattern_size, &lpi,
					err_info, &err_info_len)) != 0) {
		fd_set_error(doc_type, ret, err_info, err_info_len);

		if (ret == 0x80 || ret == 0x8c) {
			if (err_info_len > 0) {
#if defined __GNUC__ && (__GNUC__ < 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
				uint16_t tag = *(uint16_t *)err_info;
#if defined __GNUC__ && (__GNUC__ < 8)
#pragma GCC diagnostic pop
#endif
				uint8_t ex_err = err_info[2];

				printf("error in tag %.4d -> %.2x\n", tag, ex_err);
			}
		}

		goto LOut;
	}

LOut:
	free(pattern);

	return ret;
}
