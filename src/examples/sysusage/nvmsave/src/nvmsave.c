//nvmsave.c
//Example of using NVM saving in Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

//This example depends on the system setting up NVM saving for us.
//That means we need to specify a valid Volume ID in our ISO9660 PVD on the card.
//The included Makefile passes the right option to mkisofs.
//This Volume ID is then used as the name of our savegame at the system menu.
//By the time this program executes, the savegame will already be ready to go.

//System call entry-point definitions from SDK
#include <sc.h>

//Integer type definitions from SDK
#include <stdint.h>

//Structure of our savegame data
typedef struct example_nvm_s
{
	//How many times this example has been started
	int boots;
	
	//How many seconds the example has been running
	int seconds;
	
} example_nvm_t;

//Space for savegame data as loaded
example_nvm_t nvm;

//Framebuffer, for showing our statistics onscreen
uint16_t fb[240][320];

//Draws a digit on the screen
void drawdigit(int x, int y, int d)
{
	//Font for drawing numbers
	static const char * const font[] = 
	{
		" xxxxx  ", "  x     ", " xxxxx  ", "xxxxxx  ", "x    x  ", "xxxxxxx ", "  xxxxx ", "xxxxxxx ", " xxxxx  ", " xxxxx  ",
		"x     x ", "  x     ", "x     x ", "      x ", "x    x  ", "x       ", " x      ", "     x  ", "x     x ", "x     x ",
		"x     x ", "  x     ", "      x ", "      x ", "x    x  ", "x       ", "x       ", "  xxxxx ", "x     x ", "x     x ",
		"x     x ", "  x     ", "     x  ", "xxxxxx  ", " xxxxxx ", "xxxxxx  ", "xxxxxx  ", "   x    ", " xxxxx  ", " xxxxxx ",
		"x     x ", "  x     ", "    x   ", "      x ", "     x  ", "      x ", "x     x ", "  x     ", "x     x ", "      x ",
		"x     x ", "  x     ", "   x    ", "      x ", "     x  ", "      x ", "x     x ", "  x     ", "x     x ", "      x ",
		" xxxxx  ", "  x     ", "xxxxxxx ", "xxxxxx  ", "     x  ", "xxxxxx  ", " xxxxx  ", "  x     ", " xxxxx  ", "      x ",
		"        ", "        ", "        ", "        ", "        ", "        ", "        ", "        ", "        ", "        ",
	};
	
	//Iterate over row within the digit, 8 rows
	for(int rr = 0; rr < 8; rr++)
	{
		//Iterate over column within the digit, 8 columns
		for(int cc = 0; cc < 8; cc++)
		{
			//Check data from font array, see if it should be white or black
			if(font[ (rr*10) + d ][cc] != ' ')
			{
				fb[y + rr][x + cc] = 0xFFFF;
			}
			else
			{
				fb[y + rr][x + cc] = 0x0000;
			}
		}
	}
}

//Draws a number on the screen
void drawnumber(int x, int y, int num)
{
	//Draw each digit of the number
	x += 64;
	for(int dd = 0; dd < 8; dd++)
	{
		x -= 8;
		drawdigit(x, y, num % 10);
		num /= 10;
	}
}

int main(int argc, const char **argv)
{
	//Ignore command-line arguments
	(void)argc;
	(void)argv;
	
	//Load our savegame
	_sc_nvm_load(&nvm, sizeof(nvm));
	
	//We're now booted once more
	nvm.boots++;
	
	//Count how long we've been running
	int start_time = _sc_getticks();
	int counted_seconds = 0;
	
	//Main loop
	while(1)
	{
		//Display the statistics
		drawnumber(64, 64, nvm.boots);
		drawnumber(64, 128, nvm.seconds);
		_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fb);
		
		//See if we've passed another second
		if( (_sc_getticks() - start_time) > (counted_seconds * 1000) )
		{
			//Another second has elapsed, update timing
			counted_seconds++;
			
			//Save it back to the NVM
			nvm.seconds++;
			_sc_nvm_save(&nvm, sizeof(nvm));
		}
	}
}