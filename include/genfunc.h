/*
 *	�� �㭪樨 �ᯮ������� ࠧ���묨 ����ﬨ �ନ����.
 *	(c) gsr 2000-2003, 2018
 */

#if !defined GENFUNC_H
#define GENFUNC_H

#if defined __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h>
#include "sysdefs.h"

/*
 * ��� � �६� �࠭���� � ᯥ樠�쭮� �ଠ�, �⮡�
 * �������� �஡��� ��ॢ��� time_t ��� ࠧ����� �ᮢ�� ���ᮢ.
 * �஬� ⮣�, �� ⠪�� �।�⠢����� ���� � �६��� � �ଠ� ������
 * Little Endian �������� ����� �ࠢ������ ��� ���������� 楫� �᫠
 * (祬 ����� ���, ⥬ ����� �᫮).
 * �������� ���祭��: 01.01.2000 00:00:00 -- 31.12.2063 23:59:59.
 */
struct date_time {
	union {
		struct {
			uint32_t sec:6;
			uint32_t min:6;
			uint32_t hour:5;
			uint32_t day:5;
			uint32_t mon:4;
			uint32_t year:6;
		} __attribute__((__packed__));
		uint32_t dt;
	} __attribute__((__packed__));
} __attribute__((__packed__));

extern struct date_time *time_t_to_date_time(time_t t, struct date_time *dt);
extern time_t date_time_to_time_t(const struct date_time *dt);

/* ��४���஢�� KOI7 <--> CP866 */
extern int	recode(uint8_t c);
extern char	*recode_str(char *s, int l);

/* ��४���஢�� KOI7 <--> Win1251 */
extern int recode_win(uint8_t c);
extern char *recode_str_win(char *s, int l);

extern bool	fill_file(int fd, uint32_t len);

/* ���� �������� ��᫥����⥫쭮�� */
extern uint8_t *memfind(uint8_t *mem, size_t mem_len, const uint8_t *tmpl,
	size_t tmpl_len);

#if defined __cplusplus
}
#endif

#endif		/* GENFUNC_H */
