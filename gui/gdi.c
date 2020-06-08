#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <vga.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/kd.h>

#include "genfunc.h"
#include "gui/gdi.h"
#include "gui/scr.h"
#include "gui/options.h"

const Color clBlack		= RGB(0, 0, 0);
const Color clMaroon	= RGB(128, 0, 0);
const Color clGreen		= RGB(0, 128, 0);
const Color clOlive		= RGB(128, 128, 0);
const Color clNavy		= RGB(0, 0, 128);
const Color clPurple	= RGB(128, 0, 128);
const Color clTeal		= RGB(0, 128, 128);
const Color clGray		= RGB(128, 128, 128);
const Color clSilver	= RGB(192, 192, 192);
const Color clRed		= RGB(255, 0, 0);
const Color clLime		= RGB(0, 255, 0);
const Color clYellow	= RGB(255, 255, 0);
const Color clBlue		= RGB(0, 0, 255);
const Color clFuchsia	= RGB(255, 0, 255);
const Color clAqua		= RGB(0, 255, 255);
const Color clWhite		= RGB(255, 255, 255);

/* взяты из strider */
const Color clRopnetBlue		= RGB(0x99, 0xcc, 0xff);
const Color clRopnetDarkBlue	= RGB(0x00, 0x66, 0xcc);
const Color clRopnetLightBlue	= RGB(0xcc, 0xee, 0xff);
const Color clRopnetGreen		= RGB(0x99, 0xcc, 0x99);
const Color clRopnetDarkGreen	= RGB(0x33, 0x66, 0x00);
const Color clRopnetBrown		= RGB(0xff, 0xcc, 0x99);
const Color clRopnetDarkBrown	= RGB(0xcc, 0x99, 0x33);


#define SWAP(x, y) { int temp = (x); (x) = (y); (y) = temp; }
#define SWAPT(x, y, t) { t temp = (x); (x) = (y); (y) = temp; }

static void *__gmem; /* Указатель на линейную видеопамять */

static int fbdev_fd;
static size_t fbdev_memory;
static size_t fbdev_startaddressrange;
/*static struct console_font_op fbdev_font;*/
static struct fb_var_screeninfo fbdev_textmode;
static struct fb_var_screeninfo fbdev_curmode;

static struct
{
	struct consolefontdesc font;
	char chardata[32 * 512];
} console_save;

#define ERRMSG printf

static bool set_console_mode(bool graphics)
{
	int tty_fd;
	bool ret = true;

	if ((tty_fd = open("/dev/tty", O_RDWR)) == -1)
	{
		ERRMSG("Error in open /dev/tty while set console mode: %s", strerror(errno));
		return false;
	}

	if (!graphics && ioctl(tty_fd, KDSETMODE, KD_TEXT) != 0)
	{
		ERRMSG("Error in set text console mode: %s\n", strerror(errno));
		ret = false;
	}

	if (ioctl(tty_fd, graphics ? GIO_FONTX : PIO_FONTX, &console_save.font) != 0)
	{
		ERRMSG("Error in %s console fonts: %s\n", graphics ? "save" : "restore",
				strerror(errno));
		ret = false;
	}

	if (graphics && ioctl(tty_fd, KDSETMODE, KD_GRAPHICS) != 0)
	{
		ERRMSG("Error in set graphics console mode: %s\n", strerror(errno));
		ret = false;
	}


	close(tty_fd);

	return ret;
}


#undef TEST
#ifdef TEST
void DrawTuneTable(void)
{
	GCPtr pGC = CreateGC(0, 0, DISCX, DISCY);
	int i, j;
	unsigned short c = 0;

	pGC->pencolor = 0xFFFF;
	pGC->brushcolor = 0;
	FillBox(pGC, 0, 0, DISCX, DISCY);

	for (j = 0; j < 50; j++)
	{
		for (i = 0; i < 16; i++)
		{
#define WIDTH	50
			c = 0xf800; /*1 << i;*/
			pGC->brushcolor = c;
			
			FillBox(pGC, i*WIDTH, 100, WIDTH, WIDTH);
/*			Line(pGC, i*WIDTH, 100, (i + 1)*WIDTH, 100);
			Line(pGC, i*WIDTH, 100 + WIDTH, (i + 1)*WIDTH, 100 + WIDTH);
			Line(pGC, i*WIDTH, 100, i*WIDTH, 100 + WIDTH);
			Line(pGC, (i + 1)*WIDTH, 100, (i + 1)*WIDTH, 100 + WIDTH);*/

		}
	}
	
	DeleteGC(pGC);
}
#endif


bool InitVGA()
{
/*    if (vga_init())
	{
	    printf("vga_init: %s\n", strerror(errno));
	    return false;
    }
    if (vga_setmode(G800x600x64K))
	{
	    printf("vga_setmode: %s\n", strerror(errno));
	    return false;
    }
	vga_setlinearaddressing();
	__gmem = vga_getgraphmem();*/

	struct fb_fix_screeninfo info;
	int fd;

	console_save.font.charcount = 512;
	console_save.font.charheight = 32;
	console_save.font.chardata = console_save.chardata;
	memset(console_save.chardata, 0, sizeof(console_save.chardata));


	if ((fd = open("/dev/fb0", O_RDWR)) < 0)
	{
		return false;
	}

	if (ioctl(fd, FBIOGET_FSCREENINFO, &info))
	{
		close(fd);
		return false;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &fbdev_textmode))
	{
		close(fd);
		return false;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &fbdev_curmode))
	{
		close(fd);
		return false;
	}


	fbdev_memory = info.smem_len;
	fbdev_fd = fd;

	fbdev_startaddressrange = 65536;
	while (fbdev_startaddressrange < fbdev_memory)
	{
		fbdev_startaddressrange <<= 1;
	}
	fbdev_startaddressrange -= 1;

	__gmem = mmap(0, fbdev_memory, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);


	set_console_mode(true);

	
#ifdef TEST
	DrawTuneTable();
	getchar();
#endif
	
	return true;
}

void ReleaseVGA()
{
	set_console_mode(false);

	munmap(__gmem, fbdev_memory);
	close(fbdev_fd);
//    vga_setmode(TEXT);
}

/* Функции создания и удаления шрифта */

size_t fsize(FILE *f)
{
	size_t pos, size;

	pos = ftell(f);
	if (pos == -1)
		return -1;
	if (fseek(f, 0, SEEK_END) == -1)
		return -1;
	size = ftell(f);
	if (size == -1)
		return -1;
	if (fseek(f, pos, SEEK_SET) == -1)
		return -1;

	return size;
}

void LoadGlyph(FontPtr font, GLYPH *glyph, uint8_t *buffer, int ofs,
		bool var_size)
{
	int i, j;
	int width, max_width;
    uint8_t mask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };
	uint8_t *glyph_ptr;
	uint8_t *ptr;
	int glyph_mask;
	int top, bottom;
	bool en_bottom;

	max_width = 0;
	top = bottom = -1;
	en_bottom = true;
	bottom = font->max_height;
	for (j = 0, glyph_ptr = glyph->buffer; j < font->max_height;
			j++, glyph_ptr += glyph->pitch)
	{
		glyph_mask = 0x80;
		for (i = 0, width = 0, ptr = glyph_ptr; i < font->max_width; i++)
		{
			if (*buffer & mask[ofs])
			{
				width = i;
				*ptr |= glyph_mask;
			}

			glyph_mask >>= 1;
			if (!glyph_mask)
			{
				ptr++;
				glyph_mask = 0x80;
			}
			
			if (ofs == 7)
			{
				ofs = 0;
				buffer++;
			}
			else
				ofs++;
		}

		if (!width)
		{
			if (en_bottom)
			{
				bottom = j-1;
				en_bottom = false;
			}
		}
		else if (top == -1)
		{
			top = j;
			en_bottom = true;
		}

		if (width > max_width)
			max_width = width;
	}

	if (top != -1)
	{
		if (font->max_top > top)
			font->max_top = top;
		if (font->max_bottom < bottom)
			font->max_bottom = bottom;
	}

	if ((char)(glyph - font->glyphs) == '_')
	{
		font->underline_y1 = top;
		font->underline_y2 = bottom;
	}

	max_width++;

	glyph->width = (var_size) ? max_width : font->max_width;
}

FontPtr CreateFont(const char *file_name, bool var_size)
{
	FILE *f;
	FontPtr font;
	size_t size;
	uint8_t *buffer;
	uint16_t glyph_pitch;
	size_t glyph_size;
	GLYPH *glyph;
	int i;
	int bit_pos;
	int glyph_bits;

	if (!(f = fopen(file_name, "rb")))
		return NULL;

	if ((size = fsize(f)) == -1)
	{
		fclose(f);
		return NULL;
	}

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

	if (!(font = malloc(sizeof(FONT))))
	{
		free(buffer);
		return NULL;
	}

	font->max_width = buffer[0];
	font->max_height = buffer[1];
	font->max_top = font->max_height;
	font->max_bottom = 0;

	glyph_pitch = ((font->max_width + 7) >> 3);
	glyph_size = glyph_pitch*font->max_height;
	glyph_bits = font->max_width*font->max_height;
	
	for (i = 0, bit_pos = 0, glyph = font->glyphs;
			i < MAX_GLYPHS; i++, glyph++, bit_pos += glyph_bits)
	{
		if (!(glyph->buffer = malloc(glyph_size + 10)))
		{
			glyph--;
			while (i--)
			{
				free(glyph->buffer);
				glyph--;
			}
			free(buffer);
			free(font);
			return NULL;
		}
		memset(glyph->buffer, 0, glyph_size);
		glyph->pitch = glyph_pitch;
		LoadGlyph(font, glyph, buffer + (bit_pos >> 3) + 2, bit_pos & 0x7,
				var_size);
	}

	font->var_size = var_size;
	font->glyphs[(uint8_t)' '].width = font->glyphs[(uint8_t)'^'].width;
	free(buffer);

	return font;
}

void DeleteFont(FontPtr pFont)
{
    if (pFont)
    {
		int i;
		GlyphPtr glyph;
			
		for (i = 0, glyph = pFont->glyphs; i < MAX_GLYPHS; i++, glyph++)
			if (glyph->buffer)
				free(glyph->buffer);
		free(pFont);
    }
}


/* ///////////////////////////////////////////////////////////////////////////// */
/* Создание и удаление Bitmap-ов */

#pragma pack(1)
/* windows style*/
typedef struct {
	/* BITMAPFILEHEADER*/
	uint8_t	bfType[2];
	uint32_t	bfSize;
	uint16_t	bfReserved1;
	uint16_t	bfReserved2;
	uint32_t	bfOffBits;
	/* BITMAPINFOHEADER*/
	uint32_t	BiSize;
	int32_t		BiWidth;
	int32_t		BiHeight;
	uint16_t	BiPlanes;
	uint16_t	BiBitCount;
	uint32_t	BiCompression;
	uint32_t	BiSizeImage;
	int32_t		BiXpelsPerMeter;
	int32_t		BiYpelsPerMeter;
	uint32_t	BiClrUsed;
	uint32_t	BiClrImportant;
} BMPFILEHEADER;
#pragma pack()


BitmapPtr CreateBitmap(const char *file_name)
{
	FILE *f;
	BMPFILEHEADER bfh;
	BitmapPtr bmp;
	int pitch;
	int i, j;
	uint8_t *buffer;
	uint8_t *bmp_ptr;

	if (!(bmp = malloc(sizeof(BITMAP)))){
		fprintf(stderr, "%s: memory allocation error.\n", __func__);
		return NULL;
	}

	if (!(f = fopen(file_name, "rb")))
	{
		fprintf(stderr, "%s: cannot open %s for reading: %m.\n", __func__, file_name);
		free(bmp);
		return NULL;
	}

   	if ((fread(&bfh, sizeof(BMPFILEHEADER), 1, f) != 1) ||
			bfh.bfType[0] != 'B' || bfh.bfType[1] != 'M' ||
			bfh.BiBitCount != 24 || bfh.BiCompression != 0 ||
			bfh.BiSize == 12)
   	{
		fprintf(stderr, "%s: cannot read from %s: %m.\n", __func__, file_name);
		fclose(f);
		free(bmp);
		return NULL;
   	}

   	bmp->width = bfh.BiWidth;
   	bmp->height = bfh.BiHeight;
	
	bmp->pitch = (bfh.BiWidth*2 + 3) & ~3;
	pitch = (bfh.BiWidth*3 + 3) & ~3;

	if (!(bmp->data = malloc(bmp->pitch*bmp->height)))
	{
		fprintf(stderr, "%s: cannot allocate BMP data.\n", __func__);
		fclose(f);
		free(bmp);
		return NULL;
	}

	if (!(buffer = malloc(pitch)))
	{
		fprintf(stderr, "%s: buffer allocation error.\n", __func__);
		fclose(f);
		free(bmp->data);
		free(bmp);
		return NULL;
	}

   	fseek(f, bfh.bfOffBits, SEEK_SET);

	bmp_ptr = (uint8_t *)bmp->data + bmp->pitch*(bmp->height - 1);
	for (j = 0; j < bmp->height; j++, bmp_ptr -= bmp->pitch)
	{
		uint8_t *buf_ptr;
		Color *ptr;
		
		if (fread(buffer, pitch, 1, f) != 1)
		{
			fprintf(stderr, "%s: error reading from %s: %m.\n", __func__, file_name);
			fclose(f);
			free(bmp->data);
			free(bmp);
			return NULL;
		}

		ptr = (Color *)bmp_ptr;
		buf_ptr = buffer;
		for (i = 0; i < bmp->width; i++, buf_ptr += 3)
			*ptr++ = (uint16_t)(RGB(buf_ptr[2], buf_ptr[1], buf_ptr[0]));
	}

	free(buffer);

	fclose(f);
	
	return bmp;
}

void DeleteBitmap(BitmapPtr pBitmap)
{
    if (pBitmap)
    {
		free(pBitmap->data);
		free(pBitmap);
    }
}

/* 
	Выделение прямоугольной области, вписанной в отсекающий прямоугольник
	(clipx, clipy, clipw, cliph), если это возможно
*/
static inline bool ClippingBox(int clipx, int clipy, int clipw, int cliph,
	int *x, int *y, int *w, int *h)
{
	register int clipx1=clipx;
	register int clipy1=clipy;
	register int clipx2=clipx1+clipw;
	register int clipy2=clipy1+cliph;
	
	if (*x + *w < clipx1 || *x > clipx2)
		return false;
	if (*y + *h < clipy1 || *y > clipy2)
		return false;
	if (*x < clipx1) {		/* left adjust */
		*w += *x - clipx1;
		*x = clipx1;
	}
	if (*y < clipy1) {		/* top adjust */
		*h += *y - clipy1;
		*y = clipy1;
	}
	if (*x + *w > clipx2)		/* right adjust */
		*w = clipx2 - *x + 1;	/* FIXME */
	if (*y + *h > clipy2)		/* bottom adjust */
		*h = clipy2 - *y + 1;	/* FIXME */
	return true;	
}


GCPtr CreateGC(int x, int y, int w, int h)
{
	if (x>=DISCX) return NULL;
	else if (x < 0) { w +=x; x=0;  }
	if (y>=DISCY) return NULL;
	else if (y < 0) { h +=y; y=0;  }
	if (x+w > DISCX) w = DISCX-x;	
	if (y+h > DISCY) h = DISCY-y;
	
	{
		GCPtr pGC = __new(GC);
		
		pGC->vbuf 			= __gmem;
		pGC->pencolor 		= clBlack;
		pGC->brushcolor 	= clWhite;
		pGC->rop2mode 		= R2_COPY;
		pGC->pFont 			= NULL;
		/* pGC->pRgn			= NULL; */
		pGC->box.x			= x;
		pGC->box.y			= y;
		pGC->box.width		= w;
		pGC->box.height		= h;
		pGC->scanline		= DISCX;
		
		pGC->ow = 1; pGC->oh = 0;
		pGC->textcolor = clBlack;
		
		return pGC;
	}
}

GCPtr CreateMemGC(int w, int h)
{
	GCPtr pGC = NULL;
	if (w > 0 && h > 0) {
		if (w > DISCX) w = DISCX;
		if (h > DISCY) h = DISCY;
		pGC = CreateGC(0, 0, w, h);
		if (pGC) pGC->vbuf 	= __calloc(w*h, Color);
		pGC->scanline		= w;
	}
	return pGC;
}

void DeleteGC(GCPtr pGC)
{
	if (pGC) {
		if (pGC->vbuf != __gmem)
			free(pGC->vbuf);
		free(pGC);
	}		
}

int SetBrushColor(GCPtr pGC, Color c)
{
	if (pGC) {
		SWAPT(c, pGC->brushcolor, Color);
		return c;
	} else
		return -1;
}

int SetPenColor(GCPtr pGC, Color c)
{
	if (pGC) {
		SWAPT(c, pGC->pencolor, Color);
		return c;
	} else
		return -1;
}

int SetTextColor(GCPtr pGC, Color c)
{
	if (pGC) {
		SWAPT(c, pGC->textcolor, Color);
		return c;
	} else
		return -1;
}

FontPtr SetFont(GCPtr pGC, FontPtr pFont)
{
	if (pGC) {
		SWAPT(pFont, pGC->pFont, FontPtr);
		return pFont;
	} else
		return NULL;
}

int SetRop2Mode(GCPtr pGC, int r2)
{
	if (pGC) {
		SWAPT(r2, pGC->rop2mode, int);
		return r2;
	} else
		return -1;
}

/*RegionPtr SetRegion(GCPtr pGC, RegionPtr pRgn)
{
	if (pGC) {
		SWAPT(pRgn, pGC->pRgn, RegionPtr);
		return pRgn;
	} else
		return NULL;
}*/

void MoveGC(GCPtr pGC, int x, int y)
{
	if (pGC && pGC->vbuf == __gmem) {
		pGC->box.x = x;
		pGC->box.y = y;
	}
}

void ChangeGCSize(GCPtr pGC, int w, int h)
{
	if (pGC) {
		pGC->box.width = w;
		pGC->box.height = h;
		if (pGC->vbuf != __gmem)
			pGC->vbuf = realloc(pGC->vbuf, w*h*sizeof(Color));
	}
}

void SetGCBounds(GCPtr pGC, int x, int y, int w, int h)
{
	if (pGC) {
		pGC->box.width = w; pGC->box.height = h;
		
		if (pGC->vbuf != __gmem)
			pGC->vbuf = realloc(pGC->vbuf, w*h*sizeof(Color));
		else
		{
			pGC->box.x = x; pGC->box.y = y;
		}
	}
}

void __memset2(void *s, unsigned short c, int w)
{
/*	int i;
	unsigned int *ptr = (unsigned int *)s;
	unsigned int c4 = (((unsigned int)c << 16) | (unsigned int)c);
	
	for (i = 0; i < w/2; i++)
		*ptr++ = c4;
	
	if (w & 1)
		*(unsigned short *)ptr = c;*/
		

	int i;
	unsigned short *ptr = (unsigned short *)s;

	for (i = 0; i < w; i++)
		*ptr++ = c;
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
} 
#endif

#define __memcpy(to, from, n) \
	memcpy(to, from, n)
	

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
		
#define BYTEWIDTH	(pGC->scanline*sizeof(Color))
#define VBUF		((unsigned char *)(pGC->vbuf))
#define ASSIGNVPOFFSET16(x, y, vp) vp = (y) * BYTEWIDTH + (x) * 2;

void FillBox(GCPtr pGC, int x, int y, int w, int h)
{
	if (pGC) {
	 	x+=pGC->box.x; y+=pGC->box.y;
	 	if  (ClippingBox(pGC->box.x, pGC->box.y, pGC->box.width, pGC->box.height,
				&x, &y, &w, &h)) {
			register Color c = pGC->brushcolor;
#if 0
			if (pGC->vbuf == __gmem) {
    			int vp;
    			int page;
    			int i;
    			ASSIGNVPOFFSET16(x, y, vp);
    			page = vp >> 16;
    			vp &= 0xffff;
    			vga_setpage(page);
    			for (i = 0; i < h; i++) {
					if (vp + w * 2 > 0x10000) {
	    				if (vp >= 0x10000) {
							page++;
							vga_setpage(page);
							vp &= 0xffff;
	    				} else {		/* page break within line */
							__memset2(VBUF + vp, c, (0x10000 - vp) / 2);
							page++;
							vga_setpage(page);
							__memset2(VBUF, c, ((vp + w * 2) & 0xffff) / 2);
							vp = (vp + BYTEWIDTH) & 0xffff;
							continue;
	    				}
        			}
					__memset2(VBUF + vp, c, w);
					vp += BYTEWIDTH;
				}	
    		} else
#endif
			{
				register Color *s = (Color *)pGC->vbuf + (y)*(pGC->scanline) + (x);
				register int j;
				register int cy = (h);
				for (j=0; j<cy; j++, s+=pGC->scanline) {
					__memset2((s), (c), (w));
				}	
			}
		}	
	}
}

/* ///////////////////////////////////////////////////////////////////////////// */
/* Line */

#if 0
static inline int muldiv64(int m1, int m2, int d)
{
/* int32 * int32 -> int64 / int32 -> int32 */
    int result;
    int dummy;
    __asm__(
	       "imull %%edx\n\t"
	       "idivl %4\n\t"
  :	       "=a"(result), "=d"(dummy)	/* out */
  :	       "0"(m1), "1"(m2), "g"(d)		/* in */
	       /***rjr***:	       "ax", "dx" ***/	/* mod */
	);
    return result;
}

#define INC_IF_NEG(y, result)				\
{							\
	__asm__("btl $31,%1\n\t"			\
		"adcl $0,%0"				\
		: "=r" ((int) result)			\
		: "rm" ((int) (y)), "0" ((int) result)	\
		);					\
}
#endif

static inline int muldiv64(int m1, int m2, int d)
{
    return (float) m1 * (float) m2 / ((float) d);
}


static inline int regioncode(int x, int y, int clipx1, int clipy1, int clipx2, int clipy2)
{
    int result = 0;
    if (x < clipx1)
		result |= 1;
    else if (x > clipx2)
		result |= 2;
    if (y < clipy1)
		result |= 4;
    else if (y > clipy2)
		result |= 8;
    return result;
}

#if 0
static inline int regioncode (int x, int y, 
	register int clipx1, register int clipy1, 
	register int clipx2, register int clipy2)
{
    int dx1, dx2, dy1, dy2;
    int result;
    result = 0;
    dy2 = clipy2 - y;
    INC_IF_NEG (dy2, result);
    result <<= 1;
    dy1 = y - clipy1;
    INC_IF_NEG (dy1, result);
    result <<= 1;
    dx2 = clipx2 - x;
    INC_IF_NEG (dx2, result);
    result <<= 1;
    dx1 = x - clipx1;
    INC_IF_NEG (dx1, result);
    return result;
}
#endif

#define line_start_linear(vb, s) \
	    vp = vb + y * bytesperrow + x * s;

#define line_loop_linear_a(m,i,u,v)	\
	    {    \
	    int d = ay - (ax >> 1);	\
	    if ((x = abs (dx)))	\
		do {	\
		    i;	\
		    if (d m 0) {	\
			vp v;	\
			d -= ax;	\
		    }	\
		    vp u;	\
		    d += ay;	\
		} while (--x);    \
	    }

#define line_loop_linear_b(m,i,u,v)	\
		{    \
		int d = ax - (ay >> 1);	\
		if ((y = abs (dy)))	\
		    do {	\
			i;	\
			if (d m 0) {	\
			    vp u;	\
			    d -= ay;	\
			}	\
			vp v;	\
			d += ax;	\
		    } while (--y);    \
		}
		
#define line_start_paged(s) \
	    fp = y * bytesperrow + x * s;	\
	    vga_setpage (fpp = (fp >> 16));	\
	    fp &= 0xFFFF;
		
#define line_loop_paged_a(m,i,u,v)	\
	    {    \
	    int d = ay - (ax >> 1);	\
	    if ((x = abs (dx)))	\
		do {	\
		    i;	\
		    if (d m 0) {	\
			fp v;	\
			d -= ax;	\
		    }	\
		    fp u;	\
		    d += ay;	\
		    if (fp & 0xFFFF0000) {	/* has it cross a page boundary ? */	\
			fpp += fp >> 16;	\
			vga_setpage (fpp);	\
		    }	\
		    fp &= 0x0000FFFF;	\
		} while (--x);    \
	    }

#define line_loop_paged_b(m,i,u,v)	\
		{    \
		int d = ax - (ay >> 1);	\
		if ((y = abs (dy)))	\
		    do {	\
			i;	\
			if (d m 0) {	\
			    fp u;	\
			    d -= ay;	\
			}	\
			fp v;	\
			d += ax;	\
			if (fp & 0xFFFF0000) {	\
			    fpp += fp >> 16;	\
			    vga_setpage (fpp);	\
			}	\
			fp &= 0x0000FFFF;	\
		    } while (--y);    \
		}

void Line(GCPtr pGC, int x1, int y1, int x2, int y2)
{
	if (pGC) {
    	register unsigned char *vp = NULL;
    	register int dx, dy, ax, ay, sx, sy, x, y;
    	register int bytesperrow = BYTEWIDTH;
		register int clipx1 = pGC->box.x, clipy1 = pGC->box.y,
					 clipx2 = clipx1+pGC->box.width-1, 
					 clipy2 = clipy1+pGC->box.height-1;
					 
/*		if (x1 > x2) SWAP(x1, x2);
		if (y1 > y2) SWAP(y1, y2);*/
		
		x1+=pGC->box.x; y1+=pGC->box.y;
		x2+=pGC->box.x; y2+=pGC->box.y;

		/* Cohen & Sutherland algorithm */
		for (;;) {
	    	register int r1 = regioncode (x1, y1, clipx1, clipy1, clipx2, clipy2);
	    	register int r2 = regioncode (x2, y2, clipx1, clipy1, clipx2, clipy2);
	    	if (!(r1 | r2))
				break;		/* completely inside */
	    	if (r1 & r2)
				return;		/* completely outside */
	    	if (r1 == 0) {
				SWAP (x1, x2);	/* make sure first */
				SWAP (y1, y2);	/* point is outside */
				r1 = r2;
	    	}
	    	if (r1 & 1) {	/* left */
				y1 += muldiv64 (clipx1 - x1, y2 - y1, x2 - x1);
				x1 = clipx1;
	    	} else if (r1 & 2) {	/* right */
				y1 += muldiv64 (clipx2 - x1, y2 - y1, x2 - x1);
				x1 = clipx2;
	    	} else if (r1 & 4) {	/* top */
				x1 += muldiv64 (clipy1 - y1, x2 - x1, y2 - y1);
				y1 = clipy1;
	    	} else if (r1 & 8) {	/* bottom */
				x1 += muldiv64 (clipy2 - y1, x2 - x1, y2 - y1);
				y1 = clipy2;
	    	}
		}	
		
#define	showline(s, cnt, op, rop) { \
	for(; cnt--; s op) \
		*s = *s rop c; \
}	

    	dx = x2 - x1;
    	dy = y2 - y1;
		ax = abs (dx) << 1;
    	ay = abs (dy) << 1;
    	sx = (dx >= 0) ? 1 : -1;
    	sy = (dy >= 0) ? 1 : -1;
    	x = x1;
    	y = y1;
		
#define insert_pixel { \
	register Color *v = (Color *)vp; \
	register int c = pGC->pencolor;	 \
	switch(pGC->rop2mode) { \
		case R2_COPY: \
			*v = c; \
			break; \
		case R2_XOR: \
			*v = *v ^ c; \
			break; \
		case R2_AND: \
			*v = *v & c; \
			break; \
		case R2_OR: \
			*v = *v | c; \
			break; \
	} \
}			

#define insert_pixel_p { \
	register Color *v = (unsigned short *) (vp + fp); \
	register int c = pGC->pencolor;	 \
	switch(pGC->rop2mode) { \
		case R2_COPY: \
			*v = c; \
			break; \
		case R2_XOR: \
			*v = *v ^ c; \
			break; \
		case R2_AND: \
			*v = *v & c; \
			break; \
		case R2_OR: \
			*v = *v | c; \
			break; \
	} \
}			
//		if (pGC->vbuf != __gmem) {	
			line_start_linear(pGC->vbuf, 2);
			if (ax > ay) {
				if(sx > 0) {
		    		line_loop_linear_a(>=,insert_pixel,+=2,+=bytesperrow*sy);
				} else {
		    		line_loop_linear_a(>,insert_pixel,-=2,+=bytesperrow*sy);
				}
			} else {
				sx <<= 1;
				if(sy > 0) {
		    		line_loop_linear_b(>=,insert_pixel,+=sx,+=bytesperrow);
				} else {
		    		line_loop_linear_b(>,insert_pixel,+=sx,-=bytesperrow);
				}
			}
    		//insert_pixel;
/*		} else {
			int fpp;
			int fp;
			vp = VBUF;
			
	    	line_start_paged(2);
	    	if (ax > ay) {
				if(sx > 0) {
		    		line_loop_paged_a(>=,insert_pixel_p,+=2,+=bytesperrow*sy);
				} else {
		    		line_loop_paged_a(>,insert_pixel_p,-=2,+=bytesperrow*sy);
				}
	    	} else {
				sx <<= 1;
				if(sy > 0) {
		    		line_loop_paged_b(>=,insert_pixel_p,+=sx,+=bytesperrow);
				} else {
		    		line_loop_paged_b(>,insert_pixel_p,+=sx,-=bytesperrow);
				}
	   		}
	    	insert_pixel_p;
		}	*/
	}	
}

void ClearGC(GCPtr pGC, Color c)
{
	if (pGC) {
		SWAPT(c, pGC->brushcolor, Color);
		FillBox(pGC, 0, 0, GetCX(pGC), GetCY(pGC));
		SWAPT(c, pGC->brushcolor, Color);
	}	
}

#define ASSIGNVPOFFSET8(x, y, vp) vp = (y) * BYTEWIDTH + (x);
#define ASSIGNVPOFFSET8_EX(x, y, vp, gc) vp = (y) * (gc)->scanline*2 + (x);
#define BYTEWIDTH_EX(gc) (gc)->scanline*sizeof(Color)
#define VBUF_EX(gc) ((char *)(gc)->vbuf)

void CopyGC(GCPtr pGC, int x, int y, 
	GCPtr pSrc, int sx, int sy, int sw, int sh)
{
	if (pGC && pSrc)
	{
		int nx=sx+pSrc->box.x, ny=sy+pSrc->box.y, nw = sw, nh = sh;
		int nx1=x+pGC->box.x, ny1=y+pGC->box.y;
		int i;
		
		if (ClippingBox(pSrc->box.x, pSrc->box.y, pSrc->box.width-1, pSrc->box.height-1,
					&nx, &ny, &nw, &nh) &&
				ClippingBox(pGC->box.x, pGC->box.y, pGC->box.width-1, pGC->box.height-1,
					&nx1, &ny1, &nw, &nh))
		{
			register Color *bp = ((Color *)pSrc->vbuf + ny*pSrc->scanline + nx);
			register Color *bp1 = ((Color *)pGC->vbuf + ny1*pGC->scanline + nx1);


			for (i = 0; i < nh; i++)
			{
				memmove(bp1, bp, nw*2);
				//__memcpy(bp1, bp, nw*2);
				bp += pSrc->scanline; 
				bp1 += pGC->scanline;
			}
		}
	}
			
/*    		register int vp;
    		register int page;
    		register int i;
			int bw = nw*2;
			nx1 *= sizeof(Color);
			nw 	*= 2;
    		ASSIGNVPOFFSET8(nx1, ny1, vp);
    		page = vp >> 16;
    		vp &= 0xffff;
    		vga_setpage(page);
    		for (i = 0; i < nh; i++) {
				if (vp + nw > 0x10000) {
	    			if (vp >= 0x10000) {
						page++;
						vga_setpage(page);
						vp &= 0xffff;
	    			} else {		// page break within line
						__memcpy(VBUF + vp, bp, 0x10000 - vp);
						page++;
						vga_setpage(page);
						__memcpy(VBUF, bp + 0x10000 - vp,
						(vp + nw) & 0xffff);
						vp = (vp + BYTEWIDTH) & 0xffff;
						bp += bw;
						continue;
	    			}
        		}
				__memcpy(VBUF + vp, bp, nw);
				bp += bw;
				vp += BYTEWIDTH;
   			}*/
#if 0
		if (pGC->vbuf == __gmem && pSrc->vbuf != __gmem) {
			int nx=sx+pSrc->box.x, ny=sy+pSrc->box.y, nw=sw, nh=sh;
			int nx1=x+pGC->box.x, ny1=y+pGC->box.y;
			if (ClippingBox(pSrc->box.x, pSrc->box.y, pSrc->box.width-1, pSrc->box.height-1, &nx, &ny, &nw, &nh) && ClippingBox(pGC->box.x, pGC->box.y, pGC->box.width-1, pGC->box.height-1,	&nx1, &ny1, &nw, &nh)) {
				register char *bp = (char *)((Color *)pSrc->vbuf + ny*pSrc->scanline + nx);
    			register int vp;
    			register int page;
    			register int i;
				int bw = nw*2;
				nx1 *= sizeof(Color);
				nw 	*= 2;
    			ASSIGNVPOFFSET8(nx1, ny1, vp);
    			page = vp >> 16;
    			vp &= 0xffff;
    			vga_setpage(page);
    			for (i = 0; i < nh; i++) {
					if (vp + nw > 0x10000) {
	    				if (vp >= 0x10000) {
							page++;
							vga_setpage(page);
							vp &= 0xffff;
	    				} else {		/* page break within line */
							__memcpy(VBUF + vp, bp, 0x10000 - vp);
							page++;
							vga_setpage(page);
							__memcpy(VBUF, bp + 0x10000 - vp,
			 				(vp + nw) & 0xffff);
							vp = (vp + BYTEWIDTH) & 0xffff;
							bp += bw;
							continue;
	    				}
        			}
					__memcpy(VBUF + vp, bp, nw);
					bp += bw;
					vp += BYTEWIDTH;
    			}
			}	
		} else if (pGC->vbuf != __gmem && pSrc->vbuf == __gmem) {
			int nx=sx+pSrc->box.x, ny=sy+pSrc->box.y, nw=sw, nh=sh;
			int nx1=x+pGC->box.x, ny1=y+pGC->box.y;
			if (ClippingBox(pSrc->box.x, pSrc->box.y, pSrc->box.width-1, pSrc->box.height-1, &nx, &ny, &nw, &nh) && ClippingBox(pGC->box.x, pGC->box.y, pGC->box.width-1, pGC->box.height-1, &nx1, &ny1, &nw, &nh)) {
    			int vp;
    			int page;
    			char *bp;
    			int i;
				int bw = nw*2;
				nx1 *= sizeof(Color);
				nx *= sizeof(Color);
				nw 	*= 2;
    			bp = (char *)((Color *)pGC->vbuf + pGC->scanline*ny1 + nx1);
					
    			ASSIGNVPOFFSET8_EX(nx, ny, vp, pSrc);
    			page = vp >> 16;
    			vp &= 0xffff;
    			vga_setpage(page);
    			for (i = 0; i < nh; i++) {
					if (vp + nw > 0x10000) {
	    				if (vp >= 0x10000) {
							page++;
							vga_setpage(page);
							vp &= 0xffff;
	    				} else {		/* page break within line */
							__memcpy(bp, VBUF_EX(pSrc) + vp, 0x10000 - vp);
							page++;
							vga_setpage(page);
							__memcpy(bp + 0x10000 - vp, VBUF_EX(pSrc),
								(vp + nw) & 0xffff);
							vp = (vp + BYTEWIDTH_EX(pSrc)) & 0xffff;
							bp += bw;
							continue;
	    				}
        			};
					__memcpy(bp, VBUF_EX(pSrc) + vp, nw);
					bp += bw;
					vp += BYTEWIDTH_EX(pSrc);
    			}
			}
		}	
	}
#endif

}	

void DrawRect(GCPtr pGC, int x, int y, int w, int h)
{
	if (pGC) {
		int x1=x+w, y1=y+h;
		Line(pGC, x, y, x1, y);
		Line(pGC, x, y1, x1, y1);
		Line(pGC, x, y, x, y1);
		Line(pGC, x1, y, x1, y1);
	}
}

void DrawBorder(GCPtr pGC, int x, int y, int w, int h, int bw,
	Color topColor, Color bottomColor)
{
	if (pGC) {
		int x1 = x + w - 1, y1 = y + h - 1, i;
		Color prevc = pGC->pencolor;
		for (i=0; i<bw; i++) {
			pGC->pencolor = topColor;
			Line(pGC, x+i, y+i, x1-i, y+i);
			Line(pGC, x+i, y+i, x+i, y1 - i + 1);
			pGC->pencolor = bottomColor;
			Line(pGC, x + i, y1-i, x1 - i + 1, y1-i);
			Line(pGC, x1-i, y+i, x1-i, y1-i);
		}	
		pGC->pencolor = prevc;
	}

}

void DrawHorzGrad(GCPtr pGC, int x, int y, int w, int h,
	Color fromColor, Color toColor)
{
	if (!pGC) return;
	x+=pGC->box.x; y+=pGC->box.y;
	if (ClippingBox(pGC->box.x, pGC->box.y, 
					pGC->box.width, pGC->box.height, &x, &y, &w, &h)) {
		float r = GetRValue(fromColor);
		float g = GetGValue(fromColor);
		float b = GetBValue(fromColor);
		float dr = (GetRValue(toColor)-r)/h;
		float dg = (GetGValue(toColor)-g)/h;
		float db = (GetBValue(toColor)-b)/h;
		register int i,c;
		if (VBUF != __gmem) {
			register Color *s = (Color *)pGC->vbuf + y*pGC->scanline + x;
			for (i=0; i<h; i++, s += BYTEWIDTH) {
				c = RGB((int)r, (int)g, (int)b);
				__memset2(s, c, w);
				r+=dr; g+=dg; b+=db;
			}
		} else {
			x-=pGC->box.x; y-=pGC->box.y;
			for (i=0; i<h; i++) {
				pGC->pencolor = RGB((int)r, (int)g, (int)b);
				Line(pGC, x, y+i, x + w - 1, y+i);
				r+=dr; g+=dg; b+=db;
			}
		}	
	}
}

#define round(v) (int)(v + .5)

void DrawVertGrad(GCPtr pGC, int x, int y, int w, int h,
	Color fromColor, Color toColor)
{
/*	float r = GetRValue(fromColor);
	float g = GetGValue(fromColor);
	float b = GetBValue(fromColor);
	float dr = (GetRValue(toColor)-r)/w;
	float dg = (GetGValue(toColor)-g)/w;
	float db = (GetBValue(toColor)-b)/w;*/
	float cl = fromColor, dc = (double)(toColor - fromColor) / w;
	int i;
	for (i = 0; i < w; i++) {
//		pGC->pencolor = RGB(round(r), round(g), round(b));
		pGC->pencolor = round(cl);
		Line(pGC, x + i, y, x + i, y + h - 1);
//		r += dr; g += dg; b += db;
		cl += dc;
	}
}

void PutBox(GCPtr pGC, int x, int y, int w, int h, int pw, void *ptr)
{
	x+=pGC->box.x; y+=pGC->box.y;
	if (ClippingBox(pGC->box.x, pGC->box.y, pGC->box.width, pGC->box.height,
					&x, &y, &w, &h)) {
		if (pGC->vbuf != __gmem) {			
			register int i;
			register int cnt = w<<1;
			register Color *s = (Color *)pGC->vbuf + y*pGC->scanline + x;
			register Color *p = (Color *)ptr;
			for (i=h; i; i--, p+=pw, s+=pGC->scanline)
				__memcpy(s, p, cnt);
		} /*else {	
			register char *bp = ptr;
			register int vp;
			register int page;
			register int i;
			register int bw = pw*sizeof(Color);
		
			x *= sizeof(Color);
			w *= 2;
    		ASSIGNVPOFFSET8(x, y, vp);
    		page = vp >> 16;
    		vp &= 0xffff;
    		vga_setpage(page);
    		for (i = 0; i < h; i++) {
				if (vp + w > 0x10000) {
					if (vp >= 0x10000) {
						page++;
						vga_setpage(page);
						vp &= 0xffff;
	    			} else {		// page break within line
						__memcpy(VBUF + vp, bp, 0x10000 - vp);
						page++;
						vga_setpage(page);
						__memcpy(VBUF, bp + 0x10000 - vp,
			 			(vp + w) & 0xffff);
						vp = (vp + BYTEWIDTH) & 0xffff;
						bp += bw;
						continue;
	    			}
        		}
				__memcpy(VBUF + vp, bp, w);
				bp += bw;
				vp += BYTEWIDTH;
    		}
		}	*/
	}				
}

/* Рисование растрового изображения */
void DrawBitmap(GCPtr pGC, BitmapPtr pBitmap, int x, int y, int w, int h,
	bool trans, unsigned transColor)
{
	uint8_t *bmp_ptr;
	Color *vid_ptr;
	Color *src_ptr;
	Color *dst_ptr;
	int i, j;
	
	if (!pGC || !pBitmap || pGC->vbuf == __gmem)
		return;
	
	x+=pGC->box.x;
	y+=pGC->box.y;
	
	if (w < 0 || w > pBitmap->width)
		w = pBitmap->width;
	
	if (h < 0 || h > pBitmap->height)
		h = pBitmap->height;
	
	if (!ClippingBox(pGC->box.x, pGC->box.y, pGC->box.width,
				pGC->box.height, &x, &y, &w, &h))
		return;

	bmp_ptr = pBitmap->data;
	vid_ptr = (Color *)pGC->vbuf + y*pGC->scanline + x;
	
	if (trans)
	{
		if (transColor == -1)
			transColor = *(Color *)(bmp_ptr +
					pBitmap->pitch*(pBitmap->width - 1));

		for (j = 0; j < h; j++, vid_ptr += pGC->scanline,
				bmp_ptr += pBitmap->pitch)
		{
			dst_ptr = vid_ptr;
			src_ptr = (Color *)bmp_ptr;
			for (i = 0; i < w; i++, dst_ptr++, src_ptr++)
			{
				if (*src_ptr != transColor)
					*dst_ptr = *src_ptr;
			}
		}
	}
	else
	{
		w *= 2;
		for (j = 0; j < h; j++, vid_ptr += pGC->scanline,
				bmp_ptr += pBitmap->pitch)
			__memcpy(vid_ptr, bmp_ptr, w);
	}
}	

int TextWidth(FontPtr pFont, const char *sText)
{
	int text_width;
	
    if (!pFont && !sText)
		return 0;
	
	for (text_width = 0; *sText; sText++)
			text_width += pFont->glyphs[(uint8_t)*sText].width;

	return text_width;
}

int TextHeight(FontPtr pFont)
{
    if (pFont)
		return (!pFont) ? 0 : pFont->max_height/* + pFont->oh*/;
    else
		return -1;
}

int GetTextWidth(GCPtr pGC, const char *text) {
	if (pGC->pFont != NULL)
		return TextWidth(pGC->pFont, text);
	return 0;
}

int GetMaxCharWidth(GCPtr pGC) {
	if (pGC->pFont != NULL)
		return pGC->pFont->max_width;
	return 0;
}

int GetTextHeight(GCPtr pGC) {
	if (pGC->pFont != NULL)
		return pGC->pFont->max_height;
	return 0;
}

static void text_out(GCPtr pGC, int x, int y,
		int bound_x1, int bound_y1, int bound_x2, int bound_y2,
		const char *text, int count)
{
	FontPtr font;
	int clip_x1, clip_y1, clip_x2, clip_y2;
	int glyph_width;
	
	if (!pGC || !pGC->pFont || !text || !count)
		return;

	font = pGC->pFont;

	clip_x1 = pGC->box.x;
	clip_y1 = pGC->box.y;
	clip_x2 = clip_x1 + pGC->box.width - 1;
	clip_y2 = clip_y1 + pGC->box.height - 1;

	x += clip_x1;
	y += clip_y1;

	bound_x1 += clip_x1;
	bound_x2 += clip_x1;
	bound_y1 += clip_y1;
	bound_y2 += clip_y1;

	if (clip_x1 < bound_x1)
		clip_x1 = bound_x1;
	if (clip_y1 < bound_y1)
		clip_y1 = bound_y1;
	
	if (clip_x2 > bound_x2)
		clip_x2 = bound_x2;
	if (clip_y2 > bound_y2)
		clip_y2 = bound_y2;


	if (y >= clip_y2 || y + font->max_height <= clip_y1)
		return;

	for (; *text && count > 0; text++, x += glyph_width, count--)
	{
		GLYPH *glyph;
		int ofs_x, ofs_y;
		uint8_t *glyph_ptr, *ptr;
		int width, height;
		int i, j;
		int x_draw, y_draw;
		Color text_color;

		if (isspace(*text) && *text != ' ')
		{
			glyph_width = 0;
			continue;
		}

		glyph = &font->glyphs[(uint8_t)*text];
		glyph_width = glyph->width;
		
		if (x + glyph_width <= clip_x1)
			continue;

		if (x >= clip_x2)
			break;

		if (x < clip_x1)
		{
			ofs_x = clip_x1 - x;
			width = glyph->width - ofs_x;
		}
		else
		{
			ofs_x = 0;
			width = glyph->width;
		}

		if (y < clip_y1)
		{
			ofs_y = clip_y1 - y;
			height = font->max_height - ofs_y;
		}
		else
		{
			ofs_y = 0;
			height = font->max_height;
		}

		if (x + width > clip_x2)
			width = clip_x2 - x;

		if (y + height > clip_y2)
			height = clip_y2 - y;

		x_draw = x + ofs_x;
		y_draw = y + ofs_y;

		glyph_ptr = glyph->buffer + glyph->pitch*ofs_y + (ofs_x >> 3);
		ofs_x &= 0x7;

		text_color = pGC->textcolor;

		//if (pGC->vbuf != __gmem)
		{
			Color *vp = (Color *)pGC->vbuf + y_draw*pGC->scanline + x_draw;

			for (j = 0; j < height;
					j++, glyph_ptr += glyph->pitch, vp += pGC->scanline)
			{
				Color *vptr;
				uint32_t mask;
				uint32_t b0;
				
				mask = 0x80 >> ofs_x;
				ptr = glyph_ptr;
				b0 = *ptr++;
				vptr = vp;
				
				for (i = 0; i < width; i++, vptr++)
				{
					if (b0 & mask)
						*vptr = text_color;
					mask >>= 1;
					if (!mask)
					{
						mask = 0x80;
						b0 = *ptr++;
					}
				}
			}
		}
/*		else
		{
			for (j = 0; j < height; j++, glyph_ptr += glyph->pitch, y_draw++)
			{
				uint32_t mask;
				uint32_t b0;
				int x_draw1;
				
				mask = 0x80 >> ofs_x;
				ptr = glyph_ptr;
				b0 = *ptr++;
				
				for (i = 0, x_draw1 = x_draw; i < width; i++, x_draw1++)
				{
					if (b0 & mask)
					{
						int vp;
						ASSIGNVPOFFSET16(x_draw1, y_draw, vp);
						vga_setpage(vp >> 16);
						*(uint16_t *) (VBUF + (vp & 0xffff)) = text_color;
					}
					mask >>= 1;
					if (!mask)
					{
						mask = 0x80;
						b0 = *ptr++;
					}
				}
			}
		}*/
	}
}

void TextOut(GCPtr pGC, int x, int y, const char *text)
{
	text_out(pGC, x, y, 0, 0, pGC->box.width, pGC->box.height, text,
			strlen(text));
}

void TextOutN(GCPtr pGC, int x, int y, const char *text, int count)
{
	text_out(pGC, x, y, 0, 0, pGC->box.width, pGC->box.height, text,
			count);
}

void DrawText(GCPtr pGC, int x, int y, int w, int h, const char *text,
	int flags)
{
	int x1, y1, x2, y2;
	
	if (!flags)
		flags = DT_CENTER | DT_VCENTER;

	x1 = x;
	y1 = y;
	x2 = x1 + w;
	y2 = y1 + h;
	
	if (flags & DT_RIGHT)
		x = x2 - TextWidth(pGC->pFont, text);
	if (flags & DT_BOTTOM)
		y = y2 - TextHeight(pGC->pFont);
	if (flags & DT_CENTER)
		x = (x1 + x2 - TextWidth(pGC->pFont, text)) >> 1;
	if (flags & DT_VCENTER)
		y = (y1 + y2 - TextHeight(pGC->pFont)) >> 1;
	
	text_out(pGC, x, y, x1, y1, x2, y2, text, strlen(text));
}

static Color palette[] =
{
	RGB(0x00, 0x00, 0x00),		/* черный */
	RGB(0x00, 0x00, 0xff),		/* синий */
	RGB(0x00, 0xff, 0x00),		/* зеленый */
	RGB(0x00, 0xff, 0xff),		/* голубой */
	RGB(0xff, 0x00, 0x00),		/* красный */
	RGB(0xff, 0x00, 0xff),		/* пурпурный */
	RGB(0xff, 0xff, 0x00),		/* желтый */
	RGB(0xff, 0xff, 0xff),		/* белый */
};

Color* symbol_palette = palette;

void draw_rich_text(GCPtr pGC, int x, int y, rich_text_t *text)
{
	FontPtr font;
	int clip_x1, clip_y1, clip_x2, clip_y2;
	int glyph_width, glyph_height;
	int n;
	char *s;
	Color *vp;
	Color fg, bg;

	if (!pGC || !pGC->pFont || pGC->pFont->var_size || 
			pGC->vbuf == __gmem || !text || !text->count)
		return;

	font = pGC->pFont;
	glyph_width = font->max_width;
	glyph_height = font->max_height;

	if (y > pGC->box.height-1 || y + glyph_height < 0)
		return;

	s = text->text;

	clip_x1 = pGC->box.x;
	clip_y1 = pGC->box.y;
	clip_x2 = clip_x1 + pGC->box.width-1;
	clip_y2 = clip_y1 + pGC->box.height-1;

	x += clip_x1;
	y += clip_y1;

	fg = palette[(text->fg < ASIZE(palette)) ? text->fg : ASIZE(palette)-1];
	bg = palette[(text->bg < ASIZE(palette)) ? text->bg : ASIZE(palette)-1];
	if (text->attr & SYM_ATTR_INVERT)
		SWAP(fg, bg);

	if ((text->attr & SYM_ATTR_BLINK) && text->blink_hide)
	{
		int x1, x2, y1, y2;
		int i;

		x1 = x;
		y1 = y;
		x2 = x1 + text->count*glyph_width;
		y2 = y1 + glyph_height;

		if (x1 < clip_x1)
			x1 = clip_x1;
		else if (x1 > clip_x2)
			return;
		if (y1 < clip_y1)
			y1 = clip_y1;
		else if (y1 > clip_y2)
			return;
		if (x2 > clip_x2)
			x2 = clip_x2;
		else if (x2 < clip_x1)
			return;
		if (y2 > clip_y2)
			y2 = clip_y2;
		else if (y2 < clip_y1)
			return;

		vp = (Color *)pGC->vbuf + y1*pGC->scanline + x1;
		for (i = y1; i<=y2; i++, vp += pGC->scanline)
			__memset2(vp, bg, x2-x1+1);
		return;
	}

	for (n = 0; n < text->count; s++, x += glyph_width)
	{
		GLYPH *glyph;
		int ofs_x, ofs_y;
		uint8_t *glyph_ptr, *ptr;
		int width, height;
		int i, j;
		int x_draw, y_draw;

		glyph = &font->glyphs[(uint8_t)*s];
		
		if (x + glyph_width <= clip_x1)
			continue;

		if (x >= clip_x2)
			break;

		if (x < clip_x1)
		{
			ofs_x = clip_x1 - x;
			width = glyph_width - ofs_x;
		}
		else
		{
			ofs_x = 0;
			width = glyph_width;
		}

		if (y < clip_y1)
		{
			ofs_y = clip_y1 - y;
			height = glyph_height - ofs_y;
		}
		else
		{
			ofs_y = 0;
			height = glyph_height;
		}

		x_draw = x + ofs_x;
		y_draw = y + ofs_y;

		glyph_ptr = glyph->buffer + glyph->pitch*ofs_y + (ofs_x >> 3);
		ofs_x &= 0x7;

		vp = (Color *)pGC->vbuf + y_draw*pGC->scanline + x_draw;

		if (*s == ' ')
		{
			for (j = 0; j < height; j++, vp += pGC->scanline)
				__memset2(vp, bg, width);
		}
		else
		{
			for (j = 0; j < height;
					j++, glyph_ptr += glyph->pitch, vp += pGC->scanline)
			{
				Color *vptr;
				uint32_t mask;
				uint32_t b0;
				
				mask = 0x80 >> ofs_x;
				ptr = glyph_ptr;
				b0 = *ptr++;
				vptr = vp;
				
				for (i = 0; i < width; i++, vptr++)
				{
					*vptr = (b0 & mask) ? fg : bg;
					mask >>= 1;
					if (!mask)
					{
						mask = 0x80;
						b0 = *ptr++;
					}
				}
			}
		}

		if (text->attr & SYM_ATTR_UNDERLINE)
		{
			int i;

			vp = (Color *)pGC->vbuf +
				(y + font->underline_y1)*pGC->scanline + x_draw;
			
			for (i = y + font->underline_y1; i <= y + font->underline_y2;
					i++, vp += pGC->scanline)
			{
				if (i > clip_y1 && i < clip_y2)
					__memset2(vp, fg, width);
			}
		}
	}
}

/*
 * Создать подчеркнутый символ. В текстовом режиме мы просто хранили
 * подчеркнутые символы в специальной области таблицы символов. Поэтому
 * для создания подчеркнутого символа использовалась его перекодировка.
 * В графическом режиме измененный код указывает на необходимость
 * подчеркнуть символ после его вывода на экран.
 */
uint8_t underline_ch(uint8_t ch)
{
	if (isdigit(ch))
		return ch+0xc2;
	else if ((ch >= 0x80) && (ch < 0xa0))
		return ch-0x80;
	else
		return ch;
}

/* Получить реальный символ для подчеркнутого */
bool map_underline(uint8_t *ch)
{
	if (ch == NULL)
		return false;
	else if ((*ch >= 0x40) && (*ch <= 0x5f))	/* Латинские символы */
		return true;
	else if ((*ch >= 0xf2) && (*ch <= 0xfb)){	/* Цифры */
		*ch -= 0xc2;
		return true;
	}else if (*ch <= 0x1f){				/* Русские символы */
		*ch += 0x80;
		return true;
	}else
		return false;
}

/* Возвращает символ и его цвет */
uint8_t GetRealColor(uint16_t c, Color *fg, Color *bg)
{
	static Color attr_pal[] = { 
		RGB(0x00, 0x00, 0x00),		/* черный */
		RGB(0x00, 0x00, 0xff),		/* синий */
		RGB(0x00, 0xff, 0x00),		/* зеленый */
		RGB(0x00, 0xff, 0xff),		/* голубой */
		RGB(0xff, 0x00, 0x00),		/* красный */
		RGB(0xff, 0x00, 0xff),		/* пурпурный */
		RGB(0xff, 0xff, 0x00),		/* желтый */
		RGB(0xff, 0xff, 0xff),		/* белый */
	};
	uint8_t ch = c & 0xff, attr;
	if (!scr_is_req())
		ch = recode(ch);
	attr = c >> 8;
	if (attr == 0){		/* атрибуты по умолчанию */
		if (ch < 0x20){
			*fg = cfg.bg_color;
			*bg = cfg.rus_color;
			ch += 0x40;
		}else{
			if (is_lat(ch))
				*fg = cfg.lat_color;
			else
				*fg = cfg.rus_color;
			*bg = cfg.bg_color;
		}
	}else if (attr == 0xff){		/* выделенная область */
		if (ch < 0x20)
			ch += 0x40;
		*fg = cfg.bg_color;
		*bg = cfg.rus_color;
	}else{
		*fg = attr_pal[(attr & 0x0f) % ASIZE(attr_pal)];
		*bg = attr_pal[(attr >> 4) % ASIZE(attr_pal)];
	}
	return ch;
}

void RichTextOut(GCPtr pGC, int x, int y, const uint16_t *text, int l)
{
	FontPtr font;
	int clip_x1, clip_y1, clip_x2, clip_y2;
	int glyph_width;
	int n;
	
	if (!pGC || !pGC->pFont || pGC->vbuf == __gmem || !text)
		return;

	font = pGC->pFont;

	clip_x1 = pGC->box.x;
	clip_y1 = pGC->box.y;
	clip_x2 = clip_x1 + pGC->box.width;
	clip_y2 = clip_y1 + pGC->box.height;

	x += clip_x1;
	y += clip_y1;

	if (y >= clip_y2 || y + font->max_height <= clip_y1)
		return;

	for (n = 0; n < l; n++, text++, x += glyph_width)
	{
		GLYPH *glyph;
		int ofs_x, ofs_y;
		uint8_t *glyph_ptr, *ptr;
		int width, height;
		int i, j;
		int x_draw, y_draw;
		Color *vp;
		Color fg_color;
		Color bg_color;
		uint8_t ch;
		bool need_underline;

		ch = GetRealColor(*text, &fg_color, &bg_color);
		need_underline = map_underline(&ch);

		if (isspace(ch) && ch != ' ')
		{
			glyph_width = 0;
			continue;
		}

		glyph = &font->glyphs[(uint8_t)ch];
		glyph_width = glyph->width;
		
		if (x + glyph_width <= clip_x1)
			continue;

		if (x >= clip_x2)
			break;

		if (x < clip_x1)
		{
			ofs_x = clip_x1 - x;
			width = glyph->width - ofs_x;
		}
		else
		{
			ofs_x = 0;
			width = glyph->width;
		}

		if (y < clip_y1)
		{
			ofs_y = clip_y1 - y;
			height = font->max_height - ofs_y;
		}
		else
		{
			ofs_y = 0;
			height = font->max_height;
		}

		x_draw = x + ofs_x;
		y_draw = y + ofs_y;

		glyph_ptr = glyph->buffer + glyph->pitch*ofs_y + (ofs_x >> 3);
		ofs_x &= 0x7;

		vp = (Color *)pGC->vbuf + y_draw*pGC->scanline + x_draw;

		if (ch == ' ')
		{
			for (j = 0; j < height; j++, vp += pGC->scanline)
				__memset2(vp, bg_color, width);
		}
		else
		{
			for (j = 0; j < height;
					j++, glyph_ptr += glyph->pitch, vp += pGC->scanline)
			{
				Color *vptr;
				uint32_t mask;
				uint32_t b0;
				
				mask = 0x80 >> ofs_x;
				ptr = glyph_ptr;
				b0 = *ptr++;
				vptr = vp;
				
				for (i = 0; i < width; i++, vptr++)
				{
					*vptr = (b0 & mask) ? fg_color : bg_color;
					mask >>= 1;
					if (!mask)
					{
						mask = 0x80;
						b0 = *ptr++;
					}
				}
			}
		}

		if (need_underline)
		{
			vp = (Color *)pGC->vbuf +
				(y_draw + font->max_height - 3)*pGC->scanline +
				x_draw + 1;
			__memset2(vp, fg_color, width-2);
		}
	}
}

