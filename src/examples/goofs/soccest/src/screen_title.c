//screen_title.c
//Title screen for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_title.h"
#include "images.h"
#include "fbs.h"

void screen_title(void)
{
	images_purge();
	images_loadrange(IMF_TITLE_AAA, IMF_TITLE_ZZZ);
	
	while(1)
	{
		images_draw(IMF_TITLE_BG, 0, 0);
		images_draw(IMF_TITLE_LOGO, 32, 32);
		images_draw(IMF_TITLE_START, 300, 300);
		images_draw(IMF_TITLE_BALL, 64, 300);
		fbs_flip();
	}
}
