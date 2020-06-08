/* Реализация алгоритма подсчета контрольной суммы MD5 (RFC-1321) */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "md5.h"

/*
 * В RFC эти функции сделаны в виде макросов для ускорения работы алгоритма,
 * но из-за этого возможны side effects и неправильное приведение типов.
 */
static uint32_t T[64] = {
	0xd76aa478,	/* 1  */
	0xe8c7b756,	/* 2  */
	0x242070db,	/* 3  */
	0xc1bdceee,	/* 4  */
	0xf57c0faf,	/* 5  */
	0x4787c62a,	/* 6  */
	0xa8304613,	/* 7  */
	0xfd469501,	/* 8  */
	0x698098d8,	/* 9  */
	0x8b44f7af,	/* 10 */
	0xffff5bb1,	/* 11 */
	0x895cd7be,	/* 12 */
	0x6b901122,	/* 13 */
	0xfd987193,	/* 14 */
	0xa679438e,	/* 15 */
	0x49b40821,	/* 16 */
	0xf61e2562,	/* 17 */
	0xc040b340,	/* 18 */
	0x265e5a51,	/* 19 */
	0xe9b6c7aa,	/* 20 */
	0xd62f105d,	/* 21 */
	0x02441453,	/* 22 */
	0xd8a1e681,	/* 23 */
	0xe7d3fbc8,	/* 24 */
	0x21e1cde6,	/* 25 */
	0xc33707d6,	/* 26 */
	0xf4d50d87,	/* 27 */
	0x455a14ed,	/* 28 */
	0xa9e3e905,	/* 29 */
	0xfcefa3f8,	/* 30 */
	0x676f02d9,	/* 31 */
	0x8d2a4c8a,	/* 32 */
	0xfffa3942,	/* 33 */
	0x8771f681,	/* 34 */
	0x6d9d6122,	/* 35 */
	0xfde5380c,	/* 36 */
	0xa4beea44,	/* 37 */
	0x4bdecfa9,	/* 38 */
	0xf6bb4b60,	/* 39 */
	0xbebfbc70,	/* 40 */
	0x289b7ec6,	/* 41 */
	0xeaa127fa,	/* 42 */
	0xd4ef3085,	/* 43 */
	0x04881d05,	/* 44 */
	0xd9d4d039,	/* 45 */
	0xe6db99e5,	/* 46 */
	0x1fa27cf8,	/* 47 */
	0xc4ac5665,	/* 48 */
	0xf4292244,	/* 49 */
	0x432aff97,	/* 50 */
	0xab9423a7,	/* 51 */
	0xfc93a039,	/* 52 */
	0x655b59c3,	/* 53 */
	0x8f0ccc92,	/* 54 */
	0xffeff47d,	/* 55 */
	0x85845dd1,	/* 56 */
	0x6fa87e4f,	/* 57 */
	0xfe2ce6e0,	/* 58 */
	0xa3014314,	/* 59 */
	0x4e0811a1,	/* 60 */
	0xf7537e82,	/* 61 */
	0xbd3af235,	/* 62 */
	0x2ad7d2bb,	/* 63 */
	0xeb86d391,	/* 64 */
};

/* Исходный блок данных */
static const uint8_t *data;
/* Длина в байтах исходного блока */
static uint32_t n_bytes;
/* Индекс для работы конечного автомата MD5 */
static uint32_t md5_index;
/* Длина в битах исходного блока */
static uint32_t n_bits[2];
/* Длина в байтах дополненного блока */
static uint32_t n_bytes_ex;
/* Дескриптор файла для подсчета MD5 */
static int md5_fd = -1;
/* Буфер для работы с файлом */
static uint8_t file_data[4096];
/* Текущая длина данных в буфере */
static uint32_t file_data_len;

/* Состояния конечного автомата входного потока */
enum {
	md5_data,
	md5_pad0,
	md5_pad,
	md5_len0,
	md5_len1,
	md5_stop,
};

static int md5_st = md5_data;

static void set_md5_len(uint32_t l)
{
	n_bytes = l;
	n_bits[0] = n_bytes << 3;
	n_bits[1] = n_bytes >> 29;
	n_bytes_ex = ((n_bytes + 8) / 64 + 1) * 64;
	if (n_bytes > 0)
		md5_st = md5_data;
	else
		md5_st = md5_pad0;
	md5_index = 0;
}

/* Инициализация конечного автомата подсчета MD5 буфера памяти */
static bool init_md5(const uint8_t *p, uint32_t l)
{
	if (p == NULL)
		return false;
	data = p;
	set_md5_len(l);
/*	printf("n_bytes = %lu, n_bits = %lu:%lu, n_bytes_ex = %lu\n",
		n_bytes, n_bits[0], n_bits[1], n_bytes_ex);*/
	return true;
}

/* Завершение работы с конечным автоматом подсчета MD5 файла */
static void release_md5_file(void)
{
	if (md5_fd != -1){
		close(md5_fd);
		md5_fd = -1;
	}
}

/* Чтение очередного блока данных из файла */
static bool read_file_block(void)
{
	int l = read(md5_fd, file_data, sizeof(file_data));
	md5_index = 0;
	if (l > 0){
		file_data_len = l;
		return true;
	}else{
		file_data_len = 0;
		return false;
	}
}

/* Инициализация конечного автомата подсчета MD5 файла */
static bool init_md5_file(const char *name)
{
	struct stat st;
	release_md5_file();
	if (stat(name, &st) == -1)
		return false;
	md5_fd = open(name, O_RDONLY);
	if (md5_fd == -1)
		return false;
	set_md5_len(st.st_size);
	file_data_len = 0;
	return read_file_block();
}

/* Вызывается, когда md5_st == md5_data */
static uint8_t on_md5_data(void)
{
	uint8_t b;
	if (md5_fd == -1){
		b = data[md5_index++];
		if (md5_index >= n_bytes)
			md5_st = md5_pad0;
	}else{
		b = file_data[md5_index++];
		if ((md5_index >= file_data_len) && !read_file_block())
			md5_st = md5_pad0;
	}
	return b;
}

/* Получение следующего байта из входного потока */
static uint8_t next_byte(void)
{
	uint8_t b = 0;
	switch (md5_st){
		case md5_data:
			b = on_md5_data();
			break;
		case md5_pad0:
			b = 0x80;
			md5_index = n_bytes_ex - n_bytes - 9;
			if (md5_index == 0)
				md5_st = md5_len0;
			else
				md5_st = md5_pad;
			break;
		case md5_pad:
			b = 0;
			md5_index--;
			if (md5_index == 0)
				md5_st = md5_len0;
			break;
		case md5_len0:
			b = *((const uint8_t *)n_bits + md5_index++);
			if (md5_index == 4){
				md5_index = 0;
				md5_st = md5_len1;
			}
			break;
		case md5_len1:
			b = *((const uint8_t *)(n_bits + 1) + md5_index++);
			if (md5_index == 4)
				md5_st = md5_stop;
			break;
	}
	return b;
}

/* Получение очередного uint32_t */
static uint32_t next_dword(void)
{
	uint32_t dw = 0, i;
	uint8_t *p = (uint8_t *)&dw;
	for (i = 0; i < sizeof(uint32_t); i++)
		*p++ = next_byte();
	return dw;
}

/* Вычисление MD5 после инициализации конечного автомата */
static bool do_md5(struct md5_hash *md5)
{
	struct md5_hash tmp;
	uint32_t X[16], i, j;
	if (md5 == NULL) 
		return false;
	md5->a = 0x67452301;
	md5->b = 0xefcdab89;
	md5->c = 0x98badcfe;
	md5->d = 0x10325476;
	for (i = 0; i < n_bytes_ex / 64; i++){
		for (j = 0; j < 16; j++)
			X[j] = next_dword();
		memcpy(&tmp, md5, sizeof(tmp));
/* Round 1, 1-я строка */
		md5->a = FF(md5->a, md5->b, md5->c, md5->d, X[0],  7,  T[0]);
		md5->d = FF(md5->d, md5->a, md5->b, md5->c, X[1],  12, T[1]);
		md5->c = FF(md5->c, md5->d, md5->a, md5->b, X[2],  17, T[2]);
		md5->b = FF(md5->b, md5->c, md5->d, md5->a, X[3],  22, T[3]);
/* Round 1, 2-я строка */
		md5->a = FF(md5->a, md5->b, md5->c, md5->d, X[4],  7,  T[4]);
		md5->d = FF(md5->d, md5->a, md5->b, md5->c, X[5],  12, T[5]);
		md5->c = FF(md5->c, md5->d, md5->a, md5->b, X[6],  17, T[6]);
		md5->b = FF(md5->b, md5->c, md5->d, md5->a, X[7],  22, T[7]);
/* Round 1, 3-я строка */
		md5->a = FF(md5->a, md5->b, md5->c, md5->d, X[8],  7,  T[8]);
		md5->d = FF(md5->d, md5->a, md5->b, md5->c, X[9],  12, T[9]);
		md5->c = FF(md5->c, md5->d, md5->a, md5->b, X[10], 17, T[10]);
		md5->b = FF(md5->b, md5->c, md5->d, md5->a, X[11], 22, T[11]);
/* Round 1, 4-я строка */
		md5->a = FF(md5->a, md5->b, md5->c, md5->d, X[12], 7,  T[12]);
		md5->d = FF(md5->d, md5->a, md5->b, md5->c, X[13], 12, T[13]);
		md5->c = FF(md5->c, md5->d, md5->a, md5->b, X[14], 17, T[14]);
		md5->b = FF(md5->b, md5->c, md5->d, md5->a, X[15], 22, T[15]);

/* Round 2, 1-я строка */
		md5->a = GG(md5->a, md5->b, md5->c, md5->d, X[1],  5,  T[16]);
		md5->d = GG(md5->d, md5->a, md5->b, md5->c, X[6],  9,  T[17]);
		md5->c = GG(md5->c, md5->d, md5->a, md5->b, X[11], 14, T[18]);
		md5->b = GG(md5->b, md5->c, md5->d, md5->a, X[0],  20, T[19]);
/* Round 2, 2-я строка */
		md5->a = GG(md5->a, md5->b, md5->c, md5->d, X[5],  5,  T[20]);
		md5->d = GG(md5->d, md5->a, md5->b, md5->c, X[10], 9,  T[21]);
		md5->c = GG(md5->c, md5->d, md5->a, md5->b, X[15], 14, T[22]);
		md5->b = GG(md5->b, md5->c, md5->d, md5->a, X[4],  20, T[23]);
/* Round 2, 3-я строка */
		md5->a = GG(md5->a, md5->b, md5->c, md5->d, X[9],  5,  T[24]);
		md5->d = GG(md5->d, md5->a, md5->b, md5->c, X[14], 9,  T[25]);
		md5->c = GG(md5->c, md5->d, md5->a, md5->b, X[3],  14, T[26]);
		md5->b = GG(md5->b, md5->c, md5->d, md5->a, X[8],  20, T[27]);
/* Round 2, 4-я строка */
		md5->a = GG(md5->a, md5->b, md5->c, md5->d, X[13], 5,  T[28]);
		md5->d = GG(md5->d, md5->a, md5->b, md5->c, X[2],  9,  T[29]);
		md5->c = GG(md5->c, md5->d, md5->a, md5->b, X[7],  14, T[30]);
		md5->b = GG(md5->b, md5->c, md5->d, md5->a, X[12], 20, T[31]);

/* Round 3, 1-я строка */
		md5->a = HH(md5->a, md5->b, md5->c, md5->d, X[5],  4,  T[32]);
		md5->d = HH(md5->d, md5->a, md5->b, md5->c, X[8],  11, T[33]);
		md5->c = HH(md5->c, md5->d, md5->a, md5->b, X[11], 16, T[34]);
		md5->b = HH(md5->b, md5->c, md5->d, md5->a, X[14], 23, T[35]);
/* Round 3, 2-я строка */
		md5->a = HH(md5->a, md5->b, md5->c, md5->d, X[1],  4,  T[36]);
		md5->d = HH(md5->d, md5->a, md5->b, md5->c, X[4],  11, T[37]);
		md5->c = HH(md5->c, md5->d, md5->a, md5->b, X[7],  16, T[38]);
		md5->b = HH(md5->b, md5->c, md5->d, md5->a, X[10], 23, T[39]);
/* Round 3, 3-я строка */
		md5->a = HH(md5->a, md5->b, md5->c, md5->d, X[13], 4,  T[40]);
		md5->d = HH(md5->d, md5->a, md5->b, md5->c, X[0],  11, T[41]);
		md5->c = HH(md5->c, md5->d, md5->a, md5->b, X[3],  16, T[42]);
		md5->b = HH(md5->b, md5->c, md5->d, md5->a, X[6],  23, T[43]);
/* Round 3, 4-я строка */
		md5->a = HH(md5->a, md5->b, md5->c, md5->d, X[9],  4,  T[44]);
		md5->d = HH(md5->d, md5->a, md5->b, md5->c, X[12], 11, T[45]);
		md5->c = HH(md5->c, md5->d, md5->a, md5->b, X[15], 16, T[46]);
		md5->b = HH(md5->b, md5->c, md5->d, md5->a, X[2],  23, T[47]);

/* Round 4, 1-я строка */
		md5->a = II(md5->a, md5->b, md5->c, md5->d, X[0],  6,  T[48]);
		md5->d = II(md5->d, md5->a, md5->b, md5->c, X[7],  10, T[49]);
		md5->c = II(md5->c, md5->d, md5->a, md5->b, X[14], 15, T[50]);
		md5->b = II(md5->b, md5->c, md5->d, md5->a, X[5],  21, T[51]);
/* Round 4, 2-я строка */
		md5->a = II(md5->a, md5->b, md5->c, md5->d, X[12], 6,  T[52]);
		md5->d = II(md5->d, md5->a, md5->b, md5->c, X[3],  10, T[53]);
		md5->c = II(md5->c, md5->d, md5->a, md5->b, X[10], 15, T[54]);
		md5->b = II(md5->b, md5->c, md5->d, md5->a, X[1],  21, T[55]);
/* Round 4, 3-я строка */
		md5->a = II(md5->a, md5->b, md5->c, md5->d, X[8],  6,  T[56]);
		md5->d = II(md5->d, md5->a, md5->b, md5->c, X[15], 10, T[57]);
		md5->c = II(md5->c, md5->d, md5->a, md5->b, X[6],  15, T[58]);
		md5->b = II(md5->b, md5->c, md5->d, md5->a, X[13], 21, T[59]);
/* Round 4, 4-я строка */
		md5->a = II(md5->a, md5->b, md5->c, md5->d, X[4],  6,  T[60]);
		md5->d = II(md5->d, md5->a, md5->b, md5->c, X[11], 10, T[61]);
		md5->c = II(md5->c, md5->d, md5->a, md5->b, X[2],  15, T[62]);
		md5->b = II(md5->b, md5->c, md5->d, md5->a, X[9],  21, T[63]);

		md5->a += tmp.a;
		md5->b += tmp.b;
		md5->c += tmp.c;
		md5->d += tmp.d;
	}
	return true;
}

/* Вычисление MD5 для блока памяти */
bool get_md5(const uint8_t *p, int l, struct md5_hash *md5)
{
	return init_md5(p, l) && do_md5(md5);
}

/* Вычисление MD5 для файла */
bool get_md5_file(const char *name, struct md5_hash *md5)
{
	bool flag = init_md5_file(name) && do_md5(md5);
	release_md5_file();
	return flag;
}

/* Вывод числа uint32_t, начиная с младшего байта */
static void print_dword(uint32_t dw)
{
	uint8_t *p = (uint8_t *)&dw;
	int i;
	for (i = 0; i < 4; i++)
		printf("%.2x", p[i]);
}

/* Вывод контрольной суммы MD5 в стандартной форме */
void print_md5(const struct md5_hash *md5)
{
	if (md5 == NULL)
		return;
	printf("0x");
	print_dword(md5->a);
	print_dword(md5->b);
	print_dword(md5->c);
	print_dword(md5->d);
}

/* Сравнение двух значений MD5 */
bool cmp_md5(const struct md5_hash *v1, const struct md5_hash *v2)
{
	if ((v1 == NULL) || (v2 == NULL))
		return false;
	return	(v1->a == v2->a) && (v1->b == v2->b) &&
		(v1->c == v2->c) &&(v1->d == v2->d);
}

#if 0
int main(int argc, char **argv)
{
/*	static const char *data = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";*/
	struct md5_hash md5;
	int i;
	if (argc < 2)
		return 1;
	for (i = 1; i < argc; i++){
		printf("%s: ", argv[i]);
		if (get_md5_file(argv[i], &md5))
			print_md5(&md5);
		else
			printf("failed\n");
	}
	return 0;
}
#endif
