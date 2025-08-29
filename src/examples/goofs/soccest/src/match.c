//match.c
//Soccer match
//Bryan E. Topp <betopp@betopp.com> 2025

#include <sc.h>

#include "teamdata.h"
#include "fbs.h"

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
		int32_t ycoord = cam_radius * PITCHTEX_DIM/16 * (yy) / (1024 - yy);
		//ycoord += cam_center[1] + (cam_radius * PITCHTEX_DIM);
		
		uint16_t *texrow = &(pitchtex[ (ycoord >> 8) % PITCHTEX_DIM ][0]);
		uint16_t *fbrow = &(fbs[fbs_next][479-yy][0]);
		int32_t texfrac = ((PITCHTEX_DIM/2)*256) + cam_center[0];
		int32_t texstep = cam_radius * PITCHTEX_DIM / 65536;
		texstep *= 1024;
		texstep /= (1024 - yy);
		texfrac -= texstep * 320;
		for(int xx = 0; xx < 640; xx++)
		{
			fbrow[xx] = texrow[ (texfrac >> 8) % PITCHTEX_DIM];
			texfrac += texstep;
		}
	}
}

int match(int argc, const char **argv)
{
	(void)argc;
	(void)argv;

	printf("nvm size %d\n", (int)sizeof(nvm_t));
	
	pitchtex_load();
	
	cam_center[0] = 0;
	cam_center[1] = 0;
	
	cam_radius = 8 * 256;
	
	
	while(1)
	{
		drawpitch();
		fbs_flip();
		
		int buttons = 0;
		_sc_input_t ii = {0};
		while(_sc_input(&ii, sizeof(ii), sizeof(ii)) > 0)
		{
			if(ii.format == 'A')
				buttons = ii.buttons;
		}
		
		if(buttons & _SC_BTNBIT_UP)
			cam_radius++;
		if(buttons & _SC_BTNBIT_DOWN)
			cam_radius--;
	}
	
}
