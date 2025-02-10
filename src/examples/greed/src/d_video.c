/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Raven 3D Engine                                                         */
/* Copyright (C) 1996 by Softdisk Publishing                               */
/*                                                                         */
/* Original Design:                                                        */
/*  John Carmack of id Software                                            */
/*                                                                         */
/* Enhancements by:                                                        */
/*  Robert Morgan of Channel 7............................Main Engine Code */
/*  Todd Lewis of Softdisk Publishing......Tools,Utilities,Special Effects */
/*  John Bianca of Softdisk Publishing..............Low-level Optimization */
/*  Carlos Hasan..........................................Music/Sound Code */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
/* Ported to PVMK (Neki32 game console) by Bryan E. Topp, 2025 */

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include "d_global.h"
#include "d_ints.h"
#include "d_video.h"
#include "d_misc.h"
#include "d_disk.h"
#include "r_public.h"
#include "r_refdef.h"
#include "audio.h"

#include <sc.h>


//Triple-buffered truecolor output for PVMK
uint16_t pvmk_fbs[3][240][320];
int pvmk_fbnext;

//Screen buffer as indexed color for PVMK
//Todo - with a lot more effort we could skip this and draw directly into truecolor buffers
uint8_t pvmk_screen[SCREENHEIGHT*SCREENWIDTH];

//Palette used in de-palettizing screen into truecolor buffers
uint16_t pvmk_pal[256];


byte* screen = (byte*)pvmk_screen;
byte* ylookup[SCREENHEIGHT];
byte* transparency;
byte* translookup[255];

extern SoundCard SC;



/**** FUNCTIONS ****/

void VI_SetTextMode(void)
{
}


void VI_SetVGAMode(void)
{
}


void PvmkPresent(void)
{
	//Depalettize to the already-chosen buffer
	uint8_t *src = pvmk_screen;
	for(int yy = 0; yy < 200; yy++)
	{
		for(int xx = 0; xx < 320; xx++)
		{
			pvmk_fbs[pvmk_fbnext][yy][xx] = pvmk_pal[*src];
			src++;
		}
	}
	
	//Enqueue a flip to that buffer
	int displayed = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, &(pvmk_fbs[pvmk_fbnext][0][0]));
	if(displayed < 0)
	{
		//Error enqueuing flip
		return;
	}
	
	//Find out what buffer we'll use next - neither displayed nor enqueued
	for(int bb = 0; bb < 3; bb++)
	{
		if(pvmk_fbnext == bb)
		{
			//Just enqueued this one
			continue;
		}
		
		if((intptr_t)displayed == (intptr_t)(pvmk_fbs[pvmk_fbnext]))
		{
			//This one is being displayed
			continue;
		}
		
		//Found the free one
		pvmk_fbnext = bb;
		break;
	}
}

void VI_WaitVBL(int vbls)
{
	int oldtx = timecount;
	while(timecount < oldtx + vbls) { PvmkTimerSim(); }
}

void VI_FillPalette(int red, int green, int blue)
{
	for(int cc = 0; cc < 256; cc++)
	{
		VI_SetColor(cc, red, green, blue);
	}
}


void VI_SetColor(int color, int red, int green, int blue)
{
	uint16_t px = 0;
	
	px <<= 5;
	px |= (  red & 0x3F) >> 1;
	
	px <<= 6;
	px |= (green & 0x3F) >> 0;
	
	px <<= 5;
	px |= ( blue & 0x3F) >> 1;
	
	pvmk_pal[color & 0xFF] = px;
}


void VI_GetColor(int color, int* red, int* green, int* blue)
{
	uint16_t px = pvmk_pal[color & 0xFF];
	
	if(blue != NULL)
		*blue = (px & 0x1F) << 1;
	
	px >>= 5;
	
	if(green != NULL)
		*green = (px & 0x3F) << 0;
	
	px >>= 6;
	
	if(red != NULL)
		*red = (px & 0x1F) << 1;
	
	px >>= 5;
}


void VI_SetPalette(byte* palette)
{
	for(int cc = 0; cc < 256; cc++)
	{
		VI_SetColor(cc, palette[0], palette[1], palette[2]);
		palette += 3;
	}
}


void VI_GetPalette(byte* palette)
{
	for(int cc = 0; cc < 256; cc++)
	{
		int r = 0, g = 0, b = 0;
		VI_GetColor(cc, &r, &g, &b);
		palette[0] = r;
		palette[1] = g;
		palette[2] = b;
		palette += 3;
	}
}


void VI_FadeOut(int start, int end, int red, int green, int blue, int steps)
{
    byte        basep[768];
    signed char px[768], pdx[768], dx[768];
    int         i, j;

    VI_GetPalette(basep);
    memset(dx, 0, 768);
    for (j = start; j < end; j++)
    {
        pdx[j * 3]     = (basep[j * 3] - red) % steps;
        px[j * 3]      = (basep[j * 3] - red) / steps;
        pdx[j * 3 + 1] = (basep[j * 3 + 1] - green) % steps;
        px[j * 3 + 1]  = (basep[j * 3 + 1] - green) / steps;
        pdx[j * 3 + 2] = (basep[j * 3 + 2] - blue) % steps;
        px[j * 3 + 2]  = (basep[j * 3 + 2] - blue) / steps;
    }
    start *= 3;
    end *= 3;
    for (i = 0; i < steps; i++)
    {
        for (j = start; j < end; j++)
        {
            basep[j] -= px[j];
            dx[j] += pdx[j];
            if (dx[j] >= steps)
            {
                dx[j] -= steps;
                --basep[j];
            }
            else if (dx[j] <= -steps)
            {
                dx[j] += steps;
                ++basep[j];
            }
        }
        Wait(1);
        VI_SetPalette(basep);
    }
    VI_FillPalette(red, green, blue);
}


void VI_FadeIn(int start, int end, byte* palette, int steps)
{
    byte        basep[768], work[768];
    signed char px[768], pdx[768], dx[768];
    int         i, j;

    VI_GetPalette(basep);
    memset(dx, 0, 768);
    memset(work, 0, 768);
    start *= 3;
    end *= 3;
    for (j = start; j < end; j++)
    {
        pdx[j] = (palette[j] - basep[j]) % steps;
        px[j]  = (palette[j] - basep[j]) / steps;
    }
    for (i = 0; i < steps; i++)
    {
        for (j = start; j < end; j++)
        {
            work[j] += px[j];
            dx[j] += pdx[j];
            if (dx[j] >= steps)
            {
                dx[j] -= steps;
                ++work[j];
            }
            else if (dx[j] <= -steps)
            {
                dx[j] += steps;
                --work[j];
            }
        }
        Wait(1);
        VI_SetPalette(work);
    }
    VI_SetPalette(palette);
}


void VI_ColorBorder(int color)
{
	(void)color;
}


void VI_Plot(int x, int y, int color) { *(ylookup[y] + x) = color; }


void VI_Hlin(int x, int y, int width, int color) { memset(ylookup[y] + x, color, (size_t) width); }


void VI_Vlin(int x, int y, int height, int color)
{
    byte* dest;

    dest = ylookup[y] + x;
    while (height--)
    {
        *dest = color;
        dest += SCREENWIDTH;
    }
}


void VI_Bar(int x, int y, int width, int height, int color)
{
    byte* dest;

    dest = ylookup[y] + x;
    while (height--)
    {
        memset(dest, color, (size_t) width);
        dest += SCREENWIDTH;
    }
}


void VI_DrawPic(int x, int y, pic_t* pic)
{
    byte *dest, *source;
    int   width, height;

    width  = pic->width;
    height = pic->height;
    source = &pic->data;
    dest   = ylookup[y] + x;
    while (height--)
    {
        memcpy(dest, source, width);
        dest += SCREENWIDTH;
        source += width;
    }
}


void VI_DrawMaskedPic(int x, int y, pic_t* pic)
/* Draws a formatted image to the screen, masked with zero */
{
    byte *dest, *source;
    int   width, height, xcor;

    x -= pic->orgx;
    y -= pic->orgy;
    height = pic->height;
    source = &pic->data;
    while (y < 0)
    {
        source += pic->width;
        height--;
        y++;
    }
    while (height--)
    {
        if (y < 200)
        {
            dest  = ylookup[y] + x;
            xcor  = x;
            width = pic->width;
            while (width--)
            {
                if (xcor >= 0 && xcor <= 319 && *source)
                    *dest = *source;
                xcor++;
                source++;
                dest++;
            }
        }
        y++;
    }
}


void VI_DrawTransPicToBuffer(int x, int y, pic_t* pic)
/* Draws a transpartent, masked pic to the view buffer */
{
    byte *dest, *source;
    int   width, height;

    x -= pic->orgx;
    y -= pic->orgy;
    height = pic->height;
    source = &pic->data;
    while (y < 0)
    {
        source += pic->width;
        height--;
        y++;
    }
    while (height-- > 0)
    {
        if (y < 200)
        {
            dest  = viewylookup[y] + x;
            width = pic->width;
            while (width--)
            {
                if (*source)
                    *dest = *(translookup[*source - 1] + *dest);
                source++;
                dest++;
            }
        }
        y++;
    }
}

//pvmk - guessing here
void VI_DrawMaskedPicToBuffer(int x, int y, pic_t* pic)
/* Draws a transpartent, masked pic to the view buffer */
{
    byte *dest, *source;
    int   width, height;

    x -= pic->orgx;
    y -= pic->orgy;
    height = pic->height;
    source = &pic->data;
    while (y < 0)
    {
        source += pic->width;
        height--;
        y++;
    }
    while (height-- > 0)
    {
        if (y < 200)
        {
            dest  = viewylookup[y] + x;
            width = pic->width;
            while (width--)
            {
                if (*source)
                    *dest = *source;
                source++;
                dest++;
            }
        }
        y++;
    }
}

void VI_DrawMaskedPicToBuffer2(int x, int y, pic_t* pic)
/* Draws a masked pic to the view buffer */
{
    byte *dest, *source, *colormap = NULL;
    int   width, height, maplight;

    // x-=pic->orgx;
    // y-=pic->orgy;
    height = pic->height;
    source = &pic->data;

    wallshadow = mapeffects[player.mapspot];
    if (wallshadow == 0)
    {
        maplight = ((int) maplights[player.mapspot] << 3) + reallight[player.mapspot];
        if (maplight < 0)
            colormap = zcolormap[0];
        else if (maplight > MAXZLIGHT)
            colormap = zcolormap[MAXZLIGHT];
        else
            colormap = zcolormap[maplight];
    }
    else if (wallshadow == 1)
        colormap = colormaps + (wallglow << 8);
    else if (wallshadow == 2)
        colormap = colormaps + (wallflicker1 << 8);
    else if (wallshadow == 3)
        colormap = colormaps + (wallflicker2 << 8);
    else if (wallshadow == 4)
        colormap = colormaps + (wallflicker3 << 8);
    else if (wallshadow >= 5 && wallshadow <= 8)
    {
        if (wallcycle == wallshadow - 5)
            colormap = colormaps;
        else
        {
            maplight = ((int) maplights[player.mapspot] << 3) + reallight[player.mapspot];
            if (maplight < 0)
                colormap = zcolormap[0];
            else if (maplight > MAXZLIGHT)
                colormap = zcolormap[MAXZLIGHT];
            else
                colormap = zcolormap[maplight];
        }
    }
    else if (wallshadow == 9)
    {
        maplight
            = ((int) maplights[player.mapspot] << 3) + reallight[player.mapspot] + wallflicker4;
        if (maplight < 0)
            colormap = zcolormap[0];
        else if (maplight > MAXZLIGHT)
            colormap = zcolormap[MAXZLIGHT];
        else
            colormap = zcolormap[maplight];
    }
    if (height + y > windowHeight)
        height = windowHeight - y;
    while (height-- > 0)
    {
        dest  = viewylookup[y] + x;
        width = pic->width;
        while (width--)
        {
            if (*source)
                *dest = *(colormap + *(source));
            source++;
            dest++;
        }
        y++;
    }
}


void VI_Init(int specialbuffer)
{
    int y;

    if (specialbuffer)
    {
        screen = (byte*) malloc(SCREENBYTES);
        if (screen == NULL)
            MS_Error("VI_Init: Out of memory for vrscreen");
    }
    else
        screen = pvmk_screen; //screen = (byte*) SCREEN; //pvmk
    for (y = 0; y < SCREENHEIGHT; y++)
        ylookup[y] = screen + y * SCREENWIDTH;
    transparency = CA_CacheLump(CA_GetNamedNum("TRANSPARENCY"));
    for (y = 0; y < 255; y++)
        translookup[y] = transparency + 256 * y;
    VI_SetVGAMode();
}




