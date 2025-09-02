//screen_padteams.c
//Screen for putting gamepads on teams in soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_padteams.h"

#include "images.h"
#include "fbs.h"
#include "pads.h"

int screen_padteams_selection[4];

bool screen_padteams(void)
{
	images_purge();
	images_loadrange(IMF_PADTEAMS_AAA, IMF_PADTEAMS_ZZZ);
	
	while(1)
	{
		images_draw(IMF_PADTEAMS_BG, 0, 0);
		images_draw(IMF_PADTEAMS_HEAD, 10, 10);
		images_draw(IMF_PADTEAMS_TEAMS, 170, 70);
		images_draw(IMF_PADTEAMS_LETTERS, 120, 120);
		
		int yy = 120;
		for(int pp = 0; pp < 4; pp++)
		{
			images_draw(IMF_PADTEAMS_BLUE, 200, yy);
			images_draw(IMF_PADTEAMS_RED,  490, yy);
			
			int padx = 345;
			if(screen_padteams_selection[pp] == 1)
				padx = 200;
			if(screen_padteams_selection[pp] == 2)
				padx = 490;
			
			images_draw(IMF_PADTEAMS_PAD, padx+6, yy+6);
			
			yy += 80;
		}
		
		fbs_flip();
		
		uint16_t presses = pads_edge(PAD_ANY);
		if(presses & BTNBIT_B)
			return false;
		if(presses & BTNBIT_START)
			return true;
		
		for(int pp = 0; pp < 4; pp++)
		{
			uint16_t dirs = pads_edge(pp);
			
			if(dirs & BTNBIT_LEFT)
				screen_padteams_selection[pp] |= 1;
			if(dirs & BTNBIT_RIGHT)
				screen_padteams_selection[pp] |= 2;
			
			if(screen_padteams_selection[pp] >= 3)
				screen_padteams_selection[pp] = 0;
		}
	}
}

int screen_padteams_team(int pad)
{
	return screen_padteams_selection[pad];
}

