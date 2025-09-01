//screen_teamselect.h
//Team select screen for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef SCREEN_TEAMSELECT_H
#define SCREEN_TEAMSELECT_H

#include "teamdata.h"
#include <stdbool.h>

//Runs team-select screen. Returns whether they picked teams (didn't hit back)
bool screen_teamselect(const leaguedata_t *league);

//Returns which team the player chose
int screen_teamselect_getteam(int side);

#endif //SCREEN_TEAMSELECT_H
