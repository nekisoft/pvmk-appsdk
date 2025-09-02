//screen_matchopt.c
//Match options screen for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_matchopt.h"
#include "screen_padteams.h"
#include "fbs.h"
#include "images.h"
#include "pads.h"
#include "font.h"
#include <stdio.h>

typedef struct screen_matchopt_option_s
{
	const char *name;
	const char **options;
} screen_matchopt_option_t;
const screen_matchopt_option_t screen_matchopt_options[] = 
{
	{ .name = "Duration", .options = (const char*[]){ "10 Minutes", "30 Minutes", "90 Minutes", NULL } },
	{ .name = "Ball Size", .options = (const char*[]){ "Normal", "Big Balls", NULL } },
	{ .name = "Manslaughter", .options = (const char*[]){ "Yellow Card", "Red Card", "Legal", NULL } },
	{ .name = "START GAME!", .options = (const char*[]){ "", NULL } },
	{0}
};
int screen_matchopt_selections[sizeof(screen_matchopt_options)/sizeof(screen_matchopt_options[0])];

bool screen_matchopt(const leaguedata_t *league, int tl, int tr)
{
	images_purge();
	images_loadrange(IMF_MATCHOPT_AAA, IMF_MATCHOPT_ZZZ);
	
	int itemsel = 0;
	
	while(1)
	{
		
		images_draw(IMF_MATCHOPT_BG, 0, 0);
		images_draw(IMF_MATCHOPT_CHALK, 10, 370);
		images_draw(IMF_MATCHOPT_ERASER, 530, 370);
		images_draw(IMF_MATCHOPT_VUVU, 530, 10);
		images_draw(IMF_MATCHOPT_TITLE, 5, 5);
		
		int yiter = 70;
		int xloc = 70;
		
		//Make header text - who's playing what
		
		const char *playerstrings[16] = 
		{
			            "",        "(Pad A) ",        "(Pad B) ",       "(Pads A, B) ",
			    "(Pad C) ",    "(Pads A, C) ",    "(Pads B, C) ",    "(Pads A, B, C) ",
			    "(Pad D) ",    "(Pads A, D) ",    "(Pads B, D) ",    "(Pads A, B, D) ",
			"(Pads C, D) ", "(Pads A, C, D) ", "(Pads B, C, D) ", "(Pads A, B, C, D) ",
			
		};
		
		int players_tl = 0;
		int players_tr = 0;
		for(int pp = 0; pp < 4; pp++)
		{
			if(screen_padteams_team(pp) == 1)
				players_tl |= (1u << pp);
			if(screen_padteams_team(pp) == 2)
				players_tr |= (1u << pp);
		}
		
		char txtbuf[256] = {0};
		
		snprintf(txtbuf, sizeof(txtbuf)-1, "%s %s %svs.", 
			league->teams[tl].city, league->teams[tl].name, playerstrings[players_tl]);
		font_draw(txtbuf, 0xffff, xloc, yiter); yiter += 30;
		
		snprintf(txtbuf, sizeof(txtbuf)-1, "%s %s %sat",
			league->teams[tr].city, league->teams[tr].name, playerstrings[players_tr]);
		font_draw(txtbuf, 0xffff, xloc, yiter); yiter += 30;
		
		snprintf(txtbuf, sizeof(txtbuf)-1, "%s", league->teams[tl].venue);
		font_draw(txtbuf, 0xffff, xloc, yiter); yiter += 30;
		
		//Show options
		yiter += 30;
		for(int ii = 0; screen_matchopt_options[ii].name != NULL; ii++)
		{
			const screen_matchopt_option_t *optr = &(screen_matchopt_options[ii]);
			
			uint16_t namecolor = (ii == itemsel) ? 0xffff : 0x8d3e;
			uint16_t valcolor = (ii == itemsel) ? 0xffff : 0x58de;
			font_draw(optr->name, namecolor, xloc, yiter);
			font_draw(optr->options[screen_matchopt_selections[ii]], valcolor, xloc + 200, yiter);
			yiter += 30;
		}
		
		fbs_flip();
		
		uint16_t presses = pads_edge(PAD_ANY);
		if(presses & BTNBIT_START)
			return true;
		if(presses & BTNBIT_B)
			return false;
		
		if(presses & BTNBIT_DOWN)
			itemsel++;
		if(presses & BTNBIT_UP)
			itemsel--;
		
		if(itemsel < 0)
			itemsel = 0;
		if(screen_matchopt_options[itemsel].name == NULL)
			itemsel--;
		
		
		if(presses & BTNBIT_LEFT)
			screen_matchopt_selections[itemsel]--;
		if(presses & BTNBIT_RIGHT)
			screen_matchopt_selections[itemsel]++;
		
		if(screen_matchopt_selections[itemsel] < 0)
			screen_matchopt_selections[itemsel] = 0;
		
		if(screen_matchopt_options[itemsel].options[screen_matchopt_selections[itemsel]] == NULL)
			screen_matchopt_selections[itemsel]--;
		
	}
}
