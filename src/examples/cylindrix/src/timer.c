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

#include "timer.h"
#include "keys.h"
#include "input.h"
#include <time.h>

#include <sc.h> //pvmk - using system call for timer ticks

#include <string.h> /* For memset */

/* GLOBALS */
long TIMER_CLICKS_PER_SECOND = 1000; //pvmk - milliseconds tracked by OS
long GAME_CLICKS_PER_SECOND  = 15;

long timer = 0;
long pvmk_timer_offset = 0;
unsigned long absolute_time = 0; /* Absolute number of clicks since the beginning of the game */
long click_threshold = 15; /* 15 is arbitrary */

void Init_Timer( void ) { }
void Kill_Timer( void ) { }
long Check_Timer( void )
{
	return _sc_getticks() + pvmk_timer_offset;
}
void Set_Timer( long number )
{
	pvmk_timer_offset = number - _sc_getticks();
}
void Set_Timer_Speed( unsigned short new_count ) { (void)new_count; } 

int Test_Timer( void )
{
	int clicks_per_second = 1000; //Check_Timer(); //pvmk - always milliseconds
	fprintf(stderr, "%d clicks went by in one second \n", clicks_per_second);
	Get_Keypress();
	return clicks_per_second;
}
