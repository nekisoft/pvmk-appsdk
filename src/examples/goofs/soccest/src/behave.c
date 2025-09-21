//behave.c
//NPC behavior in match
//Bryan E. Topp <betopp@betopp.com> 2025

#include "behave.h"
#include "statfunc.h"
#include "trigfunc.h"
#include <stddef.h>
#include <stdio.h>

//Changes behavior step and resets counter
#define BEHAVE_GOTO(bs) do { state->step_now = bs; state->step_age = 0; } while(0)

//Checks if this is a newly entered behavior step
#define BEHAVE_FRESH (state->step_age <= 1)

//Out-of-bounds (+/- this amount)
static const int32_t behave_bounds[2] = { 39 * 100 * 256, 30 * 100 * 256 };

//Goal location (+/- this amount)
static const int32_t behave_goal = 39 * 100 * 256;

//Behavior step - hover around formation point
static void behave_sim_hover(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{	
	//Pick a point around our formation position to go to. Repick sometimes.
	if(BEHAVE_FRESH || (state->waypoint_age > 700))
	{
		state->objective_pos[0] = in->form_pos[0];
		state->objective_pos[1] = in->form_pos[1];
		state->objective_age = 0;
		
		state->waypoint_pos[0] = state->objective_pos[0];
		state->waypoint_pos[1] = state->objective_pos[1];
		state->waypoint_pos[0] += 256 * 5 * statfunc_gauss_8b(100,25);
		state->waypoint_pos[1] += 256 * 5 * statfunc_gauss_8b(100,25);
		state->waypoint_age = 4 * statfunc_gauss_8b(100, 30);
	}
	
	//Go
	out->kick = false;
	out->sprint = false;
	out->dest[0] = state->waypoint_pos[0];
	out->dest[1] = state->waypoint_pos[1];
	out->target[0] = out->dest[0];
	out->target[1] = out->dest[1];
	
	if(in->have_ball)
	{
		//Handle cases where we got the ball
		static const behave_step_t onpossess[BR_MAX] = 
		{
			[BR_GOALIE]     = BS_PASSFORE,
			[BR_SWEEPER]    = BS_PASSFORE,
			[BR_FULLBACK]   = BS_PASSFORE,
			[BR_CENTREBACK] = BS_PASSFORE,
			[BR_WINGBACK]   = BS_PASSFORE,
			[BR_DEFMID]     = BS_PASSFORE,
			[BR_CTRMID]     = BS_ATTACK,
			[BR_ATKMID]     = BS_ATTACK,
			[BR_WINGER]     = BS_ATTACK,
			[BR_FORWARD]    = BS_ATTACK,
			[BR_STRIKER]    = BS_ATTACK,
			[BR_KINDIE]     = BS_TAKESHOT,
		};
		BEHAVE_GOTO(onpossess[in->role]);
	}
	else
	{
		
		//Handle cases where we get bored
		static const int boredom_threshold[BR_MAX] = 
		{
			[BR_GOALIE] = 4000,
			[BR_SWEEPER] = 1000,
			[BR_FULLBACK] = 1000,
			[BR_CENTREBACK] = 1000,
			[BR_WINGBACK] = 1000,
			[BR_DEFMID] = 500,
			[BR_CTRMID] = 400,
			[BR_ATKMID] = 30,
			[BR_WINGER] = 20,
			[BR_FORWARD] = 20,
			[BR_STRIKER] = 20,
			[BR_KINDIE] = 10,
		};
		
		static const behave_step_t boredom_options[BR_MAX][2] = 
		{
			[BR_GOALIE]     = { BS_HOVER,   BS_HOVER   },
			[BR_SWEEPER]    = { BS_COVER,   BS_COVER   },
			[BR_FULLBACK]   = { BS_HOVER,   BS_COVER   },
			[BR_CENTREBACK] = { BS_HOVER,   BS_COVER   },
			[BR_WINGBACK]   = { BS_COVER,   BS_COVER   },
			[BR_DEFMID]     = { BS_MARK,    BS_MARK    },
			[BR_CTRMID]     = { BS_MARK,    BS_MARK    },
			[BR_ATKMID]     = { BS_MARK,    BS_POSSESS },
			[BR_WINGER]     = { BS_MARK,    BS_POSSESS },
			[BR_FORWARD]    = { BS_POSSESS, BS_POSSESS },
			[BR_STRIKER]    = { BS_POSSESS, BS_POSSESS },
			[BR_KINDIE]     = { BS_POSSESS, BS_POSSESS },
		};
		
		if(state->step_age > boredom_threshold[in->role] + statfunc_rand_8b())
			BEHAVE_GOTO(boredom_options[in->role][statfunc_rand_8b()&1]);
	}
}


//Behavior step - move to gap in teammates
static void behave_sim_cover(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	//Look for a point to cover periodically
	if(BEHAVE_FRESH || state->waypoint_age > 200)
	{
		//Sample a few points around us.
		//See which one puts us furthest from all our teammates, without going out of bounds.
		int32_t candidate[2] = {in->self_pos[0], in->self_pos[1]};
		int32_t best_candidate[2] = {candidate[0], candidate[1]};
		int32_t best_score = 0;
		for(int attempt = 0; attempt < 10; attempt++)
		{
			//Note that we initialize the candidate point, first, to be our current position.
			//Later iterations get a randomized one.
			
			//Check if the candidate point is valid
			int inbounds = 1;
			if(candidate[0] > behave_bounds[0] || candidate[0] < -behave_bounds[0])
				inbounds = 0;
			if(candidate[1] > behave_bounds[1] || candidate[1] < -behave_bounds[1])
				inbounds = 0;
			
			if(inbounds)
			{
				//Check the distance from the candidate point to teammates.
				int32_t candidate_score = 0;
				for(int tt = 0; tt < 11; tt++)
				{
					if(in->team_pos[tt][0] == in->self_pos[0] && in->team_pos[tt][1] == in->self_pos[1])
						continue;
					
					int32_t dx = (in->team_pos[tt][0] - candidate[0]) / 256;
					int32_t dy = (in->team_pos[tt][1] - candidate[1]) / 256;
					candidate_score += (dx*dx) + (dy*dy);
				}
				
				//See if this was the furthest so far
				if(candidate_score > best_score)
				{
					best_score = candidate_score;
					best_candidate[0] = candidate[0];
					best_candidate[1] = candidate[1];
				}
			}
			
			//Pick a new candidate point next time around
			candidate[0] = in->self_pos[0] + (256 * 2 * statfunc_gauss_8b(100, 50));
			candidate[1] = in->self_pos[1] + (256 * 2 * statfunc_gauss_8b(100, 50));
		}
		
		state->objective_pos[0] = state->waypoint_pos[0] = best_candidate[0];
		state->objective_pos[1] = state->waypoint_pos[1] = best_candidate[1];
		state->objective_age    = state->waypoint_age    = 0;
		
		//printf("Covering: %d\n", best_score);
		
		//If there isn't really any big gaps, give up on covering and go back to formation
		if(best_score < 30000000)
			BEHAVE_GOTO(BS_HOVER);
	}
	
	//Go there
	out->kick = false;
	out->sprint = true;
	out->dest[0] = state->waypoint_pos[0];
	out->dest[1] = state->waypoint_pos[1];
	out->target[0] = out->dest[0];
	out->target[1] = out->dest[1];
	
	//Get bored sometimes and go back to formation
	static const int boredom_threshold[BR_MAX] = 
	{
		[BR_GOALIE] = 4000,
		[BR_SWEEPER] = 1000,
		[BR_FULLBACK] = 1000,
		[BR_CENTREBACK] = 1000,
		[BR_WINGBACK] = 1000,
		[BR_DEFMID] = 500,
		[BR_CTRMID] = 400,
		[BR_ATKMID] = 300,
		[BR_WINGER] = 200,
		[BR_FORWARD] = 200,
		[BR_STRIKER] = 200,
		[BR_KINDIE] = 10,
	};
	if(state->step_age > boredom_threshold[in->role] + statfunc_rand_8b())
		BEHAVE_GOTO(BS_HOVER);
}

//Behavior step - get ball
static void behave_sim_possess(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	//Run straight at the ball!!!
	out->kick = false;
	out->sprint = true;
	out->dest[0] = in->ball_pos[0];
	out->dest[1] = in->ball_pos[1];
	out->target[0] = in->ball_pos[0];
	out->target[1] = in->ball_pos[1];
	
	
	//If we get the ball then figure out what to do
	if(in->have_ball)
	{
		//Handle cases where we got the ball
		static const behave_step_t onpossess[BR_MAX] = 
		{
			[BR_GOALIE]     = BS_PASSFORE,
			[BR_SWEEPER]    = BS_PASSFORE,
			[BR_FULLBACK]   = BS_PASSFORE,
			[BR_CENTREBACK] = BS_PASSFORE,
			[BR_WINGBACK]   = BS_PASSFORE,
			[BR_DEFMID]     = BS_PASSFORE,
			[BR_CTRMID]     = BS_ATTACK,
			[BR_ATKMID]     = BS_ATTACK,
			[BR_WINGER]     = BS_ATTACK,
			[BR_FORWARD]    = BS_ATTACK,
			[BR_STRIKER]    = BS_ATTACK,
			[BR_KINDIE]     = BS_TAKESHOT,
		};
		BEHAVE_GOTO(onpossess[in->role]);
		
	}
	else if(in->team_ball)
	{
		//Handle cases where our team got the ball
		static const behave_step_t onteampossess[BR_MAX] = 
		{
			[BR_GOALIE]     = BS_HOVER,
			[BR_SWEEPER]    = BS_HOVER,
			[BR_FULLBACK]   = BS_HOVER,
			[BR_CENTREBACK] = BS_HOVER,
			[BR_WINGBACK]   = BS_HOVER,
			[BR_DEFMID]     = BS_HOVER,
			[BR_CTRMID]     = BS_MARK,
			[BR_ATKMID]     = BS_MARK,
			[BR_WINGER]     = BS_MARK,
			[BR_FORWARD]    = BS_MARK,
			[BR_STRIKER]    = BS_MARK,
			[BR_KINDIE]     = BS_POSSESS, //KEEP RUNNING AT IT
		};
		BEHAVE_GOTO(onteampossess[in->role]);
	}
	else
	{
		//If the opposing team has it, and we're close, kick it
		int32_t dx = (in->self_pos[0] - in->ball_pos[0]) / 256;
		int32_t dy = (in->self_pos[1] - in->ball_pos[1]) / 256;
		int32_t distsq = (dx*dx)+(dy*dy);
		int32_t kickdist = 90;
		if(distsq < (kickdist * kickdist))
		{
			out->kick = true;
			int balldir = trigfunc_atan2(dy, dx);
			out->target[0] = in->self_pos[0] + (trigfunc_cos8(balldir) * 100);
			out->target[1] = in->self_pos[1] + (trigfunc_sin8(balldir) * 100);
		}
	}
}

//Behavior step - move with ball towards opponent goal
static void behave_sim_attack(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	//Go towards the goal
	//Todo - avoid opposing players along the way
	out->kick = false;
	out->sprint = false;
	out->dest[0] = in->self_pos[0] + in->attack_dir * 256 * 100;
	out->dest[1] = in->self_pos[1];
	out->target[0] = in->self_pos[0] + in->attack_dir * 256 * 100;
	out->target[1] = in->self_pos[1];
	
	//If we're close enough to the goal, take the shot
	//Todo - decide based on opponent positions whether to pass instead of shoot
	int32_t to_goal = in->self_pos[0] - (behave_goal * in->attack_dir);
	if(to_goal < 0)
		to_goal *= -1;
	
	if(to_goal < 256 * 100 * 10)
		BEHAVE_GOTO(BS_TAKESHOT);
	
	//If we lost the ball, try something else
	if(!in->have_ball)
	{
		static const behave_step_t onlose[BR_MAX] = 
		{
			[BR_GOALIE]     = BS_HOVER,
			[BR_SWEEPER]    = BS_HOVER,
			[BR_FULLBACK]   = BS_HOVER,
			[BR_CENTREBACK] = BS_HOVER,
			[BR_WINGBACK]   = BS_HOVER,
			[BR_DEFMID]     = BS_COVER,
			[BR_CTRMID]     = BS_POSSESS,
			[BR_ATKMID]     = BS_POSSESS,
			[BR_WINGER]     = BS_COVER,
			[BR_FORWARD]    = BS_POSSESS,
			[BR_STRIKER]    = BS_POSSESS,
			[BR_KINDIE]     = BS_POSSESS, //NOOOOOOO
		};
		
		BEHAVE_GOTO(onlose[in->role]);
	}
}

//Behavior step - try to get in the way of the ball
static void behave_sim_defend(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	(void)in; (void)state; (void)out;
	BEHAVE_GOTO(BS_HOVER);
	
	//Based on the ball's velocity, find the line it's moving along.
	//Then, move towards the closest point on that line.
	//todo
}

//Behavior step - try to shoot the opponent's goal
static void behave_sim_takeshot(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	//Shoot at the goal
	out->kick = true;
	out->sprint = false;
	out->dest[0] = in->self_pos[0];
	out->dest[1] = in->self_pos[1];
	out->target[0] = in->attack_dir * behave_goal;
	out->target[1] = 0; //Todo - try to aim around opponent players
	
	//This should result in us losing the ball
	if(!in->have_ball)
	{
		//I dunno, this assumes that we didn't score...so maybe try again?
		static const behave_step_t onshot[BR_MAX] = 
		{
			[BR_GOALIE]     = BS_POSSESS,
			[BR_SWEEPER]    = BS_POSSESS,
			[BR_FULLBACK]   = BS_POSSESS,
			[BR_CENTREBACK] = BS_POSSESS,
			[BR_WINGBACK]   = BS_POSSESS,
			[BR_DEFMID]     = BS_POSSESS,
			[BR_CTRMID]     = BS_POSSESS,
			[BR_ATKMID]     = BS_POSSESS,
			[BR_WINGER]     = BS_POSSESS,
			[BR_FORWARD]    = BS_POSSESS,
			[BR_STRIKER]    = BS_POSSESS,
			[BR_KINDIE]     = BS_POSSESS,
		};
		
		BEHAVE_GOTO(onshot[in->role]);
	}
	
}

//Behavior step - pass to teammate ahead of you
static void behave_sim_passfore(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	(void)in; (void)state; (void)out;
	BEHAVE_GOTO(BS_HOVER);
}

//Behavior step - pass to teammate behind you
static void behave_sim_passback(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	(void)in; (void)state; (void)out;
	BEHAVE_GOTO(BS_HOVER);
}

//Behavior step - harass an opponent
static void behave_sim_mark(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	(void)in; (void)state; (void)out;
	BEHAVE_GOTO(BS_HOVER);
}

//Table of behavior steps
typedef void (*behave_step_fn_t)(const behave_input_t *in, behave_state_t *state, behave_output_t *out);
static const behave_step_fn_t behave_step_fn_table[BS_MAX] = 
{
	[BS_HOVER] = &behave_sim_hover,
	[BS_COVER] = &behave_sim_cover,
	[BS_POSSESS] = &behave_sim_possess,
	[BS_ATTACK] = &behave_sim_attack,
	[BS_DEFEND] = &behave_sim_defend,
	[BS_TAKESHOT] = &behave_sim_takeshot,
	[BS_PASSFORE] = &behave_sim_passfore,
	[BS_PASSBACK] = &behave_sim_passback,
	[BS_MARK] = &behave_sim_mark,
};

void behave_sim(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	//Advance ages of everything
	state->step_age++;
	state->objective_age++;
	state->waypoint_age++;
	
	//Validate state before taking a function-pointer based on it
	if(state->step_now <= 0 || state->step_now >= BS_MAX || behave_step_fn_table[state->step_now] == NULL)
	{
		//Invalid state - go back to hanging out at default position
		state->step_now = BS_HOVER;
		state->step_age = 0;
	}
	
	//Call appropriate function for the behavior step (state) we're in
	(*behave_step_fn_table[state->step_now])(in, state, out);
}


const char *behave_dbgstr(const behave_state_t *st)
{
	static const char *stnames[BS_MAX] = 
	{
		[BS_HOVER] = "hover",
		[BS_COVER] = "cover",
		[BS_POSSESS] = "possess",
		[BS_ATTACK] = "attack",
		[BS_DEFEND] = "defend",
		[BS_TAKESHOT] = "takeshot",
		[BS_PASSFORE] = "passfore",
		[BS_PASSBACK] = "passback",
		[BS_MARK] = "mark",
	};
	const char *stname = "?";
	if(st->step_now >= 0 && st->step_now < BS_MAX && stnames[st->step_now] != NULL)
		stname = stnames[st->step_now];
	
	return stname;
	
}
