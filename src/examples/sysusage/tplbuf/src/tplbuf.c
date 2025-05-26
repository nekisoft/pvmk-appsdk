//tplbuf.c
//Triple-buffered animation example for Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

//This example uses the C SDK.
//The approach, of triple-buffering and running ahead of the video, doesn't require any special SDK code.
//Only one system-call is used - _sc_gfx_flip to enqueue a change of buffers.

//System-call definitions from SDK
#include <sc.h>

//Integer types defined in SDK
#include <stdint.h>

//Framebuffers - three of them, as this is triple-buffering
uint16_t fbs[3][240][320];

//Entry point
int main(int argc, const char **argv)
{
	//Command-line arguments aren't typically used for anything.
	//(The SDK supports passing them in an exec() call though.)
	(void)argc;
	(void)argv;
	
	//We'll animate a vertical bar moving left and right, to demonstrate the lack of tearing.
	int barx = 0;
	int bardir = 1;
	
	//Pick a buffer to initially be our backbuffer
	int back = 0;
	
	//Loop forever
	while(1)
	{
		//Fill the backbuffer with black pixels
		for(int yy = 0; yy < 240; yy++)
		{
			for(int xx = 0; xx < 320; xx++)
			{
				fbs[back][yy][xx] = 0;
			}
		}
		
		//Paint a line down the screen at a particular X coordinate
		for(int yy = 0; yy < 240; yy++)
		{
			fbs[back][yy][barx] = 0xFFFF;
		}
		
		//Update that X coordinate for the next frame
		barx += bardir;
		if(barx >= 320)
		{
			barx = 319;
			bardir = -1;
		}
		if(barx < 0)
		{
			barx = 0;
			bardir = 1;
		}
		
		//Enqueue the frame we just made, to be displayed starting at the next vertical-blanking interval
		int flipped = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, &(fbs[back][0][0]));
		
		//We know which buffer we just enqueued for display. It might get displayed soon.
		//The return from _sc_gfx_flip tells us which buffer is already displayed now.
		//Figure out which buffer isn't used, then.
		int buffer_used[3] = {0,0,0};
		for(int bb = 0; bb < 3; bb++)
		{
			if( bb == back )
				buffer_used[bb]++; //The buffer is the one we just enqueued
			
			if( (void*)&(fbs[bb][0][0]) == (void*)(flipped) )
				buffer_used[bb]++; //The buffer is the one being displayed now
		}
		
		//Our next frame should get drawn in the remaining buffer, the one neither displayed nor enqueued.
		if(buffer_used[0] == 0)
			back = 0;
		else if(buffer_used[1] == 0)
			back = 1;
		else
			back = 2;
		
		//Note that we just plow ahead rendering more frames, without waiting.
		//Triple-buffering allows a game to run at a framerate decoupled from the video scanout.
	}
}
