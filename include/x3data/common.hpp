/* Общие определения для всех файлов. (c) gsr 2024 */

#pragma once

typedef void (*InitializationNotify_t)(bool done, const char *message);

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <vector>
using std::equal;
using std::find_if;
using std::list;
using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;

/* Команды для работы со сжатым изображением */
/* Признак начала команды */
static const uint8_t CPIC_ESC = 0x1b;
/* Повтор предыдущего байта (C) */
static const uint8_t CPIC_REP_CHAR = 0x43;
/* Повтор предыдущей линии */
static const uint8_t CPIC_REP_LINE = 0x4c;

/* Проверка существования каталога и создание его при необходимости */
extern bool create_folder_if_need(const char *path);

/* Сжатие файла изображения (w и h задаются в точках) */
extern bool compress_picture(const uint8_t *src, size_t len, size_t w, size_t h,
	vector<uint8_t> &dst);
/* Чтение данных файла изображения */
extern const uint8_t *read_bmp(const char *path, size_t &len, size_t &w, size_t &h,
	size_t min_w, size_t max_w, size_t min_h, size_t max_h);
