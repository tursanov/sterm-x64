#if !defined SERIALIZE_H
#define SERIALIZE_H

#include "list.h"

__BEGIN_DECLS

extern int s_open(const char *file_name, int open_for_write);
extern int s_close(int fd);

extern int save_string(int fd, const char *s);
extern int load_string(int fd, char **ret);
extern int load_array_string(int fd, char *s, size_t max_len);
extern int save_data(int fd, const void *data, size_t size);
extern int load_data(int fd, void **data, size_t *size);

extern int save_int(int fd, uint64_t v, size_t size);
extern int load_int(int fd, uint64_t *v, size_t size);

typedef void * (*load_item_func_t)(int fd);
extern int save_list(int fd, list_t *list, list_item_func_t save_item_func);
extern int load_list(int fd, list_t *list, load_item_func_t load_item_func);


#define SAVE_INT(fd, v) save_int((fd), (v), sizeof(v))
#define LOAD_INT(fd, v) load_int((fd), (uint64_t *)&(v), sizeof(v))

__END_DECLS

#endif		/* SERIALIZE_H */
