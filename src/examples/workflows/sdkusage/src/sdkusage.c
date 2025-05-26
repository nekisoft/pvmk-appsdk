//sdkusage.c
//Example of using the C SDK to write a Neki32 program
//Bryan E. Topp <betopp@betopp.com> 2024

//The SDK includes standard headers with implementations from picolibc.
#include <stdio.h>
#include <stdlib.h>

//The SDK includes, also, a header defining all supported system-call functions.
#include <sc.h>

//Buffer for holding image data
uint16_t image_buffer[64][64];

//Framebuffers
uint16_t fbs[2][240][320];

//The SDK handles setting things up and calling main.
int main(int argc, const char **argv)
{
	//Open a file from the card.
	//Behind the scenes, the SDK contains an ISO9660 implementation to handle this.
	//The operating system itself still just handles reading the sectors.
	FILE *imagefile = fopen("/IMAGE.BIN", "rb");
	if(imagefile != NULL)
	{
		//Load the image data
		fread(image_buffer, 1, sizeof(image_buffer), imagefile);
		fclose(imagefile);
	}
	
	//Main loop - draw frames to alternating buffers
	int back = 0;  //Which framebuffer we are drawing to
	int drawx = 33; //Location to draw image into framebuffer, X
	int drawy = 62; //Location to draw image info framebuffer, Y
	int movex = 1; //Direction of motion of image, X
	int movey = 1; //Direction of motion of image, Y
	while(1)
	{
		//Scoot the image coordinates around
		drawx += movex;
		if(drawx > 320-64)
		{
			drawx = 320-64;
			movex = -1;
		}
		if(drawx < 0)
		{
			drawx = 0;
			movex = 1;
		}
		
		drawy += movey;
		if(drawy > 240-64)
		{
			drawy = 240-64;
			movey = -1;
		}
		if(drawy < 0)
		{
			drawy = 0;
			movey = 1;
		}
		
		//Fill the backbuffer with black
		for(int yy = 0; yy < 240; yy++)
		{
			for(int xx = 0; xx < 320; xx++)
			{
				fbs[back][yy][xx] = 0;
			}
		}
		
		//Draw the image in
		for(int yy = 0; yy < 64; yy++)
		{
			for(int xx = 0; xx < 64; xx++)
			{
				fbs[back][yy + drawy][xx + drawx] = image_buffer[yy][xx];
			}
		}
		
		//Show the result
		while(1)
		{
			int flipped = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[back]);
			if((void*)flipped == fbs[back])
			{
				//Image is now displayed
				break;
			}
			else
			{
				//Image not displayed yet
				_sc_pause();
				continue;
			}
		}
		
		//Use the other buffer next time
		back = back ? 0 : 1;
	}
}