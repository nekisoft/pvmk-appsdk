/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "../source/gamelib/types.h"
#include "video.h"
#include "sblaster.h"
#include "../source/gamelib/vga.h"
#include "../source/globals.h"
#include "../source/gamelib/screen.h"

#include <sc.h>

uint16_t bufs[3][240][320];
uint16_t *bufptrs[3] = { &(bufs[0][0][0]), &(bufs[1][0][0]), &(bufs[2][0][0]) };
void *displayed_buf = NULL;
void *enqueued_buf = NULL;

uint16_t palette[256];

void video_init() { }
void video_exit() { }

int video_set_mode(s_videomodes videomodes)
{
	(void)videomodes;
	return 1;
}

int video_copy_screen(s_screen* src)
{
	//Pump sound periodically
	SB_pump();
	
	//Find a buffer that is neither displayed nor enqueued, so we don't cause tearing.
	void *avail_buf = bufptrs[0];
	for(int bb = 0; bb < 3; bb++)
	{
		if(bufptrs[bb] == displayed_buf || bufptrs[bb] == enqueued_buf)
			continue;
		
		avail_buf = bufptrs[bb];
		break;
	}
	
	//Copy the screen over
	if(src->pixelformat == PIXEL_16)
	{
		//Can copy directly
		memcpy(avail_buf, src->data, 320*240*2);
	}
	else if(src->pixelformat == PIXEL_8)
	{
		//Depaletteize
		const uint8_t *srcpx = (const uint8_t*)(src->data);
		uint16_t *dstpx = (uint16_t*)avail_buf;
		for(int pp = 0; pp < 320*240; pp++)
		{
			dstpx[pp] = palette[srcpx[pp]];
		}
	}
	else if(src->pixelformat == PIXEL_32)
	{
		//Compress 32-bit pixels to 16-bit
		const uint32_t *srcpx = (const uint32_t*)(src->data);
		uint16_t *dstpx = (uint16_t*)avail_buf;
		for(int pp = 0; pp < 320*240; pp++)
		{
			dstpx[pp] = 0;
			dstpx[pp] |= (srcpx[pp] <<  8) & (0x1F << 11);
			dstpx[pp] |= (srcpx[pp] >>  5) & (0x3F <<  5);
			dstpx[pp] |= (srcpx[pp] >> 19) & (0x1F <<  0);
		}
	}
	
	//Tell the system about the new one
	int flip_result = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, avail_buf);
	if(flip_result < 0)
		return 1; //Ignore failures...
	
	//We know what we asked for, and we know what the system is currently displaying.
	//Neither is safe as we don't control the timing of the flip at vblank.
	displayed_buf = (uint16_t*)flip_result;
	enqueued_buf = avail_buf;
	return 1;
}

void video_clearscreen()
{
	
}

void video_stretch(int enable)
{
	(void)enable;
}

void video_set_color_correction(int gm, int br)
{
	(void)gm;
	(void)br;
}

void vga_vwait(void)
{
	//Just resubmit the last enqueued flip until we see it made active
	if(enqueued_buf == NULL)
		return;
	
	while(1)
	{
		//Pump sound while we wait
		SB_pump();
		
		//Check if the last request has been displayed yet
		int flip_result = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, enqueued_buf);
		if(flip_result == (int)enqueued_buf)
			return;
		else
			_sc_pause();
	}
}

void vga_setpalette(unsigned char* pal)
{
	int i;
	for(i=0;i<256;i++)
	{
		palette[i] = (pal[0]>>3<<11) | (pal[1]>>2<<5) | (pal[2]>>3);
		pal+=3;
	}
}


