//behave.h
//NPC behavior in match
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef BEHAVE_H
#define BEHAVE_H

#include <stdint.h>
#include <stdbool.h>

//Positions that an NPC can play
typedef enum behave_role_s
{
	BR_NONE = 0,
	
	BR_GOALIE,
	BR_SWEEPER,
	BR_FULLBACK,
	BR_CENTREBACK,
	BR_WINGBACK,
	BR_DEFMID,
	BR_CTRMID,
	BR_ATKMID,
	BR_WINGER,
	BR_FORWARD,
	BR_STRIKER,
	BR_KINDIE,
	
	BR_MAX
} behave_role_t;

//Things that an NPC can be trying to do
typedef enum behave_step_s
{
	BS_NONE = 0,  
	
	BS_HOVER,     //Hang out around default position in the field
	BS_COVER,     //Move to gap in teammates
	BS_POSSESS,   //Get the ball
	BS_ATTACK,    //Move towards the opponent goal with the ball
	BS_DEFEND,    //Get in the way of the ball's likely trajectory
	BS_TAKESHOT,  //Take a shot at the goal
	BS_PASSFORE,  //Pass the ball to a player forward of us
	BS_PASSBACK,  //Pass the ball to a player behind us
	BS_MARK,      //Frustrate an opposing player
	
	BS_MAX,
} behave_step_t;

//Inputs to behavior simulation
typedef struct behave_input_s
{
	behave_role_t role;      //Role we play on our team
	bool have_ball;          //Whether we have the ball 
	bool team_ball;          //Whether our team has the ball
	int32_t attack_dir;      //Direction of opponent goal in X
	int32_t self_pos[2];     //Our position
	int32_t ball_pos[2];     //Ball's position
	int32_t form_pos[2];     //Our intended formation position
	int32_t team_pos[11][2]; //Teammates positions
	int32_t oppo_pos[11][2]; //Opponents positions
} behave_input_t;

//State necessary for behavior simulation
typedef struct behave_state_s
{
	//Immediate behavior goal
	behave_step_t step_now;
	int step_age;
	
	//Where we'd like to get to eventually
	int32_t objective_pos[2];
	int objective_age;
	
	//Where we've next decided to go
	int32_t waypoint_pos[2];
	int waypoint_age;
	
} behave_state_t;

//Outputs from behavior simulation
typedef struct behave_output_s
{
	int32_t dest[2]; //Where we should be going
	int32_t target[2]; //Where we would kick the ball
	bool kick; //Whether we should kick the ball
	bool sprint; //Whether we want to be sprinting
} behave_output_t;

//Updates NPC behavior
void behave_sim(const behave_input_t *in, behave_state_t *state, behave_output_t *out);

#endif //BEHAVE_H
