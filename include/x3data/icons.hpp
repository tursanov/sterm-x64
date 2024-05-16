/* Работа с пиктограммами БПУ/ККТ. (c) gsr, 2017, 2019, 2022, 2024 */

#pragma once

#include "x3data/common.hpp"

/* Минимальная ширина пиктограммы в точках */
#define ICON_MIN_WIDTH		8
/* Максимальная ширина пиктограммы в точках */
#define ICON_MAX_WIDTH		640
/* Минимальная высота пиктограммы в точках */
#define ICON_MIN_HEIGHT		8
/* Максимальная высота пиктограммы в точках */
#define ICON_MAX_HEIGHT		640

/* Информация о пиктограмме для БПУ/ККТ */
class IconInfo {
private:
/* Номер пиктограммы в "Экспресс-3" */
	uint8_t _nr;
public:
/* Номер пиктограммы в "Экспресс-3" */
	uint8_t nr() const {return _nr;}
private:
/* Идентификатор "Экспресс-3" пиктограммы */
	uint8_t _id;
public:
/* Идентификатор "Экспресс-3" пиктограммы */
	uint8_t id() const {return _id;}
/*
* В настоящее время используются следующие идентификаторы пиктограмм: 0x30 -- 0x39 и 0x41 -- 0x5a
* (0-9, A-Z, всего 36). Номера пиктограмм отличаются от идентификаторов:
* нумерация начинается с 1 и ведётся непрерывно.
*/
/* Проверка правильности идентификатора "Экспресс-3" пиктограммы */
	static bool isIdValid(uint8_t id)
	{
		return ((id >= 0x30) && (id <= 0x39)) || ((id >= 0x41) && (id <= 0x5a));
	}

/* Проверка правильности номера пиктограммы */
	static bool isNrValid(uint8_t nr)
	{
		return (nr > 0) && (nr < 37);
	}

/* Преобразование номера пиктограммы в идентификатор "Экспресс-3" */
	static uint8_t nrToId(uint8_t nr)
	{
		return isNrValid(nr) ? nr + ((nr < 11) ? 0x2f : 0x36) : 0;
	}

/* Преобразование идентификатора "Экспресс-3" пиктограммы в номер */
	static uint8_t idToNr(uint8_t id)
	{
		return isIdValid(id) ? id - ((id < 0x3a) ? 0x2f : 0x36) : 0;
	}
private:
/* Имя пиктограммы */
	string _name;
public:
/* Имя пиктограммы */
	const string &name() const {return _name;}
private:
/* Префикс пиктограммы */
	string _prefix;
public:
/* Префикс пиктограммы */
	const string &prefix() const {return _prefix;}
private:
/* Дата создания пиктограммы */
	time_t _date;
public:
/* Дата создания пиктограммы */
	time_t date() const {return _date;}

/* Сравнение двух пиктограмм по дате создания */
	bool isNewer(const IconInfo &ii) const {return _date > ii._date;}

/* Разбор имени файла */
	bool parse(const char *name);
/* Разбор данных, полученных из "Экспресс-3" */
	bool parse(const uint8_t *data, size_t len);

/* Создание экземпляра класса IconInfo со значениями по умолчанию */
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
