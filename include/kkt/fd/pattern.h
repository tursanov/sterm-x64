#ifndef __PATTERN_H__
#define __PATTERN_H__

#if defined __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Освобождает память, занятую шаблонами
void kkt_free_patterns(void);
// Перезагружает шаблоны для указанного ИНН
void kkt_reload_patterns(uint64_t inn);
// Находит шаблон по типу документа и индексу
const uint8_t *kkt_find_pattern(uint8_t docType, uint8_t index, size_t *size);

#if defined __cplusplus
}
#endif

#endif
