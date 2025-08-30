//screen_teamselect.c
//Team select screen for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_teamselect.h"
#include "fbs.h"
#include "images.h"
#include "pads.h"
#include "font.h"

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
		
		font_draw("Bullsbrook", 0xBEEF, 60, 240);
		font_draw("Bin-Chickens", 0xBEEF, 60, 270);
		
		font_draw("Cockburn", 0xBEEF, 410, 240);
		font_draw("Cockatoos", 0xBEEF, 410, 270);
		
		fbs_flip();
		
	}
	
	(void)league;
}

int screen_teamselect_getteam(int side)
{
	return screen_teamselect_selections[side];
}
