/* Работа с шаблонами печати ККТ. (c) gsr, 2022, 2024 */

#pragma once

#include "x3data/common.hpp"

extern bool check_x3_kkt_patterns(const uint8_t *data, size_t len, string &version);
