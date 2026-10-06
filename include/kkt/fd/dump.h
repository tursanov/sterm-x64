#ifndef __KKT_FD_DUMP_H__
#define __KKT_FD_DUMP_H__

#include <stdio.h>

void K_dump(FILE *f, K *k, const char* desc, int indent);
void K_list_dump(FILE *f, list_t *list, const char* desc, int indent);


#endif // __KKT_FD_DUMP_H__
