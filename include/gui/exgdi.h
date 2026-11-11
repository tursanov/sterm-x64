#if !defined GUI_EXGDI_H
#define GUI_EXGDI_H

#include "gdi.h"

__BEGIN_DECLS

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

extern PixmapFontPtr CreatePixmapFont(const char *sFileName, Color fg, Color bg);
extern void DeletePixmapFont(PixmapFontPtr pFont);
extern void PixmapTextOut(GCPtr pGC, PixmapFontPtr pFont, int x, int y,
	const char *sText);
	


extern void InitExGDI(void);


/*   Рисует окно на экране, при этом графический контекст принимает */
/*   размеры клиентской области */
extern void DrawWindow(GCPtr pGC, int x, int y, int w, int h, const char *sTitle,
	GCPtr pClientGC);
extern int titleCY;
extern int borderCX;

extern void DrawButton(GCPtr pGC, int x, int y, int cx, int cy,
	const char *text, bool down);

__END_DECLS

#endif		/* GUI_EXGDI_H */
