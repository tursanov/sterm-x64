#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/exgdi.h"

Color clBtnFace;
Color clBtnHighlight;
Color clBtnShadow;
Color clBtnText;
Color clWindowCaptionFrom;
Color clWindowCaptionTo;
Color clWindowText;
Color clWindowTextShadow;

int titleCY 	= 20;
int borderCX = 1;

extern void PutBox(GCPtr pGC, int x, int y, int w, int h, int pw, void *ptr);
extern size_t fsize(FILE *f);

void InitExGDI()
{
	clBtnFace		= clSilver;
	clBtnHighlight		= clWhite;
	clBtnShadow		= clGray;
	clBtnText		= clBlack;
	clWindowCaptionFrom	= clBlue;
	clWindowCaptionTo	= clNavy;
	clWindowText		= clWhite;
	clWindowTextShadow	= clBlue;
}

/* 
   Рисует окно на экране, при этом графический контекст принимает
   размеры клиентской области
*/
void DrawWindow(GCPtr pGC, int x, int y, int w, int h, const char *sTitle,
	GCPtr pClientGC)
{
	int client_x = x + borderCX;
	int client_y = y + titleCY + borderCX;
	int client_w = w - 2 * borderCX;
	int client_h = h - (titleCY + borderCX);
	int title_y = y + borderCX;
	int title_h = titleCY - 2 * borderCX;
	Color prevc;
	DrawBorder(pGC, x, y, w, titleCY, borderCX, clBtnHighlight, clBtnShadow);
	DrawBorder(pGC, x, y, w, h, borderCX, clBtnHighlight, clBtnShadow);
	DrawHorzGrad(pGC, client_x, title_y, client_w, title_h,
		clWindowCaptionFrom, clWindowCaptionTo);

	if (pGC->pFont) {
		prevc = SetTextColor(pGC, clWindowTextShadow);
		SetTextColor(pGC, clWindowText);
		DrawText(pGC, client_x, title_y, client_w, title_h,
			sTitle, DT_CENTER | DT_VCENTER);
		SetTextColor(pGC, prevc);
	}
	if (pClientGC)
		SetGCBounds(pClientGC, client_x, client_y, client_w, client_h);
}

void DrawButton(GCPtr pGC, int x, int y, int cx, int cy,
    const char *text, bool down)
{
	if (pGC) {
    	int i;
	Color hl, sd/*, fc*/;
    	int ofs;
    	float R,G,B, DR,DG,DB;
    	int x1=x+cx, y1=y+cy;
    	int fh = cy-5;
    
   		if (!down)
    	{
			hl = clBtnHighlight;
			sd = clBtnShadow;
//			fc = clBtnFace;
			ofs = 0;
    	}
    	else
    	{
			hl = clBtnShadow;
			sd = clBtnHighlight;
//			fc = clBtnFace;
			ofs = 1;
    	}
        
    	R = GetRValue(hl);
    	G = GetGValue(hl);
    	B = GetBValue(hl);
    	DR = (GetRValue(sd)-R)/cy;
    	DG = (GetGValue(sd)-G)/cy;
    	DB = (GetBValue(sd)-B)/cy;
    
    	SetBrushColor(pGC, clBtnFace);
    	FillBox(pGC, x+1, y, cx-1, cy);
    
    	SetPenColor(pGC, clBlack);        
    	Line(pGC, x+1, y, x1-1, y);
    	Line(pGC, x+1, y1, x1-1, y1);
    	Line(pGC, x, y+1, x, y1);
    	Line(pGC, x1, y+1, x1, y1);
    	SetPenColor(pGC, sd);
    	DrawRect(pGC, x+1, y+1, cx-2, cy-2);
    
   		SetPenColor(pGC, hl);
    	for (i=0; i<2; i++)
    	{
			Line(pGC, x+2+i, y+2+i, x+2+i, y1-2-i);
			Line(pGC, x+2+i, y+2+i, x1-2, y+2+i);
    	}

    	for (i=0; i<fh; i++)
    	{
			if (i > 0) { R += DR; G += DG; B += DB; }
			SetPenColor(pGC, RGB((int)R, (int)G, (int)B));
			Line(pGC, x+4, y+4+i, x1-2, y+4+i);
    	}
    
    	if (text && pGC->pFont)
    	{
    		SetTextColor(pGC, hl);
			DrawText(pGC, x+5, y+5+ofs, cx-8, cy-8, text, 0);
    		SetTextColor(pGC, clBtnText);
			DrawText(pGC, x+4, y+4+ofs, cx-8, cy-8, text, 0);
    	}
	}	
}


/* //////////////////////////////////////////////////////////////////////////// */

PixmapFontPtr CreatePixmapFont(const char *sFileName, Color fg, Color bg)
{
	unsigned char *buffer;
	size_t size;
	FILE *f;
	PixmapFontPtr pFont;
	int fontsize, i, bp, bo;
	uint8_t *data;
	Color *ptr;
	const uint8_t mask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };


	if (!(f = fopen(sFileName, "rb")))
		return NULL;
	size = fsize(f);

	if (!(buffer = malloc(size)))
	{
		fclose(f);
		return NULL;
	}

	if (fread(buffer, size, 1, f) != 1)
	{
		free(buffer);
		fclose(f);
		return NULL;
	}

	fclose(f);

	pFont = __new(PIXMAPFONT);
		
	pFont->max_width = buffer[0];
	pFont->max_height = buffer[1];
	fontsize = pFont->max_width*pFont->max_height*256;
	pFont->fg = fg;
	pFont->bg = bg;
	
	pFont->data = malloc(fontsize*sizeof(Color));
	data = buffer + 2;
		
	for (i=0, ptr=pFont->data; i<fontsize; i++, ptr++)
	{
		bp = i >> 3; bo = i % 8;
		if (data[bp] & mask[bo])
			*ptr = fg;
		else
			*ptr = bg;	
	}

	free(buffer);
	
        return pFont;
}

void DeletePixmapFont(PixmapFontPtr pFont)
{
	if (pFont) {
		free(pFont->data);
		free(pFont);
	}
}

#define __memcpy(to, from, n) memcpy(to, from, n)
	
	
#if 0
#define __memcpy(to, from, n) { \
  register int dummy1; \
  register long dummy2, dummy3; \
    __asm__ __volatile__("cld\n\t" \
    	    "cmpl $0,%%edx\n\t" \
    	    "jle 2f\n\t" \
	    "movl %%edi,%%ecx\n\t" \
	    "andl $1,%%ecx\n\t" \
	    "subl %%ecx,%%edx\n\t" \
	    "rep ; movsb\n\t"	/* 16-bit align destination */ \
	    "movl %%edx,%%ecx\n\t" \
	    "shrl $2,%%ecx\n\t" \
	    "jz 3f\n\t" \
	    "rep ; movsl\n\t" \
	    "3:\n\t" \
	    "testb $1,%%dl\n\t" \
	    "je 1f\n\t" \
	    "movsb\n" \
	    "1:\ttestb $2,%%dl\n\t" \
	    "je 2f\n\t" \
	    "movsw\n" \
	    "2:\n" \
  :         "=d"(dummy1), "=D"(dummy2), "=S"(dummy3)   /* fake output */  \
  :	    "0"(n), "1"((long) (to)), "2"((long) (from)) \
  :	    "cx"/***rjr***, "dx", "di", "si"***/ \
  ); \
}
#endif


/*#define PutBox(vbuf, w, h, vw, pw, ptr) { \
		register int i; \
		register int cnt = (w)<<1; \
		register Color *s = (Color *)(vbuf); \
		register Color *p = (Color *)(ptr); \
		for (i=h; i; i--, p+=(pw), s+=(vw)) \
			__memcpy(s, p, cnt); \
}*/

void PixmapTextOut(GCPtr pGC, PixmapFontPtr pFont, int x, int y,
	const char *sText)
{
	if (pGC && pFont && sText) {
		register uint8_t *p;
		register Color *ptr, *vbuf;
		register int fw=pFont->max_width, fh=pFont->max_height;
		register int sym_w = fw;
		const int sym_size = fw*fh;
		const int sl = strlen(sText);
		register int cx = sl*fw;

		if (y+fh > pGC->box.height)
			fh=pGC->box.height-y;
		if (x + cx > pGC->box.width)
			cx = pGC->box.width-x;
			
		if (fh <= 0 && cx <= 0)
			return;
		x+=pGC->box.x; y+=pGC->box.y; cx+=x;
		
		vbuf = pGC->vbuf + y*(pGC->scanline<<1) + x;
		for (p=(uint8_t*)sText; *p && x<cx; p++, x+=fw, vbuf+=sym_w) {
			if ((x+sym_w)>cx) sym_w = cx-x;
			ptr = (Color *)pFont->data + (*p)*sym_size;
			/* PutBox(vbuf, sym_w, fh, pGC->scanline, fw, ptr); */
			PutBox(pGC, x, y, sym_w, fh, fw, ptr);
			
		}
	}
}

inline int getlen(register uint16_t *s)
{
	register uint16_t *p = s;
	while (*p) p++;
	return p-s-1;
}

#if 0
#define __memset2(s, c, w) { \
 	register int dummy1; \
  	register long dummy2; \
  	register int dummy3; \
	register size_t count = (w); \
    __asm__ __volatile__( \
	       "cld\n\t" \
	       "cmpl $12,%%edx\n\t" \
	       "jl 1f\n\t"	/* if (count >= 12) */ \
\
	       "movzwl %%ax,%%eax\n\t" \
	       "movl %%eax,%%ecx\n\t" \
	       "shll $16,%%ecx\n\t"	/* c |= c << 16 */ \
	       "orl %%ecx,%%eax\n\t" \
\
	       "movl %%edi,%%ecx\n\t" \
	       "andl $2,%%ecx\n\t"	/* s & 2 */ \
	       "jz 2f\n\t" \
	       "decl %%edx\n\t"	/* count -= 1 */ \
	       "movw %%ax,(%%edi)\n\t"	/* align to longword boundary */ \
	       "addl $2,%%edi\n\t" \
\
	       "2:\n\t" \
	       "movl %%edx,%%ecx\n\t" \
	       "shrl $1,%%ecx\n\t" \
	       "rep ; stosl\n\t"	/* fill longwords */ \
\
	       "andl $1,%%edx\n"	/* one 16-bit uint16_t left? */ \
	       "jz 3f\n\t"	/* no, finished */ \
	       "1:\tmovl %%edx,%%ecx\n\t"	/* <= 12 entry point */ \
	       "rep ; stosw\n\t" \
	       "3:\n\t" \
  :            "=a"(dummy1), "=D"(dummy2), "=d"(dummy3) /* fake outputs */ \
  :	       "0"(c), "1"(s), "2"(count) \
  :	       /***rjr***"ax",*/ "cx"/*, "dx", "di"*/ \
  ); \
} \

#define __memcpy(to, from, n) { \
  register int dummy1; \
  register long dummy2, dummy3; \
    __asm__ __volatile__("cld\n\t" \
    	    "cmpl $0,%%edx\n\t" \
    	    "jle 2f\n\t" \
	    "movl %%edi,%%ecx\n\t" \
	    "andl $1,%%ecx\n\t" \
	    "subl %%ecx,%%edx\n\t" \
	    "rep ; movsb\n\t"	/* 16-bit align destination */ \
	    "movl %%edx,%%ecx\n\t" \
	    "shrl $2,%%ecx\n\t" \
	    "jz 3f\n\t" \
	    "rep ; movsl\n\t" \
	    "3:\n\t" \
	    "testb $1,%%dl\n\t" \
	    "je 1f\n\t" \
	    "movsb\n" \
	    "1:\ttestb $2,%%dl\n\t" \
	    "je 2f\n\t" \
	    "movsw\n" \
	    "2:\n" \
  :         "=d"(dummy1), "=D"(dummy2), "=S"(dummy3)   /* fake output */  \
  :	    "0"(n), "1"((long) (to)), "2"((long) (from)) \
  :	    "cx"/***rjr***, "dx", "di", "si"***/ \
  ); \
}
#endif
