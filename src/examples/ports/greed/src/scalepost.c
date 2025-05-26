//scalepost.c
//Implementation of ScalePost in C
//Bryan E. Topp <betopp@betopp.com> 2025

#include "d_global.h"
#include "r_public.h"
#include "protos.h"

void (*actionhook)(void); // not sure what to do with this... used for one callback, fireweapon

void RF_BlitView(void)
{
	if(windowWidth == SCREENWIDTH)
	{
		uint8_t *src = viewbuffer;
		uint8_t *dst = (uint8_t*)viewLocation;
		for(int bb = 0; bb < windowSize; bb++)
		{
			dst[bb] = src[bb];
		}
	}
	else
	{
		uint8_t *src = viewbuffer;
		uint8_t *dst = (uint8_t*)viewLocation;
		for(int yy = 0; yy < windowHeight; yy++)
		{
			for(int xx = 0; xx < windowWidth; xx++)
			{
				dst[xx] = src[xx];
			}
			src += windowWidth;
			dst += SCREENWIDTH;
		}
	}
}

//Now isn't this easier than fucking around in the assembler?
//The ARM9 on the Neki32 tends to be bandwidth-limited, rather than ALU-limited.
//So we have plenty of time to spend on CPU cycles trying to get this done. -betopp
byte*       sp_dest;      // the bottom most pixel to be drawn (in vie
byte*       sp_source;    // the first pixel in the vertical post (may
byte*       sp_colormap;  // pointer to a 256 byte color number to pal
int32_t     sp_frac;      // fixed point location past sp_source
int32_t     sp_fracstep;  // fixed point step value
int         sp_count;     // the number of pixels to draw
int32_t     sp_loopvalue;
void ScalePost(void)
{
	//byte *sp_dest_iter = sp_dest;
	sp_dest -= SCREENWIDTH * sp_count;
	for(int pp = 0; pp < sp_count; pp++)
	{
		sp_dest += SCREENWIDTH;
		
		*sp_dest = sp_colormap[sp_source[(sp_frac & (sp_loopvalue-1)) >> FRACBITS]];
		
		sp_frac += sp_fracstep;
		
	}
}

void ScaleMaskedPost(void)
{
	//byte *sp_dest_iter = sp_dest;
	sp_dest -= SCREENWIDTH * sp_count;
	for(int pp = 0; pp < sp_count; pp++)
	{
		sp_dest += SCREENWIDTH;
		
		if(sp_source[(sp_frac & (sp_loopvalue-1)) >> FRACBITS])
			*sp_dest = sp_colormap[sp_source[(sp_frac & (sp_loopvalue-1)) >> FRACBITS]];
		
		sp_frac += sp_fracstep;
		
	}
}

byte*       mr_dest;      // the left most pixel to be drawn (in viewb
byte*       mr_picture;   // pointer to a raw 64*64 pixel picture
byte*       mr_colormap;  // pointer to a 256 byte color number to pal
int         mr_xfrac;     // starting texture coordinate
int         mr_yfrac;     // starting texture coordinate
int         mr_xstep;     // fixed point step value
int         mr_ystep;     // fixed point step value
int         mr_count;     // the number of pixels to draw
void MapRow(void)
{
	byte *mr_dest_iter = mr_dest;
	for(int pp = 0; pp < mr_count; pp++)
	{
		int xpx = ((unsigned int)mr_xfrac >> 16) & 0x3F;
		int ypx = ((unsigned int)mr_yfrac >> 16) & 0x3F;
		*mr_dest_iter = mr_colormap[mr_picture[ (ypx << 6) + xpx ]];
		mr_dest_iter++;
		mr_xfrac += mr_xstep;
		mr_yfrac += mr_ystep;
	}
}


