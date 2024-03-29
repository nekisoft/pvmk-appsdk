/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */


#include <unistd.h>
#include <string.h>

#include <sc.h>

#include "../source/globals.h"
#include "sblaster.h"
#include "../source/gamelib/soundmix.h"

#define SB_CHUNK_LEN 512
int16_t sb_buffer[SB_CHUNK_LEN][2];

void SB_exit()
{
}

void SB_playstop()
{
	
}

//void update_sample(unsigned char *buf, int size)
//update_sample((u8 *) &priv[0].pcmout[priv[0].pcmout_pos][priv[0].pcm_indx], MAX_PCMOUT);

int SB_playstart(int bits, int samplerate)
{
	return 1;
}

void SB_setvolume(char dev, char volume)
{
	(void)dev;
	(void)volume;
}

void SB_updatevolume(int volume)
{
	(void)volume;
}


void SB_pump(void)
{
	while(_sc_snd_play(_SC_SND_MODE_48K_16B_2C, sb_buffer, sizeof(sb_buffer), 16*sizeof(sb_buffer)) >= 0)
	{
		//Enqueued a chunk of audio, need to generate the next one
		update_sample(sb_buffer, sizeof(sb_buffer));
	}
}