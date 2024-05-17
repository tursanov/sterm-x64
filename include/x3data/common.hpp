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
