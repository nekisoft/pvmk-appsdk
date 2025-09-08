//match.c
//Soccer match
//Bryan E. Topp <betopp@betopp.com> 2025

#include <sc.h>

#include "match.h"

#include "teamdata.h"
#include "fbs.h"
#include "pads.h"
#include "font.h"
#include "images.h"

#include "stb_image.h"
#include <string.h>


//Pitch-space (0,0 at center of pitch) where camera looks - centimeters 24.8
int32_t cam_center[2];

//Horizontal extent of camera at center of screen - centimeters 24.8
int32_t cam_radius;

//Velocity of camera
int32_t cam_vel[2];


//Position of ball on pitch - centimeters 24.8
int32_t ball_pos[3];

//Velocity of ball - cm/s 24.8
int32_t ball_vel[3];

//Ball animation with fraction towards the next frame - xx.8
int32_t ball_frame;

//Location of goalposts
int32_t goalpost_pos[4][3];


#define PITCHTEX_DIM 2048
#define PITCHTEX_DIM_LOG2 11
uint16_t pitchtex[PITCHTEX_DIM][PITCHTEX_DIM];
void pitchtex_load(void)
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


void drawpitch(void)
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
void drawcard(images_file_t imf, int vx, int vy, int vz, int vh)
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

//Runs at 100hz to simulate match
void match_tick(void)
{
	//Temp - let player slap that ball around
	if(pads[PAD_A] & _SC_BTNBIT_UP)
		ball_vel[1] += 1000;
	if(pads[PAD_A] & _SC_BTNBIT_DOWN)
		ball_vel[1] -= 1000;
	if(pads[PAD_A] & _SC_BTNBIT_LEFT)
		ball_vel[0] -= 1000;
	if(pads[PAD_A] & _SC_BTNBIT_RIGHT)
		ball_vel[0] += 1000;
	if(pads[PAD_A] & BTNBIT_A)
		cam_radius += 10;
	if(pads[PAD_A] & BTNBIT_B)
		cam_radius -= 10;
	if(pads[PAD_A] & BTNBIT_C)
		ball_vel[2] = 20000;
	
	
	
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

void match(void)
{

	printf("nvm size %d\n", (int)sizeof(nvm_t));
	
	cam_center[0] = 0;//800*256; //cm 24.8
	cam_center[1] = 0;//800*256; //cm 24.8
	
	cam_radius = 800; //cm
	
	ball_pos[0] = 0;
	ball_pos[1] = 0;
	ball_pos[2] = 0;
	
	int goalwidth = 732 * 256; //Spacing between goal posts
	int goalheight = 244 * 256; //Height of goal top beam
	int goaldist = 39*100*256; //Distance from center of field to goal line
	
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
	
	
	
	pitchtex_load();
	
	images_purge();
	images_loadrange(IMF_CARD_AAA, IMF_CARD_ZZZ);
	
	int last_sim = _sc_getticks();
	while(1)
	{
		//Draw the world
		drawpitch();
		
		char txtbuf[256];
		snprintf(txtbuf, sizeof(txtbuf)-1, "%8d %8d", cam_radius,cam_center[1]);
		font_draw(txtbuf, 0x8000,0,0);
		
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
		
		fbs_flip();
		
		//Allow quitting (temp)
		if(pads[PAD_A] & BTNBIT_MODE)
			return;
		
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
	}
	
}
