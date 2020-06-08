/* ��騥 �㭪樨 ��� ࠡ��� � �ਭ�ࠬ�. (c) gsr 2009 */

#include "prn/generic.h"
#include "cfg.h"
#include "gd.h"

/* ���� ���� */
uint8_t prn_buf[PRN_BUF_LEN];
/* ����� ������ � ���� ���� */
int prn_buf_len = 0;

/* �஢�ઠ ����� �ਭ�� */
bool prn_number_correct(uint8_t *number)
{
	int i, m;
	if (number == NULL)
		return false;
	for (i = m = 0; i < PRN_NUMBER_LEN; i++){
		if ((number[i] < 0x20) || (number[i] > 0x7e))
			break;
		else if (number[i] != 0x30)
			m++;
	}
	return (i == PRN_NUMBER_LEN) && (number[i] == 0) && (m > 0);
}

/* �஢�ઠ ����஢ �ਭ�஢ */
bool prn_numbers_ok(void)
{
	return	((!cfg.has_xprn || prn_number_correct((uint8_t *)cfg.xprn_number)) &&
		 (!cfg.has_aprn || prn_number_correct((uint8_t *)cfg.aprn_number)));
}
