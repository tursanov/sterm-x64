/*
 * �������� ��業��� ���/��� � �ନ���� � �ନ஢���� ���� 㤠�����.
 * �� 㤠����� � MBR �����뢠���� ᯥ樠��� 䫠�, � ���짮��⥫�
 * �뤠���� 32-���筮� ��⭠����筮� �᫮. ��᫮ �ନ�����
 * �� �᭮�� ⥪�饩 ����, ⠡���� ��砩��� �ᥫ (rndtbl.c) �
 * ��� �����᪮�� ����� �ନ����.
 * (c) gsr 2006, 2011, 2019.
 */

#if !defined RMLIC_H
#define RMLIC_H

#include "md5.h"
#include "rndtbl.h"

/* �������� 䠩�� ��業��� */
extern bool rm_lic_file(const char *name, const char *lic_name);
/* ��ନ஢���� ���� 㤠����� ��業��� ��� ��������� �����᪮�� ����� �ନ���� */
extern bool mk_del_hash_for_tnumber(uint8_t *uint8_t, const struct md5_hash *number,
	const struct rnd_rec *rnd_tbl, int n);
/* ��ନ஢���� ���� 㤠����� ��業��� */
extern bool mk_del_hash(uint8_t *buf, const struct rnd_rec *rnd_tbl, int n);
/* �뢮� ���� 㤠����� ��業��� �� �࠭ */
extern void print_del_hash(const uint8_t *del_hash, const char *lic_name);
/* ����� ���⢥ত���� 㤠����� ��業��� */
extern bool ask_rmlic_confirmation(const char *lic_name);

#endif		/* RMLIC_H */
