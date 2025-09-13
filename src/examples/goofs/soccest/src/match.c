//match.c
//Soccer match
//Bryan E. Topp <betopp@betopp.com> 2025

#include <sc.h>
#include <stdbool.h>

#include "match.h"
#include "statfunc.h"
#include "trigfunc.h"
#include "teamdata.h"
#include "fbs.h"
#include "pads.h"
#include "font.h"
#include "images.h"
#include "proj.h"
#include "chanim.h"

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
/*
static const char *match_state_names[] = 
{
	[MS_NONE] = "NONE",
	[MS_INTRO] = "INTRO",
	[MS_PLAY] = "PLAY",
	[MS_GOAL] = "GOAL",
	[MS_DONE] = "DONE",
};
*/

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
	UI_SCORES,
	UI_PADINFO,
	UI_MAX
} match_uipart_t;
static bool match_uipart_visible[UI_MAX];

static void show(match_uipart_t pp)
{
	match_uipart_visible[pp] = true;
}

//static void hide(match_uipart_t pp)
//{
//	match_uipart_visible[pp] = false;
//}

static void hideall(void)
{
	memset(match_uipart_visible, 0, sizeof(match_uipart_visible));
}



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

//Team in possession of the ball
static int32_t ball_team;

//Player on the team in possession of the ball
static int32_t ball_person;

//How close to the ball to contest or maintain possession, integer cm
static const int32_t ball_stick_extent = 75;

//Location of goal
static const int goalwidth = 732 * 256; //Spacing between goal posts
static const int goalheight = 244 * 256; //Height of goal top beam
static const int goaldist = 39*100*256; //Distance from center of field to goal line

//Location of goalposts
static int32_t goalpost_pos[4][3];

//Information about a player on the field
typedef struct person_s
{
	int32_t pos[3]; //Virtual position 24.8 cm
	int32_t vel[3]; //Virtual velocity 24.8 cm/s
	int32_t dir; //Angle 0=east 16384=north 32768=west 49152=south
	chanim_idx_t anim; //Frame of animation displayed
	int animticks; //How many ticks has the frame been displayed
	int sticky; //Ball possession score, higher = holds onto ball better, negative = loses ball alwayss
	persondata_t *data; //Reference to stats about the person from team data
} person_t;
person_t person_table[2][11];

//Teams participating
teamdata_t *teamdata[2];

//Which team each gamepad belongs to
int pad_team[4];

//Which person on the team each gamepad is controlling
int pad_person[4];

//Team scores
static int scores[2];

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

//Draws UI element - scores
static void match_uipart_draw_scores(void)
{
	for(int tt = 0; tt < 2; tt++)
	{
		for(int dd = 0; dd < 2; dd++)
		{
			int imf = IMF_MATCH_SCORE0 + (scores[tt] / (dd?1:10));
			int xp = 320 + (tt?-1:1)*((dd^tt)?64:36);
			int yp = 432;
			images_draw(imf, xp, yp);
		}
	}
	images_draw(IMF_MATCH_SCOREHOME, 312 - 50, 464);
	images_draw(IMF_MATCH_SCOREAWAY, 312 + 50, 464);
}

//Draws UI element - gamepad/player info
static void match_uipart_draw_padinfo(void)
{
	for(int pp = 0; pp < 4; pp++)
	{
		if(pad_team[pp] == -1)
			continue; //Pad not playing
		
		const person_t *pptr = &(person_table[pad_team[pp]][pad_person[pp]]);
		
		int basex = pp * 160;
		int basey = 0;
		
		font_draw(&("A:\0B:\0C:\0D:\0"[3*pp]), 0x4E46, basex+16, basey);
		font_draw(pptr->data->name[1], 0x4E46, basex+48, basey);
		
	}
}

//Table of draw functions for each UI element
typedef void (*match_uipart_drawfn_t)(void);
match_uipart_drawfn_t match_uipart_drawfns[UI_MAX] =
{
	[UI_GOAL] = &match_uipart_draw_goal,
	[UI_READY] = &match_uipart_draw_ready,
	[UI_SCORES] = &match_uipart_draw_scores,
	[UI_PADINFO] = &match_uipart_draw_padinfo,
};


//Draws all the "3d view" elements at the current camera position.
void drawworld(void)
{
	//Set up projection for projected drawing calls
	proj_cam(cam_center[0], cam_center[1], 0, cam_radius);
	
	//Draw texture-mapped floor
	drawpitch();
	
	//Project cones to test scale of projection
	//proj_card(IMF_CARD_CONE, 0, 0, 0, 100*256);
	//proj_card(IMF_CARD_CONE, 800*256, 0, 0, 100*256);
	//proj_card(IMF_CARD_CONE, -800*256, 0, 0, 100*256);
	//proj_card(IMF_CARD_CONE, 0, 800*256, 0, 100*256);
	//proj_card(IMF_CARD_CONE, 0, -800*256, 0, 100*256);
	
	//Project ball shadow + ball
	proj_card(IMF_CARD_BALLSH, ball_pos[0], ball_pos[1], -256*5, 16*256);
	proj_card(IMF_CARD_BALL0 + ((ball_frame >> 8) & 0x3u), ball_pos[0], ball_pos[1], ball_pos[2], 24*256);
	
	//Project goalposts
	for(int pp = 0; pp < 4; pp++)
	{
		proj_card(IMF_CARD_GOALPOST, 
			goalpost_pos[pp][0], goalpost_pos[pp][1], goalpost_pos[pp][2], 
			goalheight);
		
		int bump = 100 * ((goalpost_pos[pp][0] > 0) ? 256 : -256);
		
		proj_card(IMF_CARD_GOALSIDE,
			goalpost_pos[pp][0] + bump, goalpost_pos[pp][1], goalpost_pos[pp][2], 
			goalheight);
	}
	
	//Project goal crossbeams
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
			proj_card(IMF_CARD_GOALTOP, pos[0], pos[1], pos[2], 32*256);
			pos[0] += step[0];
			pos[1] += step[1];
			pos[2] += step[2];
		}
	}	
	
	//Project players
	for(int tt = 0; tt < 2; tt++)
	{
		for(int pp = 0; pp < 11; pp++)
		{
			const person_t *pptr = &(person_table[tt][pp]);
			chanim(pptr->anim, pptr->dir, pptr->pos[0], pptr->pos[1], pptr->pos[2]);
			
			for(int pad = 0; pad < 4; pad++)
			{
				if(pad_team[pad] == tt && pad_person[pad] == pp)
				{
					proj_card(IMF_CARD_SELECTION, pptr->pos[0], pptr->pos[1], pptr->pos[2]-(32*256), 96*256);
				}
			}
		}
	}
	
	//Draw all projected cards
	proj_draw();
	
	
	//Draw debug text on top
	//char txtbuf[256];
	//snprintf(txtbuf, sizeof(txtbuf)-1, "%8d %8d", cam_radius,cam_center[1]);
	//font_draw(txtbuf, 0x8000,0,0);
	
	//snprintf(txtbuf, sizeof(txtbuf)-1, "%s: %d", match_state_names[match_state], match_state_ticks);
	//font_draw(txtbuf, 0x8000, 16, 0);
	
	//snprintf(txtbuf, sizeof(txtbuf)-1, "%d %d %d", ball_team, ball_person, person_table[0][0].sticky);
	//font_draw(txtbuf, 0x8000, 16, 0);
}

//Resets all persons to formation positions instantly
static void goformation(void)
{
	for(int tt = 0; tt < 2; tt++)
	{
		int role = 0;
		int order = 0;
		int formation[5] = { 1, 3, 4, 3 };
		for(int pp = 0; pp < 11; pp++)
		{
			person_table[tt][pp].pos[0] = (40 - (role * 10)) * 100 * 256;
			person_table[tt][pp].pos[1] = ( (2 * order) - formation[role] ) * 5 * 100 * 256;
			person_table[tt][pp].pos[2] = 0;
			person_table[tt][pp].vel[0] = 0;
			person_table[tt][pp].vel[1] = 0;
			person_table[tt][pp].vel[2] = 0;
			person_table[tt][pp].dir = 32768;
			person_table[tt][pp].anim = AN_STAND;
			person_table[tt][pp].animticks = 0;
			
			if(tt == 0)
			{
				person_table[tt][pp].pos[0] *= -1;
				person_table[tt][pp].dir = 32768 - person_table[tt][pp].dir;
			}
			
			order++;
			if(order >= formation[role])
			{
				order = 0;
				role++;
			}
		}
	}
	
	//Temphack
	person_table[0][0].pos[0] = 0;
	person_table[0][0].pos[1] = 0;
	
}

//Triggers a goal
static void goal(int team)
{
	scores[team]++;
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
	ball_frame += ((ball_vel[0] < 0)?-1:1) * ball_vel[0] / 1000;
	ball_frame += ((ball_vel[1] < 0)?-1:1) * ball_vel[1] / 1000;
	
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
	
	//Update player stickiness scores ( = who is best at possessing the ball)
	for(int tt = 0; tt < 2; tt++)
	{
		for(int pp = 0; pp < 11; pp++)
		{
			//Decay old stickyness value
			person_table[tt][pp].sticky *= 127;
			person_table[tt][pp].sticky /= 128;
			
			//Compute distance to ball, integer only
			int32_t dx = (person_table[tt][pp].pos[0] - ball_pos[0])/256;
			int32_t dy = (person_table[tt][pp].pos[1] - ball_pos[1])/256;
			int32_t dz = (person_table[tt][pp].pos[2] - ball_pos[2])/256;
			int32_t distsq = (dx*dx)+(dy*dy)+(dz*dz);
			
			//Closer to ball -> more stickyness point
			if(distsq < 65536)
			{
				person_table[tt][pp].sticky += (65536 - distsq) / 4096;
			}
			
			
		}
	}
	
	//Check if the ball changes possession
	int best_team = -1;
	int best_person = -1;
	int best_stick = 0;
	for(int tt = 0; tt < 2; tt++)
	{
		for(int pp = 0; pp < 11; pp++)
		{
			//Compute integer-only distance to ball, check if they're close enough
			int32_t dx = (person_table[tt][pp].pos[0] - ball_pos[0])/256;
			int32_t dy = (person_table[tt][pp].pos[1] - ball_pos[1])/256;
			int32_t dz = (person_table[tt][pp].pos[2] - ball_pos[2])/256;
			int32_t distsq = (dx*dx)+(dy*dy)+(dz*dz);
			if(distsq > (ball_stick_extent * ball_stick_extent))
			{
				//Person is too far from the ball to possess it
				continue;
			}
			
			if(person_table[tt][pp].sticky < best_stick)
			{
				//Someone else is already holding it harder
				continue;
			}
			
			//This person is a candidate to possess the ball...
			best_stick = person_table[tt][pp].sticky;
			best_team = tt;
			best_person = pp;
		}
	}
	
	//See if the most-sticky player is the same as it was last time
	if(best_team != ball_team || best_person != ball_person)
	{
		//Ball changes possession.
		
		//Someone who has the ball taken gets most of their stickyness taken away (hysteresis)
		if(ball_team != -1 && ball_person != -1)
		{
			person_table[ball_team][ball_person].sticky -= 50;
		}
		
		//Someone gaining possession gets a bonus (hysteresis)
		if(best_team != -1 && best_person != -1)
		{
			person_table[best_team][best_person].sticky += 50;
		}
		
		//Ball has changed possession
		ball_team = best_team;
		ball_person = best_person;
	}
	
	//Move towards player in possession of ball
	if(ball_team != -1 && ball_person != -1)
	{
		person_t *holder = &(person_table[ball_team][ball_person]);
		
		int32_t target_pos[3] = { holder->pos[0], holder->pos[1], holder->pos[2] };
		target_pos[0] += 50 * trigfunc_cos8(holder->dir);
		target_pos[1] += 50 * trigfunc_sin8(holder->dir);
		target_pos[0] += holder->vel[0] / 16;
		target_pos[1] += holder->vel[1] / 16;
		
		int32_t dp[3] =
		{
			target_pos[0] - ball_pos[0],
			target_pos[1] - ball_pos[1],
			target_pos[2] - ball_pos[2]
		};
		
		//intentionally xy only
		for(int dd = 0; dd < 2; dd++)
		{
			ball_vel[dd] *= 7;
			ball_vel[dd] /= 8;
			ball_vel[dd] += dp[dd];
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
	if(cam_radius > 800)
		cam_radius--;
	
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
	
	//Adjust velocity based on direction input
	//Quake-style wishvel system
	int32_t wishvel[2] = {0, 0};
	int plspeed = 200;
	switch(pads[pad] & (BTNBIT_UP | BTNBIT_DOWN | BTNBIT_LEFT | BTNBIT_RIGHT))
	{
		case BTNBIT_RIGHT:               wishvel[0] = plspeed *  256; wishvel[1] = plspeed *    0; break;
		case BTNBIT_RIGHT | BTNBIT_UP:   wishvel[0] = plspeed *  181; wishvel[1] = plspeed *  181; break;
		case BTNBIT_UP:                  wishvel[0] = plspeed *    0; wishvel[1] = plspeed *  256; break;
		case BTNBIT_UP | BTNBIT_LEFT:    wishvel[0] = plspeed * -181; wishvel[1] = plspeed *  181; break;
		case BTNBIT_LEFT:                wishvel[0] = plspeed * -256; wishvel[1] = plspeed *    0; break;
		case BTNBIT_LEFT | BTNBIT_DOWN:  wishvel[0] = plspeed * -181; wishvel[1] = plspeed * -181; break;
		case BTNBIT_DOWN:                wishvel[0] = plspeed *    0; wishvel[1] = plspeed * -256; break;
		case BTNBIT_DOWN | BTNBIT_RIGHT: wishvel[0] = plspeed *  181; wishvel[1] = plspeed * -181; break;
		default: break;
	}
	int64_t wishvel_magsq = (wishvel[0]*wishvel[0])+(wishvel[1]*wishvel[1]);
	
	pptr->vel[0] *= 7;
	pptr->vel[0] += wishvel[0];
	pptr->vel[0] /= 8;
	pptr->vel[1] *= 7;
	pptr->vel[1] += wishvel[1];
	pptr->vel[1] /= 8;
	//int64_t vel_magsq = (pptr->vel[0] * pptr->vel[0]) + (pptr->vel[1] * pptr->vel[1]);
	
	
	//Adjust facing based on direction input
	int wantangle = pptr->dir;
	switch(pads[pad] & (BTNBIT_UP | BTNBIT_DOWN | BTNBIT_LEFT | BTNBIT_RIGHT))
	{
		case BTNBIT_RIGHT:               wantangle = 8192 * 0; break;
		case BTNBIT_RIGHT | BTNBIT_UP:   wantangle = 8192 * 1; break;
		case BTNBIT_UP:                  wantangle = 8192 * 2; break;
		case BTNBIT_UP | BTNBIT_LEFT:    wantangle = 8192 * 3; break;
		case BTNBIT_LEFT:                wantangle = 8192 * 4; break;
		case BTNBIT_LEFT | BTNBIT_DOWN:  wantangle = 8192 * 5; break;
		case BTNBIT_DOWN:                wantangle = 8192 * 6; break;
		case BTNBIT_DOWN | BTNBIT_RIGHT: wantangle = 8192 * 7; break;
		default: break;
	}
	
	int angdiff = ((wantangle - pptr->dir) + 65536) % 65536;
	if(angdiff > 32768)
		angdiff = angdiff - 65536;

	pptr->dir += angdiff / 16;
	
	//Cap angle
	if(pptr->dir < 0)
		pptr->dir += 65536;
	if(pptr->dir > 65536)
		pptr->dir -= 65536;
	
	//Update animation
	pptr->animticks++;
	
	if(pptr->anim >= AN_RUN0 && pptr->anim <= AN_RUN3)
	{
		//In the run cycle, continue based on speed
		if( pptr->animticks > 10)
		{
			pptr->animticks = 0;
			pptr->anim++;
			if(pptr->anim > AN_RUN3)
				pptr->anim = AN_RUN0;
		}
		
		//Drop back to standing if too slow
		if(wishvel_magsq < 100)
		{
			pptr->anim = AN_STAND;
		}
		
	}
	
	if(pptr->anim == AN_STAND)
	{
		//Standing - start to run?
		if(wishvel_magsq > 100)
		{
			pptr->anim = AN_RUN0;
		}
	}
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
			
			
		}
	}
	
	
}

//Match tick function - introduction sequence
void match_tick_intro(void)
{
	hideall();
	if( (match_state_ticks / 32) & 1 )
		show(UI_READY);
	
	sim_cam_toball();
	
	if(match_state_ticks >= 32*6)
	{
		hideall();
		state(MS_PLAY);
	}
}

//Match tick function - playing
void match_tick_play(void)
{
	hideall();
	show(UI_SCORES);
	show(UI_PADINFO);
	
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
	show(UI_SCORES);
	
	if(match_state_ticks > 200)
	{
		goformation();
		
		memset(ball_pos, 0, sizeof(ball_pos));
		memset(ball_vel, 0, sizeof(ball_vel));
		memset(cam_center, 0, sizeof(cam_center));
		memset(cam_vel, 0, sizeof(cam_vel));
		
		state(MS_INTRO);
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

void match(teamdata_t *tdata[2], int padmap[4])
{
	//Load data for match
	images_purge();
	images_loadrange(IMF_CARD_AAA, IMF_CARD_ZZZ);
	images_loadrange(IMF_MATCH_AAA, IMF_MATCH_ZZZ);
	pitchtex_load();
	
	//Set aside team and pad info
	pad_team[0] = padmap[0];
	pad_person[0] = (padmap[0] >= 0) ? 0 : -1;
	pad_team[1] = padmap[1];
	pad_person[1] = (padmap[1] >= 0) ? 1 : -1;
	pad_team[2] = padmap[2];
	pad_person[2] = (padmap[2] >= 0) ? 2 : -1;
	pad_team[3] = padmap[3];
	pad_person[3] = (padmap[3] >= 0) ? 3 : -1;
	
	teamdata[0] = tdata[0];
	teamdata[1] = tdata[1];
	
	memset(person_table, 0, sizeof(person_table));
	for(int tt = 0; tt < 2; tt++)
	{
		for(int pp = 0; pp < 11; pp++)
		{
			person_table[tt][pp].data = &(teamdata[tt]->persons[pp]);
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
	goformation();

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
