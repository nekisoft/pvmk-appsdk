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

//#include <dos.h>

/* #include <bios.h> */

#include <stdio.h>
#include <math.h>

/* #include "dpmi.h" */




#define TIME_KEEPER_INT 0x8 /* 0x1C */
#define CONTROL_8253  0x43  /* the 8253's control register */
#define CONTROL_WORD  0x3C  /* the control word to set mode 2,binary least/most */
#define COUNTER_0     0x40  /* counter 0 */

#define TIMER_FAST    0x1000 /* ?? hz */
#define TIMER_220HZ   0x1555 
#define TIMER_60HZ    0x4DAE /* 60 hz */
#define TIMER_30HZ    0x965C /* 30 hz */
#define TIMER_20HZ    0xE90B /* 20 hz */
#define TIMER_18HZ    0xFFFF /* 18.2 hz (the standard count) */

#define LOW_BYTE(n) (n & 0x00ff)
#define HI_BYTE(n) ((n>>8) & 0x00ff)


void Timer( void );
void Init_Timer( void );
void Kill_Timer( void );    
long Check_Timer( void );
void Set_Timer( long number );
void Set_Timer_Speed( unsigned short new_count );
int Test_Timer( void );
