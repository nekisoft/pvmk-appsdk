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
#include "../source/gamelib/vga.h"
#include "../source/globals.h"
#include "../source/gamelib/screen.h"

#include <sc.h>

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
	enqueued_buf = (void*)(src->data);
	
	int flip_result = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, enqueued_buf);
	(void)flip_result;
	
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
	//Just resubmit the last enqueued flip until we see it active
	while(1)
	{
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


