//screen_padteams.h
//Screen for putting gamepads on teams in soccer game
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef SCREEN_PADTEAMS_H
#define SCREEN_PADTEAMS_H

#include <stdbool.h>

//Runs pad/team assignment screen, returning whether the player wants to continue
bool screen_padteams(void);

//Returns teams that each pad plays on - 0 for unassigned, 1 for home, 2 for away
int screen_padteams_team(int pad);

#endif //SCREEN_PADTEAMS_H
