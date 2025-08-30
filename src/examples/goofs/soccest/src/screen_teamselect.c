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
	
	int select_side = 0;
	while(1)
	{
		images_draw(IMF_TEAMSELECT_BG, 0, 0);
		images_draw(IMF_TEAMSELECT_HEADERS, 60, 170);
		images_draw(IMF_TEAMSELECT_LOGO, 200, 10);
		images_draw(IMF_TEAMSELECT_SHIRTS, 10, 10);
		
		uint16_t throb_table[16] = 
		{
			0x1082 *  8, 0x1082 *  9, 0x1082 * 10, 0x1082 * 11,
			0x1082 * 12, 0x1082 * 13, 0x1082 * 14, 0x1082 * 15,
			0x1082 * 15, 0x1082 * 14, 0x1082 * 13, 0x1082 * 12,
			0x1082 * 11, 0x1082 * 10, 0x1082 *  9, 0x1082 *  8,
		};
		uint16_t throb_color = throb_table[((_sc_getticks() / 16) % 16)];
		uint16_t plain_color = 0xBEEF;
		
		uint16_t leftcolor = (select_side == 0) ? throb_color : plain_color;
		uint16_t rightcolor = (select_side == 1) ? throb_color : plain_color;
		
		font_draw("Bullsbrook", leftcolor, 60, 240);
		font_draw("Bin-Chickens", leftcolor, 60, 270);
		
		font_draw("Cockburn", rightcolor, 410, 240);
		font_draw("Cockatoos", rightcolor, 410, 270);
		
		fbs_flip();
		
		uint16_t presses = pads_edge(PAD_ANY);
		if(presses & BTNBIT_LEFT)
			select_side = 0;
		if(presses & BTNBIT_RIGHT)
			select_side = 1;
	}
	
	(void)league;
}

int screen_teamselect_getteam(int side)
{
	return screen_teamselect_selections[side];
}
