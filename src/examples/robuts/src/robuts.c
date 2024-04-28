//robuts.c
//Robot shooter
//Bryan E. Topp <betopp@betopp.com> 2024

#include <sc.h>
#include <stdint.h>

#define STBI_NO_THREAD_LOCALS 1
#define STB_IMAGE_IMPLEMENTATION 1
#define STBI_ONLY_PNG 1
#include "stb_image.h"

//Framebuffers
uint16_t fbs[2][240][320];
int fbs_next;

//Images to load
typedef enum imgidx_e
{
	II_NONE = 0,
	
	II_LOAD,
	II_TITLE,
	II_BG,
	II_OVER,
	
	II_PLAYER,
	II_ROBUTS,
	II_PBULS,
	
	II_MAX
} imgidx_t;

//Names of images to load
const char *imgnames[II_MAX] = 
{
	[II_LOAD]   = "load.png",
	[II_TITLE]  = "title.png",
	[II_BG]     = "bg.png",
	[II_OVER]   = "over.png",
	[II_PLAYER] = "player.png",
	[II_ROBUTS] = "robuts.png",
	[II_PBULS]  = "pbuls.png",
};

//Data of images loaded
typedef struct img_s
{
	int width;
	int height;
	uint16_t *pixels;
} img_t;
static img_t imgs[II_MAX];

//Robuts on the field
typedef struct robut_s
{
	//Type of robut (or 0 for unused)
	int which;
	
	//Location on the screen
	int x, y;
	
	//Frame of animation
	int frame;
	
	//Damage already dealt
	int damage;
	
} robut_t;
#define ROBUT_MAX 32
static robut_t robuts[ROBUT_MAX];

//Players on the field
typedef struct player_s
{
	//If the player is alive
	int alive;
	
	//Location on the screen
	int x, y;
	
	//How soon they can shoot
	int refire;

} player_t;
#define PLAYER_MAX 4
static player_t players[PLAYER_MAX];

//Player bullets
typedef struct pbul_s
{
	int type;
	int x8, y8;
	int vx8, vy8;
	int life;
} pbul_t;
#define PBUL_MAX 16
static pbul_t pbuls[PBUL_MAX];

//Bounds of screen
static const int xmax = 320 - 16 - 4;
static const int xmin = 0;
static const int ymax = 240 - 16 - 4;
static const int ymin = 16;

//Player inputs
static uint16_t inputs;
static uint16_t inputs_new;

//Clears backbuffer
static void clear(void)
{
	memset(fbs[fbs_next], 0, sizeof(fbs[fbs_next]));
}

//Flips video buffers
static void flip(void)
{
	while(_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[fbs_next]) != (int)(fbs[fbs_next])) { _sc_pause(); }
	fbs_next = fbs_next ? 0 : 1;
}

//Updates player input state
static void pollin(void)
{
	_sc_input_t input;
	while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
	{
		if(input.format != 'A')
			continue;
		
		inputs_new = input.buttons & ~inputs;
		inputs = input.buttons;
	}
}

//Copies image to the screen
static void blit(int src, int x, int y)
{
	uint16_t *srcptr = imgs[src].pixels;
	for(int yy = 0; yy < imgs[src].height; yy++)
	{
		for(int xx = 0; xx < imgs[src].width; xx++)
		{
			if(*srcptr)
				fbs[fbs_next][yy + y][xx + x] = *srcptr;
			
			srcptr++;
		}
	}
}

//Copies 16x16 tile from image to the screen
static void blit16(int src, int sx, int sy, int x, int y)
{
	uint16_t *srcptr = imgs[src].pixels;
	
	srcptr += 16 * imgs[src].width * sy;
	srcptr += 16 * sx;
	
	for(int yy = 0; yy < 16; yy++)
	{
		for(int xx = 0; xx < 16; xx++)
		{
			if(*srcptr)
				fbs[fbs_next][yy + y][xx + x] = *srcptr;
			
			srcptr++;
		}
		
		srcptr -= 16;
		srcptr += imgs[src].width;
	}	
}

//Runs start-screen loop
static void startscreen(void)
{
	while(1)
	{
		clear();
		blit(II_TITLE, 0, 0);
		flip();
		
		pollin();
		if(inputs_new & _SC_BTNBIT_START)
			return;
	}
}

//Runs game level loop
static int level(int lvl)
{
	(void)lvl;
	
	//Init players
	players[0].x = (320 - 16) / 2;
	players[0].y = (240 - 16) / 2;
	players[0].alive = 1;
	
	while(1)
	{
		//clear();
		
		//Background
		blit(II_BG, 0, 0);
		
		//Draw robuts
		for(int rr = 0; rr < ROBUT_MAX; rr++)
		{
			if(robuts[rr].which == 0)
				continue;
			
			blit16(II_ROBUTS, robuts[rr].which, 0, robuts[rr].x, robuts[rr].y);
		}
		
		//Draw players
		for(int pp = 0; pp < PLAYER_MAX; pp++)
		{
			if(players[pp].alive == 0)
				continue;
			
			blit16(II_PLAYER, 0, 0, players[pp].x, players[pp].y);
		}
		
		//Draw player bullets
		for(int pp = 0; pp < PBUL_MAX; pp++)
		{
			if(pbuls[pp].type == 0)
				continue;
			
			blit16(II_PBULS, 0, 0, pbuls[pp].x8 >> 8, pbuls[pp].y8 >> 8);
		}
		
		flip();
		
		//Update game
		pollin();
		
		//Players
		for(int pp = 0; pp < PLAYER_MAX; pp++)
		{
			if(!players[pp].alive)
				continue;
			
			//Allow movement
			if(inputs & _SC_BTNBIT_UP)
				players[pp].y--;
			if(inputs & _SC_BTNBIT_DOWN)
				players[pp].y++;
			if(inputs & _SC_BTNBIT_LEFT)
				players[pp].x--;
			if(inputs & _SC_BTNBIT_RIGHT)
				players[pp].x++;
			
			//Allow shooting
			if(players[pp].refire > 0)
			{
				//Still waiting for the next shot
				players[pp].refire--;
			}
			else if(inputs_new & _SC_BTNBIT_A)
			{
				//Try to fire. Need to find a new slot.
				int newslot = -1;
				for(int pp = 0; pp < PBUL_MAX; pp++)
				{
					if(pbuls[pp].type == 0)
					{
						newslot = pp;
						break;
					}
				}
				
				//If we have a new slot we can spawn a bullet
				if(newslot != -1)
				{
					//Figure out which direction to fire
					int vdir = 0;
					int hdir = 0;
					if(inputs & _SC_BTNBIT_UP)
						vdir--;
					if(inputs & _SC_BTNBIT_DOWN)
						vdir++;
					if(inputs & _SC_BTNBIT_LEFT)
						hdir--;
					if(inputs & _SC_BTNBIT_RIGHT)
						hdir++;
						
					int dir = 0;
					dir += vdir + vdir + vdir;
					dir += hdir;
					if(dir != 0)
					{
						//Have a valid slot and direction. Fire.
						switch(dir)
						{
							case -4: //Up left
								pbuls[newslot].vx8 = -181;
								pbuls[newslot].vy8 = -181;
								break;
							case -3: //Up
								pbuls[newslot].vx8 =    0;
								pbuls[newslot].vy8 = -256;
								break;
							case -2: //Up right
								pbuls[newslot].vx8 =  181;
								pbuls[newslot].vy8 = -181;
								break;
							case -1: //Left
								pbuls[newslot].vx8 = -256;
								pbuls[newslot].vy8 =    0;
								break;
							case 1: //Right
								pbuls[newslot].vx8 =  256;
								pbuls[newslot].vy8 =    0;
								break;
							case 2: //Down left
								pbuls[newslot].vx8 = -181;
								pbuls[newslot].vy8 =  181;
								break;
							case 3: //Down
								pbuls[newslot].vx8 =    0;
								pbuls[newslot].vy8 =  256;
								break;
							case 4: //Down right 
								pbuls[newslot].vx8 =  181;
								pbuls[newslot].vy8 =  181;
								break;
							default:
								break;
						}
						
						pbuls[newslot].vx8 *= 5;
						pbuls[newslot].vy8 *= 5;
						
						pbuls[newslot].life = 256;
						pbuls[newslot].x8 = players[pp].x << 8;
						pbuls[newslot].y8 = players[pp].y << 8;
						pbuls[newslot].type = 1;
						
						players[pp].refire = 2;
					}
				}
			}
				
			//Cap position			
			if(players[pp].x > xmax)
				players[pp].x = xmax;
			if(players[pp].x < xmin)
				players[pp].x = xmin;
			if(players[pp].y > ymax)
				players[pp].y = ymax;
			if(players[pp].y < ymin)
				players[pp].y = ymin;
		}
		
		//Player bullets
		for(int pp = 0; pp < PBUL_MAX; pp++)
		{
			if(pbuls[pp].type == 0)
				continue;
			
			pbuls[pp].life--;
			if(pbuls[pp].life <= 0)
				pbuls[pp].type = 0;
			
			pbuls[pp].x8 += pbuls[pp].vx8;
			pbuls[pp].y8 += pbuls[pp].vy8;
			
			if( (pbuls[pp].x8 >> 8) > xmax)
				pbuls[pp].type = 0;
			if( (pbuls[pp].x8 >> 8) < xmin)
				pbuls[pp].type = 0;
			if( (pbuls[pp].y8 >> 8) > ymax)
				pbuls[pp].type = 0;
			if( (pbuls[pp].y8 >> 8) < ymin)
				pbuls[pp].type = 0;
		}
	}
	
	return 1; //Victory
}

//Runs game-over screen loop
static void gameover(void)
{
	for(int ii = 0; ii < 60; ii++)
	{
		clear();
		blit(II_OVER, 0, 0);
		flip();
		
		pollin();
		if(inputs_new & _SC_BTNBIT_START)
			return;
	}
}

//Entry point
int main(void)
{
	//Load images
	img_t default_img = { .width = 2, .height = 2, .pixels = (uint16_t[]){ 0, -1, -1, 0 } };
	for(int ii = 0; ii < II_MAX; ii++)
	{
		if(imgnames[ii] == NULL)
		{
			imgs[ii] = default_img;
			continue;
		}
		
		int x = 0;
		int y = 0;
		int n = 0;
		uint8_t *rgba = stbi_load(imgnames[ii], &x, &y, &n, 4);
		if(rgba == NULL)
		{
			imgs[ii] = default_img;
			continue;
		}

		imgs[ii].width = x;
		imgs[ii].height = y;
		imgs[ii].pixels = malloc(x*y*2);
		if(imgs[ii].pixels == NULL)
		{
			imgs[ii] = default_img;
			stbi_image_free(rgba);
			continue;
		}
		
		uint8_t *srcptr = rgba;
		for(int yy = 0; yy < y; yy++)
		{
			for(int xx = 0; xx < x; xx++)
			{
				uint8_t r = srcptr[0];
				uint8_t g = srcptr[1];
				uint8_t b = srcptr[2];
				uint8_t a = srcptr[3];
				srcptr += 4;
				
				uint16_t rgb565 = 0;
				rgb565 = (rgb565 << 5) | (r >> 3);
				rgb565 = (rgb565 << 6) | (g >> 2);
				rgb565 = (rgb565 << 5) | (b >> 3);
				if(a == 0)
					rgb565 = 0;
				
				imgs[ii].pixels[ (yy * imgs[ii].width) + xx ] = rgb565;
			}
		}
		
		//Cheap hack - assume first image is the loading screen.
		//Put it onscreen ASAP.
		if(ii == II_LOAD)
		{
			blit(ii, 0, 0);
			flip();
		}
		
		stbi_image_free(rgba);
	}
	
	//Run game
	while(1)
	{
		startscreen();
		for(int ll = 1; ll < 100; ll++)
		{
			if(!level(ll))
				break;
		}
		gameover();
	}
	return 0;
}
