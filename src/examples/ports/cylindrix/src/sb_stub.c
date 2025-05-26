/*
    Copyright (C) 2001 Hotwarez LLC, Goldtree Enterprises
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; 
    version 2 of the License.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <sc.h>

#include <memory.h>

#include "pcx.h"
#include "jonsb.h"

#include "sb_lib.h"
#include "types.h"
#include "config.h"
#include "prim.h"

uint16_t pvmk_palette[256];

/* if game is to be installed globally, change this to the games data direcoty */
//#define CONFIG_PATH "./"

extern game_configuration_type game_configuration;

//Johnm 9/5/2002 - Beginning to move to a configurable resolution
const int g_XResolution = 640;
const int g_YResolution = 480;

extern long program_over;    /* TRUE when user wishes to leave the program */

/* Base path of data files */
char g_DataPath[255];

void MainLoop( int argc, char *argv[] );

int main(  int argc, char *argv[]  ) {
	
	/* Modify this line to wherever the appropriate hard-coded data path should be */
	sprintf(g_DataPath, "./");
	Load_Game_Configuration( &game_configuration );
	MainLoop(  argc, argv );
	return(0);
}


//Johnm 12/2/2001 - these are just stubs to make Cylindrix compile without
//Sound library

void sb_free_mod_file(sb_mod_file *pmod) {
	(void)pmod;
}

void sb_mod_play(sb_mod_file *pmod) {
	(void)pmod;
}

void sb_mod_pause(void) {

}


/* Pass: Pointer to sb_sample structure                                       */
/* Returns:                                                                   */
void sb_free_sample(sb_sample *sample) {
	jon_sb_free_sample(sample);
}

#define SAMPLE uint16_t

extern boolean voice_done;
SAMPLE *g_SampleQueue[MAX_VOICE_SAMPLES];
int g_nCurrentQueue = 0;
int g_nCurrentVoice = -1;

int voice_check(int what)
{
	(void)what;
	return 0;
}

//Hack function that's called from the buffer swapping to 
//Update Queue'd samples...
void Hack_Update_Queue( void ) {
	/*
	if( g_nCurrentVoice == -1 )
		voice_done = 1;
	else if( !voice_check(g_nCurrentVoice) ) {
		if( g_nCurrentQueue > 0 ) {
			g_nCurrentQueue--;
			g_nCurrentVoice = allocate_voice(g_SampleQueue[g_nCurrentQueue]);
			if( g_nCurrentVoice != -1 ) {
				voice_start(g_nCurrentVoice);
				voice_set_priority(g_nCurrentVoice, 255);
				release_voice(g_nCurrentVoice);
			}
		}
		else {
			voice_done = 1;
			g_nCurrentVoice = -1;
		}
	}
	*/
	
	voice_done = 1;
	g_nCurrentVoice = -1;
}

/* Pass: Pointer to sb_sample structure created by sb_load_sample()           */
/* Returns: status of attempt as an sb_status enum.                           */
sb_status sb_queue_sample(sb_sample *sample) {
	(void)sample;
	return SB_SUCCESS;
	/*
	
	int bAdd = 0;

	if( g_nCurrentVoice == -1 ) {
		bAdd = 1;
	}
	else if( !voice_check(g_nCurrentVoice ) ) {
		bAdd = 1;			
	}

	if( bAdd ) {
		g_nCurrentVoice = allocate_voice((SAMPLE *)sample->data);
		voice_start(g_nCurrentVoice);
		voice_set_priority(g_nCurrentVoice, 255);
		release_voice(g_nCurrentVoice);
	}
	else {
		g_SampleQueue[g_nCurrentQueue++] = (SAMPLE *)sample->data;
	}

	return(SB_SUCCESS);
	*/
}


/* Pass: Pointer to file-name string; format of file (see sb_defs.h)          */
/* Returns: Pointer to allocated sb_sample structure                          */
sb_sample sample_crap;
sb_sample *sb_load_sample(char *pFoo, int nFoo) {
	(void)pFoo;
	(void)nFoo;
	//return NULL;
	return &sample_crap;
	
	/*
	sb_sample *pSample;
	char newfilename[512];

	sprintf(newfilename,"%s%s",g_DataPath,pFoo);


	pSample = 0;

	pSample = malloc(sizeof(sb_sample));
	memset(pSample, 0, sizeof(sb_sample));

	pSample->data = (char *)load_sample(newfilename); //(SAMPLE *)load_sample(pFoo);

	return(pSample);*/
}

sb_mod_file *sb_load_mod_file(char *pFoo) {
	(void)pFoo;
	return(0);
}


/* Pass: Pointer to sb_sample structure created by sb_load_sample()           */
/* Returns:                                                                   */
void sb_mix_sample(sb_sample *psample) {
	//Need to change this to support panning
	//int nVolume = (psample->left_volume + psample->right_volume) / 2;
	
	(void)psample;
	//play_sample((SAMPLE *)(psample->data), nVolume, 120, 1000, 0);

}

void New_Play_Menu_Sound( BYTE *data ) {
	//play_sample((SAMPLE *)data, 255, 120, 1000, 0);
	(void)data;
}

sb_sample *jon_sb_load_sample(char *fname, int format) {
	return(sb_load_sample(fname, format));
}

void jon_sb_free_sample(sb_sample *ss) {
	(void)ss;
	
	
	//if( ss ) {
	//	if( ss->data )
	//		destroy_sample((SAMPLE *)ss->data);
	//	free(ss);
	//}
}

char sb_driver_error[80];


/* Pass:                                                                      */
/* Returns:                                                                   */
void sb_uninstall_driver(void) {
	//remove_sound();
}

/* Pass: Frequency (Hz) all your samples will be running at.                  */
/* Returns: sb_status enum indicating what happened (see sb_defs.h)           */
sb_status sb_install_driver(int x) {
(void)x;
 //  if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
//	  return(1);
  // }

	return(SB_SUCCESS);
}

int getkey(void) {
	return(0);
}
int _conio_kbhit(void) {
	return(0);
}

void gotoxy( int x, int y ) {
	(void)x; (void)y;
}

void textcolor( int nColor ) {
	(void)nColor;
}

void textbackground( int nColor) {
	(void)nColor;
}

//pvmk implementation of palette
typedef struct { unsigned char r; unsigned char g; unsigned char b; } PALETTE[256];
uint16_t pvmk_palette[256];
void set_palette(PALETTE p)
{
	for(int ii = 0; ii < 256; ii++)
	{
		pvmk_palette[ii] = 0;
		
		pvmk_palette[ii] <<= 5;
		pvmk_palette[ii] |= (p[ii].r & 0x3F) >> 1;
		
		pvmk_palette[ii] <<= 6;
		pvmk_palette[ii] |= (p[ii].g & 0x3F) >> 0;
		
		pvmk_palette[ii] <<= 5;
		pvmk_palette[ii] |= (p[ii].b & 0x3F) >> 1;
	}
}

//THIS IS GOING TO AFFECT EVERYTHING!!!
void delay( int nFoo ) {
	
	int start = _sc_getticks();
	while(_sc_getticks() < start + nFoo)
	{
		_sc_pause();
	}
}


void fade_out(int what)
{
	PALETTE pp = {0};
	for(int col = 0; col < 256; col++)
	{
		pp[col].b = ((pvmk_palette[col] >> 0) & 0x1F) << 1;
		pp[col].g = ((pvmk_palette[col] >> 5) & 0x3F) << 0;
		pp[col].r = ((pvmk_palette[col] >> 11) & 0x1F) << 1;
	}
	
	PALETTE pp_scaled = {0};
	for(int step = 0; step <= what; step++)
	{
		int frac = 256 * (what-step) / what;
		for(int col = 0; col < 256; col++)
		{
			pp_scaled[col].r = (pp[col].r * frac) / 256;
			pp_scaled[col].g = (pp[col].g * frac) / 256;
			pp_scaled[col].b = (pp[col].b * frac) / 256;
		}
		set_palette(pp_scaled);
		Swap_Buffer();
		delay(20);
	}
}
void fade_in(PALETTE pp, int what)
{
	PALETTE pp_scaled = {0};
	for(int step = 0; step <= what; step++)
	{
		int frac = 256 * step / what;
		for(int col = 0; col < 256; col++)
		{
			pp_scaled[col].r = (pp[col].r * frac) / 256;
			pp_scaled[col].g = (pp[col].g * frac) / 256;
			pp_scaled[col].b = (pp[col].b * frac) / 256;
		}
		set_palette(pp_scaled);
		Swap_Buffer();
		delay(20);
	}
}


void New_Brighten_Palette( pcx_picture_ptr image) {
	PALETTE obPalette;
	int i;

	for( i = 0; i < 256; i++ ) {
		obPalette[i].r = image->palette[i].red;
		obPalette[i].g = image->palette[i].green;
		obPalette[i].b = image->palette[i].blue;
	}

	fade_in(obPalette, 15);
}

void New_Enable_Palette( palette_type palette  ) {
	PALETTE obPalette;
	int i;

	for( i = 0; i < 256; i++ ) {
		obPalette[i].r = palette[i].red;
		obPalette[i].g = palette[i].green;
		obPalette[i].b = palette[i].blue;
	}

	set_palette(obPalette);

}

void New_Fade_Palette( void ) {
	fade_out(15);
}


