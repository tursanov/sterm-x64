/* Общие определения для всех файлов. (c) gsr, 2024 */

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

/* Команды для работы со сжатым изображением */
/* Признак начала команды */
static const uint8_t CPIC_ESC = 0x1b;
/* Повтор предыдущего байта (C) */
static const uint8_t CPIC_REP_CHAR = 0x43;
/* Повтор предыдущей линии */
static const uint8_t CPIC_REP_LINE = 0x4c;

/* Сжатие файла изображения (w и h задаются в точках) */
extern bool compress_picture(const uint8_t *src, size_t len, size_t w, size_t h,
	vector<uint8_t> &dst);
