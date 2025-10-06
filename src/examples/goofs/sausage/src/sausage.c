//sausage.c
//SAUSAGE TOSSER
//Bryan E. Topp <betopp@betopp.com> 2025

#include <sc.h>
#include <stdlib.h>
#include <stddef.h>
#include "fbs.h"
#include "images.h"
#include "pads.h"
#include "trigfunc.h"
#include "font.h"

void title(void)
{
	images_purge();
	images_loadrange(IMF_TITLE_AAA, IMF_TITLE_ZZZ);
	
	int starttime = _sc_getticks();
	while(1)
	{
		int tt = _sc_getticks() - starttime;
		
		images_draw(IMF_TITLE_BG, 0, 0);
		
		images_draw(IMF_TITLE_SAUSAGE, 140 + trigfunc_sin8(tt * 64)/8, 70);
		images_draw(IMF_TITLE_TOSSER, 140 + trigfunc_cos8(tt * 64)/8, 130);
		
		images_draw(IMF_TITLE_SAUSAGES, 000, 480 - trigfunc_cos8((tt+1024*3)*6)/4);
		images_draw(IMF_TITLE_SAUSAGES, 100, 480 - trigfunc_cos8((tt+1024*4)*6)/4);
		images_draw(IMF_TITLE_SAUSAGES, 200, 480 - trigfunc_cos8((tt+1024*5)*6)/4);
		images_draw(IMF_TITLE_SAUSAGES, 300, 480 - trigfunc_cos8((tt+1024*6)*6)/4);
		images_draw(IMF_TITLE_SAUSAGES, 400, 480 - trigfunc_cos8((tt+1024*7)*6)/4);
		images_draw(IMF_TITLE_SAUSAGES, 500, 480 - trigfunc_cos8((tt+1024*8)*6)/4);
		
		font_draw(FS_FAT, "PRESS START", 0x1111, 222, 302 + trigfunc_sin8((tt*33))/64);
		font_draw(FS_FAT, "PRESS START", 0xEEEE, 220, 300 + trigfunc_sin8((tt*33))/64);
		
		fbs_flip();
		if(pads_detect(PAD_ANY, BTNBIT_START))
			return;
	}
}


//Table of target types in the game
typedef struct target_type_s
{
	images_file_t file;
	const char *name;
	int score;
} target_type_t;
target_type_t target_type_table[] = 
{
	{0},
	{ .file = IMF_GAME_TARGET0, .name = "Capicola",    .score = 100 },
	{ .file = IMF_GAME_TARGET1, .name = "Soppressata", .score = 200 },
	{ .file = IMF_GAME_TARGET2, .name = "Prosciutto",  .score = 300 },
	{ .file = IMF_GAME_TARGET3, .name = "Spam",        .score = -10 },
	{0}
};

//Targets on the playfield
#define FIELD_TILE 32
#define FIELD_COLS (640/FIELD_TILE)
#define FIELD_ROWS (480/FIELD_TILE)
int field[FIELD_ROWS][FIELD_COLS];

//Vertical scrolling of the playfield
int fieldscroll;

//Whether we've failed the game and should play game-over
int gamelost;
int gamewon;

//Player's aiming angle (65536 = full circle)
int plangle;

//Sausages in flight
#define SAUSAGE_MAX 8
typedef struct sausage_s
{
	int valid;
	int pos[2];
	int vel[2];
	int spin;
	int svel;
} sausage_t;
sausage_t sausage_table[SAUSAGE_MAX];

//Refire timer
int refire;


//Draws sausages
void drawsausages(void)
{
	for(int ss = 0; ss < SAUSAGE_MAX; ss++)
	{
		if(!sausage_table[ss].valid)
			continue;
		
		images_draw(IMF_GAME_SAUSAGE0 + ((sausage_table[ss].spin / 16384)%4), 
			sausage_table[ss].pos[0] / 256,
			sausage_table[ss].pos[1] / 256);
	}
}

//Draws the targets on the field
void drawfield(void)
{
	for(int rr = 0; rr < FIELD_ROWS; rr++)
	{
		for(int cc = 0; cc < FIELD_COLS; cc++)
		{
			if(field[rr][cc] == 0)
				continue;
		
			int xx = cc * FIELD_TILE;
			int yy = (rr * FIELD_TILE) + (fieldscroll >> 16) - FIELD_TILE;
			images_draw(target_type_table[field[rr][cc]].file, xx, yy);
		}
	}	
}

//Draws the player character
void drawpc(void)
{
	//Compute which rotated graphic to use
	int step = (plangle+512) * 8 / 16384;
	if(step < 0)
		step = 0;
	if(step > 7)
		step = 7;
	
	//Paint rotated graphic
	images_draw(IMF_GAME_LAUNCH0 + step, 0, 480-96);
	
	//Paint aiming line
	for(int dd = 128; dd < 512; dd += 32)
	{
		int xx = trigfunc_cos8(plangle) * dd / 256;
		int yy = -trigfunc_sin8(plangle) * dd / 256;
		images_draw(IMF_GAME_DOT, xx + 64-8, yy + 480-32);
	}
	
}

//Spawns a new projectile
void shoot(void)
{



	sausage_t *sptr = NULL;
	for(int ss = 0; ss < SAUSAGE_MAX; ss++)
	{
		if(!sausage_table[ss].valid)
		{
			sptr = &(sausage_table[ss]);
			break;
		}
	}
	if(sptr == NULL)
		return;
	
	sptr->valid = 1;
	sptr->pos[0] = 60 << 8;
	sptr->pos[1] = 420 << 8;
	sptr->vel[0] = trigfunc_cos8(plangle) * 6;
	sptr->vel[1] = -trigfunc_sin8(plangle) * 6;
	sptr->pos[0] += sptr->vel[0] * 8;
	sptr->pos[1] += sptr->vel[1] * 8;
	sptr->spin = 0;
	sptr->svel = 2000;
	
	refire += 30;
}

void gametick(void)
{
	//Advance field scrolling
	fieldscroll += 10000;
	if( (fieldscroll >> 16) > FIELD_TILE )
	{
		//See if any targets reached the bottom
		for(int cc = 0; cc < FIELD_COLS; cc++)
		{
			if(field[FIELD_ROWS-1][cc] != 0)
			{
				//Game over!
				gamelost = 1;
				fieldscroll = FIELD_TILE << 16;
				return;
			}
		}
		
		//Reset the less-than-one-tile scroll
		fieldscroll -= FIELD_TILE << 16;
		
		//Move the whole field down instead
		for(int rr = FIELD_ROWS-1; rr > 0; rr--)
		{
			for(int cc = 0; cc < FIELD_COLS; cc++)
			{
				field[rr][cc] = field[rr-1][cc];
			}
		}
		
		//Figure out how many types of target we can spawn
		int ntargets = 1;
		while(target_type_table[ntargets].name != NULL)
		{
			ntargets++;
		}
		ntargets--; //First row doesn't count
		
		//Figure out how many columns of target we make - one more than already exists
		int mincol = 0;
		for(int cc = FIELD_COLS - 1; cc >= 0; cc--)
		{
			if(field[0][cc] == 0)
			{
				mincol = cc;
				break;
			}
		}
		
		//Spawn new targets
		for(int cc = mincol; cc < FIELD_COLS; cc++)
		{
			field[0][cc] = 1 + (rand()%ntargets);
		}
	}
	
	//Update sausages
	for(int ss = 0; ss < SAUSAGE_MAX; ss++)
	{
		sausage_t *sptr = &(sausage_table[ss]);
		if(!sptr->valid)
			continue;
		
		sptr->pos[0] += sptr->vel[0];
		sptr->pos[1] += sptr->vel[1];
		
		sptr->vel[1] += 10;
		
		sptr->spin += sptr->svel;
		
		if(sptr->pos[1] > (SCREENY<<8))
			sptr->valid = 0;
		
	}
	
	//Let player adjust angle
	if(pads[PAD_A] & BTNBIT_UP)
	{
		plangle += 100;
	}
	if(pads[PAD_A] & BTNBIT_DOWN)
	{
		plangle -= 100;
	}
	if(plangle < 0)
	{
		plangle = 0;
	}
	if(plangle >= 16384)
	{
		plangle = 16384 - 1;
	}
	
	//Let them fire
	if(refire <= 0)
	{
		if(pads[PAD_A] & BTNBIT_A)
			shoot();
	}
	else
	{
		refire--;
	}
}

void lose(void)
{
	
}

void win(void)
{
	
}

void game(void)
{
	images_purge();
	images_loadrange(IMF_GAME_AAA, IMF_GAME_ZZZ);
	
	int starttime = _sc_getticks();
	int simtime = starttime;
	gamelost = 0;
	gamewon = 0;
	while(1)
	{
		//Run game updates
		int tt = _sc_getticks() - starttime;
		if(tt < simtime || tt > simtime + 1000)
			simtime = tt;
		while(tt > simtime)
		{
			gametick();
			simtime += 10;
		}
		
		//If we lost the game, show the "game lost" animation
		if(gamelost)
		{
			lose();
			return;
		}
		
		//If we won, show the "game won" animation
		if(gamewon)
		{
			win();
			return;
		}
		
		//Draw game
		images_draw(IMF_GAME_BG, 0, 0);
		drawfield();
		drawpc();
		drawsausages();
		fbs_flip();
	}
}

int main(void)
{
	while(1)
	{
		title();
		game();
	}
	return 0;
}
