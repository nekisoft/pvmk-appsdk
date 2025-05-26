//showinput.c
//Example of getting user input on Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

//System-call definitions from the SDK, including input format
#include <sc.h>

//Integer types from the SDK
#include <stdint.h>

//Framebuffer for showing results on-screen
uint16_t fb[240][320];

//Locations to draw graphical representation of each button
//(Uses indexes defined in sc.h for each button bit position)
const int btnpoints[16][2] = 
{
	   [_SC_BTNIDX_UP] = { 1, 0 },
	 [_SC_BTNIDX_LEFT] = { 0, 1 },
	 [_SC_BTNIDX_DOWN] = { 1, 2 },
	[_SC_BTNIDX_RIGHT] = { 2, 1 },
	    [_SC_BTNIDX_A] = { 7, 1 },
	    [_SC_BTNIDX_B] = { 8, 1 },
	    [_SC_BTNIDX_C] = { 9, 1 },
	    [_SC_BTNIDX_X] = { 7, 0 },
	    [_SC_BTNIDX_Y] = { 8, 0 },
	    [_SC_BTNIDX_Z] = { 9, 0 },
	[_SC_BTNIDX_START] = { 5, 2 },
	 [_SC_BTNIDX_MODE] = { 4, 2 },
};

//Entry point for the example, called by the C SDK startup code
int main(int argc, const char **argv)
{
	//Command-line arguments not used (the system doesn't pass anything useful, but exec() supports it in the SDK)
	(void)argc;
	(void)argv;
	
	//Last buttons received for each player
	int player_buttons[4] = {0};
	
	//Main loop forever
	while(1)
	{
		//Poll input events until no more are reported
		while(1)
		{
			//Run a system-call to return up to 10 events in an array
			_sc_input_t events[10] = {0};
			int nevents = _sc_input(events, sizeof(events[0]), sizeof(events));
		
			//Work through all events returned
			for(int ee = 0; ee < nevents; ee++)
			{
				switch(events[ee].format)
				{
					case 'A': //Port A gamepad
						player_buttons[0] = events[ee].buttons;
						break;
					case 'B': //Port B gamepad
						player_buttons[1] = events[ee].buttons;
						break;
					case 'C': //Port C gamepad
						player_buttons[2] = events[ee].buttons;
						break;
					case 'D': //Port D gamepad
						player_buttons[3] = events[ee].buttons;
						break;
					default: //All others
						//Ignore unrecognized events for forward-compatibility
						break;
				}
			}
			
			if(nevents <= 0)
				break; //No more events at this time
			else
				continue; //Keep looping so we can get more events if they're piled up
		}
		
		//Draw a colored square on the screen, for each button, for each player.
		for(int player = 0; player < 4; player++)
		{
			for(int button = 0; button < 16; button++)
			{
				//Only use button bits that we've defined a graphical position for
				if(btnpoints[button][0] == 0 && btnpoints[button][1] == 0)
					continue;
				
				//Compute where the graphic goes
				const int sqsize = 8;
				const int top    = (1 + (player * 4) + btnpoints[button][1]) * sqsize;
				const int left   = (1                + btnpoints[button][0]) * sqsize;
				const int bottom = top + sqsize;
				const int right  = left + sqsize;
				
				//Decide on color - whether the player pressed the button or not
				const int fill = (player_buttons[player] & (1u << button)) ? 0xEEEE : 0x1111;
				
				//Fill in the square
				for(int yy = top; yy < bottom; yy++)
				{
					for(int xx = left; xx < right; xx++)
					{
						fb[yy][xx] = fill;
					}
				}
			}
		}
		
		//Present the result on the screen
		_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fb);
	}
}