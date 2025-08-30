//screen_teamselect.c
//Team select screen for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_teamselect.h"
#include "fbs.h"
#include "images.h"
#include "pads.h"

int screen_teamselect_selections[2];

void screen_teamselect(const leaguedata_t *league)
{
	screen_teamselect_selections[0] = 0;
	screen_teamselect_selections[1] = 1;
	
	images_purge();
	images_loadrange(IMF_TEAMSELECT_AAA, IMF_TEAMSELECT_ZZZ);
	
	while(1)
	{
		images_draw(IMF_TEAMSELECT_BG, 0, 0);
		images_draw(IMF_TEAMSELECT_HEADERS, 60, 170);
		images_draw(IMF_TEAMSELECT_LOGO, 200, 10);
		images_draw(IMF_TEAMSELECT_SHIRTS, 10, 10);
		
		fbs_flip();
		
	}
	
	(void)league;
}

int screen_teamselect_getteam(int side)
{
	return screen_teamselect_selections[side];
}
