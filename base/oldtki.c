/*
 * ����� � �।��騬� ��ਠ�⠬� ���祢�� ���ଠ樨 �ନ���� (�. oldtki.h).
 * (c) gsr 2004.
 */

#include <stdlib.h>
#include "oldtki.h"

/* ������ ��ਠ�� ���祢�� ���ଠ樨 */
void early_decrypt_data(byte *p, int l)
{
	int i;
	for (i = l; i > 0; i--)
		p[i % l] ^= p[i - 1];
}
