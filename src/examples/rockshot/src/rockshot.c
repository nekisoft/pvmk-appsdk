//rockshot.c
//Space shooting game
//Bryan E. Topp <betopp@betopp.com> 2024

#include <sc.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

//Size of the screen
#define HIRES 1
#if HIRES
	#define SCRX 640
	#define SCRY 480
	#define SCRM _SC_GFX_MODE_VGA_16BPP
#else
	#define SCRX 320
	#define SCRY 240
	#define SCRM _SC_GFX_MODE_320X240_16BPP
#endif

//Framebuffers, and which is to be drawn to
uint16_t fbs[3][SCRY][SCRX];
int fbnext;

//Buffers of lines to draw (or drawn) on each framebuffer
typedef struct line_s
{
	int x0, y0, x1, y1;
} line_t;
#define LINE_MAX 1024
line_t lines[3][LINE_MAX];
int linesn[3];

//Buttons pressed by the user
int buttons;

//Camera center in world coordinates 16.16
int campos[2];
int camvel[2];

//Camera scale and its reciprocal
int camscale;
int camrecip;

//Player position and rotation
int player_pos[2];
int player_vel[2];
int player_ang;
int player_avel;

//Player's bullets
#define BULLET_MAX 32
typedef struct bullet_s
{
	int pos[2];
	int vel[2];
	int timeleft;
} bullet_t;
bullet_t bullets[BULLET_MAX];

//Player refire timer
int refire;

//Background noise - positions and angles
int bgflurry[8][8][3];

//Rocks
#define ROCK_MAX 16
typedef struct rock_s
{
	int health;
	int pos[2];
	int vel[2];
	int ang;
	int avel;
	int shape[8][2];
} rock_t;
rock_t rocks[ROCK_MAX];

//Debris
#define DEBRIS_MAX 16
typedef struct debris_s
{
	int life;
	int pos[2];
	int vel[2];
	int ang;
	int avel;
} debris_t;
debris_t debris[DEBRIS_MAX];

//How many lives the player has left
//int lives;

//If the player is waiting to respawn, and for how long
int spawntimer;

//Draws a line into the current backbuffer. Bresenham spans algorithm.
void drawline(int x0, int y0, int x1, int y1, uint16_t color)
{
	if(x0 == x1)
	{
		//Vertical line
		int ymin = (y0 < y1) ? y0 : y1;
		int ymax = (y0 > y1) ? y0 : y1;		
		for(int yy = ymin; yy < ymax; yy++)
		{
			fbs[fbnext][yy][x0] = color;
		}
		return;
	}
	
	if(y0 == y1)
	{
		//Horizontal line
		int xmin = (x0 < x1) ? x0 : x1;
		int xmax = (x0 > x1) ? x0 : x1;		
		for(int xx = xmin; xx < xmax; xx++)
		{
			fbs[fbnext][y0][xx] = color;
		}
		return;
	}
	
	int xmag = abs(x0-x1);
	int ymag = abs(y0-y1);
	if(xmag == ymag)
	{
		//Diagonal line
		int xmin = (x0 < x1) ? x0 : x1;
		int ymin = (y0 < y1) ? y0 : y1;
		int ymax = (y0 > y1) ? y0 : y1;
		
		if( ((x0 - x1) ^ (y0 - y1)) < 0 )
		{			
			int xx = xmin;
			int yy = ymax;
			for(int dd = 0; dd < xmag; dd++)
			{
				fbs[fbnext][yy][xx] = color;
				xx++;
				yy--;
			}
		}
		else
		{
			int xx = xmin;
			int yy = ymin;
			for(int dd = 0; dd < xmag; dd++)
			{
				fbs[fbnext][yy][xx] = color;
				xx++;
				yy++;
			}
		}
		
		return;
	}
	
	if(xmag > ymag)
	{
		//X-major line
		int ymin = (y0 < y1) ? y0 : y1;
		int ymax = (y0 > y1) ? y0 : y1;
		int span = xmag / ymag;
		int rem = xmag % ymag;
		int frac = 0;
		if( ((x0 - x1) ^ (y0 - y1)) > 0 )
		{
			//Upward slope
			int xx = (x0 < x1) ? x0 : x1;
			for(int yy = ymin; yy < ymax; yy++)
			{				
				for(int ss = 0; ss < span; ss++)
				{				
					fbs[fbnext][yy][xx] = color;
					xx++;
				}
				
				frac += rem;
				if(frac > ymag)
				{
					frac -= ymag;
					fbs[fbnext][yy][xx] = color;
					xx++;
				}
			}
		}
		else
		{
			//Downward slope
			int xx = (x0 > x1) ? x0 : x1;
			for(int yy = ymin; yy < ymax; yy++)
			{
				for(int ss = 0; ss < span; ss++)
				{
					fbs[fbnext][yy][xx] = color;
					xx--;
				}
				
				frac += rem;
				if(frac > ymag)
				{
					frac -= ymag;
					fbs[fbnext][yy][xx] = color;
					xx--;
				}
			}
		}
	}
	else
	{
		//Y-major line
		int xmin = (x0 < x1) ? x0 : x1;
		int xmax = (x0 > x1) ? x0 : x1;
		int span = ymag / xmag;
		int rem = ymag % xmag;
		int frac = 0;
		if( ((y0 - y1) ^ (x0 - x1)) > 0 )
		{
			//Upward slope
			int yy = (y0 < y1) ? y0 : y1;
			for(int xx = xmin; xx < xmax; xx++)
			{
				for(int ss = 0; ss < span; ss++)
				{
					fbs[fbnext][yy][xx] = color;
					yy++;
				}
				
				frac += rem;
				if(frac > xmag)
				{
					frac -= xmag;
					fbs[fbnext][yy][xx] = color;
					yy++;
				}
			}
		}
		else
		{
			//Downward slope
			int yy = (y0 > y1) ? y0 : y1;
			for(int xx = xmin; xx < xmax; xx++)
			{
				for(int ss = 0; ss < span; ss++)
				{
					fbs[fbnext][yy][xx] = color;
					yy--;
				}
				
				frac += rem;
				if(frac > xmag)
				{
					frac -= xmag;
					fbs[fbnext][yy][xx] = color;
					yy--;
				}
			}
		}
	}
}

//Draws all enqueued lines into the current backbuffer. Enqueues a flip of the buffers. Clears a new backbuffer.
void refresh(void)
{
	//Paint lines to the next framebuffer
	for(int ll = 0; ll < linesn[fbnext]; ll++)
	{
		const line_t *lptr = &(lines[fbnext][ll]);
		drawline(lptr->x0, lptr->y0, lptr->x1, lptr->y1, 0xFFFF);
	}
	
	//Enqueue flip to that next framebuffer
	int visible = _sc_gfx_flip(SCRM, &(fbs[fbnext][0][0]));
	if(visible < 0)
	{
		//Error flipping
		return;
	}
	
	//Figure out which framebuffer is free - neither the one we just enqueued, nor the one currently displayed
	uint16_t *visible_ptr = (uint16_t*)(uintptr_t)(visible);
	uint16_t *enqueued_ptr = &(fbs[fbnext][0][0]);
	for(int ff = 0; ff < 3; ff++)
	{
		uint16_t *candidate_ptr = &(fbs[ff][0][0]);
		if(candidate_ptr != visible_ptr && candidate_ptr != enqueued_ptr)
		{
			fbnext = ff;
			break;
		}
	}
	
	//Now that we know which framebuffer we'll be using, clear it out for the new image.
	for(int ll = 0; ll < linesn[fbnext]; ll++)
	{
		const line_t *lptr = &(lines[fbnext][ll]);
		drawline(lptr->x0, lptr->y0, lptr->x1, lptr->y1, 0);
	}
	linesn[fbnext] = 0;
	
	//Read inputs
	_sc_input_t inp = {0};
	while(_sc_input(&inp, sizeof(inp), sizeof(inp)) > 0)
	{
		if(inp.format != 'A')
			continue;
		
		if(inp.buttons & _SC_BTNBIT_START)
			exit(0);
		
		buttons = inp.buttons;
	}
}

//Enqueues a line to be drawn at the next refresh.
void qline(int x0, int y0, int x1, int y1)
{
	//Clip the given coordinates (naive method... probably can figure out a faster option)
	for(int clip = 0; clip < 8; clip++)
	{		
		//Discard lines that are trivially off the screen
		if(x0 < 0 && x1 < 0)
			return;
		if(x0 >= SCRX && x1 >= SCRX)
			return;
		if(y0 < 0 && y1 < 0)
			return;
		if(y0 >= SCRY && y1 >= SCRY)
			return;
		
		if(x0 < 0)
		{
			//Point 0 off the left side of the screen
			y0 -= x0 * (y1 - y0) / (x1 - x0) ;
			x0 = 0;
		}
		else if(x1 < 0)
		{
			//Point 1 off the left side of the screen
			y1 -= x1 * (y1 - y0) / (x1 - x0) ;
			x1 = 0;
		}
		else if(x0 >= SCRX)
		{
			//Point 0 off the right side of the screen
			y0 -= (x0 - SCRX) * (y1 - y0) / (x1 - x0) ;
			x0 = SCRX;
		}
		else if(x1 >= SCRX)
		{
			//Point 1 off the right side of the screen
			y1 -= (x1 - SCRX) * (y1 - y0) / (x1 - x0) ;
			x1 = SCRX;
		}
		else if(y0 < 0)
		{
			//Point 0 off the top of the screeen
			x0 -= y0 * (x1 - x0) / (y1 - y0) ;
			y0 = 0;
		}
		else if(y1 < 0)
		{
			//Point 1 off the top of the screen
			x1 -= y1 * (x1 - x0) / (y1 - y0) ;
			y1 = 0;
		}
		else if(y0 >= SCRY)
		{
			//Point 0 off the bottom of the screen
			x0 -= (y0 - SCRY) * (x1 - x0) / (y1 - y0) ;
			y0 = SCRY;
		}
		else if(y1 >= SCRY)
		{
			//Point 1 off the bottom of the screen
			x1 -= (y1 - SCRY) * (x1 - x0) / (y1 - y0) ;
			y1 = SCRY;
		}
		else
		{
			//Fully clipped
			line_t *lptr = &(lines[fbnext][linesn[fbnext]]);
			linesn[fbnext] = (linesn[fbnext] + 1) % LINE_MAX;
			
			lptr->x0 = x0;
			lptr->y0 = y0;
			lptr->x1 = x1;
			lptr->y1 = y1;
			
			return;
		}
	}

	
}

//Scales and enqueues the given world-space line for drawing at the next refresh
void qwsline(int wsx0, int wsy0, int wsx1, int wsy1)
{
	//Transform with camera
	int relx0 = (((wsx0 - campos[0]) / 65536) * camrecip) / 65536;
	int rely0 = (((wsy0 - campos[1]) / 65536) * camrecip) / 65536;
	int relx1 = (((wsx1 - campos[0]) / 65536) * camrecip) / 65536;
	int rely1 = (((wsy1 - campos[1]) / 65536) * camrecip) / 65536;
	qline( (SCRX/2) + relx0, (SCRY/2) + rely0, (SCRX/2) + relx1, (SCRY/2) + rely1);
}

//Fixed-point math - cosine of 16-bit angle as 0.16 result
int cos16(int ang16)
{
	//temp - replace with table-based method or something
	float angle_in_radians = (ang16 / 65536.0f) * (3.1415926535f * 2.0f);
	float sine_float = sinf(angle_in_radians);
	return sine_float * 65536.0f;
}
int sin16(int ang16)
{
	return cos16(ang16 + 16384);
}

//Generates a new rock
void rockspawn(void)
{
	int rnum = -1;
	for(int rr = 0; rr < ROCK_MAX; rr++)
	{
		if(rocks[rr].health <= 0)
		{
			rnum = rr;
			break;
		}
	}
	if(rnum == -1)
		return;
	
	int offsx = (rand() % 512) - 256;
	int offsy = (rand() % 512) - 256;
	
	rocks[rnum].pos[0] = player_pos[0] + (offsx * 65536);
	rocks[rnum].pos[1] = player_pos[1] + (offsy * 65536);
	rocks[rnum].vel[0] = (rand() % 65536) - 32768;
	rocks[rnum].vel[1] = (rand() % 65536) - 32768;
	
	rocks[rnum].ang = rand() % 65536;
	rocks[rnum].avel = rand() % 256;
	
	rocks[rnum].health = 5;
	
	int radius = 25;
	int rrand = 5;
	for(int vv = 0; vv < 8; vv++)
	{
		rocks[rnum].shape[vv][0] = cos16(65536 * vv / 8) * (radius + (rand() % rrand)) / 65536;
		rocks[rnum].shape[vv][1] = sin16(65536 * vv / 8) * (radius + (rand() % rrand)) / 65536;
	}
}

//Enqueues a list of vertexes for drawing at the given worldspace position with rotation
void qmodel(const int verts[][2], int nverts, const int pos[2], int ang)
{
	for(int vv = 0; vv < nverts; vv++)
	{
		int verts_xf[2][2];
		
		int vva = (vv + 0);
		int vvb = (vv + 1);
		if(vvb >= nverts)
			vvb = 0;
		
		//Todo - don't need to actually compute this all twice 
		verts_xf[0][0] = pos[0] + (cos16(ang) * verts[vva][0]) + (sin16(ang) * verts[vva][1]);
		verts_xf[0][1] = pos[1] + (sin16(ang) * verts[vva][0]) - (cos16(ang) * verts[vva][1]);				
		
		verts_xf[1][0] = pos[0] + (cos16(ang) * verts[vvb][0]) + (sin16(ang) * verts[vvb][1]);
		verts_xf[1][1] = pos[1] + (sin16(ang) * verts[vvb][0]) - (cos16(ang) * verts[vvb][1]);	
		
		qwsline(verts_xf[0][0], verts_xf[0][1], verts_xf[1][0], verts_xf[1][1]);
	}	
}

void gamedraw(void)
{
	
	//Player model
	if(spawntimer <= 0)
	{
		static const int plverts[][2] = 
		{
			{  4, -4 },
			{ -4, -4 },
			{  0,  8 },
		};
		qmodel(plverts, sizeof(plverts)/sizeof(plverts[0]), player_pos, player_ang);
	}
	
	//Bullets
	for(int bb = 0; bb < BULLET_MAX; bb++)
	{
		if(bullets[bb].timeleft <= 0)
			continue;
		
		static const int bverts[][2] = 
		{
			{  2,  0 },
			{  0,  2 },
			{ -2,  0 },
			{  0, -2 }
		};
		qmodel(bverts, sizeof(bverts)/sizeof(bverts[0]), bullets[bb].pos, 0);
	}
	
	//Rocks
	for(int rr = 0; rr < ROCK_MAX; rr++)
	{
		if(rocks[rr].health <= 0)
			continue;
		
		//Each rock has their own shape (void* to avoid array qualifier conversion rules)
		qmodel((void*)(rocks[rr].shape), sizeof(rocks[rr].shape)/sizeof(rocks[rr].shape[0]), rocks[rr].pos, rocks[rr].ang);
	}
	
	//Background flurry
	for(int yy = 0; yy < 8; yy++)
	{
		for(int xx = 0; xx < 8; xx++)
		{
			static const int fverts[][2] = 
			{
				{-1, 1},
				{1, -1},
			};
			qmodel(fverts, sizeof(fverts)/sizeof(fverts[0]), bgflurry[yy][xx], bgflurry[yy][xx][2]);		
		}
	}
	
	//Debris
	for(int dd = 0; dd < DEBRIS_MAX; dd++)
	{
		if(debris[dd].life <= 0)
			continue;
		
		static const int fverts[][2] = 
		{
			{-2, 2},
			{2, -2},
		};
		qmodel(fverts, sizeof(fverts)/sizeof(fverts[0]), debris[dd].pos, debris[dd].ang);	
	}
	
	refresh();
}

//Spawns debris at the given position
void dodebris(int spawnpos[2])
{
	for(int dd = 0; dd < DEBRIS_MAX; dd++)
	{
		if(debris[dd].life > 0)
		{
			//Slot already in use
			continue;
		}
		
		//Alright, have a spot to put the new debris.
		debris[dd].pos[0] = spawnpos[0];
		debris[dd].pos[1] = spawnpos[1];
		debris[dd].ang = 0;
		
		debris[dd].vel[0] = (rand() % 65536) - 32768;
		debris[dd].vel[1] = (rand() % 65536) - 32768;
		debris[dd].avel = rand() % 16384;
		
		
		debris[dd].life = 50;
		return;
	}
}

//Called when the player dies
void death(void)
{
	//Spawn debris
	dodebris(player_pos);
	dodebris(player_pos);
	dodebris(player_pos);
	dodebris(player_pos);
	
	player_pos[0] = campos[0];
	player_pos[1] = campos[1];
	
	camvel[0] = 0;
	camvel[1] = 0;
	
	player_vel[0] = 0;
	player_vel[1] = 0;
	
	//Wait for respawn
	//lives--;
	spawntimer = 100;
}

void gametick(void)
{	
	//Allow player controls
	if(spawntimer <= 0)
	{
		//Tank controls (Asteroids style)
		/*
		if(buttons & _SC_BTNBIT_B)
		{
			//Accelerate
			player_vel[0] += sin16(player_ang) / 16;
			player_vel[1] += -cos16(player_ang) / 16;
		}
		if(buttons & _SC_BTNBIT_Y)
		{
			//Reverse
			player_vel[0] -= sin16(player_ang) / 16;
			player_vel[1] -= -cos16(player_ang) / 16;
		}
		
		if(buttons & _SC_BTNBIT_LEFT)
		{
			//Turn left
			player_avel += 10;
		}
		
		if(buttons & _SC_BTNBIT_RIGHT)
		{
			//Turn right
			player_avel -= 10;
		}*/
		
		//Direct controls (Sinistar style)
		int intended_angle = 0;
		int nodirection = 0;
		if(buttons & _SC_BTNBIT_RIGHT)
		{
			if(buttons & _SC_BTNBIT_UP)
			{
				intended_angle = 8192 * 1;
			}
			else if(buttons & _SC_BTNBIT_DOWN)
			{
				intended_angle = 8192 * -1;
			}
			else
			{
				intended_angle = 0;
			}
		}
		else if(buttons & _SC_BTNBIT_LEFT)
		{
			if(buttons & _SC_BTNBIT_UP)
			{
				intended_angle = 8192 * 3;
			}
			else if(buttons & _SC_BTNBIT_DOWN)
			{
				intended_angle = 8192 * 5;
			}
			else
			{
				intended_angle = 8192 * 4;
			}
		}
		else if(buttons & _SC_BTNBIT_UP)
		{
			intended_angle = 8192 * 2;
		}
		else if(buttons & _SC_BTNBIT_DOWN)
		{
			intended_angle = 8192 * 6;
		}
		else
		{
			intended_angle = player_ang;
			nodirection = 1;
		}
		
		if(intended_angle - player_ang > 32768)
			intended_angle -= 65536;
		if(intended_angle - player_ang < -32768)
			intended_angle += 65536;
		
		player_ang *= 7;
		player_ang += intended_angle;
		player_ang /= 8;
		
		if( abs(intended_angle - player_ang) < 8192 && !nodirection)
		{
			//Accelerate
			player_vel[0] += sin16(player_ang) / 16;
			player_vel[1] += -cos16(player_ang) / 16;
		}
		
		
		if(buttons & _SC_BTNBIT_A)
		{
			//Shoot
			if(refire <= 0)
			{
				for(int bb = 0; bb < BULLET_MAX; bb++)
				{
					if(bullets[bb].timeleft == 0)
					{
						bullets[bb].vel[0] = player_vel[0] + (4 *  sin16(player_ang));
						bullets[bb].vel[1] = player_vel[1] + (4 * -cos16(player_ang));
						bullets[bb].pos[0] = player_pos[0];
						bullets[bb].pos[1] = player_pos[1];
						bullets[bb].timeleft = 100;
						refire += 10;
						break;
					}
				}
			}
		}
		
		if(refire > 0)
			refire--;
		
		
		//Cap and accumulate player momentum
		#define MAXVEL 256 * 65536
		for(int dd = 0; dd < 2; dd++)
		{
			if(player_vel[dd] > MAXVEL)
				player_vel[dd] = MAXVEL;
			if(player_vel[dd] < -MAXVEL)
				player_vel[dd] = -MAXVEL;
			
			player_pos[dd] += player_vel[dd];
		}
		
		#define MAXAVEL 256 * 65536
		if(player_avel > MAXAVEL)
			player_avel = MAXAVEL;
		if(player_avel < -MAXAVEL)
			player_avel = -MAXAVEL;
		
		player_ang += player_avel;
		player_ang = player_ang % 65536;
		
		//Decay momentum
		player_avel = (player_avel * 31) / 32;
		player_vel[0] = (player_vel[0] * 255) / 256;
		player_vel[1] = (player_vel[1] * 255) / 256;
	}
		
	//Move bullets
	for(int bb = 0; bb < BULLET_MAX; bb++)
	{
		if(bullets[bb].timeleft <= 0)
			continue;
		
		bullets[bb].pos[0] += bullets[bb].vel[0];
		bullets[bb].pos[1] += bullets[bb].vel[1];
		bullets[bb].timeleft--;
		
		//Bullets that are too far away get cleared
		int xout = abs(bullets[bb].pos[0] - player_pos[0]) > (512 * 65536);
		int yout = abs(bullets[bb].pos[1] - player_pos[1]) > (512 * 65536);
		if(xout || yout)
		{
			bullets[bb].timeleft = 0;
		}
	}
	
	//Move rocks
	for(int rr = 0; rr < ROCK_MAX; rr++)
	{
		if(rocks[rr].health <= 0)
			continue;
		
		rocks[rr].pos[0] += rocks[rr].vel[0];
		rocks[rr].pos[1] += rocks[rr].vel[1];
		rocks[rr].ang += rocks[rr].avel;
		rocks[rr].ang %= 65536;
	}
	
	//Move debris
	for(int dd = 0; dd < DEBRIS_MAX; dd++)
	{
		if(debris[dd].life <= 0)
			continue;
		
		debris[dd].pos[0] += debris[dd].vel[0];
		debris[dd].pos[1] += debris[dd].vel[1];
		debris[dd].ang += debris[dd].avel;
		debris[dd].life--;
	}
	
	//Move camera
	for(int dd = 0; dd < 2; dd++)
	{
		camvel[dd] += (player_pos[dd] - campos[dd]) / 512;
		camvel[dd] *= 31;
		camvel[dd] /= 32;
		campos[dd] += camvel[dd];
	}
	
	//Spin background flurry, keep in range of the player
	for(int yy = 0; yy < 8; yy++)
	{
		for(int xx = 0; xx < 8; xx++)
		{
			//Keep in X range of the player
			int xdiff = bgflurry[yy][xx][0] - player_pos[0];
			if(xdiff > 512 * 65536)
				bgflurry[yy][xx][0] -= 1024 * 65536;
			else if(xdiff < -512 * 65536)
				bgflurry[yy][xx][0] += 1024 * 65536;
			
			//Keep in Y range of the player
			int ydiff = bgflurry[yy][xx][1] - player_pos[1];
			if(ydiff > 512 * 65536)
				bgflurry[yy][xx][1] -= 1024 * 65536;
			else if(ydiff < -512 * 65536)
				bgflurry[yy][xx][1] += 1024 * 65536;
			
			
			//Spin
			bgflurry[yy][xx][2] += 16;
			bgflurry[yy][xx][2] &= 0xFFFF;
		}
	}
	
	//Check if the player has run into a rock
	if(spawntimer <= 0)
	{
		for(int rr = 0; rr < ROCK_MAX; rr++)
		{
			if(rocks[rr].health <= 0)
				continue;
			
			int diffx = (rocks[rr].pos[0] - player_pos[0]) >> 16;
			int diffy = (rocks[rr].pos[1] - player_pos[1]) >> 16;
			int dd = (diffx * diffx) + (diffy * diffy);
			if(dd < 25*25)
			{
				//Collision
				death();
				break;
			}
		}
	}
	
	//Respawn player
	if(spawntimer > 0)
	{
		spawntimer--;
	}
}

int dolevel(int levelnum)
{
	//Reset player position and momentum
	player_pos[0] = 0;
	player_pos[1] = 0;
	player_ang = 0;
	player_vel[0] = 0;
	player_vel[1] = 0;
	player_avel = 0;
	
	//Reset camera
	campos[0] = 0;
	campos[1] = 0;
	
	#if HIRES
		camscale = 1;
	#else
		camscale = 2;
	#endif
	
	camrecip = 65536 / camscale;
	
	//Reset background flurry
	for(int yy = 0; yy < 8; yy++)
	{
		for(int xx = 0; xx < 8; xx++)
		{
			bgflurry[yy][xx][0] = (xx * 128 * 65536) + (rand() % (128 * 65536));
			bgflurry[yy][xx][1] = (yy * 128 * 65536) + (rand() % (128 * 65536));
			bgflurry[yy][xx][2] = rand() % 65536;
		}
	}
	
	//Reset rocks
	memset(rocks, 0, sizeof(rocks));
	for(int ll = 0; ll < (levelnum + 1) * 4; ll++)
	{
		rockspawn();
	}
	
	int simulated = _sc_getticks();
	while(1)
	{
		//Run ticks
		int timenow = _sc_getticks();
		if(timenow > simulated + 1000)
			simulated = timenow - 1000;
		
		while(simulated < timenow)
		{
			gametick();
			simulated += 10;
			
			/*if(lives <= 0)
			{
				//Ran out of lives
				return 0;
			}*/
		}
		
		//Render
		gamedraw();
	}
	
	//Success
	return 1;
}


void mainmenu(void)
{

}

void gameover(void)
{
	
}

int main(void)
{
	while(1)
	{
		mainmenu();
		
		//lives = 3;
		int level = 0;
		while(dolevel(level)) { level++; }
		
		gameover();
	}
	
	return 0;
}
