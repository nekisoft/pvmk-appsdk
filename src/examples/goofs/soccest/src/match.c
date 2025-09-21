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
#include "behave.h"

#include "stb_image.h"
#include <string.h>

//State that the game flow can be in
typedef enum match_state_e
{
	MS_NONE = 0,
	MS_INTRO,
	MS_PLAY,
	MS_GOAL,
	MS_OOB,
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

//Information about ball going out of bounds
static int32_t oob_fault_team; //Last team to touch it
static int32_t oob_xdir; //Which goal line it went off of (-1, 1) or 0 if it didn't
static int32_t oob_ydir; //Which touch line it went off of (-1, 1) or 0 if it didn't

//How close to the ball to contest or maintain possession, integer cm
static const int32_t ball_stick_extent = 75;

//Location of goal
static const int goalwidth = 732 * 256; //Spacing between goal posts
static const int goalheight = 244 * 256; //Height of goal top beam
static const int goaldist = 39*100*256; //Distance from center of field to goal line
static const int goalback = goaldist + (100*256); //Distance from center of field to back of goal net

//Location of goalposts
static int32_t goalpost_pos[4][3];

//Out-of-bounds size (+/- these dimensions, 24.8 cm)
static int32_t bounds_extent[2] = { 39 * 100 * 256, 34 * 100 * 256 };

//Information about a player on the field
typedef struct person_s
{
	int32_t pos[3]; //Virtual position 24.8 cm
	int32_t vel[3]; //Virtual velocity 24.8 cm/s
	int32_t dir; //Angle 0=east 16384=north 32768=west 49152=south
	
	int32_t dest[3]; //Where they're trying to go
	int sprinting; //Whether they are trying to sprint or not
	
	int32_t target[3]; //What they're trying to kick at
	int trykick; //Whether they're trying to kick the ball
	int kickdelay; //How long before we can kick again
	
	chanim_idx_t anim; //Frame of animation displayed
	int animticks; //How many ticks has the frame been displayed
	
	int sticky; //Ball possession score, higher = holds onto ball better, negative = loses ball alwayss
	int stamina; //Current stamina value, 16.16 (stats are integer-only!)
	
	behave_state_t behave_state; //State of NPC behavior
	int32_t formpos[2]; //Position when in formation
	behave_role_t role; //Role to play when simulating NPC behavior
	
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
			for(int xx = 0; xx < 640; xx++)
			{
				fbrow[xx] = 0; //pitchtex[yy%32][xx];
			}
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
				fbrow[xx] = 0; //texrow[xx%32];
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
	
	if(match_state_ticks < 52)
		basex = 640 - match_state_ticks*10;
	else if(match_state_ticks < 100)
		basex = 120;
	else
		basex = 120 - (match_state_ticks - 100)*10;
	
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
		
		
		images_draw(IMF_MATCH_PADA + pp, basex, basey);
		
		font_draw(FS_SMALL, pptr->data->name[1], 0x4E46, basex+52, basey+4);
		
		
		int stam_cur = pptr->stamina / 65536;
		int stam_max = pptr->data->staminamax;
		if(stam_max <= 0)
			stam_max = 1;
		if(stam_cur >= stam_max)
			stam_cur = stam_max;
		if(stam_cur <= 0)
			stam_cur = 0;
		
		int stam_start = basex + 87; 
		int stam_end = stam_start + (64 * stam_cur / stam_max);
		for(int yy = basey + 25; yy < basey + 27; yy++)
		{
			for(int xx = stam_start; xx < stam_end; xx++)
			{
				BACKBUF[yy][xx] = 0xF000;
			}
		}
		
		//char stambuf[4] = {0};
		//snprintf(stambuf, sizeof(stambuf)-1, "%d", stam_cur);
		//font_draw(FS_SMALL, stambuf, 0xF000, basex + 100, basey + 20);
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
	
	for(int ll = 0; ll < 22; ll++)
	{
		char txtbuf[256];
		person_t *pptr = &(person_table[ll/11][ll%11]);
		snprintf(txtbuf, sizeof(txtbuf)-1, "%s", behave_dbgstr(&pptr->behave_state));
		font_draw(FS_SMALL, txtbuf, 0x8000, 16, 16 + (ll * 16));
	}
		
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
			person_table[tt][pp].pos[1] = ( (2 * order) + 1 - formation[role] ) * 5 * 100 * 256;
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
			
			//temp
			person_table[tt][pp].formpos[0] = person_table[tt][pp].pos[0];
			person_table[tt][pp].formpos[1] = person_table[tt][pp].pos[1];
			person_table[tt][pp].role = pp + 1;
			
			//reset behavior
			memset(&(person_table[tt][pp].behave_state), 0, sizeof(behave_state_t));
			
			order++;
			if(order >= formation[role])
			{
				order = 0;
				role++;
			}
		}
	}
}

//Triggers a goal
static void goal(int team)
{
	scores[team]++;
	state(MS_GOAL);
}

//Triggers an out-of-bounds
static void oob(int xdir, int ydir)
{
	oob_xdir = xdir;
	oob_ydir = ydir;
	state(MS_OOB);
}

//Sends the ball towards a given virtual point
void kick(const int32_t *target, uint8_t power, uint8_t accuracy)
{
	//Compute ball-to-target vector
	int32_t shotvec[3] = 
	{
		target[0] - ball_pos[0],
		target[1] - ball_pos[1],
		target[2] - ball_pos[2],
	};
	
	//Express ball-to-target as distance, heading, and elevation
	int shotvec_whole[3] = 
	{
		shotvec[0] / 256,
		shotvec[1] / 256,
		shotvec[2] / 256,
	};
	int distsq = 0;
	distsq += shotvec_whole[0] * shotvec_whole[0];
	distsq += shotvec_whole[1] * shotvec_whole[1];
	distsq += shotvec_whole[2] * shotvec_whole[2];
	
	int heading = trigfunc_atan2(shotvec[1], shotvec[0]);
	int elevation = trigfunc_atan2(distsq, (goaldist/256)*(goaldist/256)); //not accurate but cool
	
	//Roll for inaccuracy
	int flub = statfunc_gauss_8b(128, 256 / (accuracy+1)) - 128;
	heading += flub * 64;
	

	//Apply force
	ball_vel[0] = trigfunc_cos8(heading) * power * 16;
	ball_vel[1] = trigfunc_sin8(heading) * power * 16;
	ball_vel[2] = trigfunc_sin8(elevation) * power * 16;
}

//Simulates out-of-bounds rules
void sim_bounds(void)
{
	//Check against each bound
	if     (ball_pos[0] >  bounds_extent[0]) oob( 1,  0);
	else if(ball_pos[0] < -bounds_extent[0]) oob(-1,  0);
	else if(ball_pos[1] >  bounds_extent[1]) oob( 0,  1);
	else if(ball_pos[1] < -bounds_extent[1]) oob( 0, -1);
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
	
	
	//Bounce off inside of goal
	if(ball_pos[1] >= -(goalwidth / 2) && ball_pos[1] <= (goalwidth / 2) && ball_pos[2] < goalheight)
	{
		if(ball_pos[0] < -goaldist || ball_pos[0] > goaldist)
		{
			int32_t nextpos[3] = 
			{
				ball_pos[0] + (ball_vel[0] / 99),
				ball_pos[1] + (ball_vel[1] / 99),
				ball_pos[2] + (ball_vel[2] / 99),
			};
			
			if(nextpos[0] < -goalback || nextpos[0] > goalback)
			{
				ball_vel[0] *= -1;
				ball_vel[0] /= 8;
			}
			if(nextpos[1] > (goalwidth/2) || nextpos[1] < -(goalwidth/2))
			{
				ball_vel[1] *= -1;
				ball_vel[1] /= 8;
			}
			if(nextpos[2] > goalheight)
			{
				ball_vel[2] *= -1;
				ball_vel[2] /= 8;
			}
		}
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
			person_table[tt][pp].sticky *= 7;
			person_table[tt][pp].sticky /= 8;
			
			//Compute distance to ball, integer only
			int32_t dx = (person_table[tt][pp].pos[0] - ball_pos[0])/256;
			int32_t dy = (person_table[tt][pp].pos[1] - ball_pos[1])/256;
			int32_t dz = (person_table[tt][pp].pos[2] - ball_pos[2])/256;
			int32_t distsq = (dx*dx)+(dy*dy)+(dz*dz);
			
			//Closer to ball -> more stickyness point
			if(distsq < 1048576)
			{
				person_table[tt][pp].sticky += (1048576 - distsq) / 16384;
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
		
		if(ball_team != -1)
		{
			//Record the last team to possess the ball, even if it since left possession.
			//Blame that team for out-of-bounds if it happens.
			oob_fault_team = ball_team;
		}
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
	cam_center[0] *= 15;
	cam_center[0] += ball_pos[0];
	cam_center[0] /= 16;
	cam_center[1] *= 15;
	cam_center[1] += ball_pos[1];
	cam_center[1] /= 16;
	
	cam_radius *= 15;
	cam_radius += 600;
	cam_radius /= 16;
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

//Simulates normal game camera
void sim_cam_gameplay(void)
{
	//Find average location and maximum distance pairwise of:
	//1. all gamepad-controlled persons
	//2. the ball
	//3. (maybe other stuff? the goal if the ball gets close?)
	int32_t dsq_max = 0;
	int32_t avg[3] = {0,0,0};
	int avgn = 0;
	for(int pp = 0; pp < 4; pp++)
	{
		if(pad_team[pp] == -1)
			continue;
		
		for(int qq = 0; qq < 5; qq++)
		{
			if(qq < 4 && pad_team[qq] == -1)
				continue;
			
			int32_t *posa = person_table[pad_team[pp]][pad_person[pp]].pos;
			int32_t *posb = (qq==4)?(ball_pos):(person_table[pad_team[qq]][pad_person[qq]].pos);
			
			int32_t dx = (posa[0] - posb[0]) / 256;
			int32_t dy = (posa[1] - posb[1]) / 256;
			int32_t dsq = (dx*dx)+(dy*dy);
			
			if(dsq > dsq_max)
				dsq_max = dsq;
			
			avg[0] += posb[0];
			avg[1] += posb[1];
			avg[2] += posb[2];
			avgn++;
		}
	}
	if(avgn == 0)
	{
		//no human players
		//let pad a control a free cam
		cam_radius -= (pads[PAD_A] & BTNBIT_A) ? 10 : 0;
		cam_radius += (pads[PAD_A] & BTNBIT_B) ? 10 : 0;
		
		cam_center[0] += (pads[PAD_A] & BTNBIT_RIGHT) ? (cam_radius) : 0;
		cam_center[0] -= (pads[PAD_A] & BTNBIT_LEFT)  ? (cam_radius) : 0;
		cam_center[1] += (pads[PAD_A] & BTNBIT_UP)    ? (cam_radius) : 0;
		cam_center[1] -= (pads[PAD_A] & BTNBIT_DOWN)  ? (cam_radius) : 0;
		return;
	}
	
	avg[0] /= avgn;
	avg[1] /= avgn;
	avg[2] /= avgn;
	
	//Cheap hack so we don't need sqrt...
	int target_radius = dsq_max / cam_radius;
	cam_radius *= 31;
	cam_radius += target_radius;
	cam_radius /= 32;
	if(cam_radius < 800)
		cam_radius = 800;
	
	//exponential smooth position too
	cam_center[0] *= 31;
	cam_center[0] += avg[0];
	cam_center[0] /= 32;
	
	cam_center[1] *= 31;
	cam_center[1] += avg[1];
	cam_center[1] /= 32;
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
	
	//Set destination for movement, based on direction buttons
	int32_t wishvel[2] = {0, 0};
	switch(pads[pad] & (BTNBIT_UP | BTNBIT_DOWN | BTNBIT_LEFT | BTNBIT_RIGHT))
	{
		case BTNBIT_RIGHT:               wishvel[0] = 100 *  256; wishvel[1] = 100 *    0; break;
		case BTNBIT_RIGHT | BTNBIT_UP:   wishvel[0] = 100 *  181; wishvel[1] = 100 *  181; break;
		case BTNBIT_UP:                  wishvel[0] = 100 *    0; wishvel[1] = 100 *  256; break;
		case BTNBIT_UP | BTNBIT_LEFT:    wishvel[0] = 100 * -181; wishvel[1] = 100 *  181; break;
		case BTNBIT_LEFT:                wishvel[0] = 100 * -256; wishvel[1] = 100 *    0; break;
		case BTNBIT_LEFT | BTNBIT_DOWN:  wishvel[0] = 100 * -181; wishvel[1] = 100 * -181; break;
		case BTNBIT_DOWN:                wishvel[0] = 100 *    0; wishvel[1] = 100 * -256; break;
		case BTNBIT_DOWN | BTNBIT_RIGHT: wishvel[0] = 100 *  181; wishvel[1] = 100 * -181; break;
		default: break;
	}
	pptr->dest[0] = pptr->pos[0] + wishvel[0];
	pptr->dest[1] = pptr->pos[1] + wishvel[1];
	pptr->dest[2] = pptr->pos[2];
	
	//Set sprinting flag from B button
	pptr->sprinting = (pads[pad] & BTNBIT_B);
	
	//Action buttons depend a lot on whether this person has the ball
	if(ball_team == team && ball_person == person)
	{
		//They have the ball.
		
		//X, Y, Z used for strategic moves - a particular kick
		
		//X button takes a shot on the goal.
		if(pads_detect(pad, BTNBIT_X))
		{
			pptr->target[0] = team?-goaldist:goaldist;
			pptr->target[1] = 0;
			pptr->target[2] = 0;
			pptr->trykick = true;
		}
		
		//Y/Z passes to the next teammate ahead/behind us.
		if(pads_detect(pad, (BTNBIT_Y | BTNBIT_Z)))
		{
			//Find the closest teammate in the appropriate direction
			int want_dir = ((pads[pad] & BTNBIT_Y)?1:-1) * (team?-1:1);
			int best_teammate = -1;
			int best_distsq = 1024*1024*1024;
			for(int pp = 0; pp < 11; pp++)
			{
				if(pp == person)
					continue; //Don't pass to ourselves
				
				person_t *tptr = &(person_table[team][pp]);
				
				int xdiff = tptr->pos[0] - pptr->pos[0];
				if(xdiff * want_dir <= 0)
					continue; //Teammate is in the wrong direction
				
				int dx = (tptr->pos[0] - pptr->pos[0]) / 256;
				int dy = (tptr->pos[1] - pptr->pos[1]) / 256;
				int dz = (tptr->pos[2] - pptr->pos[2]) / 256;
				int distsq = (dx*dx)+(dy*dy)+(dz*dz);
				if(distsq < best_distsq)
				{
					best_teammate = pp;
					best_distsq = distsq;
				}
			}
			
			//If we found anyone there, pass to them
			if(best_teammate != -1)
			{
				//Kick the ball
				pptr->target[0] = person_table[team][best_teammate].pos[0];
				pptr->target[1] = person_table[team][best_teammate].pos[1];
				pptr->target[2] = person_table[team][best_teammate].pos[2];
				pptr->trykick = true;
				
				//If the teammate isn't currently controlled by a pad, control them
				int teammate_is_controlled = 0;
				for(int pp = 0; pp < 4; pp++)
				{
					if(pad_team[pp] == team && pad_person[pp] == best_teammate)
						teammate_is_controlled = 1;
				}
				if(!teammate_is_controlled)
					pad_person[pad] = best_teammate;
			}
		}
	}
	else
	{
		//Don't have the ball.
		
		//X changes player control to the player closest to the ball.
		if(pads_detect(pad, BTNBIT_X))
		{
			//Find the closest not-currently-controlled teammate to the ball.
			int best_teammate = -1;
			int best_distsq = 1024*1024*1024;
			for(int pp = 0; pp < 11; pp++)
			{
				if(pp == person)
					continue; //Don't switch to ourselves
				
				person_t *tptr = &(person_table[team][pp]);
				
				int teammate_is_controlled = 0;
				for(int qq = 0; qq < 4; qq++)
				{
					if(pad_team[qq] == team && pad_person[qq] == pp)
						teammate_is_controlled = 1;
				}
				if(teammate_is_controlled)
					continue; //Someone already controlling this teammate
				
				int dx = (tptr->pos[0] - ball_pos[0]) / 256;
				int dy = (tptr->pos[1] - ball_pos[1]) / 256;
				int dz = (tptr->pos[2] - ball_pos[2]) / 256;
				int distsq = (dx*dx)+(dy*dy)+(dz*dz);
				if(distsq < best_distsq)
				{
					best_teammate = pp;
					best_distsq = distsq;
				}
			}
			
			//If we found anyone there, change to them
			if(best_teammate != -1)
			{
				pad_person[pad] = best_teammate;
			}
		}
		
		//Y and Z change player control to the next player forward/backward.
		if(pads_detect(pad, (BTNBIT_Y | BTNBIT_Z)))
		{
			//Find the closest teammate in the appropriate direction
			int want_dir = ((pads[pad] & BTNBIT_Y)?1:-1) * (team?-1:1);
			int best_teammate = -1;
			int best_distsq = 1024*1024*1024;
			for(int pp = 0; pp < 11; pp++)
			{
				if(pp == person)
					continue; //Don't switch to ourselves
				
				person_t *tptr = &(person_table[team][pp]);
				
				int xdiff = tptr->pos[0] - pptr->pos[0];
				if(xdiff * want_dir <= 0)
					continue; //Teammate is in the wrong direction
				
				int teammate_is_controlled = 0;
				for(int qq = 0; qq < 4; qq++)
				{
					if(pad_team[qq] == team && pad_person[qq] == pp)
						teammate_is_controlled = 1;
				}
				if(teammate_is_controlled)
					continue; //Someone already controlling this teammate
				
				int dx = (tptr->pos[0] - pptr->pos[0]) / 256;
				int dy = (tptr->pos[1] - pptr->pos[1]) / 256;
				int dz = (tptr->pos[2] - pptr->pos[2]) / 256;
				int distsq = (dx*dx)+(dy*dy)+(dz*dz);
				if(distsq < best_distsq)
				{
					best_teammate = pp;
					best_distsq = distsq;
				}
			}
			
			//If we found anyone there, change to them
			if(best_teammate != -1)
			{
				pad_person[pad] = best_teammate;
			}
		}
	}
	
	//A button just kicks whatever's here
	if(pads_detect(pad, BTNBIT_A))
	{
		pptr->trykick = true;
		pptr->target[0] = pptr->pos[0] + trigfunc_cos8(pptr->dir) * 100;
		pptr->target[1] = pptr->pos[1] + trigfunc_sin8(pptr->dir) * 100;
		pptr->target[2] = pptr->pos[2];
	}
}

//Simulates a person using CPU behaviour
void sim_person_cpu(int team, int person)
{
	//If we get the ball, try to give control over to a human on the same team
	if(ball_team == team && ball_person == person)
	{
		//Find the gamepad currently controlling the nearest person of the same team
		int best_pad = -1;
		int32_t best_distsq = 1024*1024*1024;
		for(int pp = 0; pp < 4; pp++)
		{
			if(pad_team[pp] != team)
				continue;
			
			int32_t *controlled_pos = person_table[pad_team[pp]][pad_person[pp]].pos;
			int32_t *our_pos = person_table[team][person].pos;
			
			int32_t dx = (controlled_pos[0] - our_pos[0]) / 256;
			int32_t dy = (controlled_pos[1] - our_pos[1]) / 256;
			int32_t distsq = (dx*dx)+(dy*dy);
			if(distsq < best_distsq)
			{
				best_pad = pp;
				best_distsq = distsq;
			}
		}
		
		if(best_pad != -1)
		{
			//A human with a gamepad is controlling a nearby person, make them control us instead
			pad_person[best_pad] = person;
			return;
		}
	}
	
	//Could also be that there's no humans on our team! In which case, the CPU plays the ball.
	person_t *pptr = &(person_table[team][person]);
	
	//Prepare inputs for behavior simulation
	behave_input_t inputs = 
	{
		.role = pptr->role,
		.have_ball = (ball_team == team && ball_person == person),
		.team_ball = (ball_team == team),
		.attack_dir = (team ? -1 : 1),
		.self_pos = { pptr->pos[0], pptr->pos[1] },
		.ball_pos = { ball_pos[0], ball_pos[1] },
		.form_pos = { pptr->formpos[0], pptr->formpos[1] },
		.team_pos = {
			{ person_table[team][ 0].pos[0], person_table[team][ 0].pos[1] },
			{ person_table[team][ 1].pos[0], person_table[team][ 1].pos[1] },
			{ person_table[team][ 2].pos[0], person_table[team][ 2].pos[1] },
			{ person_table[team][ 3].pos[0], person_table[team][ 3].pos[1] },
			{ person_table[team][ 4].pos[0], person_table[team][ 4].pos[1] },
			{ person_table[team][ 5].pos[0], person_table[team][ 5].pos[1] },
			{ person_table[team][ 6].pos[0], person_table[team][ 6].pos[1] },
			{ person_table[team][ 7].pos[0], person_table[team][ 7].pos[1] },
			{ person_table[team][ 8].pos[0], person_table[team][ 8].pos[1] },
			{ person_table[team][ 9].pos[0], person_table[team][ 9].pos[1] },
			{ person_table[team][10].pos[0], person_table[team][10].pos[1] },
		},
		.oppo_pos =  {
			{ person_table[!team][ 0].pos[0], person_table[!team][ 0].pos[1] },
			{ person_table[!team][ 1].pos[0], person_table[!team][ 1].pos[1] },
			{ person_table[!team][ 2].pos[0], person_table[!team][ 2].pos[1] },
			{ person_table[!team][ 3].pos[0], person_table[!team][ 3].pos[1] },
			{ person_table[!team][ 4].pos[0], person_table[!team][ 4].pos[1] },
			{ person_table[!team][ 5].pos[0], person_table[!team][ 5].pos[1] },
			{ person_table[!team][ 6].pos[0], person_table[!team][ 6].pos[1] },
			{ person_table[!team][ 7].pos[0], person_table[!team][ 7].pos[1] },
			{ person_table[!team][ 8].pos[0], person_table[!team][ 8].pos[1] },
			{ person_table[!team][ 9].pos[0], person_table[!team][ 9].pos[1] },
			{ person_table[!team][10].pos[0], person_table[!team][10].pos[1] },
		},
	};
	
	//Todo - mangle inputs based on character stats / game difficulty

	//Run behavior to find what they do
	behave_output_t outputs = {0};
	behave_sim(&inputs, &pptr->behave_state, &outputs);

	//Perform the action as if a controller set it up (see person common)
	pptr->dest[0] = outputs.dest[0];
	pptr->dest[1] = outputs.dest[1];
	pptr->sprinting = outputs.sprint;
	if(outputs.kick)
	{
		//Can kick if we're close enough, or possess the ball
		pptr->trykick = true;
		pptr->target[0] = outputs.target[0];
		pptr->target[1] = outputs.target[1];
		pptr->target[2] = 0;
	}
}


//Performs common simulation for player- and cpu-controlled persons
void sim_person_common(int team, int person)
{
	person_t *pptr = &(person_table[team][person]);
	
	//Fudge factor to make things faster
	int speedfudge = 2;
	
	//Compute direction and distance to movement target
	int wantangle = trigfunc_atan2(pptr->dest[1] - pptr->pos[1], pptr->dest[0] - pptr->pos[0]);
	int destdx = (pptr->dest[0] - pptr->pos[0]) / 256;
	int destdy = (pptr->dest[1] - pptr->pos[1]) / 256;
	int destdsq = (destdx*destdx)+(destdy*destdy);
	if(destdsq > 10)
	{
		//Have somewhere to go
	
		//Add velocity towards movement destination
		int plspeed = pptr->data->runspeed;
		if(ball_team == team && ball_person == person)
			plspeed = pptr->data->ballspeed;
		if(pptr->sprinting && (pptr->stamina > 0) )
			plspeed = pptr->data->sprintspeed;
		
		pptr->vel[0] *= 7;
		pptr->vel[0] += speedfudge * plspeed * trigfunc_cos8(wantangle);
		pptr->vel[0] /= 8;
		pptr->vel[1] *= 7;
		pptr->vel[1] += speedfudge * plspeed * trigfunc_sin8(wantangle);
		pptr->vel[1] /= 8;
		
		//Adjust facing based on direction to movement target
		int angdiff = ((wantangle - pptr->dir) + 65536) % 65536;
		if(angdiff > 32768)
			angdiff = angdiff - 65536;
		
		pptr->dir += angdiff / 16;
	}
	else
	{
		//Nowhere to go
		//Decay velocity
		pptr->vel[0] *= 7;
		pptr->vel[0] /= 8;
		pptr->vel[1] *= 7;
		pptr->vel[1] /= 8;
	}
	
	//Cap angle
	if(pptr->dir < 0)
		pptr->dir += 65536;
	if(pptr->dir > 65536)
		pptr->dir -= 65536;
	
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
	
	//Update stamina
	int vsq = ((pptr->vel[0]/256)*(pptr->vel[0]/256))+((pptr->vel[1]/256)*(pptr->vel[1]/256));
	if(vsq > speedfudge * speedfudge * pptr->data->sprintspeed * pptr->data->runspeed)
	{
		//Sprinting, consume stamina
		pptr->stamina -= 8192;
	}
	else
	{
		//Not sprinting, allow stamina recover
		pptr->stamina += pptr->data->staminarecover * pptr->data->staminamax;
		if(pptr->stamina <= 0)
		{
			pptr->stamina = 0;
		}	
		if(pptr->stamina >= pptr->data->staminamax * 65536)
		{
			pptr->stamina = pptr->data->staminamax * 65536;
		}
	}
	
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
		if(destdsq < 100)
		{
			pptr->anim = AN_STAND;
		}
		
	}
	
	if(pptr->anim == AN_STAND)
	{
		//Standing - start to run?
		if(destdsq > 100)
		{
			pptr->anim = AN_RUN0;
		}
	}
	
	//If they're trying to kick, see what happens
	if(pptr->kickdelay > 0)
		pptr->kickdelay--;
	
	if(pptr->trykick)
	{
		pptr->trykick = false;
		
		uint8_t accuracy = pptr->data->accuracy;
		uint8_t power = pptr->data->kickpower;
		bool inrange = (ball_person == person && ball_team == team);
		if(!inrange)
		{
			//Not possessing the balll...
			//Check if the ball is nearby us tho.
			int32_t dx = (ball_pos[0] - pptr->pos[0]) / 256;
			int32_t dy = (ball_pos[1] - pptr->pos[1]) / 256;
			
			int32_t maxd = 100; //cm
			int32_t distsq = (dx*dx)+(dy*dy);
			if(distsq < (maxd * maxd))
			{
				//Ball is close enough, smash that.
				//Halve our accuracy and power because we're not in possession.
				inrange = true;
				accuracy /= 2;
				power /= 2;
			}
		}
		
		if(inrange && (pptr->kickdelay <= 0))
		{
			kick(pptr->target, power, accuracy);
			pptr->kickdelay = 50;
			
			//The person kicking the ball gets a bonus to their possession,
			//anyone else gets less
			if(ball_person != -1 && ball_team != -1)
				person_table[ball_team][ball_person].sticky -= 100;
			
			person_table[team][person].sticky += 200;
		}
		
	}
	
	//Bump off others
	for(int tt = 0; tt < 2; tt++)
	{
		for(int pp = 0; pp < 11; pp++)
		{
			if(tt == team && pp == person)
				continue; //Don't bump into self
			
			const person_t *other = &(person_table[tt][pp]);
			
			int32_t dx = (other->pos[0] - pptr->pos[0]) / 256;
			int32_t dy = (other->pos[1] - pptr->pos[1]) / 256;
			int32_t dsq = (dx*dx)+(dy*dy);
			int32_t radius = 50;
			if(dsq > (radius*radius))
				continue; //Too far, no bump
			
			int bumpdir = trigfunc_atan2(dy, dx);
			pptr->pos[0] -= 4 * trigfunc_cos8(bumpdir);
			pptr->pos[1] -= 4 * trigfunc_sin8(bumpdir);
		}
	}
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
			
			//Run common updates
			sim_person_common(tt, pp);
		}
	}
	
	
}

//Match tick function - introduction sequence
void match_tick_intro(void)
{
	hideall();
	if( (match_state_ticks / 32) & 1 )
		show(UI_READY);
	
	sim_cam_gameplay();
	
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
	sim_goals();
	sim_cam_gameplay();
}

//Match tick function - goal scored
void match_tick_goal(void)
{
	hideall();
	show(UI_GOAL);
	show(UI_SCORES);
	
	sim_persons();
	sim_ball();
	sim_cam_toball();
	
	if(match_state_ticks > 300)
	{
		goformation();
		
		memset(ball_pos, 0, sizeof(ball_pos));
		memset(ball_vel, 0, sizeof(ball_vel));
		memset(cam_center, 0, sizeof(cam_center));
		memset(cam_vel, 0, sizeof(cam_vel));
		
		state(MS_INTRO);
	}
}

//Match tick function - out of bounds
void match_tick_oob(void)
{
	hideall();
	//show(UI_OOB);
	show(UI_SCORES);
	
	//Temp - just reset for now like a goal was scored
	goformation();
	
	memset(ball_pos, 0, sizeof(ball_pos));
	memset(ball_vel, 0, sizeof(ball_vel));
	memset(cam_center, 0, sizeof(cam_center));
	memset(cam_vel, 0, sizeof(cam_vel));
	
	state(MS_INTRO);
	
}

//Table of tick functions for different match states
typedef void (*match_state_tickfn_t)(void);
match_state_tickfn_t match_state_tickfns[MS_MAX] = 
{
	[MS_INTRO] = &match_tick_intro,
	[MS_PLAY]  = &match_tick_play,
	[MS_GOAL]  = &match_tick_goal,
	[MS_OOB]   = &match_tick_oob,
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
