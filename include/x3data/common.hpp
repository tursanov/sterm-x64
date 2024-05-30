/* ��騥 ��।������ ��� ��� 䠩���. (c) gsr, 2024 */

#pragma once

typedef void (*InitializationNotify_t)(bool done, const char *message);

#include <boost/container/list.hpp>
using boost::container::list;

#include <boost/container/string.hpp>
using boost::container::string;

#include <boost/smart_ptr/scoped_ptr.hpp>
using boost::scoped_ptr;

#include <boost/container/vector.hpp>
using boost::container::vector;

#include <boost/range/algorithm/equal.hpp>
using boost::range::equal;

#include <boost/range/algorithm/find_if.hpp>
using boost::range::find_if;

/* ������� ��� ࠡ��� � ᦠ�� ����ࠦ����� */
/* �ਧ��� ��砫� ������� */
static const uint8_t CPIC_ESC = 0x1b;
/* ����� �।��饣� ���� (C) */
static const uint8_t CPIC_REP_CHAR = 0x43;
/* ����� �।��饩 ����� */
static const uint8_t CPIC_REP_LINE = 0x4c;

/* ���⨥ 䠩�� ����ࠦ���� (w � h �������� � �窠�) */
extern bool compress_picture(const uint8_t *src, size_t len, size_t w, size_t h,
	vector<uint8_t> &dst);
/* �⥭�� ������ 䠩�� ����ࠦ���� */
extern const uint8_t *read_bmp(const char *path, size_t &len, size_t &w, size_t &h,
	size_t min_w, size_t max_w, size_t min_h, size_t max_h);
