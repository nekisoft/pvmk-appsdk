/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#include <stdint.h>
#include <sc.h>

#include "quakedef.h"
#include "d_local.h"

extern viddef_t	vid;				// global video state

#define	BASEWIDTH	320
#define	BASEHEIGHT	240

uint16_t vid_bufs[3][BASEWIDTH*BASEHEIGHT] __attribute__((aligned(1048576)));
int vid_bufs_next = 0;

short	zbuffer[BASEWIDTH*BASEHEIGHT];
byte	surfcache[256*1024*4] __attribute__((aligned(16)));

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];

unsigned short colormap16[64*256];

void	VID_SetPalette (unsigned char *palette)
{
	for(int cc = 0; cc < 256; cc++)
	{
		d_8to16table[cc] = 0;
		d_8to16table[cc] |= (palette[ (cc*3) + 2 ] >> 3) << 0;
		d_8to16table[cc] |= (palette[ (cc*3) + 1 ] >> 2) << 5;
		d_8to16table[cc] |= (palette[ (cc*3) + 0 ] >> 3) << 11;
		
		d_8to24table[cc] = 0;
		d_8to24table[cc] |= (palette[ (cc*3) + 0 ] >> 0) << 0;
		d_8to24table[cc] |= (palette[ (cc*3) + 1 ] >> 0) << 8;
		d_8to24table[cc] |= (palette[ (cc*3) + 2 ] >> 0) << 16;
	}
	
	scr_copyeverything = 1;
	scr_fullupdate = 0;
}

void	VID_ShiftPalette (unsigned char *palette)
{
	VID_SetPalette(palette);
}

void	VID_Init (unsigned char *palette)
{	
	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 4; //this just determines how many times a status bar or whatever will be drawn
	vid.colormap = host_colormap;
	
	vid.colormap16 = colormap16;
	for(int bb = 0; bb < 64; bb++)
	{
		for(int cc = 0; cc < 256; cc++)
		{
			int r = palette[ (cc*3) + 0 ];
			int g = palette[ (cc*3) + 1 ];
			int b = palette[ (cc*3) + 2 ];
			
			r = (r * (64-bb)) / 32;
			g = (g * (64-bb)) / 32;
			b = (b * (64-bb)) / 32;
			
			if(r > 255)
				r = 255;
			if(r < 0)
				r = 0;
			
			if(g > 255)
				g = 255;
			if(g < 0)
				g = 0;
			
			if(b > 255)
				b = 255;
			if(b < 0)
				b = 0;
			
			
			colormap16[(bb * 256) + cc] = 0;
			colormap16[(bb * 256) + cc] |= (b >> 3) << 0;
			colormap16[(bb * 256) + cc] |= (g >> 2) << 5;
			colormap16[(bb * 256) + cc] |= (r >> 3) << 11;
		}
	}
	
	
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = (pixel_t*)(vid_bufs[0]);
	vid.rowbytes = vid.conrowbytes = BASEWIDTH * 2;
	
	d_pzbuffer = zbuffer;
	D_InitCaches (surfcache, sizeof(surfcache));
	VID_SetPalette(palette);
	
}

void	VID_Shutdown (void)
{
}

void	VID_Update (vrect_t *rects)
{
	(void)rects;
	
	//Enqueue a flip to the buffer that's just been drawn
	int displayed = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, vid.buffer);
	if(displayed < 0)
	{
		//Ignore and continue...
		_sc_pause();
		return;
	}
	
	//Figure out what buffer we'll use next. It shouldn't be the one displayed nor enqueued.
	//If we have a choice, prefer cycling through them 1-2-3, so status bar updates can hit all of them.
	uint16_t *nextbuf = vid_bufs[vid_bufs_next];
	for(int bb = 0; bb < 3; bb++)
	{
		vid_bufs_next = (vid_bufs_next + 1) % 3;
		
		if(vid_bufs[vid_bufs_next] == (uint16_t*)displayed)
			continue; //Can't use this one, it's displayed now
		if(vid_bufs[vid_bufs_next] == (uint16_t*)vid.buffer)
			continue; //Can't use this one, it's about to be displayed
	
		//Found one
		nextbuf = vid_bufs[vid_bufs_next];
		break;
	}
	
	vid.buffer = vid.conbuffer = (pixel_t*)nextbuf;
}

void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}




static int lastbuttons = 0;
void Sys_SendKeyEvents(void)
{
	if(cls.state == ca_dedicated)
		return;
	
	_sc_input_t input = {0};
	while( _sc_input(&input, sizeof(input), sizeof(input)) > 0 )
	{
		if(input.format != 'A')
			continue;
		
		static const int button_mapping[16] = 
		{
			[_SC_BTNIDX_UP] = K_UPARROW,
			[_SC_BTNIDX_DOWN] = K_DOWNARROW,
			[_SC_BTNIDX_LEFT] = K_LEFTARROW,
			[_SC_BTNIDX_RIGHT] = K_RIGHTARROW,
			
			[_SC_BTNIDX_A] = 'a',
			[_SC_BTNIDX_B] = 'b',
			[_SC_BTNIDX_C] = 'c',
			
			[_SC_BTNIDX_X] = 'x',
			[_SC_BTNIDX_Y] = 'y',
			[_SC_BTNIDX_Z] = 'z',
			
			//[_SC_BTNIDX_L] = 'l',
			//[_SC_BTNIDX_R] = 'r',
			
			[_SC_BTNIDX_MODE] = K_ESCAPE,
			
			[_SC_BTNIDX_START] = K_ENTER,
		};
		
		for(int bb = 0; bb < 16; bb++)
		{
			if(button_mapping[bb] == 0)
				continue;
			
			if(input.buttons & (~lastbuttons) & (1u << bb))
				Key_Event(button_mapping[bb], true);
			else if(lastbuttons & (~input.buttons) & (1u << bb))
				Key_Event(button_mapping[bb], false);
		}
		
		lastbuttons = input.buttons;
	}
}

void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

void IN_Init(void) { }
void IN_Shutdown(void) { }
void IN_Commands (void) { }
void IN_MouseMove (usercmd_t *cmd) { }
void IN_Move (usercmd_t *cmd) { }
