//behave.c
//NPC behavior in match
//Bryan E. Topp <betopp@betopp.com> 2025

#include "behave.h"
#include "statfunc.h"
#include <stddef.h>

//Changes behavior step and resets counter
#define BEHAVE_GOTO(bs) do { state->step_now = bs; state->step_age = 0; } while(0)

//Checks if this is a newly entered behavior step
#define BEHAVE_FRESH (state->step_age <= 1)

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
			[BR_ATKMID] = 300,
			[BR_WINGER] = 200,
			[BR_FORWARD] = 200,
			[BR_STRIKER] = 200,
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
		
		if(state->step_age > boredom_threshold[in->role])
			BEHAVE_GOTO(boredom_options[in->role][statfunc_rand_8b()&1]);
	}
}


//Behavior step - move to gap in teammates
static void behave_sim_cover(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	(void)in; (void)state; (void)out;
	BEHAVE_GOTO(BS_HOVER);
}

//Behavior step - get ball
static void behave_sim_possess(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	(void)in; (void)state; (void)out;
	BEHAVE_GOTO(BS_HOVER);
}

//Behavior step - move with ball towards opponent goal
static void behave_sim_attack(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	(void)in; (void)state; (void)out;
	BEHAVE_GOTO(BS_HOVER);
}

//Behavior step - try to get in the way of the ball
static void behave_sim_defend(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	(void)in; (void)state; (void)out;
	BEHAVE_GOTO(BS_HOVER);
}

//Behavior step - try to shoot the opponent's goal
static void behave_sim_takeshot(const behave_input_t *in, behave_state_t *state, behave_output_t *out)
{
	(void)in; (void)state; (void)out;
	BEHAVE_GOTO(BS_HOVER);
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

