#ifndef GDI_H
#define GDI_H

#include "sysdefs.h"
#include "gui/metrics.h"

/* Basic colors */

enum {
	BLACK,          /* dark colors */
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	LIGHTGRAY,
	DARKGRAY,       /* light colors */
	LIGHTBLUE,
	LIGHTGREEN,
	LIGHTCYAN,
	LIGHTRED,
	LIGHTMAGENTA,
	YELLOW,
	WHITE
};

/* ///////////////////////////////////////////////////////////////////////////// */
/* Color definition */
typedef unsigned short Color;
#define GetRValue(c) ((uint8_t)((c & (2048 + 4096 + 8192 + 16384 + 32768)) >> 8)) 
#define GetGValue(c) ((uint8_t)((c & (32 + 64 + 128 + 256 + 512 + 1024)) >> 3)) 
#define GetBValue(c) ((uint8_t)((c & (1 + 2 + 4 + 8 + 16)) << 3)) 
#define RGB(r, g, b) ((Color)(((r & 0xf8) << 8) + ((g & 0xfc) << 3) + (b >> 3)))

extern const Color clBlack;
extern const Color clMaroon;
extern const Color clGreen;
extern const Color clOlive;
extern const Color clNavy;
extern const Color clPurple;
extern const Color clTeal;
extern const Color clGray;
extern const Color clSilver;
extern const Color clRed;
extern const Color clLime;
extern const Color clYellow;
extern const Color clBlue;
extern const Color clFuchsia;
extern const Color clAqua;
extern const Color clWhite;

extern const Color clRopnetBlue;
extern const Color clRopnetDarkBlue;
extern const Color clRopnetLightBlue;
extern const Color clRopnetGreen;
extern const Color clRopnetDarkGreen;
extern const Color clRopnetBrown;
extern const Color clRopnetDarkBrown;

/* ///////////////////////////////////////////////////////////////////////////// */
/* Rect & Box */

typedef struct {
    short x1, x2, y1, y2;
} Box, BOX, BoxRec, *BoxPtr;

typedef struct {
    short x, y, width, height;
} RECTANGLE, RectangleRec, *RectanglePtr;

/* /////////////////////////////////////////////////////////////////////////// */
/* Растровый шрифт */

typedef struct
{
	uint16_t width;
	uint16_t pitch;
	uint8_t *buffer;
} GLYPH, GlyphRec, *GlyphPtr;

#define MAX_GLYPHS	256
typedef struct
{
	uint16_t max_width;
	uint16_t max_height;
	uint16_t max_top;
	uint16_t max_bottom;
	uint16_t underline_y1;
	uint16_t underline_y2;
	bool var_size; 
	GLYPH glyphs[MAX_GLYPHS];
} FONT, FontRec, *FontPtr; 

/* Функции создания и удаления шрифта */
FontPtr CreateFont(const char *file_name, bool var_size);
void DeleteFont(FontPtr pFont);

/* /////////////////////////////////////////////////////////////////////////// */
/* Растровое изображение (24 бита) */

typedef struct
{
    int width;	/* Ширина */
    int height;	/* Высота */
	int pitch;	/* Кол-во байт в линии */
	uint8_t *data;	/* Данные */
} BITMAP, *BitmapPtr; 

BitmapPtr CreateBitmap(const char *file_name);
void DeleteBitmap(BitmapPtr pBitmap);

/* ///////////////////////////////////////////////////////////////////////////// */
/* Graphic Context struct */

#define R2_COPY 0x0
#define	R2_AND	0x1
#define R2_OR	0x2
#define R2_XOR	0x3

typedef struct {
	void		*vbuf;			/* Указатель на видеопамять */
	int			scanline;		/* Количество пикселей в строке (для экрана - DISCX, для памяти - width) */
	RECTANGLE 	box; 			/* Прямоугольник отсечения */
	Color 		pencolor;		/* Цвет пера */
	Color 		brushcolor;		/* Цвет кисти заполнения */
	int 		rop2mode;		/* Растровые операции для линий и т. д. */
	int 		ow, oh;			/* Промежуток между текстом при рисовании */
	Color 		textcolor;		/* Цвет символов */
	FontPtr		pFont;			/* Текущий шрифт */
/*	RegionPtr	pRgn;			// Текущий регион отсечения */
} GC, *GCPtr;

/* Функции инициализации и завершения работы с графикой */
bool InitVGA();
void ReleaseVGA();

GCPtr CreateGC(int x, int y, int w, int h);
GCPtr CreateMemGC(int w, int h);
void DeleteGC(GCPtr pGC);

/* Функции изменения параметров контекста */
int SetBrushColor(GCPtr pGC, Color c);
int SetPenColor(GCPtr pGC, Color c);
int SetTextColor(GCPtr pGC, Color c);
int SetRop2Mode(GCPtr pGC, int r2);
FontPtr SetFont(GCPtr pGC, FontPtr pFont);
/* RegionPtr SetRegion(GCPtr pGC, RegionPtr pRgn); */

void MoveGC(GCPtr pGC, int x, int y);
void ChangeGCSize(GCPtr pGC, int w, int h);
void SetGCBounds(GCPtr pGC, int x, int y, int w, int h);

static inline int GetCX(GCPtr pGC)
{
	return pGC ? pGC->box.width : 0;
}

static inline int GetCY(GCPtr pGC)
{
	return pGC ? pGC->box.height : 0;
}

static inline int GetMaxX(GCPtr pGC)
{
	return pGC ? pGC->box.width - 1 : 0;
}

static inline int GetMaxY(GCPtr pGC)
{
	return pGC ? pGC->box.height - 1 : 0;
}

/* Функции рисования */
void ClearGC(GCPtr pGC, Color c);
void FillBox(GCPtr pGC, int x, int y, int w, int h);
void Line(GCPtr pGC, int x1, int y1, int x2, int y2);

/* FIXME: w и h обрабатываются неправильно (см. gui/gdi.c) */
void DrawRect(GCPtr pGC, int x, int y, int w, int h);
void DrawBorder(GCPtr pGC, int x, int y, int w, int h, int bw,
	Color topColor, Color bottomColor);
void DrawHorzGrad(GCPtr pGC, int x, int y, int w, int h,
	Color fromColor, Color toColor);
void DrawVertGrad(GCPtr pGC, int x, int y, int w, int h,
	Color fromColor, Color toColor);

/* Функции копирования контекста */
void CopyGC(GCPtr pDest, int x, int y, 
	GCPtr pSrc, int sx, int sy, int sw, int sh);
	
/* Функции отрисовки текста */

#define DT_LEFT 0x1
#define DT_RIGHT 0x2
#define DT_TOP 0x4
#define DT_BOTTOM 0x8
#define DT_CENTER 0x10
#define DT_VCENTER 0x20

int TextWidth(FontPtr pFont, const char *sText);
int TextHeight(FontPtr pFont);

int GetTextWidth(GCPtr pGC, const char *text);
int GetMaxCharWidth(GCPtr pGC);
int GetTextHeight(GCPtr pGC);
void TextOut(GCPtr pGC, int x, int y, const char *sText);
void TextOutN(GCPtr pGC, int x, int y, const char *sText, int count);
void DrawText(GCPtr pGC, int x, int y, int w, int h, const char *sText, 
	int flags);
void RichTextOut(GCPtr pGC, int x, int y, const uint16_t *sText, int l);

/* Функции отрисовки растровых изображений */
/* 	x, y - точка отсчета, откуда будет идти рисование
	w, h - длина и ширина выводимой части изображения 
	(не больше ширины и высоты pBitmap)
	trans - делать прозрачным
	если transColor = -1 и trans <> 0 тогда 
	прозрачный цвет берется из левого нижнего угла,
	иначе - это transColor
*/	
#define FROM_BMP	((unsigned)-1)
#define BMP_CX		-1
#define BMP_CY		-1
void DrawBitmap(GCPtr pGC, BitmapPtr pBitmap, int x, int y, int w, int h,
	bool trans, unsigned transColor);

/* Рисование строки символов с определенными атрибутами */
 
/* Цвет символа */
enum
{
	SYM_COLOR_BLACK = 0,		/* Черный */
	SYM_COLOR_BLUE,				/* Синий */
	SYM_COLOR_GREEN,			/* Зеленый */
	SYM_COLOR_CYAN,				/* Голубой */
	SYM_COLOR_RED,				/* Красный */
	SYM_COLOR_PURPLE,			/* Пурпурный */
	SYM_COLOR_YELLOW,			/* Желтый */
	SYM_COLOR_WHITE				/* Белый */
};

/* Аттрибут символа */
enum
{
	SYM_ATTR_DEFAULT 	= 0x0,	/* Обычный */
	SYM_ATTR_INVERT 	= 0x1,	/* Инвертированный */
	SYM_ATTR_UNDERLINE 	= 0x2,	/* Подчеркнутый */
	SYM_ATTR_BLINK 		= 0x4,	/* Мигающий */
};

/* Строка символов */
typedef struct
{
	char *text;		/* Строка символов */
	int count;		/* Количество символов для вывода */
	uint8_t fg;		/* Цвет текста */
	uint8_t bg;		/* Цвет фона */
	uint8_t attr;		/* Аттрибуты */
	bool blink_hide; /* Убрать мигание */
} __attribute__ ((packed)) rich_text_t;

extern Color* symbol_palette;
/* Рисование строки с атрибутами */
void draw_rich_text(GCPtr pGC, int x, int y, rich_text_t *text);

#endif	/* GDI_H */
