/* ����� � ���⮣ࠬ���� ���/���. (c) gsr, 2017, 2019, 2022, 2024 */

#pragma once

#include "x3data/common.hpp"

/* �������쭠� �ਭ� ���⮣ࠬ�� � �窠� */
#define ICON_MIN_WIDTH		8
/* ���ᨬ��쭠� �ਭ� ���⮣ࠬ�� � �窠� */
#define ICON_MAX_WIDTH		640
/* �������쭠� ���� ���⮣ࠬ�� � �窠� */
#define ICON_MIN_HEIGHT		8
/* ���ᨬ��쭠� ���� ���⮣ࠬ�� � �窠� */
#define ICON_MAX_HEIGHT		640

/* ���ଠ�� � ���⮣ࠬ�� ��� ���/��� */
class IconInfo {
private:
/* ����� ���⮣ࠬ�� � "������-3" */
	uint8_t _nr;
public:
/* ����� ���⮣ࠬ�� � "������-3" */
	uint8_t nr() const {return _nr;}
private:
/* �����䨪��� "������-3" ���⮣ࠬ�� */
	uint8_t _id;
public:
/* �����䨪��� "������-3" ���⮣ࠬ�� */
	uint8_t id() const {return _id;}
/*
* � �����饥 �६� �ᯮ������� ᫥���騥 �����䨪���� ���⮣ࠬ�: 0x30 -- 0x39 � 0x41 -- 0x5a
* (0-9, A-Z, �ᥣ� 36). ����� ���⮣ࠬ� �⫨����� �� �����䨪��஢:
* �㬥��� ��稭����� � 1 � ������� �����뢭�.
*/
/* �஢�ઠ �ࠢ��쭮�� �����䨪��� "������-3" ���⮣ࠬ�� */
	static bool isIdValid(uint8_t id)
	{
		return ((id >= 0x30) && (id <= 0x39)) || ((id >= 0x41) && (id <= 0x5a));
	}

/* �஢�ઠ �ࠢ��쭮�� ����� ���⮣ࠬ�� */
	static bool isNrValid(uint8_t nr)
	{
		return (nr > 0) && (nr < 37);
	}

/* �८�ࠧ������ ����� ���⮣ࠬ�� � �����䨪��� "������-3" */
	static uint8_t nrToId(uint8_t nr)
	{
		return isNrValid(nr) ? nr + ((nr < 11) ? 0x2f : 0x36) : 0;
	}

/* �८�ࠧ������ �����䨪��� "������-3" ���⮣ࠬ�� � ����� */
	static uint8_t idToNr(uint8_t id)
	{
		return isIdValid(id) ? id - ((id < 0x3a) ? 0x2f : 0x36) : 0;
	}
private:
/* ��� ���⮣ࠬ�� */
	string _name;
public:
/* ��� ���⮣ࠬ�� */
	const string &name() const {return _name;}
private:
/* ��䨪� ���⮣ࠬ�� */
	string _prefix;
public:
/* ��䨪� ���⮣ࠬ�� */
	const string &prefix() const {return _prefix;}
private:
/* ��� ᮧ����� ���⮣ࠬ�� */
	time_t _date;
public:
/* ��� ᮧ����� ���⮣ࠬ�� */
	time_t date() const {return _date;}

/* �ࠢ����� ���� ���⮣ࠬ� �� ��� ᮧ����� */
	bool isNewer(const IconInfo &ii) const {return _date > ii._date;}

/* ������ ����� 䠩�� */
	bool parse(const char *name);
/* ������ ������, ����祭��� �� "������-3" */
	bool parse(const uint8_t *data, size_t len);

/* �������� ������� ����� IconInfo � ���祭�ﬨ �� 㬮�砭�� */
	IconInfo() :
		_nr(0),
		_id(0),
		_name(),
		_prefix(),
		_date(0)
	{
	}
};

extern bool update_xprn_icons(InitializationNotify_t init_notify);
extern bool update_kkt_icons(InitializationNotify_t init_notify);
