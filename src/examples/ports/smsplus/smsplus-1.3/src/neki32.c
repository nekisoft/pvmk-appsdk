
#include <sc.h>
#include "shared.h"

#include <stdint.h>
#include <stdbool.h>

//BITMAP *bmp;
//PALETTE pal;

bool time_to_exit = false;

uint16_t fbs[2][240][320];
int fbnext;

int16_t audio_buf[1024][2];

void neki_mix(int16 **stream, int16 **output, int sample_count)
{
	sound_mixer_callback(stream, output, sample_count);
	
	for(int ll = 0; ll < sample_count && ll < 1024; ll++)
	{
		audio_buf[ll][0] = output[0][ll];
		audio_buf[ll][1] = output[1][ll];
	}
	//while(_sc_snd_play(_SC_SND_MODE_48K_16B_2C, audio_buf, sample_count*4, sample_count*4*3) == -_SC_EAGAIN) { _sc_pause(); }
	
	//Play with maxbuf to get the sound buffer exactly full
	//If we've fallen too far behind, we double-up
	//But if we're already where we want to be, the first call fails
	_sc_snd_play(_SC_SND_MODE_48K_16B_2C, audio_buf, sample_count*4, sample_count*4*2);
	_sc_snd_play(_SC_SND_MODE_48K_16B_2C, audio_buf, sample_count*4, sample_count*4*8);
}

void osd_update_input(void)
{
	_sc_input_t ii = {0};
	while(_sc_input(&ii, sizeof(ii), sizeof(ii)) > 0)
	{		
		if(ii.format == 'A')
		{
			input.system = 0;
			for(int button = 0; button < 16; button++)
			{
				unsigned short state = ii.buttons & (1u << button);
				
				static const int sysmapping[16] = 
				{
					[_SC_BTNIDX_START]     = INPUT_START,
					[_SC_BTNIDX_C]         = INPUT_PAUSE,
					[_SC_BTNIDX_Z]         = INPUT_RESET,
				};
				
				input.system |= state ? sysmapping[button] : 0;
			}
			
			if(ii.buttons & _SC_BTNBIT_Z)
				system_reset();
		
			//if(ii.buttons & _SC_BTNBIT_SELECT)
			//	time_to_exit = true;
		}
		
		if(ii.format == 'A' || ii.format == 'B')
		{
			input.pad[(ii.format == 'A') ? 0 : 1] = 0;
			for(int button = 0; button < 16; button++)
			{
				unsigned short state = ii.buttons & (1u << button);
				
				static const int mapping[16] = 
				{
					[_SC_BTNIDX_UP]    = INPUT_UP,
					[_SC_BTNIDX_DOWN]  = INPUT_DOWN,
					[_SC_BTNIDX_LEFT]  = INPUT_LEFT,
					[_SC_BTNIDX_RIGHT] = INPUT_RIGHT,
					[_SC_BTNIDX_A]     = INPUT_BUTTON1,
					[_SC_BTNIDX_B]     = INPUT_BUTTON2,
				};
				
				input.pad[(ii.format == 'A') ? 0 : 1] |= state ? mapping[button] : 0;
			}
		}
	}
}

void system_manage_sram(uint8 *sram, int slot, int mode)
{
    (void)sram;
	(void)slot;
	(void)mode;
}

int main (int argc, char *argv[])
{
    if(argc < 2)
    {
        strcpy(game_name, "default.sms");
    }
    else
    {
        strcpy(game_name, argv[1]);
    }

    /* Attempt to load game off commandline */
    if(load_rom(game_name) == 0)
    {
        printf("Error loading `%s'.\n", game_name);
        exit(1);
    }


  //  if(sms.console == CONSOLE_GG)
  //  {
  //      set_gfx_mode(GFX_AUTODETECT_WINDOWED, 160*4, 144*4, 0, 0);
  //  }
  //  else
  //  {
  //      set_gfx_mode(GFX_AUTODETECT_WINDOWED, 512, 384, 0, 0);
  //  }

   // clear(screen);
  //  bmp = create_bitmap_ex(16, 256, 256);
   // clear(bmp);

    /* Set up bitmap structure */

    
    memset(&bitmap, 0, sizeof(bitmap_t));
    bitmap.width  = 320;
    bitmap.height = 240;
    bitmap.depth  = 16;
    bitmap.granularity = 2;
    bitmap.pitch  = bitmap.width * bitmap.granularity;
    bitmap.data   = (uint8 *)&fbs[0][24][32];
    bitmap.viewport.x = 32;
    bitmap.viewport.y = 16;
    bitmap.viewport.w = 256;
    bitmap.viewport.h = 192;

    snd.fm_which = SND_EMU2413;
    snd.fps = (1) ? FPS_NTSC : FPS_PAL;
    snd.fm_clock = (1) ? CLOCK_NTSC : CLOCK_PAL;
    snd.psg_clock = (1) ? CLOCK_NTSC : CLOCK_PAL;
    snd.sample_rate = 48000;
    snd.enabled = 1;
    snd.mixer_callback = neki_mix;

    sms.territory = 0;
    sms.use_fm = 0;

    system_init();

    sms.territory = 0;
    sms.use_fm = 0;

    system_poweron();


    while(!time_to_exit)
    {
        osd_update_input();
        system_frame(0);              
       
	while(_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[fbnext]) != (int)(fbs[fbnext])) { _sc_pause(); }
	fbnext = fbnext ? 0 : 1;
	bitmap.data = (uint8 *)&fbs[fbnext][24][32];
    }

    system_poweroff();
    system_shutdown();

    return 0;
}
//END_OF_MAIN();
