/* Работа с разметками бланков БПУ/ККТ. (c) gsr, 2015-2016, 2019, 2022, 2024 */

#pragma once

#include "x3data/boost.h"

typedef void (*InitializationNotify_t)(bool done, const char *message);
typedef void (*X3SyncCallback_t)(bool done, const char *message);

/* Минимальная ширина разметки бланка в точках */
#define GRID_MIN_WIDTH		8
/* Максимальная ширина разметки бланка в точках */
#define GRID_MAX_WIDTH		1632
/* Минимальная высота разметки бланка в точках */
#define GRID_MIN_HEIGHT		8
/* Максимальная высота разметки бланка в точках */
#define GRID_MAX_HEIGHT		660

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

/*	GridInfo(uint8_t nr, string name) :
		_nr(nr),
		_id(nrToId(nr)),
		_name(name),
		_date(0)
	{
	}*/

	GridInfo() :
		_nr(0),
		_id(0),
		_name(),
		_prefix(),
		_date(0)
	{
	}
};

extern list<GridInfo> grids_to_create_xprn;
extern list<GridInfo> grids_to_remove_xprn;
extern list<GridInfo> grids_failed_xprn;

extern list<GridInfo> grids_to_create_kkt;
extern list<GridInfo> grids_to_remove_kkt;
extern list<GridInfo> grids_failed_kkt;

extern void clr_grid_lists(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove);

static inline void clr_grid_lists_xprn()
{
	clr_grid_lists(grids_to_create_xprn, grids_to_remove_xprn);
}

static inline void clr_grid_lists_kkt()
{
	clr_grid_lists(grids_to_create_kkt, grids_to_remove_kkt);
}

static inline void clr_grid_lists()
{
	clr_grid_lists_xprn();
	clr_grid_lists_kkt();
}

extern bool need_grids_update(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove);

static inline bool needGridsUpdateXPrn()
{
	return !grids_to_create_xprn.empty() || !grids_to_remove_xprn.empty();
}

static inline bool needGridsUpdateKkt()
{
	return !grids_to_create_kkt.empty() || !grids_to_remove_kkt.empty();
}

static inline bool need_grids_update()
{
	return needGridsUpdateXPrn() || needGridsUpdateKkt();
}

extern void check_stored_grids(const list<GridInfo> &x3_grids,
	list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove, bool (*find_fn)(list<GridInfo> &));
extern void check_stored_grids_xprn(const list<GridInfo> &x3_grids);
extern void check_stored_grids_kkt(const list<GridInfo> &x3_grids);
extern bool check_x3_grids(const uint8_t * data, size_t len, list<GridInfo> &x3_grids_xprn, list<GridInfo> &x3_grids_kkt);
extern bool sync_grids(list<GridInfo> &grids_to_create, list<GridInfo> &grids_to_remove, list<GridInfo> &grids_failed,
	X3SyncCallback_t cbk);

static inline bool sync_grids_xprn(X3SyncCallback_t cbk)
{
	return sync_grids(grids_to_create_xprn, grids_to_remove_xprn, grids_failed_xprn, cbk);
}

static inline bool sync_grids_kkt(X3SyncCallback_t cbk)
{
	return sync_grids(grids_to_create_kkt, grids_to_remove_kkt, grids_failed_kkt, cbk);
}

extern bool update_xprn_grids(InitializationNotify_t init_notify);
extern bool update_kkt_grids(InitializationNotify_t init_notify);
