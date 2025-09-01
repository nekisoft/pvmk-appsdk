//screen_title.c
//Title screen for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_title.h"
#include "images.h"
#include "fbs.h"
#include "pads.h"

void screen_title(void)
{
	images_purge();
	images_loadrange(IMF_TITLE_AAA, IMF_TITLE_ZZZ);
	
	int ballp[2] = {400 * 65536, 20 * 65536};
	int ballv[2] = {32768, 128};
	int balltick = 0;
	while(1)
	{
		int ticknow = _sc_getticks();
		if(balltick > ticknow || balltick < ticknow - 1000)
			balltick = ticknow - 10;
		
		while(balltick < ticknow - 10)
		{
			balltick += 10;
		
			ballv[1] += 2048;
			
			ballp[0] += ballv[0];
			ballp[1] += ballv[1];
			if(ballp[0] < 0)
			{
				ballp[0] = 0;
				ballv[0] *= -1;
			}
			if(ballp[0] > 65536 * 560)
			{
				ballp[0] = 65536 * 560;
				ballv[0] *= -1;
			}
			if(ballp[1] < 0)
			{
				ballp[1] = 0;
				ballv[1] *= -1;
			}
			if(ballp[1] > 65536 * 400)
			{
				ballp[1] = 65536 * 400;
				ballv[1] *= -1;
			}
			
		}
		
		images_draw(IMF_TITLE_BG, 0, 0);
		images_draw(IMF_TITLE_BALL, ballp[0] / 65536, ballp[1] / 65536);
		images_draw(IMF_TITLE_LOGO, 32, 32);
		images_draw(IMF_TITLE_START, 300, 300);
		fbs_flip();
		
		uint16_t presses = pads_edge(PAD_ANY);
		
		if(presses & BTNBIT_START)
			break;
		
		if(presses & BTNBIT_LEFT)
			ballv[0] -= 65536;
		if(presses & BTNBIT_RIGHT)
			ballv[0] += 65536;
		if(presses & BTNBIT_UP)
			ballv[1] -= 65536;
		if(presses & BTNBIT_DOWN)
			ballv[1] += 65536;
	}
}
