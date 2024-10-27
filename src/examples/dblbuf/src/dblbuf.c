//dblbuf.c
//Double-buffered animation example for Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

//This example uses the C SDK.
//The approach, of double-buffering and synchronizing to vertical blanking, doesn't require any special SDK code.
//Two system-calls are used - _sc_gfx_flip to enqueue a change of buffers, and _sc_pause when waiting for it to complete.

//System-call definitions from SDK
#include <sc.h>

//Integer types defined in SDK
#include <stdint.h>

//Framebuffers - two of them, as this is double-buffering
uint16_t fbs[2][240][320];

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
		
		//Try to display the new frame, as long as that takes
		while(1)
		{
			int flipped = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[back]);
			if( (void*)flipped == fbs[back] )
			{
				//The requested buffer is now displayed.
				break;
			}
			else
			{
				//Still waiting for our buffer to be displayed.
				_sc_pause();
				continue;
			}
		}
		
		//The old backbuffer has now been made visible.
		//Use the other buffer, while that old one is sent to the TV.
		back = back ? 0 : 1;
	}
}