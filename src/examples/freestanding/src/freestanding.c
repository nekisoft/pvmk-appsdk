//freestanding.c
//Example of writing freestanding C for Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

//Framebuffers
static short fbs[2][240][320];

//Position to draw a dot, 24.8 fixed-point
static int blip_pos[2];

//Velocity of dot, 24.8 fixed-point
static int blip_vel[2];

//Shim for performing a system-call, defined in assembly in setup.s
extern int freestanding_sc(int num, int p1, int p2, int p3, int p4, int p5);

//Runs system-call for switching framebuffer
static int flip_fb(void *buffer)
{
	return freestanding_sc(
		0x30,        //System-call number for _sc_gfx_flip()
		2,           //Graphics mode for 320x240 16bpp
		(int)buffer, //Buffer to show
		0,
		0,
		0);
}

//Runs "pause" system call
static void pause(void)
{
	(void)freestanding_sc(
		0x01,   //System-call number for _sc_pause()
		0,
		0,
		0,
		0,
		0);
}

//Function called from our setup.s assembly file
void freestanding_main(void)
{
	//Initialize position and velocity of dot
	blip_pos[0] = 160 << 8;
	blip_pos[1] = 120 << 8;
	blip_vel[0] = 192;
	blip_vel[1] = 104;
	
	//Pick an initial backbuffer
	int bb = 0;
	
	while(1)
	{
		//Advance position of dot
		blip_pos[0] += blip_vel[0];
		blip_pos[1] += blip_vel[1];
		
		//Bounce when it hits a wall
		if(blip_pos[0] > (316 << 8))
		{
			blip_pos[0] = 316 << 8;
			blip_vel[0] *= -1;
		}
		if(blip_pos[0] < 0)
		{
			blip_pos[0] = 0;
			blip_vel[0] *= -1;
		}
		if(blip_pos[1] > (236 << 8))
		{
			blip_pos[1] = 236 << 8;
			blip_vel[1] *= -1;
		}
		if(blip_pos[1] < 0)
		{
			blip_pos[1] = 0;
			blip_vel[1] *= -1;
		}
		
		//Draw the background
		for(int yy = 0; yy < 240; yy++)
		{
			for(int xx = 0; xx < 320; xx++)
			{
				fbs[bb][yy][xx] = 0;
			}
		}
		
		//Draw the dot
		for(int yy = 0; yy < 4; yy++)
		{
			for(int xx = 0; xx < 4; xx++)
			{
				fbs[bb][yy + (blip_pos[1]>>8)][xx + (blip_pos[0]>>8)] = 0xFFFF;
			}
		}
		
		//Display the result and wait for vblank
		while(1)
		{
			int flipped = flip_fb(fbs[bb]);
			if( (void*)flipped == fbs[bb] )
			{
				//Now displayed
				break;
			}
			else	
			{
				//Waiting to display
				pause();
				continue;
			}
		}
		
		//Use the opposite buffer as the backbuffer next time
		bb = bb ? 0 : 1;
	}
	
}
