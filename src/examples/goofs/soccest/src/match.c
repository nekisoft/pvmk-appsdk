//match.c
//Soccer match
//Bryan E. Topp <betopp@betopp.com> 2025

#include <sc.h>

#include "match.h"

#include "teamdata.h"
#include "fbs.h"
#include "pads.h"
#include "font.h"

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
		int32_t texy = cam_radius * (PITCHTEX_DIM/16) * dy / depth;
		texy += cam_center[1] ;
		
		uint32_t ycoord = texy + (PITCHTEX_DIM*128);
		
		if(ycoord < 0 || ycoord >= (PITCHTEX_DIM<<8))
		{
			memset(fbrow, 0, 640*2);
			continue;
		}
		
		uint16_t *texrow = &(pitchtex[ (ycoord >> 8) % PITCHTEX_DIM ][0]);
		
		int32_t texfrac = ((PITCHTEX_DIM/2)*256) + cam_center[0];
		int32_t texstep = cam_radius * PITCHTEX_DIM / 65536;
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
			
			//if((texfrac>>8)==((PITCHTEX_DIM/2)+(cam_center[0]>>8)))
			//	fbrow[xx] = 0xFFFF;
			
			//if((ycoord>>8)==((PITCHTEX_DIM/2)+(cam_center[1]>>8)))
			//	fbrow[xx] = 0xFFFF;
		}
	}
}

void match(void)
{

	printf("nvm size %d\n", (int)sizeof(nvm_t));
	
	pitchtex_load();
	
	cam_center[0] = 0;
	cam_center[1] = 0;
	
	cam_radius = 8192;
	
	
	while(1)
	{
		drawpitch();
		
		char txtbuf[256];
		snprintf(txtbuf, sizeof(txtbuf)-1, "%8d %8d", cam_radius,cam_center[1]);
		font_draw(txtbuf, 0x8000,0,0);
		
		
		
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
