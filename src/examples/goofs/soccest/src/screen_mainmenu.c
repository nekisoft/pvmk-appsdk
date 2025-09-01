//screen_mainmenu.c
//Main menu for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_mainmenu.h"
#include "images.h"
#include "pads.h"
#include "fbs.h"

screen_mainmenu_result_t screen_mainmenu(void)
{
	images_purge();
	images_loadrange(IMF_MAINMENU_AAA, IMF_MAINMENU_ZZZ);
	
	int selected = 1;
	pads_edge(PAD_ANY);
	while(1)
	{
		images_draw(IMF_MAINMENU_BG, 0, 0);
		
		int iix = 80;
		int iiy = 110;
		images_file_t names[] = 
		{
			[1] = IMF_MAINMENU_ITEM1,
			[2] = IMF_MAINMENU_ITEM2,
			[3] = IMF_MAINMENU_ITEM3,
		};
		
		for(int ii = 1; ii <= 3; ii++)
		{
			images_draw(IMF_MAINMENU_ITEMBG, iix + ((ii==selected) ? 20 : 0), iiy);
			images_draw(names[ii], iix+25 + ((ii==selected) ? 20 : 0), iiy+10);
			if(selected == ii)
				images_draw(IMF_MAINMENU_BALL, iix-40, iiy+5);
			
			iiy += 70;
		}
		
		images_draw(IMF_MAINMENU_TITLE, 10, 10);
		
		int shift = (_sc_getticks() / 16) % 64;
		for(int bb = 0; bb < 10; bb++)
		{
			images_draw(IMF_MAINMENU_BALL, 550, -64 + shift + (64*bb));
		}
		
		fbs_flip();
		
		uint16_t presses = pads_edge(PAD_ANY);
		if(presses & BTNBIT_UP)
			selected--;
		if(presses & BTNBIT_DOWN)
			selected++;
		if(selected < 1)
			selected = 3;
		if(selected >3)
			selected = 1;
		
		if(presses & (BTNBIT_A | BTNBIT_START))
		{
			if(selected == 1)
				return SCREEN_MAINMENU_RESULT_QUICK;
			if(selected == 2)
				return SCREEN_MAINMENU_RESULT_LEAGUE;
			if(selected == 3)
				return SCREEN_MAINMENU_RESULT_SCORES;
		}
		if(presses & BTNBIT_B)
			return SCREEN_MAINMENU_RESULT_TITLE;
	}
}
