//pvmkport.c
//Port of CDogs to Neki32
//Bryan E. Topp <betopp@betopp.com> 2025

#include "pvmkport.h"
#include "grafx.h"
#include <sc.h>
#include <stdint.h>
#include <stdbool.h>

//Note that there's other hacks around the codebase too - high score saving, "joystick" polling

void delay(int ms)
{
	int begin = _sc_getticks();
	while(_sc_getticks() < begin + ms)
	{
		_sc_pause();
	}
	return;
}

int kbhit(void)
{
	return 0;
}

//Destination for draw ops - usually a host-side buffer from malloc
//May occasionally get set to 0xA0000 to write to "the front buffer" (VGA memory)
unsigned char PvmkPhonyFrontBuf[SCREENBYTES]; //Todo - eventually we need to show this...
static unsigned char *PvmkDstScreen = PvmkPhonyFrontBuf;
void SetDstScreen( void *screen )
{
	PvmkDstScreen = screen;
	if(screen == (void*)(0xA0000))
		PvmkDstScreen = PvmkPhonyFrontBuf;
}

void *GetDstScreen( void )
{
	if(PvmkDstScreen == PvmkPhonyFrontBuf)
		return (void*)(0xA0000);
	
	return PvmkDstScreen;
}

static uint16_t PvmkPalette[256];
void SetPalette( void *palette )
{
	//Pack to 16-bit directcolor words when setting the palette
	unsigned char *pal24 = (unsigned char*)palette;
	for(int cc = 0; cc < 256; cc++)
	{
		unsigned char r = pal24[0] & 0x3F;
		unsigned char g = pal24[1] & 0x3F;
		unsigned char b = pal24[2] & 0x3F;
		pal24 += 3;
		
		PvmkPalette[cc] = 0;
		
		PvmkPalette[cc] <<= 5;
		PvmkPalette[cc] |= r >> 1;
		
		PvmkPalette[cc] <<= 6;
		PvmkPalette[cc] |= g >> 0;
		
		PvmkPalette[cc] <<= 5;
		PvmkPalette[cc] |= b >> 1;
	}
}

void SetColorZero(int r, int g, int b)
{
	PvmkPalette[0] = 0;
	
	PvmkPalette[0] <<= 5;
	PvmkPalette[0] |= (r & 0x3F) >> 1;
	
	PvmkPalette[0] <<= 6;
	PvmkPalette[0] |= (g & 0x3F) >> 0;
	
	PvmkPalette[0] <<= 5;
	PvmkPalette[0] |= (b & 0x3F) >> 1;
}

static uint16_t PvmkBuffers[3][240][320]; //Truecolor buffers, rest of game wants indexed color
static int PvmkBufferNext;
static int PvmkBufferLast;
void CopyToScreen( void )
{
	//Depallettize into the truecolor buffer that we picked earlier
	unsigned char *src = PvmkDstScreen;
	uint16_t *dst = &(PvmkBuffers[PvmkBufferNext][20][0]);
	for(int pp = 0; pp < SCREENBYTES; pp++)
	{
		*dst = PvmkPalette[*src];
		dst++;
		src++;
	}
	
	//Enqueue a flip to that buffer
	int ActiveBufAddr = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, &(PvmkBuffers[PvmkBufferNext][0][0]));
	if(ActiveBufAddr < 0)
	{
		//Error while trying to flip
		return;
	}
	
	//See what buffer we'll use next time - neither the one currently displayed, nor the one we just asked for
	PvmkBufferLast = PvmkBufferNext;
	for(int bb = 0; bb < 3; bb++)
	{
		if(PvmkBufferNext == bb)
			continue; //Just asked to present this one
		
		if((uint16_t*)(uintptr_t)ActiveBufAddr == &(PvmkBuffers[bb][0][0]))
			continue; //Was told this one is on the screen
		
		//Found the buffer that we can use next
		PvmkBufferNext = bb;
		break;
	}
}

void AltScrCopy( void )
{
	//This does the same thing as CopyToScreen but uses a different instruction sequence.
	//Maybe that mattered for some older Intel parts? Who knows.
	CopyToScreen();
}

void vsync(void)
{
	//Resubmit the last buffer flipped, wait until it's active
	while(1)
	{
		int ActiveBufAddr = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, &(PvmkBuffers[PvmkBufferLast][0][0]));
		if(ActiveBufAddr < 0)
		{
			//Error while trying to flip
			return;
		}
		if(ActiveBufAddr != (int)(uintptr_t)&(PvmkBuffers[PvmkBufferLast][0][0]))
		{
			_sc_pause();
			continue;
		}
		return;
	}
}

static int PvmkClipLeft = 0;
static int PvmkClipRight = 319;
static int PvmkClipTop = 0;
static int PvmkClipBottom = 199;
void SetClip( int left, int top, int right, int bottom )
{	
	PvmkClipLeft = left;
	PvmkClipRight = right;
	PvmkClipTop = top;
	PvmkClipBottom = bottom;
}

static uint16_t PvmkSpriteWidth;
static uint16_t PvmkSpriteHeight;
static uint16_t PvmkSpritePitch;

//Modifies x, y, width, height, image pointer. Returns whether anything remains.
static bool DoClip(int *x, int *y, void **picptr)
{
	unsigned char *pic = (unsigned char*)(*picptr); //char just to make math easier
	
	PvmkSpriteWidth  = ((uint16_t*)pic)[0];
	PvmkSpriteHeight = ((uint16_t*)pic)[1];
	PvmkSpritePitch = PvmkSpriteWidth;
	
	pic += 4; //Should point at start of image data now
	
	if(PvmkClipTop > *y)
	{
		//clipUpper
		int PastTop = PvmkClipTop - *y;
		if(PastTop >= (int)PvmkSpriteHeight)
			return false;
		
		PvmkSpriteHeight -= PastTop;
		*y = PvmkClipTop;
		pic += PvmkSpriteWidth * PastTop;
	}
	
	if(*y + PvmkSpriteHeight - 1 > PvmkClipBottom)
	{
		//clipLower
		int PastBottom = (*y + PvmkSpriteHeight - 1) - PvmkClipBottom;
		if(PastBottom >= (int)PvmkSpriteHeight)
			return false;
		
		PvmkSpriteHeight -= PastBottom;
	}
	
	if(PvmkClipLeft > *x)
	{
		//clipLeft
		int PastLeft = PvmkClipLeft - *x;
		if(PastLeft >= (int)PvmkSpriteWidth)
			return false;
		
		PvmkSpriteWidth -= PastLeft;
		*x = PvmkClipLeft;
		pic += PastLeft;
	}
	
	if(*x + PvmkSpriteWidth - 1 > PvmkClipRight)
	{
		//clipRight
		int PastRight = (*x + PvmkSpriteWidth - 1) - PvmkClipRight;
		if(PastRight >= (int)PvmkSpriteWidth)
			return false;
		
		PvmkSpriteWidth -= PastRight;
	}
	
	*picptr = (void*)pic;
	return true;
}

void DrawPic( int x, int y, void *pic, void *code )
{
	(void)code;
	
	if(!DoClip(&x, &y, &pic))
		return;
	
	unsigned char *dst = PvmkDstScreen + x + (y * 320);
	unsigned char *src = (unsigned char*)pic;
	for(uint16_t yy = 0; yy < PvmkSpriteHeight; yy++)
	{
		for(uint16_t xx = 0; xx < PvmkSpriteWidth; xx++)
		{
			dst[xx] = src[xx];
		}
		dst += 320;
		src += PvmkSpritePitch;
	}
}

void DrawTPic( int x, int y, void *pic, void *code )
{	
	(void)code;
	
	if(!DoClip(&x, &y, &pic))
		return;
	
	unsigned char *dst = PvmkDstScreen + x + (y * 320);
	unsigned char *src = (unsigned char*)pic;
	for(uint16_t yy = 0; yy < PvmkSpriteHeight; yy++)
	{
		for(uint16_t xx = 0; xx < PvmkSpriteWidth; xx++)
		{
			if(src[xx])
				dst[xx] = src[xx];
		}
		dst += 320;
		src += PvmkSpritePitch;
	}	
}
	
void DrawTTPic( int x, int y, void *pic, void *table, void *rle )
{	
	(void)rle;
	
	if(!DoClip(&x, &y, &pic))
		return;
	
	unsigned char *dst = PvmkDstScreen + x + (y * 320);
	unsigned char *src = (unsigned char*)pic;
	unsigned char *tran = (unsigned char*)table;
	for(uint16_t yy = 0; yy < PvmkSpriteHeight; yy++)
	{
		for(uint16_t xx = 0; xx < PvmkSpriteWidth; xx++)
		{
			if(src[xx])
				dst[xx] = tran[src[xx]];
		}
		dst += 320;
		src += PvmkSpritePitch;
	}
}

void DrawBTPic( int x, int y, void *pic, void *table, void *rle )
{
	(void)rle;
	
	if(!DoClip(&x, &y, &pic))
		return;
	
	unsigned char *dst = PvmkDstScreen + x + (y * 320);
	unsigned char *src = (unsigned char*)pic;
	unsigned char *tran = (unsigned char*)table;
	for(uint16_t yy = 0; yy < PvmkSpriteHeight; yy++)
	{
		for(uint16_t xx = 0; xx < PvmkSpriteWidth; xx++)
		{
			if(src[xx])
				dst[xx] = tran[dst[xx]];
		}
		dst += 320;
		src += PvmkSpritePitch;
	}	
}

