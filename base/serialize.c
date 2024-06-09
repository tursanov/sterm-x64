#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#if !defined WIN32
	#include <unistd.h>
#else
	#include <io.h>
#endif
#include <fcntl.h>
#include "serialize.h"

int s_open(const char *file_name, int open_for_write) {
	int flags = open_for_write ? O_CREAT | O_WRONLY | O_TRUNC : O_RDONLY;
	return open(file_name, flags, 0666);
}

int s_close(int fd) {
	//fsync(fd);
	return close(fd);
}

int s_write(int fd, const void *d, size_t n) {
	const uint8_t *s = (const uint8_t *)d;
	size_t size = n;
	while (n > 0) {
		int ret = write(fd, s, n);
		if (ret <= 0) {
			printf("s_write: return %d\n", ret);
			return -1;
		}
		n -= (size_t)ret;
		s += (size_t)ret;
	}
	return size;
}

int s_read(int fd, void *d, size_t n) {
	uint8_t *s = (uint8_t *)d;
	size_t size = n;
	while (n > 0) {
		int ret = read(fd, s, n);
		if (ret <= 0) {
			printf("s_read: return %d\n", ret);
			return -1;
		}
		n -= (size_t)ret;
		s += (size_t)ret;
	}
	return size;
}

int save_data(int fd, const void *data, size_t size) {
    if (s_write(fd, &size, sizeof(size)) < 0)
        return -1;
    if (size > 0 && s_write(fd, data, size) < 0)
        return -1;
    return 0;

}

int load_data(int fd, void **data, size_t *size) {
    size_t len = 0;
    if (s_read(fd, &len, sizeof(len)) < 0)
        return -1;
	if (len > 0) {
		uint8_t *s = (uint8_t *)malloc(len);
		if (s == NULL)
			return -1;

		if (s_read(fd, s, len) < 0) {
			free(s);
			return -1;
		}
		*data = s;
	} else
		*data = NULL;
	*size = len;
    return 0;
}


int save_string(int fd, const char *s) {
    size_t len = s != NULL ? strlen(s) : 0;
    if (s_write(fd, &len, sizeof(len)) < 0)
        return -1;
    if (len > 0 && s_write(fd, s, len) < 0)
        return -1;
    return 0;
}

int load_string(int fd, char **ret) {
    size_t len;
    if (s_read(fd, &len, sizeof(len)) < 0)
        return -1;
	if (len > 0) {
		char *s = (char *)malloc(len + 1);
		if (s == NULL)
			return -1;

		if (s_read(fd, s, len) < 0) {
			free(s);
			return -1;
		}
		s[len] = 0;

		*ret = s;
	} else
		*ret = NULL;
    
    return 0;
}

int save_int(int fd, uint64_t v, size_t size) {
    if (s_write(fd, &v, size) < 0)
        return -1;
    return 0;
}

int load_int(int fd, uint64_t *v, size_t size) {
    if (s_read(fd, v, size) < 0)
        return -1;
    return 0;
}

int save_list(int fd, list_t *list, list_item_func_t save_item_func) {
    if (SAVE_INT(fd, list->count) < 0 ||
        list_foreach(list, (void *)fd, save_item_func) < 0)
        return -1;
    return 0;
}

int load_list(int fd, list_t *list, load_item_func_t load_item_func) {
    size_t count = 0;
    if (LOAD_INT(fd, count) < 0)
        return -1;
    
    for (size_t i = 0; i < count; i++) {
        void *obj = load_item_func(fd);
        if (obj == NULL)
            return -1;
        if (list_add(list, obj) != 0)
            return -1;
    }
    return 0;
}

