//frametest.c
//Frame-ordering test for the SDL system-call shim.
//Bryan E. Topp <betopp@betopp.com> 2024
//
//Paints frame N a single colour that encodes N, flips it, and goes on to N+1. Used with
//SDLSC_DUMP and SDLSC_FRAMES: check.sh reads the dumped PPMs back and demands that every
//frame is one flat colour, that the colour decodes to a frame number, and that the frame
//numbers only ever go up. A frame that was copied while the application was drawing into
//it is not flat, and a frame presented out of order decodes out of order - so this is the
//test that catches tearing, whereas a hash only catches that something changed.

#include <sc.h>

#include <stdint.h>
#include <string.h>

#define W 320
#define H 240

static unsigned short fb[3][H][W];

//Frame number in the pixel, five bits red, six green, five blue: exactly what survives
//the shim's 5:6:5 to 8:8:8 expansion in the PPM dumper, so check.sh can read it back.
static unsigned short encode(unsigned int frame)
{
	return (unsigned short)(((frame & 0x1F) << 11) | (((frame >> 5) & 0x3F) << 5) | (((frame >> 11) & 0x1F) << 0));
}

int app_main(int argc, const char **argv)
{
	//"cheap" paints one pixel per frame instead of all of them. The frame checks need
	//whole frames; the timing checks need the program to be out of the way so that what
	//is being measured is the shim and the display, not this loop.
	int cheap = (argc > 1 && strcmp(argv[1], "cheap") == 0);

	//Triple buffering, the way the console is meant to be used: draw into the buffer
	//that is neither on screen nor waiting to be, and let flip tell us which one that
	//is. Drawing into a buffer that is still being scanned out is what tears.
	int back = 0;

	//Runs until the shim's frame limit stops it; the colour wraps round after 65536
	//frames, which the windowed check does not depend on (it only asks that every frame
	//be flat, because a windowed display drops frames whenever the program outruns it).
	for(unsigned int ff = 0; ff < 5000000u; ff++)
	{
		unsigned short *buffer = &(fb[back][0][0]);
		unsigned short colour = encode(ff);
		if(cheap)
			buffer[ff % (W * H)] = colour;
		else
			for(int pp = 0; pp < (W * H); pp++)
				buffer[pp] = colour;

		int flipped = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, buffer);

		int used[3] = { 0, 0, 0 };
		for(int bb = 0; bb < 3; bb++)
		{
			if(bb == back)
				used[bb]++;	//the one just enqueued
			if((const void *)&(fb[bb][0][0]) == (const void *)(intptr_t)(unsigned int)flipped)
				used[bb]++;	//the one on screen
		}
		for(int bb = 0; bb < 3; bb++)
		{
			if(used[bb] == 0)
			{
				back = bb;
				break;
			}
		}
	}

	return 0;
}

#ifndef PUMPED
int main(int argc, const char **argv)
{
	return app_main(argc, argv);
}
#else
int main(int argc, const char **argv)
{
	return _sc_shim_main(app_main, argc, argv);
}
#endif
