/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */


#include "../source/gamelib/timer.h"
#include <sc.h>

#define GETTIME_FREQ 1000

static unsigned lastinterval = 0;
unsigned newticks = 0;

void borTimerInit(){}
void borTimerExit(){}

unsigned timer_getinterval(unsigned freq)
{
	unsigned tickspassed,ebx,blocksize,now;
	now=timer_gettick()-newticks;
	ebx=now-lastinterval;
	blocksize=GETTIME_FREQ/freq;
	ebx+=GETTIME_FREQ%freq;
	tickspassed=ebx/blocksize;
	ebx-=ebx%blocksize;
	lastinterval+=ebx;
	return tickspassed;
}

unsigned timer_gettick()
{
	return _sc_getticks();
}

u64 timer_uticks()
{
	return timer_gettick() * 1000LL;
}

unsigned get_last_interval()
{
	return lastinterval;
}

void set_last_interval(unsigned value)
{
	lastinterval = value;
}

void set_ticks(unsigned value)
{
    newticks = value;
}


