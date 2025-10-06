//fbs.c
//Framebuffers in soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "fbs.h"
#include "pads.h"
#include <sc.h>

uint16_t fbs[3][SCREENY][SCREENX];
int fbs_next;

void fbs_flip(void)
{
	pads_update();
	
	int displayed = _sc_gfx_flip(_SC_GFX_MODE_VGA_16BPP, &(fbs[fbs_next][0][0]));
	if(displayed < 0)
		return;
	
	for(int bb = 0; bb < 3; bb++)
	{
		if(fbs_next == bb)
			continue;
		if((intptr_t)displayed == (intptr_t)(&(fbs[bb][0][0])))
			continue;
		
		fbs_next = bb;
		break;
	}
}
