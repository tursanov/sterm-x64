#ifndef EXGDI_H
#define EXGDI_H

#if defined __cplusplus
extern "C" {
#endif

#include "gdi.h"

extern Color clBtnFace;
extern Color clBtnHighlight;
extern Color clBtnShadow;
extern Color clBtnText;
extern Color clWindowCaptionFrom;
extern Color clWindowCaptionTo;
extern Color clWindowText;
extern Color clWindowTextShadow;

/* ��������� ���� (��� �᪮७�� ����樨 �뢮�� ⥪��) */

typedef struct {
	void *data;			/* ����� */
	int max_width, max_height;	/* ����� � �ਭ� ᨬ���� */
	Color bg;			/* ���� 䮭� */
	Color fg;			/* ���� ᨬ����� */
} PIXMAPFONT, *PixmapFontPtr;

PixmapFontPtr CreatePixmapFont(const char *sFileName, Color fg, Color bg);
void DeletePixmapFont(PixmapFontPtr pFont);
void PixmapTextOut(GCPtr pGC, PixmapFontPtr pFont, int x, int y,
	const char *sText);
	


/*  */
void InitExGDI();


/*   ����� ���� �� �࠭�, �� �⮬ ����᪨� ���⥪�� �ਭ����� */
/*   ࠧ���� ������᪮� ������ */
void DrawWindow(GCPtr pGC, int x, int y, int w, int h, const char *sTitle,
	GCPtr pClientGC);
extern int titleCY;
extern int borderCX;

void DrawButton(GCPtr pGC, int x, int y, int cx, int cy,
    const char *text, bool down);

#if defined __cplusplus
}
#endif

#endif		/* EXGDI_H */
