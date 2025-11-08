/*
	Neki32 Application SDK - This file placed in the public domain.
	Bryan Topp <betopp@betopp.com>, Nekisoft Pty Ltd (ACN 680 583 251) 2025
*/
#include "../../SDL_internal.h"

#ifdef SDL_TIMER_PVMK

#include <sc.h>

static int last_ticks = 0;
static uint64_t accum_ticks64 = 0;

void SDL_TicksInit(void)
{
	last_ticks = _sc_getticks();
	accum_ticks64 = 0;
}

void SDL_TicksQuit(void)
{
	last_ticks = _sc_getticks();
	accum_ticks64 = 0;
}

Uint64 SDL_GetTicks64(void)
{
	int new_ticks = _sc_getticks();
	int elapsed = new_ticks - last_ticks;
	last_ticks = new_ticks;
	
	if(elapsed > 0)
		accum_ticks64 += elapsed;
		
	return accum_ticks64;
}

Uint64 SDL_GetPerformanceCounter(void)
{
    return SDL_GetTicks64();
}

Uint64 SDL_GetPerformanceFrequency(void)
{
    return 1000;
}

void SDL_Delay(Uint32 ms)
{
	uint64_t target = SDL_GetTicks64() + (uint64_t)ms;
	while(SDL_GetTicks64() < target)
	{
		_sc_pause();
	}
}

#endif /* SDL_TIMER_PVMK */

/* vi: set sts=4 ts=4 sw=4 expandtab: */
