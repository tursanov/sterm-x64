/* Общие функции. (c) gsr 2024 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "kkt/io.h"
#include "x3data/bmp.h"
#include "x3data/common.h"
#include "x3data/common.hpp"
#include "termlog.h"

/* Сжатие файла шаблона (w и h задаются в точках) */
bool compress_picture(const uint8_t *src, size_t len, size_t w,
	size_t h __attribute__((unused)), vector<uint8_t> &dst)
{
	if ((src == NULL) || (len == 0) || (len > KKT_TX_BUF_LEN))
		return false;
	dst.clear();
	w /= 8;
	vector<uint8_t> ln, prev_ln;
	uint8_t lastb = 0;
	for (size_t i = 0, n = 0, m = 0; i < len; i++){	/* n -- счётчик символов, m -- счётчик строк */
		uint8_t b = src[i];
		bool eol = (i + 1) % w == 0;
		if (n == 0){
			ln.push_back(b);
			if (b == CPIC_ESC)
				ln.push_back(b);
			lastb = b;
			n++;
		}else if ((b != lastb) || (n == UINT8_MAX) || eol){
			if (b == lastb)
				n++;
			bool comp = (lastb == CPIC_ESC) ? (n > 2) : (n > 4);
			if (comp){
				ln.push_back(CPIC_ESC);
				ln.push_back(CPIC_REP_CHAR);
				ln.push_back((uint8_t)(n - 1));
			}else{
				while (--n > 0){
					ln.push_back(lastb);
					if (lastb == CPIC_ESC)
						ln.push_back(lastb);
				}
			}
			n = 0;
			if (b != lastb)
				i--;
		}else
			n++;
		if (eol){
			if (m == 0){
				dst.insert(dst.cend(), ln.cbegin(), ln.cend());
				prev_ln.assign(ln.cbegin(), ln.cend());
				m++;
			}else{
				bool eq = equal(ln, prev_ln);
				if (!eq || (m == UINT8_MAX) || (i == (len - 1))){
					if (eq)
						m++;
					if (m > 1){
						dst.push_back(CPIC_ESC);
						dst.push_back(CPIC_REP_LINE);
						dst.push_back((uint8_t)(m - 1));
					}
					if (!eq){
						dst.insert(dst.cend(), ln.cbegin(), ln.cend());
						prev_ln.assign(ln.cbegin(), ln.cend());
					}
					m = 1;
				}else
					m++;
			}
			ln.clear();
			n = 0;
		}
	}
	return true;
}

static uint8_t *pic_data = NULL;

static void clr_pic_data()
{
	if (pic_data != NULL){
		delete [] pic_data;
		pic_data = NULL;
	}
}

/* Чтение данных файла изображения */
const uint8_t *read_bmp(const char *path, size_t &len, size_t &w, size_t &h,
	size_t min_w, size_t max_w, size_t min_h, size_t max_h)
{
	log_dbg("path = %s; min_w = %zu; max_w = %zu; min_h = %zu; max_h = %zu.",
		path, min_w, max_w, min_h, max_h);
	clr_pic_data();
	len = w = h = 0;
	if (path == NULL){
		log_err("Не указано имя файла.");
		return NULL;
	}
	struct stat st;
	if (stat(path, &st) == -1){
		log_sys_err("Ошибка получения информации о файле %s:", path);
		return NULL;
	}else if ((st.st_size < (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO))) || (st.st_size > KKT_TX_BUF_LEN)){
		log_err("Неверный размер файла %s (%d байт).", path, st.st_size);
		return NULL;
	}
	int fd = open(path, O_RDONLY);
	if (fd == -1){
		log_sys_err("Ошибка открытия файла %s для чтения:", path);
		return NULL;
	}
	uint8_t *ret = NULL;
	do {
		BITMAPFILEHEADER hdr;
		if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)){
			log_sys_err("Ошибка чтения заголовка BMP из файла %s:", path);
			break;
		}else if (hdr.bfType != 0x4d42){
			log_err("Неверная сигнатура BMP в файле %s.", path);
			break;
		}else if (hdr.bfSize != st.st_size){
			log_err("Несовпадение фактического размера файла %s с размером в заголовке.", path);
			break;
		}
		BITMAPINFOHEADER bmi;
		if (read(fd, &bmi, sizeof(bmi)) != sizeof(bmi)){
			log_sys_err("Ошибка чтения информации о BMP из файла %s:", path);
			break;
		}else if ((bmi.biWidth < (min_w * 8)) || (bmi.biWidth > (max_w * 8)) ||
				((bmi.biWidth % 32) != 0)){
			log_err("Неверная ширина изображения (%d).", bmi.biWidth);
			break;
		}else if ((bmi.biHeight < (min_h * 8)) || (abs(bmi.biHeight) > (max_h * 8)) ||
				((bmi.biHeight % 8) != 0)){
			log_err("Неверная высота изображения (%d).", bmi.biHeight);
			break;
		}else if (bmi.biBitCount != 1){
			log_err("Неверная глубина цвета изображения (%d bpp).", bmi.biBitCount);
			break;
		}else if (bmi.biCompression != BI_RGB){
			log_err("Неверный формат сжатия изображения.");
			break;
		}
		size_t data_len = (bmi.biWidth / 8) * bmi.biHeight;
		if (data_len == 0){
			log_err("Длина данных изображения равна 0.");
			break;
		}else if ((bmi.biSizeImage != 0) && (bmi.biSizeImage < data_len)){
			log_err("Неверный размер изображения (%d байт).", bmi.biSizeImage);
			break;
		}
		RGBQUAD pal[2];
		if (read(fd, pal, sizeof(pal)) != sizeof(pal)){
			log_sys_err("Ошибка чтения палитры изображения:");
			break;
		}
		bool inverted = (pal[0].rgbRed == 0) && (pal[0].rgbGreen == 0) && (pal[0].rgbBlue == 0);
		if (lseek(fd, hdr.bfOffBits, SEEK_SET) != hdr.bfOffBits){
			log_sys_err("Ошибка позиционирования в файле %s для чтения данных изображения:", path);
			break;
		}
		pic_data = new uint8_t[data_len];
		if (read(fd, pic_data, data_len) != data_len){
			log_sys_err("Ошибка чтения данных изображения из %s:", path);
			clr_pic_data();
			break;
		}
		if (bmi.biHeight > 0){
			int w = bmi.biWidth / 8;
			scoped_ptr<uint8_t> buf(new uint8_t[w]);
			for (int offs1 = 0, offs2 = data_len - w; offs1 < offs2; offs1 += w, offs2 -= w){
				memcpy(buf.get(), pic_data + offs1, w);
				memcpy(pic_data + offs1, pic_data + offs2, w);
				memcpy(pic_data + offs2, buf.get(), w);
			}
		}
		if (inverted){
			for (size_t i = 0; i < data_len; i++)
				pic_data[i] ^= 0xff;
		}
		len = data_len;
		w = bmi.biWidth;
		h = abs(bmi.biHeight);
		ret = pic_data;
	} while (false);
	close(fd);
	return ret;
}

/* Вычисление контрольной суммы CRC32 x^30 + x^27 + x^18 + x^3 + x^1 блока данных */
uint32_t pic_crc32(const uint8_t *data, size_t len)
{
	if ((data == NULL) || (len == 0))
		return 0;
	uint32_t ret = 0;
	for (size_t i = 0; i < len; i++){
		uint8_t b = data[i];
		for (int j = 0; j < 8; j++){
			ret >>= 1;
			ret |=	(uint32_t)(((b & 1) ^
				((ret & (1 << 1))  >> 1)  ^
				((ret & (1 << 3))  >> 3)  ^
				((ret & (1 << 18)) >> 18) ^
				((ret & (1 << 27)) >> 27) ^
				((ret & (1 << 30)) >> 30)) << 31);
		}
	}
	return ret;
}
