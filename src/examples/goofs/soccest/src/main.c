//main.c
//Entry point for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "screen_title.h"
#include "screen_mainmenu.h"
#include "screen_teamselect.h"
#include "screen_padteams.h"

#include "teamdata.h"

typedef enum nextscreen_e
{
	NS_NONE = 0,
	NS_TITLE,
	NS_MAINMENU,
	NS_TEAMSELECT,
	NS_PADTEAMS,
	NS_MATCHOPT,
	NS_MAX
} nextscreen_t;
nextscreen_t nextscreen = 0;


int main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	
	//Generate the quick-match league data once on startup
	leaguedata_generate(&leaguedata_quick);
	
	//Show UI flow
	while(1)
	{
		switch(nextscreen)
		{
			case NS_TITLE:
			{
				screen_title();
				nextscreen = NS_MAINMENU;
			}
			break;
			case NS_MAINMENU:
			{
				screen_mainmenu_result_t mainmenu_result = screen_mainmenu();
				if(mainmenu_result == SCREEN_MAINMENU_RESULT_QUICK)
				{
					nextscreen = NS_TEAMSELECT;
				}
				else if(mainmenu_result == SCREEN_MAINMENU_RESULT_LEAGUE)
				{
					nextscreen = NS_TITLE;
				}
				else if(mainmenu_result == SCREEN_MAINMENU_RESULT_SCORES)
				{
					nextscreen = NS_TITLE;
				}
				else
				{
					nextscreen = NS_TITLE;
				}
			}
			break;
			case NS_TEAMSELECT:
			{
				if(screen_teamselect(&leaguedata_quick))
				{
					nextscreen = NS_PADTEAMS;
				}
				else
				{
					nextscreen = NS_MAINMENU;
				}
			}
			break;
			case NS_PADTEAMS:
			{
				if(screen_padteams())
				{
					nextscreen = NS_MATCHOPT;
				}
				else
				{
					nextscreen = NS_TEAMSELECT;
				}
			}
			break;
			default:
			{
				nextscreen = NS_TITLE;
				break;
			}
		}
	}
}
