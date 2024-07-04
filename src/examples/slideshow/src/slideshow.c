//slideshow.c
//Example of loading and displaying fullscreen JPEGs
//Bryan E. Topp <betopp@betopp.com> 2024

#include <sc.h>
#include <stdint.h>
#include <string.h>

//Image loading library
#define STB_IMAGE_IMPLEMENTATION 1
#define STBI_NO_THREAD_LOCALS 1
#include "stb_image.h"

//File names to load
static const char *filenames[] = 
{
	"a.jpg",
	"b.jpg",
	"c.jpg",
	"d.jpg",
	"e.jpg",
	"f.jpg",
	"g.jpg",
	"h.jpg",
	"i.jpg",
	"j.jpg",
	NULL
};

//File currently selected
int fileidx = 0;

//Framebuffers
uint16_t fbs[2][480][640];

//Next framebuffer to use as back buffer
int fbnext;


int main(void)
{
	int lastbuttons = ~0u;
	int displayedfile = -1;
	while(1)
	{
		//Poll input, allow user to change image
		_sc_input_t input = {0};
		while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
		{
			//Only process input from the first controller
			if(input.format != 'A')
				continue;
			
			//Check if they've just begun pressing A or B
			if(input.buttons & ~lastbuttons & _SC_BTNBIT_A)
				fileidx++;
			else if(input.buttons & ~lastbuttons & _SC_BTNBIT_B)
				fileidx--;
			
			//Cap the index of the file displayed, wrap around at the end
			if(fileidx < 0)
				fileidx += (sizeof(filenames)/sizeof(filenames[0])) - 1;
			if(filenames[fileidx] == NULL)
				fileidx = 0;
			
			//Track which buttons already pressed
			lastbuttons = input.buttons;
		}
		
		if(displayedfile != fileidx)
		{
			//Load the given image
			int im_x, im_y, im_n;
			uint8_t *loaded = stbi_load(filenames[fileidx], &im_x, &im_y, &im_n, 3);
			if(loaded != NULL)
			{
				//Convert to 16bpp in framebuffer
				if(im_y == 480 && im_x == 640)
				{
					uint8_t *src = loaded;
					uint16_t *dst = &(fbs[fbnext][0][0]);
					for(int pp = 0; pp < 640*480; pp++)
					{
						*dst = ((src[2] >> 3) <<  0) | ((src[1] >> 2) <<  5) | ((src[0] >> 3) << 11);
						dst++;
						src += 3;
					}
				}
				else
				{
					uint8_t *pxptr = loaded;
					for(int yy = 0; yy < im_y; yy++)
					{
						for(int xx = 0; xx < im_x; xx++)
						{
							if(yy < 480 && xx < 640)
							{
								fbs[fbnext][yy][xx] = 0;
								fbs[fbnext][yy][xx] |= (pxptr[2] >> 3) <<  0;
								fbs[fbnext][yy][xx] |= (pxptr[1] >> 2) <<  5;
								fbs[fbnext][yy][xx] |= (pxptr[0] >> 3) << 11;
							}
							pxptr += 3;
						}
					}
				}
				
				stbi_image_free(loaded);
			}
			else
			{
				//Failed to load image
				memset(fbs[fbnext], 0, sizeof(fbs[fbnext]));
			}
			
			//Display it using double-buffering
			while(1)
			{
				//Enqueue a flip to the newly-filled buffer
				int displayed = _sc_gfx_flip(_SC_GFX_MODE_VGA_16BPP, fbs[fbnext]);
				
				//If there was an error, stop trying
				if(displayed < 0)
					break;
				
				//If it's now shown, stop trying
				if(displayed == (int)(intptr_t)(fbs[fbnext]))
					break;
				
				//Otherwise, wait and try again
				_sc_pause();
			}
			fbnext = fbnext ? 0 : 1;
			displayedfile = fileidx;
		}
	}
	
	return 0;
}