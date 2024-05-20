/* Работа с таблицами XSLT. (c) gsr, 2023, 2024 */

#pragma once

#include "x3data/common.hpp"

static const size_t MAX_XSLT_NAME_LEN = 8;

/* Информация о таблице XSLT */
class XSLTInfo {
private:
/* Индекс таблицы XSLT в "Экспресс-3" */
	char _idx[3];
public:
/* Индекс таблицы XSLT в "Экспресс-3" */
	const char *idx() const {return _idx;}
/* Индекс таблицы XSLT состоит из двух символов, принимающих значения 0-9 и A-Z */
/* Проверка правильности индекса таблицы XSLT */
	static bool isIdxValid(const char *idx);
	static bool isIdxValid(char *idx)
	{
		return isIdxValid(idx.c_str());
	}
private:
/* Имя таблицы XSLT */
	string _name;
public:
/* Имя таблицы XSLT */
	const string &name() const {return _name;}
private:
/* Дата создания таблицы XSLT */
	time_t _date;
public:
/* Дата создания таблицы XSLT */
	time_t date() const {return _date;}

/* Сравнение двух таблиц XSLT по дате создания */
	bool isNewer(const XSLTInfo &xi) const {return _date > xi._date;}

/* Разбор имени файла */
	bool parse(const char *name);
/* Разбор данных, полученных из "Экспресс-3" */
	bool parse(const uint8_t *data, size_t len);

/* Создание экземпляра класса XSLTInfo со значениями по умолчанию */
	XSLTInfo() :
		_date(0)
	{
		_idx[0] = 0;
	}
};
