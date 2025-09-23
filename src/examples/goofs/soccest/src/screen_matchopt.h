//screen_matchopt.h
//Match options screen for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef SCREEN_MATCHOPT_H
#define SCREEN_MATCHOPT_H

#include "teamdata.h"
#include <stdbool.h>

//Options available
typedef enum screen_matchopt_idx_e
{	
	MO_DURATION,
	MO_BALLSIZE,
	MO_FRAGTYPE,
	
	MO_MAX
} screen_matchopt_idx_t;

//Displays match options screen
bool screen_matchopt(const leaguedata_t *league, int tl, int tr);

//Gets option value as integer
int screen_matchopt_int(screen_matchopt_idx_t opt);

#endif //SCREEN_MATCHOPT_H
