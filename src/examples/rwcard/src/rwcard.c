//rwcard.c
//Example of reading and writing to a card on Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

//System-call definitions from SDK
#include <sc.h>

//Standard IO implementation from PicoLibc in SDK
#include <stdio.h>

//This example shows a grid of data on the screen as colored squares.
//The user can modify the data by flipping a square's color.
//The colors are saved to a file on disk and loaded on next boot.

//Framebuffer for drawing data
uint16_t fb[480][640];

//Block we'll visualize on the screen
uint8_t databytes[32][64];

//Square selected
int cursor[2];

int main(int argc, const char **argv)
{
	//Ignore command-line arguments
	(void)argc;
	(void)argv;
	
	//Load the data from card
	FILE *df = fopen("/SAVE.BIN", "rb");
	if(df != NULL)
	{
		fread(databytes, 1, sizeof(databytes), df);
		fclose(df);
	}
	
	//Update loop
	int last_buttons = ~0u;
	while(1)
	{
		//Draw the image on screen as a grid of blocks
		for(int rr = 0; rr < 32; rr++)
		{
			for(int cc = 0; cc < 64; cc++)
			{
				uint16_t color = databytes[rr][cc] ? 0x2222:0x1111;
				for(int yy = 0; yy < 8; yy++)
				{
					for(int xx = 0; xx < 8; xx++)
					{
						fb[ 8 + (rr*8) + yy ][ 8 + (cc*8) + xx ] = color;
						
						//Outline cursor
						if(cursor[1] == rr && cursor[0] == cc)
						{
							if(xx == 0 || xx == 7 || yy == 0 || yy == 7)
								fb[ 8 + (rr*8) + yy ][ 8 + (cc*8) + xx ] = 0xFFFF;
						}
					}
				}
			}
		}
		
		//Present it and wait for the image to be displayed
		while(1)
		{
			int flipped = _sc_gfx_flip(_SC_GFX_MODE_VGA_16BPP, fb);
			
			if( flipped == -_SC_EAGAIN)
				_sc_pause();
			
			if( (void*)flipped == fb )
				break;
		}
			
		//Check player inputs
		_sc_input_t input = {0};
		while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
		{
			//Only process inputs coming from digital gamepad on port A
			if(input.format != 'A')
				continue; //Not an input from player A
			
			//Check each bit of buttons reported, for one that was just pressed
			for(int bb = 0; bb < 16; bb++)
			{
				if(!(input.buttons & (1u << bb) & ~last_buttons))
					continue; //Not a just-pressed button
				
				switch(bb)
				{
					case _SC_BTNIDX_A:
					{
						//User pressed "A".
						//Flip the data under their cursor.
						databytes[cursor[1]][cursor[0]] = !databytes[cursor[1]][cursor[0]];
						
						//Save the file back to disk
						FILE *sf = fopen("/SAVE.BIN", "wb");
						if(sf != NULL)
						{
							fwrite(databytes, 1, sizeof(databytes), sf);
							fclose(sf);
						}
						
						break;
					}
					case _SC_BTNIDX_LEFT:
					{
						//User pressed "left" on dpad.
						//Move cursor left.
						if(cursor[0] > 0)
							cursor[0]--;
						
						break;
					}
					case _SC_BTNIDX_RIGHT:
					{
						//User pressed "right" on dpad.
						//Move cursor right.
						if(cursor[0] < 63)
							cursor[0]++;
						
						break;
					}
					case _SC_BTNIDX_UP:
					{
						//User pressed "up" on dpad.
						//Move cursor up.
						if(cursor[1] > 0)
							cursor[1]--;
						
						break;
					}
					case _SC_BTNIDX_DOWN:
					{
						//User pressed "down" on dpad.
						//Move cursor down.
						if(cursor[1] < 31)
							cursor[1]++;
						
						break;
					}
				}	
			}
			
			//Set aside buttons just pressed, to compare against later
			last_buttons = input.buttons;
		}
	}
}