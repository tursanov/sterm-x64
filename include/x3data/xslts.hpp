/* ����� � ⠡��栬� XSLT. (c) gsr, 2023, 2024 */

#pragma once

#include "x3data/common.hpp"

static const size_t MAX_XSLT_NAME_LEN = 8;

/* ���ଠ�� � ⠡��� XSLT */
class XSLTInfo {
private:
/* ������ ⠡���� XSLT � "������-3" */
	char _idx[3];
public:
/* ������ ⠡���� XSLT � "������-3" */
	const char *idx() const {return _idx;}
/* ������ ⠡���� XSLT ��⮨� �� ���� ᨬ�����, �ਭ������ ���祭�� 0-9 � A-Z */
/* �஢�ઠ �ࠢ��쭮�� ������ ⠡���� XSLT */
	static bool isIdxValid(const char *idx);
	static bool isIdxValid(char *idx)
	{
		return isIdxValid(idx.c_str());
	}
private:
/* ��� ⠡���� XSLT */
	string _name;
public:
/* ��� ⠡���� XSLT */
	const string &name() const {return _name;}
private:
/* ��� ᮧ����� ⠡���� XSLT */
	time_t _date;
public:
/* ��� ᮧ����� ⠡���� XSLT */
	time_t date() const {return _date;}

/* �ࠢ����� ���� ⠡��� XSLT �� ��� ᮧ����� */
	bool isNewer(const XSLTInfo &xi) const {return _date > xi._date;}

/* ������ ����� 䠩�� */
	bool parse(const char *name);
/* ������ ������, ����祭��� �� "������-3" */
	bool parse(const uint8_t *data, size_t len);

/* �������� ������� ����� XSLTInfo � ���祭�ﬨ �� 㬮�砭�� */
	XSLTInfo() :
		_date(0)
	{
		_idx[0] = 0;
	}
};
