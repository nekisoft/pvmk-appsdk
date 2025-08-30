//screen_mainmenu.h
//Main menu for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef SCREEN_MAINMENU_H
#define SCREEN_MAINMENU_H

typedef enum screen_mainmenu_result_e 
{
	SCREEN_MAINMENU_RESULT_QUICK,
	SCREEN_MAINMENU_RESULT_LEAGUE,
	SCREEN_MAINMENU_RESULT_SCORES,
} screen_mainmenu_result_t;
screen_mainmenu_result_t screen_mainmenu(void);

#endif //SCREEN_MAINMENU_H
