#ifndef EXGDI_H
#define EXGDI_H

#include "gdi.h"

extern Color clBtnFace;
extern Color clBtnHighlight;
extern Color clBtnShadow;
extern Color clBtnText;
extern Color clWindowCaptionFrom;
extern Color clWindowCaptionTo;
extern Color clWindowText;
extern Color clWindowTextShadow;

/* Развернутый шрифт (для ускорения операции вывода текста) */

typedef struct {
	void *data;			/* Данные */
	int max_width, max_height;	/* Длина и ширина символа */
	Color bg;			/* Цвет фона */
	Color fg;			/* Цвет символов */
} PIXMAPFONT, *PixmapFontPtr;

PixmapFontPtr CreatePixmapFont(const char *sFileName, Color fg, Color bg);
void DeletePixmapFont(PixmapFontPtr pFont);
void PixmapTextOut(GCPtr pGC, PixmapFontPtr pFont, int x, int y,
	const char *sText);
	


/*  */
void InitExGDI();


/*   Рисует окно на экране, при этом графический контекст принимает */
/*   размеры клиентской области */
void DrawWindow(GCPtr pGC, int x, int y, int w, int h, const char *sTitle,
	GCPtr pClientGC);
extern int titleCY;
extern int borderCX;

void DrawButton(GCPtr pGC, int x, int y, int cx, int cy,
    const char *text, bool down);

#endif /* EXGDI_H */
