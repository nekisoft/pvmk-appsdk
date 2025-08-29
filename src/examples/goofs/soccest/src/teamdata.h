//teamdata.h
//Shared team data for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef TEAMDATA_H
#define TEAMDATA_H

#include <stdint.h>

//Size of name fields in bytes including NUL
#define TEAMDATA_NAMEBUF 24

//Max players rostered on a team
#define TEAMDATA_MAXPERSONS 24

//Max teams tracked in a game
#define TEAMDATA_MAXTEAMS 16

typedef struct persondata_s
{
	char name[2][TEAMDATA_NAMEBUF]; //given and family name
	uint8_t look; //Some bits of appearance
	uint8_t height; //Height, cm
	uint8_t mass; //Mass, kg
	uint8_t runspeed; //Normal running speed m/s
	uint8_t sprintspeed; //Sprinting speed m/s
	uint8_t ballspeed; //Dribbling speed m/s
	uint8_t staminamax; //Max stamina amount
	uint8_t staminarecover; //How quickly stamina recovers
	uint8_t kickpower; //How strong they kick the ball/players
	uint8_t accuracy; //How much they miss
	uint8_t hpmax; //How much they can be kicked before giving up
	uint8_t hprecover; //How quickly hp recovers
	uint8_t throwpower; //How strong they throw the ball
	uint8_t dive; //How far/fast they can dive
	uint8_t eyesight; //How well they see things far away
	uint8_t reaction; //How long it takes to react to things
	int salary; //How much they are paid each season
} persondata_t;

typedef struct teamdata_s
{
	//Identity
	char name[TEAMDATA_NAMEBUF]; //Name of team
	char city[TEAMDATA_NAMEBUF]; //Home location
	char venue[TEAMDATA_NAMEBUF]; //Home stadium
	//Persons on the team roster.
	//Index 0 is goalie.
	//Indexes 1 to 10 are also playing.
	//Further indexes are spares.
	persondata_t persons[TEAMDATA_MAXPERSONS];
	
	//How much money the team has
	int money;
	
	//Perceived marketing power due to wins/losses
	uint8_t hype;
	
	//How much the team tends to beat up their opposition
	uint8_t scandal;
	
} teamdata_t;

typedef struct leaguedata_s
{
	//Name of savegame
	char label[TEAMDATA_NAMEBUF];
	
	//All teams in league
	teamdata_t teams[TEAMDATA_MAXTEAMS];
	
	//Persons not on any team
	persondata_t unemployed[TEAMDATA_MAXPERSONS];
	
} leaguedata_t;

typedef struct nvm_s
{
	leaguedata_t leagues[4];
} nvm_t;


#endif //TEAMDATA_H
