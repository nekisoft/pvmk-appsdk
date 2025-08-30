//main.c
//Entry point for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_title.h"
#include "screen_mainmenu.h"
#include "screen_teamselect.h"

#include "teamdata.h"

int main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	
	//Generate the quick-match league data once on startup
	leaguedata_generate(&leaguedata_quick);
	
	while(1)
	{
		screen_title();
		screen_mainmenu_result_t mainmenu_result = screen_mainmenu();
		if(mainmenu_result == SCREEN_MAINMENU_RESULT_QUICK)
		{
			screen_teamselect(&leaguedata_quick);
		}
		else if(mainmenu_result == SCREEN_MAINMENU_RESULT_LEAGUE)
		{
			
		}
		else if(mainmenu_result == SCREEN_MAINMENU_RESULT_SCORES)
		{
			
		}
	}
}
