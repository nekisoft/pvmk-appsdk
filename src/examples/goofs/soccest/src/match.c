//match.c
//Soccer match
//Bryan E. Topp <betopp@betopp.com> 2025

#include <sc.h>
#include <stdbool.h>

#include "match.h"
#include "statfunc.h"
#include "teamdata.h"
#include "fbs.h"
#include "pads.h"
#include "font.h"
#include "images.h"

#include "stb_image.h"
#include <string.h>

//State that the game flow can be in
typedef enum match_state_e
{
	MS_NONE = 0,
	MS_INTRO,
	MS_PLAY,
	MS_GOAL,
	MS_DONE,
	MS_MAX
} match_state_t;
static match_state_t match_state;

//Names of match states
static const char *match_state_names[] = 
{
	[MS_NONE] = "NONE",
	[MS_INTRO] = "INTRO",
	[MS_PLAY] = "PLAY",
	[MS_GOAL] = "GOAL",
	[MS_DONE] = "DONE",
};

//How long we've been in the given state, in 100hz ticks
static int32_t match_state_ticks;

//Changes match state
static void state(match_state_t ms)
{
	match_state = ms;
	match_state_ticks = 0;
} 

//UI elements that can be shown
typedef enum match_uipart_e
{
	UI_NONE = 0,
	UI_GOAL,
	UI_READY,
	UI_MAX
} match_uipart_t;
static bool match_uipart_visible[UI_MAX];

static void show(match_uipart_t pp)
{
	match_uipart_visible[pp] = true;
}

static void hide(match_uipart_t pp)
{
	match_uipart_visible[pp] = false;
}

static void hideall(void)
{
	memset(match_uipart_visible, 0, sizeof(match_uipart_visible));
}

//Parts of person graphic
typedef enum part_idx_e
{
	PA_NONE = 0,
	PA_HEAD,
	PA_SHOULDERS,
	PA_TORSO,
	PA_HIPS,
	PA_LEGL0,
	PA_LEGL1,
	PA_LEGR0,
	PA_LEGR1,
	PA_FOOTL,
	PA_FOOTR,
	PA_ARML0,
	PA_ARML1,
	PA_ARMR0,
	PA_ARMR1,
	PA_MAX
} part_idx_t;
typedef struct part_s
{
	images_file_t imf; //Image file index
	int vh; //Virtual height, cm 24.8
	//todo - maybe alternates/rotations/whatever
} part_t;
static const part_t part_table[PA_MAX] = 
{
	[PA_HEAD]      = { .imf = IMF_CARD_SPHERE, .vh = 18*256 },
	[PA_SHOULDERS] = { .imf = IMF_CARD_SPHERE, .vh = 45*256 },
	[PA_TORSO]     = { .imf = IMF_CARD_SPHERE, .vh = 48*256 },
	[PA_HIPS]      = { .imf = IMF_CARD_SPHERE, .vh = 32*256 },
	[PA_LEGL0]     = { .imf = IMF_CARD_SPHERE, .vh = 12*256 },
	[PA_LEGL1]     = { .imf = IMF_CARD_SPHERE, .vh = 12*256 },
	[PA_LEGR0]     = { .imf = IMF_CARD_SPHERE, .vh = 12*256 },
	[PA_LEGR1]     = { .imf = IMF_CARD_SPHERE, .vh = 12*256 },
	[PA_FOOTL]     = { .imf = IMF_CARD_SPHERE, .vh = 10*256 },
	[PA_FOOTR]     = { .imf = IMF_CARD_SPHERE, .vh = 10*256 },
	[PA_ARML0]     = { .imf = IMF_CARD_SPHERE, .vh = 12*256 },
	[PA_ARML1]     = { .imf = IMF_CARD_SPHERE, .vh = 12*256 },
	[PA_ARMR0]     = { .imf = IMF_CARD_SPHERE, .vh = 12*256 },
	[PA_ARMR1]     = { .imf = IMF_CARD_SPHERE, .vh = 12*256 },
};

//Frames of animation that persons can be in
typedef enum anim_idx_e
{
	AN_NONE = 0,
	AN_STAND,
	AN_MAX
} anim_idx_t;
typedef struct anim_s
{
	part_idx_t rel;
	int32_t pos[3];
} anim_t;
static anim_t anim_table[AN_MAX][PA_MAX] = 
{
	[AN_STAND] = 
	{
		[PA_HEAD]      = { .rel = PA_SHOULDERS, .pos = { 0,       0,  19*256 } },
		[PA_SHOULDERS] = { .rel = PA_TORSO,     .pos = { 0,       0,  24*256 } },
		[PA_TORSO]     = { .rel = 0,            .pos = { 0,       0, 120*256 } },
		[PA_HIPS]      = { .rel = PA_TORSO,     .pos = { 0,       0, -24*256 } },
		[PA_LEGL0]     = { .rel = PA_HIPS,      .pos = { 0,  10*256, -24*256 } },
		[PA_LEGL1]     = { .rel = PA_LEGL0,     .pos = { 0,       0, -48*256 } },
		[PA_LEGR0]     = { .rel = PA_HIPS,      .pos = { 0, -10*256, -24*256 } },
		[PA_LEGR1]     = { .rel = PA_LEGR0,     .pos = { 0,       0, -48*256 } },
		[PA_FOOTL]     = { .rel = PA_LEGL1,     .pos = { 0,       0, -48*256 } },
		[PA_FOOTR]     = { .rel = PA_LEGR1,     .pos = { 0,       0, -48*256 } },
		[PA_ARML0]     = { .rel = PA_SHOULDERS, .pos = { 0,   8*256, -24*256 } },
		[PA_ARML1]     = { .rel = PA_ARML0,     .pos = { 0,       0, -24*256 } },
		[PA_ARMR0]     = { .rel = PA_SHOULDERS, .pos = { 0,  -8*256, -24*256 } },
		[PA_ARMR1]     = { .rel = PA_ARMR0,     .pos = { 0,       0, -24*256 } },
	},
	
};

//Pitch-space (0,0 at center of pitch) where camera looks - centimeters 24.8
static int32_t cam_center[2];

//Horizontal extent of camera at center of screen - centimeters 24.8
static int32_t cam_radius;

//Velocity of camera
static int32_t cam_vel[2];


//Position of ball on pitch - centimeters 24.8
static int32_t ball_pos[3];

//Velocity of ball - cm/s 24.8
static int32_t ball_vel[3];

//Ball animation with fraction towards the next frame - xx.8
static int32_t ball_frame;

//Location of goal
static const int goalwidth = 732 * 256; //Spacing between goal posts
static const int goalheight = 244 * 256; //Height of goal top beam
static const int goaldist = 39*100*256; //Distance from center of field to goal line

//Location of goalposts
static int32_t goalpost_pos[4][3];

//Information about a player on the field
typedef struct person_s
{
	int32_t pos[3];
	int32_t vel[3];
	anim_idx_t anim;
	int animticks;
} person_t;
person_t person_table[2][11];

//Which team each gamepad belongs to
int pad_team[4];

//Which person on the team each gamepad is controlling
int pad_person[4];

//Loads pitch (grass) texture
#define PITCHTEX_DIM 2048
#define PITCHTEX_DIM_LOG2 11
static uint16_t pitchtex[PITCHTEX_DIM][PITCHTEX_DIM];
static void pitchtex_load(void)
{
	int pitchx, pitchy, pitchn;
	uint8_t *pitchpng = stbi_load("textures/pitch.png", &pitchx, &pitchy, &pitchn, 3);
	if(pitchpng)
	{
		const uint8_t *inptr = pitchpng;
		for(int yy = 0; yy < PITCHTEX_DIM; yy++)
		{
			for(int xx = 0; xx < PITCHTEX_DIM; xx++)
			{
				uint16_t px = 0;
				px |= (inptr[0] >> 3) << 0;
				px |= (inptr[1] >> 2) << 5;
				px |= (inptr[2] >> 3) << 11;
				inptr += 3;
				pitchtex[yy][xx] = px;
			}
		}
		stbi_image_free(pitchpng);
	}	
	else
	{
		memset(pitchtex, 0xAA, sizeof(pitchtex));
	}
}

//Fills screen with projected grass texture background
static void drawpitch(void)
{
	for(int yy = 0; yy < 480; yy++)
	{
		uint16_t *fbrow = &(fbs[fbs_next][yy][0]);
		
		int32_t dy = 240 - yy;
		int32_t depth = 1024 + yy;
		int32_t texy = cam_radius * (PITCHTEX_DIM/4) * dy / depth;
		texy += cam_center[1] / 3;
		
		uint32_t ycoord = texy + (PITCHTEX_DIM*128);
		
		if(ycoord < 0 || ycoord >= (PITCHTEX_DIM<<8))
		{
			memset(fbrow, 0, 640*2);
			continue;
		}
		
		uint16_t *texrow = &(pitchtex[ (ycoord >> 8) % PITCHTEX_DIM ][0]);
		
		int32_t texfrac = ((PITCHTEX_DIM/2)*256) + (cam_center[0]/4);
		int32_t texstep = cam_radius * PITCHTEX_DIM / 8192;
		texstep *= 1024;
		texstep /= depth;
		texfrac -= texstep * 320;
		for(int xx = 0; xx < 640; xx++)
		{
			if(texfrac < 0 || texfrac >= (PITCHTEX_DIM<<8))
				fbrow[xx] = 0;
			else
				fbrow[xx] = texrow[ (texfrac >> 8) % PITCHTEX_DIM];
			
			texfrac += texstep;
			
			//if((texfrac>>8)==((PITCHTEX_DIM/2)+(cam_center[0]>>10)))
			//	fbrow[xx] = 0xFFFF;
			
			//if((ycoord>>8)==((PITCHTEX_DIM/2)+((cam_center[1]/3)>>8)))
			//	fbrow[xx] = 0xFFFF;
		}
	}
}

//Draws sprite with the given virtual (worldspace) x, y, height
//x and y are center-of-bottom coordinates
//vx, vy, vh in 24.8 fixed-point centimeters (meters * 100 * 256)
static void drawcard(images_file_t imf, int vx, int vy, int vz, int vh)
{	
	int vy_rel = vy - cam_center[1];
	int vx_rel = vx - cam_center[0];
	int vz_rel = vz;
	
	int sy = 1024 * ((cam_radius * 360) - vy_rel ) / ((cam_radius * 1536) + vy_rel);	
	int sx = 320 +  (vx_rel * (1024 + sy) / (cam_radius * 1024));
	int sh =        (    vh * (1024 + sy) / (cam_radius * 1024));
	
	sy -=           (vz_rel * (1024 + sy) / (cam_radius * (1024))); //not really correct but whatever
	
	sy+=(sh/8);	
	images_card(imf, sx, sy, sh);
}

//Draws person composed of many cards
static void drawperson(anim_idx_t anim, int facing, int vx, int vy, int vz)
{
	for(int pp = 0; pp < PA_MAX; pp++)
	{
		int32_t partpos[3] = { vx, vy, vz };
		
		(void)facing;
		partpos[0] += anim_table[anim][pp].pos[0];
		partpos[1] += anim_table[anim][pp].pos[1];
		partpos[2] += anim_table[anim][pp].pos[2];
		
		drawcard(part_table[pp].imf, partpos[0], partpos[1], partpos[2], part_table[pp].vh);
	}
}

//Draws UI element - "GOAL" overlay
static void match_uipart_draw_goal(void)
{
	int rnds[4][2] = 
	{
		{ (statfunc_rand_8b() - 128) / 16, (statfunc_rand_8b() - 128) / 16, },
		{ (statfunc_rand_8b() - 128) / 8, (statfunc_rand_8b() - 128) / 8, },
		{ (statfunc_rand_8b() - 128) / 4, (statfunc_rand_8b() - 128) / 4, },
		{ (statfunc_rand_8b() - 128) / 4, (statfunc_rand_8b() - 128) / 4, },
	};
	
	int basex = 120;
	int basey = 160;
	
	images_draw(IMF_MATCH_GOAL3, basex + rnds[3][0], basey + rnds[3][1]);
	images_draw(IMF_MATCH_GOAL2, basex + rnds[2][0], basey + rnds[2][1]);
	images_draw(IMF_MATCH_GOAL1, basex + rnds[1][0], basey + rnds[1][1]);
	images_draw(IMF_MATCH_GOAL0, basex + rnds[0][0], basey + rnds[0][1]);
	
}

//Draws UI element - "Get Ready" overlay
static void match_uipart_draw_ready(void)
{
	images_draw(IMF_MATCH_READY, 136, 100);
}

//Table of draw functions for each UI element
typedef void (*match_uipart_drawfn_t)(void);
match_uipart_drawfn_t match_uipart_drawfns[UI_MAX] =
{
	[UI_GOAL] = &match_uipart_draw_goal,
	[UI_READY] = &match_uipart_draw_ready,
};


//Draws all the "3d view" elements at the current camera position.
void drawworld(void)
{
	drawpitch();
	
	char txtbuf[256];
	//snprintf(txtbuf, sizeof(txtbuf)-1, "%8d %8d", cam_radius,cam_center[1]);
	//font_draw(txtbuf, 0x8000,0,0);
	
	snprintf(txtbuf, sizeof(txtbuf)-1, "%s: %d", match_state_names[match_state], match_state_ticks);
	font_draw(txtbuf, 0x8000, 16, 0);
	
	//Draw cones to test scale of projection
	drawcard(IMF_CARD_CONE, 0, 0, 0, 100*256);
	drawcard(IMF_CARD_CONE, 800*256, 0, 0, 100*256);
	drawcard(IMF_CARD_CONE, -800*256, 0, 0, 100*256);
	drawcard(IMF_CARD_CONE, 0, 800*256, 0, 100*256);
	drawcard(IMF_CARD_CONE, 0, -800*256, 0, 100*256);
	
	//Draw ball shadow + ball
	drawcard(IMF_CARD_BALLSH, ball_pos[0], ball_pos[1], -256*5, 16*256);
	drawcard(IMF_CARD_BALL0 + ((ball_frame >> 8) & 0x3u), ball_pos[0], ball_pos[1], ball_pos[2], 24*256);
	
	//Draw goalposts
	for(int pp = 0; pp < 4; pp++)
	{
		drawcard(IMF_CARD_GOALPOST, 
			goalpost_pos[pp][0], goalpost_pos[pp][1], goalpost_pos[pp][2], 
			goalheight);
		
		int bump = 100 * ((goalpost_pos[pp][0] > 0) ? 256 : -256);
		
		drawcard(IMF_CARD_GOALSIDE,
			goalpost_pos[pp][0] + bump, goalpost_pos[pp][1], goalpost_pos[pp][2], 
			goalheight);
	}
	
	//Draw goal crossbeams
	for(int gg = 0; gg < 2; gg++)
	{
		int pos[3];
		int step[3];
		pos[0] = goalpost_pos[ (gg*2) + 0 ][0];
		pos[1] = goalpost_pos[ (gg*2) + 0 ][1];
		pos[2] = goalpost_pos[ (gg*2) + 0 ][2] + goalheight;
		step[0] = (goalpost_pos[ (gg*2) + 1 ][0] - pos[0]) / 16;
		step[1] = (goalpost_pos[ (gg*2) + 1 ][1] - pos[1]) / 16;
		step[2] = (goalpost_pos[ (gg*2) + 1 ][2] + goalheight - pos[2]) / 16;
		
		pos[2] -= 64*256;
		
		for(int ss = 0; ss <= 16; ss++)
		{
			drawcard(IMF_CARD_GOALTOP, pos[0], pos[1], pos[2], 32*256);
			pos[0] += step[0];
			pos[1] += step[1];
			pos[2] += step[2];
		}
	}	
	
	//Draw players
	for(int tt = 0; tt < 2; tt++)
	{
		for(int pp = 0; pp < 11; pp++)
		{
			const person_t *pptr = &(person_table[tt][pp]);
			drawperson(AN_STAND, 0, pptr->pos[0], pptr->pos[1], pptr->pos[2]);
		}
	}
}

//Triggers a goal
void goal(int team)
{
	(void)team;
	state(MS_GOAL);
}

//Simulates ball for one tick
void sim_ball(void)
{
	//Add gravity to ball velocity
	ball_vel[2] -= (980 * 256) / 100;
	
	//Decay horizontal ball velocity if on ground
	if(ball_pos[2] < 256)
	{
		ball_vel[0] *= 255;
		ball_vel[0] /= 256;
		ball_vel[1] *= 255;
		ball_vel[1] /= 256;
	}
	
	//Accumulate ball velocity into ball position
	ball_pos[0] += ball_vel[0] / 100;
	ball_pos[1] += ball_vel[1] / 100;
	ball_pos[2] += ball_vel[2] / 100;
	
	//Animate ball spinning
	ball_frame += ball_vel[0] / 1000;
	ball_frame += ball_vel[1] / 1000;
	
	//Bounce ball off sides of pitch
	for(int dd = 0; dd < 2; dd++)
	{
		int32_t pitch_extents[2] = 
		{
			(105 * 100 * 256) / 2,
			(68 * 100 * 256) / 2,
		};
		
		if(ball_pos[dd] < -pitch_extents[dd])
		{
			ball_pos[dd] = -pitch_extents[dd];
			if(ball_vel[dd] < 0)
				ball_vel[dd] *= -1;
		}
		if(ball_pos[dd] > pitch_extents[dd])
		{
			ball_pos[dd] = pitch_extents[dd];
			if(ball_vel[dd] > 0)
				ball_vel[dd] *= -1;
		}
	}
	
	//Bounce ball off the floor
	if(ball_pos[2] < 0)
	{
		ball_pos[2] = 0;
		if(ball_vel[2] < 0)
		{
			if(ball_vel[2] < -100)
			{
				ball_vel[2] *= -1;
				ball_vel[2] *= 7;
				ball_vel[2] /= 8;
				ball_vel[2] *= 7;
				ball_vel[2] /= 8;
			}
			else
			{
				ball_vel[2] = 0;
			}
		}
	}
}

//Simulates camera for one tick - following ball
void sim_cam_toball(void)
{
	//Accelerate camera movement towards ball
	for(int dd = 0; dd < 2; dd++)
	{
		int veldiff = ball_vel[dd] - cam_vel[dd];
		int posdiff = ball_pos[dd] - cam_center[dd];
		
		cam_vel[dd] += veldiff / 100;
		cam_vel[dd] += posdiff / 100;
	}
	
	//Accumulate camera
	cam_center[0] += cam_vel[0] / 100;
	cam_center[1] += cam_vel[1] / 100;
	
	//Todo - camera radius feedback... maybe track multiple points?
	if(cam_radius < 100)
		cam_radius = 100;
}

//Simulates closeup camera for a single gamepad's person
void sim_cam_topad(int pad)
{
	person_t *pptr = &(person_table[pad_team[pad]][pad_person[pad]]);
	
	//Accelerate camera movement towards ball
	for(int dd = 0; dd < 2; dd++)
	{
		int veldiff = pptr->vel[dd] - cam_vel[dd];
		int posdiff = pptr->pos[dd] - cam_center[dd];
		
		cam_vel[dd] += veldiff / 100;
		cam_vel[dd] += posdiff / 100;
	}
	
	//Accumulate camera
	cam_center[0] += cam_vel[0] / 100;
	cam_center[1] += cam_vel[1] / 100;
	
	//Todo - camera radius feedback... maybe track multiple points?
	if(cam_radius < 100)
		cam_radius = 100;	
	
}

//Simulates goals for one tick, checking if a goal was scored
void sim_goals(void)
{
	//Check if a goal was scored
	if(ball_pos[1] >= -(goalwidth / 2) && ball_pos[1] <= (goalwidth / 2))
	{
		if(ball_pos[0] < -goaldist)
		{
			goal(0);
		}
		else if(ball_pos[0] > goaldist)
		{
			goal(1);
		}
	}
}

//Simulates a person using gamepad input
void sim_person_pad(int team, int person, int pad)
{
	person_t *pptr = &(person_table[team][person]);
	
	if(pads[pad] & BTNBIT_LEFT)
		pptr->vel[0] -= 200;
	if(pads[pad] & BTNBIT_RIGHT)
		pptr->vel[0] += 200;
	if(pads[pad] & BTNBIT_UP)
		pptr->vel[1] += 200;
	if(pads[pad] & BTNBIT_DOWN)
		pptr->vel[1] -= 200;
	
}

//Simulates a person using CPU behaviour
void sim_person_cpu(int team, int person)
{
	(void)team;
	(void)person;
}

//Simulates all persons on the field
void sim_persons(void)
{
	//Don't let two gamepads control the same person
	for(int ii = 0; ii < 4; ii++)
	{
		for(int jj = ii + 1; jj < 4; jj++)
		{
			if(pad_team[ii] == pad_team[jj])
			{
				if(pad_person[ii] == pad_person[jj])
				{
					pad_person[jj] = pad_person[jj] + 1;
					if(pad_person[jj] >= 11)
						pad_person[jj] = 0;
				}
			}
		}
	}
	
	//Iterate through all and either let the gamepad control them or run their AI, then advance physics
	for(int tt = 0; tt < 2; tt++)
	{
		for(int pp = 0; pp < 11; pp++)
		{
			//Check if a gamepad is controlling this person
			int controlling_pad = -1;
			for(int pad = 0; pad < 4; pad++)
			{
				if(pad_team[pad] == tt && pad_person[pad] == pp)
					controlling_pad = pad;
			}
			if(controlling_pad == -1)
			{
				//CPU controlled. Run CPU behaviour.
				sim_person_cpu(tt, pp);
			}
			else
			{
				//Gamepad controlled. Run input handler.
				sim_person_pad(tt, pp, controlling_pad);
			}
			
			//Run common updates...
			person_t *pptr = &(person_table[tt][pp]);
			
			//Apply gravity
			pptr->vel[2] -= (980*256) / 100;
			
			//Accumulate velocity into position
			pptr->pos[0] += pptr->vel[0] / 100;
			pptr->pos[1] += pptr->vel[1] / 100;
			pptr->pos[2] += pptr->vel[2] / 100;
			
			//Land firm on ground
			if(pptr->pos[2] < 0)
			{
				pptr->pos[2] = 0;
				pptr->vel[2] = 0;
			}
			
			//Decay velocity while on ground
			if(pptr->pos[2] < 256)
			{
				pptr->vel[0] *= 1023;
				pptr->vel[0] /= 1024;
				pptr->vel[1] *= 1023;
				pptr->vel[1] /= 1024;
			}
			
		}
	}
	
	
}

//Match tick function - introduction sequence
void match_tick_intro(void)
{
	hideall();
	if( (match_state_ticks / 32) & 1 )
		show(UI_READY);
	
	if(match_state_ticks >= 32*6)
	{
		hideall();
		state(MS_PLAY);
	}
}

//Match tick function - playing
void match_tick_play(void)
{
	sim_persons();
	sim_ball();
	//sim_cam_toball();
	sim_cam_topad(0);
	sim_goals();
}

//Match tick function - goal scored
void match_tick_goal(void)
{
	hideall();
	show(UI_GOAL);
	sim_ball();
	
	if(match_state_ticks > 100)
	{
		hide(UI_GOAL);
		state(MS_PLAY);
	}
}

//Table of tick functions for different match states
typedef void (*match_state_tickfn_t)(void);
match_state_tickfn_t match_state_tickfns[MS_MAX] = 
{
	[MS_INTRO] = &match_tick_intro,
	[MS_PLAY]  = &match_tick_play,
	[MS_GOAL]  = &match_tick_goal,
};

//Runs at 100hz to simulate match
void match_tick(void)
{
	//Bail out if we end up in an invalid state
	if(match_state < 0 || match_state >= MS_MAX)
	{
		match_state = MS_DONE;
		return;
	}
	if(match_state_tickfns[match_state] == NULL)
	{
		match_state = MS_DONE;
		return;
	}
	
	//Run the tick function for the state we're in
	match_state_ticks++;
	(*(match_state_tickfns[match_state]))();
}

void match(void)
{
	//Load data for match
	images_purge();
	images_loadrange(IMF_CARD_AAA, IMF_CARD_ZZZ);
	images_loadrange(IMF_MATCH_AAA, IMF_MATCH_ZZZ);
	pitchtex_load();
	
	//Flatten animation data so everything is just relative to the model root
	for(int aa = 0; aa < AN_MAX; aa++)
	{
		for(int pass = 0; pass < PA_MAX; pass++)
		{
			for(int pp = 0; pp < PA_MAX; pp++)
			{
				anim_table[aa][pp].pos[0] += anim_table[aa][anim_table[aa][pp].rel].pos[0];
				anim_table[aa][pp].pos[1] += anim_table[aa][anim_table[aa][pp].rel].pos[1];
				anim_table[aa][pp].pos[2] += anim_table[aa][anim_table[aa][pp].rel].pos[2];
				anim_table[aa][pp].rel = anim_table[aa][anim_table[aa][pp].rel].rel;
			}
		}
	}
	
	//Set up match initial state
	match_state = MS_INTRO;
	
	cam_center[0] = 0; //cm 24.8
	cam_center[1] = 0; //cm 24.8
	
	cam_radius = 800; //cm
	
	ball_pos[0] = 0;
	ball_pos[1] = 0;
	ball_pos[2] = 0;
	
	goalpost_pos[0][0] = goaldist;
	goalpost_pos[0][1] = goalwidth / 2;
	goalpost_pos[0][2] = 0;
	
	goalpost_pos[1][0] = goaldist;
	goalpost_pos[1][1] = -goalwidth / 2;
	goalpost_pos[1][2] = 0;
	
	goalpost_pos[2][0] = -goaldist;
	goalpost_pos[2][1] = -goalwidth / 2;
	goalpost_pos[2][2] = 0;
	
	goalpost_pos[3][0] = -goaldist;
	goalpost_pos[3][1] = goalwidth / 2;
	goalpost_pos[3][2] = 0;
	
	//Set up all player positions
	memset(person_table, 0, sizeof(person_table));
	for(int tt = 0; tt < 2; tt++)
	{
		int role = 0;
		int order = 0;
		int formation[5] = { 1, 3, 4, 3 };
		for(int pp = 0; pp < 11; pp++)
		{
			person_table[tt][pp].pos[0] = (40 - (role * 10)) * 100 * 256;
			person_table[tt][pp].pos[1] = ( (2 * order) - formation[role] ) * 5 * 100 * 256;
			
			if(tt == 0)
				person_table[tt][pp].pos[0] *= -1;
			
			order++;
			if(order >= formation[role])
			{
				order = 0;
				role++;
			}
		}
	}

	//Simulate until the match state-machine arrives at DONE...
	int last_sim = _sc_getticks();
	while(match_state != MS_DONE)
	{
		//Simulate at 100hz
		int ticknow = _sc_getticks();
		if( ticknow < (last_sim - 10) || ticknow > (last_sim + 1000) )
		{
			//Reset timing, something bad happened
			last_sim = ticknow - 1;
		}
		while(last_sim < ticknow)
		{
			match_tick();
			last_sim += 10;
		}
		
		//Draw the 3d elements of the world
		drawworld();
		
		//Draw UI overlays requested by state machine
		for(int uu = 0; uu < UI_MAX; uu++)
		{
			if(!match_uipart_visible[uu])
				continue;
			
			if(match_uipart_drawfns[uu] == NULL)
				continue;
			
			(*(match_uipart_drawfns[uu]))();
		}
		
		//Present the result
		fbs_flip();
	}
}
