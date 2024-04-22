/* ‡ £®«®Ά®ª δ ©«  BMP (Ά§οβ® ¨§ wingdi.h) */

#include "sysdefs.h"

typedef struct tagBITMAPFILEHEADER {
	uint16_t	bfType;
	uint32_t	bfSize;
	uint16_t	bfReserved1;
	uint16_t	bfReserved2;
	uint32_t	bfOffBits;
} BITMAPFILEHEADER;
