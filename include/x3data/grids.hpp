/* Работа с разметками бланков БПУ/ККТ. (c) gsr, 2015-2016, 2019, 2022, 2024 */

#pragma once

#include "x3data/boost.hpp"

typedef void (*InitializationNotify_t)(bool done, const char *message);

/* Минимальная ширина разметки бланка в точках */
#define GRID_MIN_WIDTH		8
/* Максимальная ширина разметки бланка в точках */
#define GRID_MAX_WIDTH		1632
/* Минимальная высота разметки бланка в точках */
#define GRID_MIN_HEIGHT		8
/* Максимальная высота разметки бланка в точках */
#define GRID_MAX_HEIGHT		660

/* Информация о разметке для БПУ/ККТ */
class GridInfo {
private:
	uint8_t _nr;
public:
	uint8_t nr() const {return _nr;}
private:
	uint8_t _id;
public:
	uint8_t id() const {return _id;}
/*
 * В настоящее время используются следующие идентификаторы разметок: 0x30 -- 0x39 (0x38 -- пустая разметка) и
 * 0x41 -- 0x5a, т.е. цифры и большие латинские буквы (всего 36). Номера разметок отличаются от идентификаторов:
 * нумерация начинается с 1 и ведётся непрерывно.
 */
	static bool isIdValid(uint8_t id)
	{
		return ((id >= 0x30) && (id <= 0x39)) || ((id >= 0x41) && (id <= 0x5a));
	}

	static bool isNrValid(uint8_t nr)
	{
		return (nr > 0) && (nr < 37);
	}

	static uint8_t nrToId(uint8_t nr)
	{
		return isNrValid(nr) ? nr + ((nr < 11) ? 0x2f : 0x36) : 0;
	}

	static uint8_t idToNr(uint8_t id)
	{
		return isIdValid(id) ? id - ((id < 0x3a) ? 0x2f : 0x36) : 0;
	}
private:
	string _name;
public:
	const string &name() const {return _name;}
private:
	string _prefix;
public:
	const string &prefix() const {return _prefix;}
private:
	time_t _date;
public:
	time_t date() const {return _date;}

	bool isNewer(const GridInfo &gi) const {return _date > gi._date;}

	bool parse(const char * name);
	bool parse(const uint8_t * data, size_t len);

	GridInfo() :
		_nr(0),
		_id(0),
		_name(),
		_prefix(),
		_date(0)
	{
	}
};

extern bool update_xprn_grids(InitializationNotify_t init_notify);
extern bool update_kkt_grids(InitializationNotify_t init_notify);
