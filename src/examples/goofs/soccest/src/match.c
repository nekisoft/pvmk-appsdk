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


//int32_t cam_scroll[2];
//int32_t cam_zoom;
//void cam_project(int32_t *out2, int32_t *in3)
//{
//	
//}

//Pitch-space (0,0 at center of pitch) where camera looks
int32_t cam_center[2];

//Horizontal extent of camera at center of screen - .8
int32_t cam_radius;

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
			
			if((texfrac>>8)==((PITCHTEX_DIM/2)+(cam_center[0]>>10)))
				fbrow[xx] = 0xFFFF;
			
			if((ycoord>>8)==((PITCHTEX_DIM/2)+((cam_center[1]/3)>>8)))
				fbrow[xx] = 0xFFFF;
		}
	}
}

//Draws sprite with the given virtual (worldspace) x, y, height
//x and y are center-of-bottom coordinates
//vx, vy, vh in 24.8 fixed-point centimeters (meters * 100 * 256)
void drawcard(images_file_t imf, int vx, int vy, int vh)
{	
	int sy = ((cam_radius * (PITCHTEX_DIM/4) * 240) - (vy * 1024 / 3) + (1024 * cam_center[1] / 3) ) / ((cam_radius * (PITCHTEX_DIM/4)) + (vy/3)  + (-cam_center[1] / 3));
	int sx = 320 +  (((vx - cam_center[0]) / cam_radius) * (1024+sy) / (1024));
	int sh = vh * (1024 + sy) / cam_radius / 256;
	sy+=(sh/8);	
	images_card(imf, sx, sy, sh);
}

void match(void)
{

	printf("nvm size %d\n", (int)sizeof(nvm_t));
	
	pitchtex_load();
	
	cam_center[0] = 0;//800*256; //cm 24.8
	cam_center[1] = 0;//800*256; //cm 24.8
	
	cam_radius = 800; //cm
	
	images_purge();
	images_loadrange(IMF_CARD_AAA, IMF_CARD_ZZZ);
	
	while(1)
	{
		drawpitch();
		
		char txtbuf[256];
		snprintf(txtbuf, sizeof(txtbuf)-1, "%8d %8d", cam_radius,cam_center[1]);
		font_draw(txtbuf, 0x8000,0,0);
		
		
		drawcard(IMF_CARD_CONE, 0, 0, 50*256);
		drawcard(IMF_CARD_CONE, 800*256, 0, 50*256);
		drawcard(IMF_CARD_CONE, -800*256, 0, 50*256);
		drawcard(IMF_CARD_CONE, 0, 800*256, 50*256);
		drawcard(IMF_CARD_CONE, 0, -800*256, 50*256);
		
		
		fbs_flip();
		
		
		if(pads[PAD_A] & _SC_BTNBIT_UP)
			cam_center[1] += 640;
		if(pads[PAD_A] & _SC_BTNBIT_DOWN)
			cam_center[1] -= 640;
		if(pads[PAD_A] & _SC_BTNBIT_LEFT)
			cam_center[0] -= 640;
		if(pads[PAD_A] & _SC_BTNBIT_RIGHT)
			cam_center[0] += 640;
		if(pads[PAD_A] & BTNBIT_A)
			cam_radius += 10;
		if(pads[PAD_A] & BTNBIT_B)
			cam_radius -= 10;
		if(pads[PAD_A] & BTNBIT_MODE)
			return;
	}
	
}
