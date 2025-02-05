/*
    Copyright (C) 2001 Hotwarez LLC, Goldtree Enterprises
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; 
    version 2 of the License.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
* prim.h: Graphics 'primitives' for the engine
*
* version 1.0
*  NOTE THAT DB_Clear_Screen() doesn't work correctly for some reason
*/

#ifndef PRIM_H

#define PRIM_H

#include "types.h"
#include "pcx.h"

/* Stuff for palette manipulation */
#define PALETTE_MASK            0x3C6
#define PALETTE_REGISTER        0x3C8
#define PALETTE_DATA            0x3C9


/* Stuff for vertical retrace */
#define VGA_INPUT_STATUS_1     0x3DA
#define VGA_VSYNC_MASK          0x08




/* Functions */

/*Wait for vertical retrace  */
void Wait_For_Vsync( void );

/* Set video mode to send mode 'mode' */
void Set_Video_Mode( int mode );

/* Clear the double buffer */
void DB_Clear_Screen( void );

/* Fade whats in the double buffer */
void Fade_Screen( void );

/* Clear video ram */
void Clear_Screen( void );

/* Write to palette register the values in color */
void Set_Palette_Register( int index, RGB_color_ptr color );

/* Allocate 64k for double buffer */
void Init_Double_Buffer( void );

/* Return a pointer to the double buffer */
char *Return_Double_Buffer( void );

/* Write the value color into the double buffer */
void DB_Fast_Pixel( int x, int y, unsigned char color );

/* Write a pixel in the double buffer but not off the screen */
void DB_Scissor_Pixel( int x, int y, unsigned char color );

/* Copy the double buffer to the video ram */
void Swap_Buffer( void );

/* Copy the buffer sent into the double buffer */
void Pop_Buffer( unsigned char *buffer );

/* Draw the polygon into the double buffer */
void DB_poly_scan( Face *p, long vert[][4], Window *win, unsigned char color );

/* Improved db_poly_scan */
void shade_DB_poly_scan( Face *p, long vert[][4], Window *win, unsigned char color );

void DB_zbuff_poly_scan( Face *p, long vert[][4], Window *win, unsigned char color, long *zbuff );

void shade_DB_zbuff_poly_scan( Face *p, long vert[][4], Window *win, unsigned char color, long *zbuff );

void fast_DB_poly_scan( Face *p, long vert[][4], Window *win,
                        unsigned char color );
                        
void DB_transparent_poly_scan( Face *p, long vert[][4], Window *win, unsigned char color );

/* draws an edge in the double buffer */

void DB_draw_edge( long vert[][4], Edge e, unsigned char color );

/* draw 1 pixel border around the double buffer */

void draw_border(void);

#endif

