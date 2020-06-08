#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "list.h"

int s_open(const char *file_name, int open_for_write);
int s_close(int fd);

int save_string(int fd, const char *s);
int load_string(int fd, char **ret);
int save_data(int fd, const void *data, size_t size);
int load_data(int fd, void **data, size_t *size);

int save_int(int fd, uint64_t v, size_t size);
int load_int(int fd, uint64_t *v, size_t size);

typedef void * (*load_item_func_t)(int fd);
int save_list(int fd, list_t *list, list_item_func_t save_item_func);
int load_list(int fd, list_t *list, load_item_func_t load_item_func);

#define SAVE_INT(fd, v) save_int((fd), (v), sizeof(v))
#define LOAD_INT(fd, v) load_int((fd), (uint64_t *)&(v), sizeof(v))

#endif // SERIALIZE_H
