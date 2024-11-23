//democard.c
//Demo card menu
//Bryan E. Topp <betopp@betopp.com> 2024

//System calls
#include <sc.h>

//Standard library functions
#include <unistd.h>
#include <stdlib.h>

//Image loading library
#define STBI_NO_THREAD_LOCALS 1
#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"

//Images
uint16_t bg[480][640]; //Background
uint16_t ar[480][640]; //Arrow
uint16_t fb[480][640]; //Framebuffer


int main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	
	//Load images
	int bg_x = 0;
	int bg_y = 0;
	int bg_n = 0;
	uint8_t *bgimage_loaded = stbi_load("demobg.png", &bg_x, &bg_y, &bg_n, 4);
	if(bgimage_loaded != NULL)
	{
		const uint8_t *bgptr = bgimage_loaded;
		for(int yy = 0; yy < bg_y; yy++)
		{
			for(int xx = 0; xx < bg_x; xx++)
			{
				uint8_t r = bgptr[0];
				uint8_t g = bgptr[1];
				uint8_t b = bgptr[2];
				//uint8_t a = bgptr[3];
				bgptr += 4;
				
				bg[yy][xx] = 0;
				bg[yy][xx] |= ((b >> 3) & 0x1F) <<  0;
				bg[yy][xx] |= ((g >> 2) & 0x3F) <<  5;
				bg[yy][xx] |= ((r >> 3) & 0x1F) << 11;
			}
		}
		stbi_image_free(bgimage_loaded);
	}
		
	int ar_x = 0;
	int ar_y = 0;
	int ar_n = 0;
	uint8_t *arimage_loaded = stbi_load("demoar.png", &ar_x, &ar_y, &ar_n, 4);
	if(arimage_loaded != NULL)
	{
		const uint8_t *arptr = arimage_loaded;
		for(int yy = 0; yy < ar_y; yy++)
		{
			for(int xx = 0; xx < ar_x; xx++)
			{
				uint8_t r = arptr[0];
				uint8_t g = arptr[1];
				uint8_t b = arptr[2];
				uint8_t a = arptr[3];
				arptr += 4;
				
				if(a > 10)
				{
					ar[yy][xx] = 0;
					ar[yy][xx] |= ((b >> 3) & 0x1F) <<  0;
					ar[yy][xx] |= ((g >> 2) & 0x3F) <<  5;
					ar[yy][xx] |= ((r >> 3) & 0x1F) << 11;
				}
			}
		}
		stbi_image_free(arimage_loaded);
	}
	
	//Show menu until the user makes a choice
	int lastbuttons = ~0u;
	int selection = 0;
	int redraw = 1;
	while(1)
	{
		if(redraw)
		{
			memcpy(fb, bg, sizeof(fb));
			int arrow_x = (60 + (selection * 220));
			int arrow_y = 380;
			for(int yy = 0; yy < ar_y; yy++)
			{
				for(int xx = 0; xx < ar_x; xx++)
				{
					if(ar[yy][xx] != 0)
						fb[yy+arrow_y][xx+arrow_x] = ar[yy][xx];
				}
			}
			redraw = 0;
		}
		_sc_gfx_flip(_SC_GFX_MODE_VGA_16BPP, fb);
		
		_sc_input_t input = {0};
		while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
		{
			if(input.format != 'A')
				continue;
			
			for(int bb = 0; bb < 16; bb++)
			{
				if(lastbuttons & (1u << bb))
					continue;
				if(!(input.buttons & (1u << bb)))
					continue;
				
				switch(bb)
				{
					case _SC_BTNIDX_LEFT:
						if(selection > 0)
						{
							redraw = 1;
							selection--;
						}
						
						break;
					case _SC_BTNIDX_RIGHT:
						if(selection < 2)
						{
							redraw = 1;
							selection++;
						}
						
						break;
					case _SC_BTNIDX_START:
					case _SC_BTNIDX_A:
						{
							switch(selection)
							{
								case 0:
									setenv("DOOMWADDIR", "/phase1", 1);
									execl("doom.nne", "doom", NULL);
									break;
								case 1:
									setenv("DOOMWADDIR", "/phase2", 1);
									execl("doom.nne", "doom", NULL);
									break;
								case 2:
									execl("fmv.nne", "fmv", NULL);
									break;
								default:
									break;
							}
						}					
					default:
						break;
				}
			}
			
			lastbuttons = input.buttons;
		}
	}
}