/* Общие функции. (c) gsr, 2024 */

#include "x3data/common.hpp"
#include "sysdefs.h"

/* Сжатие файла шаблона (w и h задаются в точках) */
bool compressPicture(const uint8_t *src, size_t len, size_t w, size_t h, vector<uint8_t> &dst)
{
	if ((src == NULL) || (len == 0) || (len > MAX_TX_LEN))
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
