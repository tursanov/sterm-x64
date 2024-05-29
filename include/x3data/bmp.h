/* ‡ £®«®Ά®ª δ ©«  BMP (Ά§οβ® ¨§ wingdi.h) */

#if !defined X3DATA_BMP_H
#define X3DATA_BMP_H

#include "sysdefs.h"

typedef struct tagBITMAPINFOHEADER{
	uint32_t	biSize;
	int32_t		biWidth;
	int32_t		biHeight;
	uint16_t	biPlanes;
	uint16_t	biBitCount;
	uint32_t	biCompression;
#define BI_RGB		0L
#define BI_RLE8		1L
#define BI_RLE4		2L
#define BI_BITFIELDS	3L
#define BI_JPEG		4L
#define BI_PNG		5L
	uint32_t	biSizeImage;
	int32_t		biXPelsPerMeter;
	int32_t		biYPelsPerMeter;
	uint32_t	biClrUsed;
	uint32_t	biClrImportant;
} __attribute__((__packed__)) BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
	uint8_t		rgbBlue;
	uint8_t		rgbGreen;
	uint8_t		rgbRed;
	uint8_t		rgbReserved;
} __attribute__((__packed__)) RGBQUAD;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD			bmiColors[1];
} __attribute__((__packed__)) BITMAPINFO;

typedef struct tagBITMAPFILEHEADER {
	uint16_t	bfType;
	uint32_t	bfSize;
	uint16_t	bfReserved1;
	uint16_t	bfReserved2;
	uint32_t	bfOffBits;
} __attribute__((__packed__)) BITMAPFILEHEADER;

#endif		/* X3DATA_BMP_H */
