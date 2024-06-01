// Rewritten by Bryan E. Topp, 2021-2023
//
// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX. Hacked up a lot for Neki32.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <unistd.h>

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>

#include <errno.h>
#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "i_sound.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"
#include "r_main.h"

#include "doomdef.h"

#include <sc.h>
#include <fcntl.h>

void I_ShutdownGraphics(void) { }
void I_StartFrame (void) { }

//
// I_StartTic
//
void I_StartTic (void)
{
	while(1)
	{
		_sc_input_t input = {0};
		if(_sc_input(&input, sizeof(input), sizeof(input)) <= 0)
			break; //No input for the moment...
	
		if(input.format != 'A')
			continue;
		
		static int buttons_last = ~0;
		for(int button = 0; button < 16; button++)
		{
			if( !((input.buttons ^ buttons_last) & (1u << button)) )
				continue;
			
			//Special case so the user can press "Y" to quit
			if( (button == _SC_BTNIDX_Y) && (input.buttons & (1u << button)) )
			{
				event_t event = {0};
				event.type = (input.buttons & (1u << button)) ? ev_keydown : ev_keyup;
				event.data1 = 'y';
				D_PostEvent(&event);
			}
			
			//Normal case for gamepad input
			static const int keymap[256] = 
			{
				[_SC_BTNIDX_UP] = KEY_UPARROW,
				[_SC_BTNIDX_DOWN] = KEY_DOWNARROW,
				[_SC_BTNIDX_LEFT] = KEY_LEFTARROW,
				[_SC_BTNIDX_RIGHT] = KEY_RIGHTARROW,
				[_SC_BTNIDX_L] = ',',
				[_SC_BTNIDX_R] = '.',
				
				[_SC_BTNIDX_START] = KEY_ENTER,
				//[_SC_BTNIDX_SELECT] = KEY_ESCAPE,
				
				[_SC_BTNIDX_A] = KEY_RCTRL,
				[_SC_BTNIDX_B] = KEY_RSHIFT,
				[_SC_BTNIDX_C] = ' ',
				
				[_SC_BTNIDX_X] = KEY_TAB,
				[_SC_BTNIDX_Y] = '1',
				[_SC_BTNIDX_Z] = '2',
				
			};
			
			event_t event = {0};
			event.type = (input.buttons & (1u << button)) ? ev_keydown : ev_keyup;
			event.data1 = keymap[button];
			if(event.data1 != 0)
				D_PostEvent(&event);
		}
		buttons_last = input.buttons;
	}
}

void I_UpdateNoBlit (void) { }

uint16_t defaultpal[256];
uint32_t sparsepal[256];
uint16_t fbs[3][SCREENHEIGHT][SCREENWIDTH] __attribute__((aligned(256)));
int fb_next;

void I_FinishUpdate (void)
{
	//Flip that to the front at vertical blanking
	int displayed = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[fb_next]);
	
	//Pick a free buffer to draw into next - not the one we just asked to display, and not what's currently displayed.
	int freebuf = 0;
	if( (freebuf == fb_next) || (fbs[freebuf] == (void*)displayed) )
		freebuf = 1;
	if( (freebuf == fb_next) || (fbs[freebuf] == (void*)displayed) )
		freebuf = 2;
	
	fb_next = freebuf;
	screens[0] = &(fbs[fb_next][0][0]);
	R_UpdateBufferPointers();
	
	//Pump sound
	for(int ss = 0; ss < 8; ss++)
	{
		I_UpdateSound();
		I_SubmitSound();
	}
}

void I_ReadScreen (vpx_t* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT*sizeof(vpx_t));
}

void I_SetPalette (byte* palette)
{
	//Doom gives us the palette entries as 1 byte each of R, G, and B.
	//Pack into a 16-bit word for RGB565 display.
	for(int cc = 0; cc < 256; cc++)
	{
		//For straight blits to the screen, just the literal RGB565 word
		defaultpal[cc] = 0;
		defaultpal[cc] |= (((uint32_t)(palette[0])) >> 3) << 11;
		defaultpal[cc] |= (((uint32_t)(palette[1])) >> 2) << 5;
		defaultpal[cc] |= (((uint32_t)(palette[2])) >> 3) << 0;
		
		//For blended/faded colors, leave 5 bits between each component.
		//This allows us to multiply by 0...31 all three channels at once.
		sparsepal[cc] = 0;
		sparsepal[cc] |= (((uint32_t)(palette[0])) >> 3) << 21;
		sparsepal[cc] |= (((uint32_t)(palette[1])) >> 2) << 10;
		sparsepal[cc] |= (((uint32_t)(palette[2])) >> 3) << 0;
		
		palette += 3;
	}
}


void I_InitGraphics(void)
{
	static int		firsttime=1;
	if (!firsttime)
		return;
	
	firsttime = 0;

	signal(SIGINT, (void (*)(int)) I_Quit);

	fb_next = 0;
	screens[0] = &(fbs[0][0][0]);
}

void I_LoadingScreen(void)
{
	int loading_graphic = open("loading.data", O_RDONLY);
	if(loading_graphic >= 0)
	{
		for(int line = 0; line < 240; line++)
		{
			static uint8_t linebuf[320][3] = {0};
			read(loading_graphic, linebuf, sizeof(linebuf));
			for(int px = 0; px < 320; px++)
			{
				fbs[fb_next][line][px] = 0;
				fbs[fb_next][line][px] |= ((uint32_t)linebuf[px][0] >> 3) << 11;
				fbs[fb_next][line][px] |= ((uint32_t)linebuf[px][1] >> 2) << 5;
				fbs[fb_next][line][px] |= ((uint32_t)linebuf[px][2] >> 3);
			}
		}
		close(loading_graphic);
		while(_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[fb_next]) != (int)(fbs[fb_next])) { _sc_pause(); }
		fb_next = fb_next ? 0 : 1;
	}
}

int inited;
