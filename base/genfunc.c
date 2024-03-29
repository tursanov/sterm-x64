/*
	�� �㭪樨 �ᯮ������� ࠧ���묨 ����ﬨ �ନ����.
	(c) gsr 2000-2003
*/

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "genfunc.h"

/* ��४���஢�� KOI-7 <--> Cp866 */
static uint8_t koi_alt[]={
/* ��砫� ⠡���� -- ��� ��������� */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0xfd, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
/* ����訥 ��⨭᪨� �㪢� -- � ᥡ� �� */
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
/* �����쪨� ��⨭᪨� �㪢� -- � ����訥 ���᪨� */
	0x9e, 0x80, 0x81, 0x96, 0x84, 0x85, 0x94, 0x83,
	0x95, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e,
	0x8f, 0x9f, 0x90, 0x91, 0x92, 0x93, 0x86, 0x82,
	0x9c, 0x9b, 0x87, 0x98, 0x9d, 0x99, 0x97, 0x9a,
/* ����訥 ���᪨� �㪢� -- � �����쪨� ��⨭᪨� */
	0x61, 0x62, 0x77, 0x67, 0x64, 0x65, 0x76, 0x7a,
	0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
	0x72, 0x73, 0x74, 0x75, 0x66, 0x68, 0x63, 0x7e,
	0x7b, 0x7d, 0x7f, 0x79, 0x78, 0x7c, 0x60, 0x71,
/* �����쪨� ���᪨� �㪢� -- � �����쪨� ��⨭᪨� */
	0x61, 0x62, 0x77, 0x67, 0x64, 0x65, 0x76, 0x7a,
	0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
/* �ᥢ����䨪� -- ����塞 ���訩 ��� */
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
/* �த������� �����쪨� ���᪨� �㪢 */
	0x72, 0x73, 0x74, 0x75, 0x66, 0x68, 0x63, 0x7e,
	0x7b, 0x7d, 0x7f, 0x79, 0x78, 0x7c, 0x60, 0x71,
/* ��⠢���� ���� ⠡���� -- ����塞 ���訩 ��� */
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x24, 0x7e, 0x7f,
};

/* ������ ��४���஢�� KOI-7 <--> Win1251 */
static uint8_t koi_win[]={
/* ��砫� ⠡���� -- ��� ��������� (�஬� 0x24) */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0xa4, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
/* ����訥 ��⨭᪨� �㪢� -- � ᥡ� �� */
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
/* �����쪨� ��⨭᪨� �㪢� -- � ����訥 ���᪨� */
	0xde, 0xc0, 0xc1, 0xd6, 0xc4, 0xc5, 0xd4, 0xc3,
	0xd5, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce,
	0xcf, 0xdf, 0xd0, 0xd1, 0xd2, 0xd3, 0xc6, 0xc2,
	0xdc, 0xdb, 0xc7, 0xd8, 0xdd, 0xd9, 0xd7, 0xda,
/* ��⠢���� ���� ⠡���� -- ��४���஢�� Win1251 <--> KOI7 */
/* �����稬�� ���� -- ����塞 ���訩 ��� */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0xa4, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
/* ����訥 ���᪨� �㪢� -- � �����쪨� ��⨭᪨� */
	0x61, 0x62, 0x77, 0x67, 0x64, 0x65, 0x76, 0x7a,
	0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
	0x72, 0x73, 0x74, 0x75, 0x66, 0x68, 0x63, 0x7e,
	0x7b, 0x7d, 0x7f, 0x79, 0x78, 0x7c, 0x60, 0x71,
/* �����쪨� ���᪨� �㪢� -- � �����쪨� ��⨭᪨� */
	0x61, 0x62, 0x77, 0x67, 0x64, 0x65, 0x76, 0x7a,
	0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
	0x72, 0x73, 0x74, 0x75, 0x66, 0x68, 0x63, 0x7e,
	0x7b, 0x7d, 0x7f, 0x79, 0x78, 0x7c, 0x60, 0x71,
};

/* ��४���஢�� ���� KOI7 <--> Cp866 */
int recode(uint8_t c)
{
	return koi_alt[c];
}

/*
 * ��४���஢�� ��ப�. ��४�������� �� ����� l ᨬ�����. �᫨ l == -1,
 * � ��ப� ��४�������� ���������.
 */
char *recode_str(char *s, int l)
{
	int i;
	if (s == NULL)
		return NULL;
	if (l == -1)
		l = strlen(s);
	for (i = 0; i < l; i++)
		s[i] = recode(s[i]);
	return s;
}

/* ��४���஢�� ���� KOI7 <--> Win1251 */
int recode_win(uint8_t c)
{
	return koi_win[c];
}

/* ��४���஢�� ��ப� KOI7 <--> Win1251 */
char *recode_str_win(char *s, int l)
{
	int i;
	if (s == NULL)
		return NULL;
	if (l == -1)
		l = strlen(s);
	for (i = 0; i < l; i++)
		s[i] = recode_win(s[i]);
	return s;
}

/* ���������� 䠩�� ��������� ࠧ��� */
#define BLOCK_SZ	4096
bool fill_file(int fd, uint32_t len)
{
	char buf[BLOCK_SZ];
	uint32_t l;
	if (fd == -1)
		return false;
	memset(buf, '\xb2', sizeof(buf));
	while (len != 0){
		l = (len >= BLOCK_SZ) ? BLOCK_SZ : len;
		if (write(fd, buf, l) != l)
			break;
		len -= l;
	}
	return len == 0;
}

/* �८�ࠧ������ time_t � struct date_time */
struct date_time *time_t_to_date_time(time_t t, struct date_time *dt)
{
	struct tm *tm = localtime(&t);
	dt->day = tm->tm_mday - 1;
	dt->mon = tm->tm_mon;
	dt->year = tm->tm_year % 100;
	dt->hour = tm->tm_hour;
	dt->min = tm->tm_min;
	dt->sec = tm->tm_sec;
	return dt;
}

time_t date_time_to_time_t(const struct date_time *dt)
{
	struct tm tm = {
		.tm_sec		= dt->sec,
		.tm_min		= dt->min,
		.tm_hour	= dt->hour,
		.tm_mday	= dt->day + 1,
		.tm_mon		= dt->mon,
		.tm_year	= dt->year + 100,
		.tm_isdst	= -1
	};
	return mktime(&tm);
}

/* ���� �������� ��᫥����⥫쭮�� */
uint8_t *memfind(uint8_t *mem, size_t mem_len, const uint8_t *tmpl, size_t tmpl_len)
{
	uint8_t *ret = NULL;
	if (tmpl_len <= mem_len){
		size_t i, j;
		for (i = 0; i <= (mem_len - tmpl_len); i++){
			if (mem[i] == tmpl[0]){
				for (j = 1; j < tmpl_len; j++){
					if (mem[i + j] != tmpl[j])
						break;
				}
				if (j == tmpl_len){
					ret = mem + i;
					break;
				}
			}
		}
	}
	return ret;
}
