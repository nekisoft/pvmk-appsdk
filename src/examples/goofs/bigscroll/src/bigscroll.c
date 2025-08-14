//bigscroll.c
//Example of scrolling a large image
//Bryan E. Topp <betopp@betopp.com> 2025

#include <sc.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

//Loaded chunks - need at least 6x5 to cover the screen when misaligned
#define LCH_DIM 8
typedef struct lch_info_s
{
	int idx_x;
	int idx_y;
	int valid;
} lch_info_t;
lch_info_t lch_infos[LCH_DIM][LCH_DIM] = { { {-1,-1,0} } }; //Just need element 0 to not be element 0 at start
uint16_t lch_pixels[LCH_DIM][LCH_DIM][64][64];

//Chunk currently being loaded
int lch_loading_x = -1;
int lch_loading_y = -1;

//Camera location - virtual upper-left 16.16
int cam_x16;
int cam_y16;

//Camera velocity
int cam_vx16;
int cam_vy16;

//Framebuffers
uint16_t fbs[3][240][320];
int fbnext;

int main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	
	//Track framerate of simulation
	int lastms = _sc_getticks();
	int frames = 0;
	int ticks = 0;
	
	//Track pressed buttons
	int buttons = 0;
	
	while(1)
	{
		//Read all inputs
		_sc_input_t input;
		while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
		{
			if(input.format == 'A')
				buttons = input.buttons;
		}

		//Count time elapsed
		int now = _sc_getticks();
		if( (now < lastms) || (now > lastms + 1000) )
		{
			//Discontinuity
			lastms = now;
			continue;
		}
		ticks += now - lastms;
		lastms = now;
		
		//Simulate frames and count frames simulated
		while(frames * 1000 < ticks * 60)
		{
			//Simulate a frame
			
			frames++;
			
			cam_vx16 *= 7;
			cam_vx16 /= 8;
			
			cam_vy16 *= 7;
			cam_vy16 /= 8;
			
			if(buttons & _SC_BTNBIT_LEFT)
				cam_vx16 -= 10000;
			if(buttons & _SC_BTNBIT_RIGHT)
				cam_vx16 += 10000;
			if(buttons & _SC_BTNBIT_UP)
				cam_vy16 -= 10000;
			if(buttons & _SC_BTNBIT_DOWN)
				cam_vy16 += 10000;
			
			cam_x16 += cam_vx16;
			cam_y16 += cam_vy16;
			
			if(cam_x16 < 0)
			{
				cam_x16 = 0;
				cam_vx16 = 0;
			}
			
			if(cam_y16 < 0)
			{
				cam_y16 = 0;
				cam_vy16 = 0;
			}
		}
		
		//Reduce time/frame counters, maintaining relationship, to prevent overflowing
		while(frames > 60 && ticks > 1000)
		{
			frames -= 60;
			ticks -= 1000;
		}
		
		//Draw visible chunks from the camera offset.
		int cam_x = cam_x16 >> 16;
		int cam_y = cam_y16 >> 16;
		
		//Work through whole screen in raster order
		for(int rr = 0; rr < 240; rr++)
		{
			for(int cc = 0; cc < 320; cc++)
			{
				//Work out screen address
				void *dst = &(fbs[fbnext][rr][cc]);
				
				//Work out chunk-relative coordinates
				int chunk_x  = (cc + cam_x) / 64;
				int chunk_y  = (rr + cam_y) / 64;
				int chunk_cc = (cc + cam_x) % 64;
				int chunk_rr = (rr + cam_y) % 64;
				
				//Work out drawing length - at most draw the whole line from the chunk
				int span = 64;
				
				//Cap to end of chunk
				if(chunk_cc + span > 64)
					span = 64 - chunk_cc;
				
				//Cap to end of screen
				if(cc + span > 320)	
					span = 320 - cc;
					
				//See if we have data to copy
				lch_info_t *info = &(lch_infos[chunk_y % LCH_DIM][chunk_x % LCH_DIM]);
				if(info->valid && info->idx_x == chunk_x && info->idx_y == chunk_y)
				{
					//Chunk is what we want - draw a span of pixels from it
					const void *src = &(lch_pixels[chunk_y % LCH_DIM][chunk_x % LCH_DIM][chunk_rr][chunk_cc]);
					memcpy(dst, src, span*sizeof(fbs[0][0][0]));
				}
				else
				{
					//Chunk isn't loaded - draw black pixels
					//Make boundaries white
					if(chunk_rr == 0 || chunk_cc == 0)
					{
						if(chunk_cc == 0)
							span = 1;
						
						memset(dst, 0xFF, span*sizeof(fbs[0][0][0]));
					}
					else
					{
						memset(dst, 0, span*sizeof(fbs[0][0][0]));
					}
				}

				//Skip over the following pixels in the span, for outer loop
				cc += span - 1;				
			}
		}
		
		//Enqueue new image to display
		int displayed = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, &(fbs[fbnext][0][0]));
		
		//Find spare buffer for the next one
		for(int bb = 0; bb < 3; bb++)
		{
			if((uintptr_t)(&(fbs[bb][0][0])) == (uintptr_t)displayed)
				continue; //This is the current frontbuffer
			if(bb == fbnext)
				continue; //This is the one we just enqueued, will be frontbuffer soon
			
			//Found the spare buffer
			fbnext = bb;
			break;
		}
		
		//If we're loading, continue loading
		if(lch_loading_x >= 0 && lch_loading_y >= 0)
		{
			//Loading. Keep trying to load.
			void *pxdst = &(lch_pixels[lch_loading_y % LCH_DIM][lch_loading_x % LCH_DIM][0][0]);
			
			//Todo... get this working async
			char fnbuf[256];
			snprintf(fnbuf, sizeof(fnbuf)-1, "data/bigscroll.%4.4d.%4.4d.565", lch_loading_y, lch_loading_x);
			FILE *lf = fopen(fnbuf, "rb");
			if(lf)
			{
				int nread = fread(pxdst, 1, sizeof(lch_pixels[0][0]), lf);
				if(nread == (int)sizeof(lch_pixels[0][0]))
				{
					//Success
				}
				else
				{
					//Failed to load?
					memset(pxdst, 0x55, sizeof(lch_pixels[0][0]));
				}
				fclose(lf);
			}
			else
			{
				//Failed to load?
				memset(pxdst, 0x55, sizeof(lch_pixels[0][0]));
			}
			
			//Done loading (or failed)! Set this entry to valid.
			lch_infos[lch_loading_y % LCH_DIM][lch_loading_x % LCH_DIM].idx_x = lch_loading_x;
			lch_infos[lch_loading_y % LCH_DIM][lch_loading_x % LCH_DIM].idx_y = lch_loading_y;
			lch_infos[lch_loading_y % LCH_DIM][lch_loading_x % LCH_DIM].valid = 1;

			//Can search for new things to load.
			lch_loading_x = -1;
			lch_loading_y = -1;
		}
		
		//If we're not loading, search for something to load
		if(lch_loading_x < 0 || lch_loading_y < 0)
		{
			//Look for the most-mismatched entry versus the current camera, all over the screen
			int worst_x = -1;
			int worst_y = -1;
			int worst_dist = 0;
			for(int rr = -64; rr < 240 + 63; rr += 32)
			{
				for(int cc = -64; cc < 320 + 63; cc += 32)
				{
					//Check the chunk that falls here on the screen
					int chunk_x  = (cc + cam_x) / 64;
					int chunk_y  = (rr + cam_y) / 64;
					if(chunk_x < 0)
						chunk_x = 0;
					if(chunk_y < 0)
						chunk_y = 0;
					
					//It might be already loaded and correct, or out-of-date.
					//Out-of-date chunks will have the wrong coordinate in their info.
					lch_info_t *infoptr = &(lch_infos[chunk_y % LCH_DIM][chunk_x % LCH_DIM]);
		
					//Check how far away from the "correct" coordinates it is
					int dx = chunk_x - infoptr->idx_x;
					int dy = chunk_y - infoptr->idx_y;
					int dist = (dx*dx)+(dy*dy);
					if(dist > worst_dist)
					{
						worst_x = chunk_x;
						worst_y = chunk_y;
						worst_dist = dist;
					}
					
				}
			}
			
			//If we found one to replace, do so
			if(worst_dist > 0)
			{
				lch_loading_x = worst_x;
				lch_loading_y = worst_y;
			}
		}
	}
}