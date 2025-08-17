//democard.c
//Demo card menu
//Bryan E. Topp <betopp@betopp.com> 2024

//System calls
#include <sc.h>

//Hackhack - we set up NVM ourselves unlike a typical game
//This shouldn't be used normally
SYSCALL_DECL int _sc_nvm_ident(const char *name)
	#define _SC_NVM_IDENT_N 0x80
	{ return _SC(_SC_NVM_IDENT_N, name, 0, 0, 0, 0); }

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

//Decompresses an image
void imgload(uint16_t dst[480][640], const char *filename, int *x_out, int *y_out)
{
	int bg_x = 0;
	int bg_y = 0;
	int bg_n = 0;
	uint8_t *bgimage_loaded = stbi_load(filename, &bg_x, &bg_y, &bg_n, 4);
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
				
				dst[yy][xx] = 0;
				dst[yy][xx] |= ((b >> 3) & 0x1F) <<  0;
				dst[yy][xx] |= ((g >> 2) & 0x3F) <<  5;
				dst[yy][xx] |= ((r >> 3) & 0x1F) << 11;
			}
		}
		stbi_image_free(bgimage_loaded);
	}
	
	if(x_out != NULL)
		*x_out = bg_x;
	if(y_out != NULL)
		*y_out = bg_y;
}

void prepnvm(const char *nvmname)
{
	int result = _sc_nvm_ident(nvmname);
	if(result >= 0)
	{
		//All good
		return;
	}
	else
	{
		//Failed to set up NVM... warn about it
		//Show warning and wait for them to press start
		imgload(bg, "nvmfail.png", NULL, NULL);
		int lastbuttons = ~0u;
		while(1)
		{
			memcpy(fb, bg, sizeof(fb));
			_sc_gfx_flip(_SC_GFX_MODE_VGA_16BPP, fb);
			
			_sc_input_t input = {0};
			while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
			{
				if(input.format != 'A')
					continue;
				
				if(input.buttons & _SC_BTNBIT_START & ~lastbuttons)
					return;
				
				lastbuttons = input.buttons;
			}
			
			_sc_pause();
		}
	}
}

int main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	
	//Load images
	imgload(bg, "demobg.png", NULL, NULL);
	
	int ar_x = 0;
	int ar_y = 0;
	imgload(ar, "demoar.png", &ar_x, &ar_y);

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
									prepnvm("Freedoom: Phase 1");
									setenv("DOOMWADDIR", "/phase1", 1);
									execl("doom.nne", "doom", NULL);
									break;
								case 1:
									prepnvm("Freedoom: Phase 2");
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