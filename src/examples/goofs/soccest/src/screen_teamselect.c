//screen_teamselect.c
//Team select screen for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_teamselect.h"
#include "fbs.h"
#include "images.h"
#include "pads.h"
#include "font.h"

#include <stdio.h>

int screen_teamselect_selections[2];

bool screen_teamselect(const leaguedata_t *league)
{
	screen_teamselect_selections[0] = 0;
	screen_teamselect_selections[1] = 1;
	
	images_purge();
	images_loadrange(IMF_TEAMSELECT_AAA, IMF_TEAMSELECT_ZZZ);
	
	
	int nteams = sizeof(league->teams)/sizeof(league->teams[0]);
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
		
		const teamdata_t *tl = &(league->teams[screen_teamselect_selections[0]]);
		const teamdata_t *tr = &(league->teams[screen_teamselect_selections[1]]);
		
		char numbuf[16];
		snprintf(numbuf, sizeof(numbuf)-1, "#%d", screen_teamselect_selections[0]+1);
		
		font_draw(numbuf,   leftcolor, 60, 210);
		font_draw(tl->city, leftcolor, 60, 240);
		font_draw(tl->name, leftcolor, 60, 270);
		
		snprintf(numbuf, sizeof(numbuf)-1, "#%d", screen_teamselect_selections[1]+1);
		
		font_draw(numbuf,   rightcolor, 410, 210);
		font_draw(tr->city, rightcolor, 410, 240);
		font_draw(tr->name, rightcolor, 410, 270);
		
		fbs_flip();
		
		uint16_t presses = pads_edge(PAD_ANY);
		if(presses & BTNBIT_LEFT)
			select_side = 0;
		if(presses & BTNBIT_RIGHT)
			select_side = 1;
		if(presses & BTNBIT_UP)
			screen_teamselect_selections[select_side]--;
		if(presses & BTNBIT_DOWN)
			screen_teamselect_selections[select_side]++;
		
		if(screen_teamselect_selections[select_side] < 0)
			screen_teamselect_selections[select_side] = nteams-1;
		if(screen_teamselect_selections[select_side] >= nteams)
			screen_teamselect_selections[select_side] = 0;
	
		if(screen_teamselect_selections[0] == screen_teamselect_selections[1])
		{
			if(presses & BTNBIT_UP)
				screen_teamselect_selections[select_side]--;
			else
				screen_teamselect_selections[select_side]++;
		}
		
		if(screen_teamselect_selections[select_side] < 0)
			screen_teamselect_selections[select_side] = nteams-1;
		if(screen_teamselect_selections[select_side] >= nteams)
			screen_teamselect_selections[select_side] = 0;
		
		
		if(presses & BTNBIT_START)
			return true;
		if(presses & BTNBIT_A)
			return true;
		
		if(presses & BTNBIT_B)
			return false;
	}
	
	(void)league;
}

int screen_teamselect_getteam(int side)
{
	return screen_teamselect_selections[side];
}
