/** @file frontend_sdl.c

  SDL2 frontend for Licar. This is the number one frontend, most advanced and
  with most features -- if a platform supports SDL2, this should probably be
  used.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sc.h>

#define LCR_SETTING_LOG_LEVEL 2
#define LCR_LOADING_COMMAND {}
#define LCR_FPS_GET_MS _sc_getticks()
#define LCR_SETTING_RESOLUTION_X 320
#define LCR_SETTING_RESOLUTION_Y 240
#define LCR_SETTING_RESOLUTION_SUBDIVIDE 1
#define LCR_SETTING_CAR_SHADOW 1
#define LCR_SETTING_CAR_ANIMATION_SUBDIVIDE 1
#define LCR_SETTING_PARTICLES 1
#define LCR_SETTING_TEXTURE_SUBSAMPLE 4
#define LCR_SETTING_FOG 0
#define LCR_SETTING_FPS 30

#include "frontend_helper.h"
#include "game.h"

static uint16_t screens[3][LCR_SETTING_RESOLUTION_X * LCR_SETTING_RESOLUTION_Y];
static int screen_back;

static FILE *musicFile = 0;

#define AUDIO_CHUNK 1024
static uint8_t audio_buf_lo[AUDIO_CHUNK]; //8-bit mono 8KHz
static uint16_t audio_buf_hi[AUDIO_CHUNK*6][2]; //16-bit stereo 48KHz

void audioFillCallback(void *userdata, uint8_t *s, int l)
{
  if (musicFile && LCR_gameMusicOn())
  {
    if (!fread(s,1,l,musicFile))
      rewind(musicFile);
  }
  else
    for (int i = 0; i < l; ++i)
      s[i] = 128;

  for (int i = 0; i < l; ++i)
    s[i] = s[i] / 2 + LCR_gameGetNextAudioSample() / 2;
}

static int pvmk_buttons = 0;
uint8_t LCR_keyPressed(uint8_t key)
{
	static const int mapping[256] = 
	{
		[LCR_KEY_UP]    = _SC_BTNBIT_UP,
		[LCR_KEY_RIGHT] = _SC_BTNBIT_RIGHT,
		[LCR_KEY_DOWN]  = _SC_BTNBIT_DOWN,
		[LCR_KEY_LEFT]  = _SC_BTNBIT_LEFT,
		[LCR_KEY_A]     = _SC_BTNBIT_A,
		[LCR_KEY_B]     = _SC_BTNBIT_B,
	};
	return ((pvmk_buttons & mapping[key]) != 0);
}

void LCR_sleep(uint16_t timeMs)
{
	int start = _sc_getticks();
	while(_sc_getticks() < start + timeMs)
		_sc_pause();
}

void LCR_drawPixel(unsigned long index, uint16_t color)
{
	screens[screen_back][index] = color;
}

int main(int argc, char *argv[])
{
	printf(
		"Licar, 3D racing game, v. " LCR_VERSION " (physics v. %c%c),"
		" SDL frontend, args:\n"
		"  -h   print help and quit\n"
		"  -wN  window (N = 1) or fullscreen (N = 0)\n"
		LCR_ARG_HELP_STR,LCR_RACING_VERSION1,LCR_RACING_VERSION2
	);

	openDataFile();

	musicFile = fopen("assets/music","rb");
	if (!musicFile)
		fputs("could not open music file\n",stderr);

	puts("initializing game");
	LCR_gameInit(argc,(const char **) argv);

	puts("starting game loop");
	while(1)
	{
		_sc_input_t input = {0};
		while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
		{
			if(input.format == 'A')
				pvmk_buttons = input.buttons;
		}

		if(!LCR_gameStep(_sc_getticks()))
		{
			LCR_gameEnd();
			exit(0);    
		}
		
		int displayed = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, &(screens[screen_back][0]));
		for(int bb = 0; bb < 3; bb++)
		{
			if(bb == screen_back)
			{
				//Don't use the one we just enqueued
				continue;
			}
			
			if((uintptr_t)displayed == (uintptr_t)(&(screens[bb][0])))
			{
				//Don't use the one currently displayed
				continue;
			}
			
			//Use this one
			screen_back = bb;
			break;
		}
		
		int played = _sc_snd_play(_SC_SND_MODE_48K_16B_2C, audio_buf_hi, sizeof(audio_buf_hi), 3*sizeof(audio_buf_hi));
		if(played >= 0)
		{
			audioFillCallback(NULL, audio_buf_lo, sizeof(audio_buf_lo));
			for(int ss = 0; ss < AUDIO_CHUNK; ss++)
			{
				for(int mm = 0; mm < 6; mm++)
				{
					audio_buf_hi[ (ss*6) + mm ][0] = audio_buf_lo[ss];
					audio_buf_hi[ (ss*6) + mm ][1] = audio_buf_lo[ss];
				}
			}
		}
	}
}
