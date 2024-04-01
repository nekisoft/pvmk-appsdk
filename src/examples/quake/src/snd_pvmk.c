/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "sound.h"

#include <sc.h>

static void SNDDMA_pump(void)
{
	while(1)
	{
		int submitted = _sc_snd_play(_SC_SND_MODE_48K_16B_2C, shm->buffer + (shm->samplepos*2), 1024, (shm->samples*2));
		if(submitted < 0)
			return;
		
		shm->samplepos += (1024/2);
		if(shm->samplepos >= (shm->samples))
			shm->samplepos -= shm->samples;
	}
}

qboolean SNDDMA_Init(void)
{
	fakedma = 1;
	return true;
}

int SNDDMA_GetDMAPos(void)
{
	SNDDMA_pump();
	return shm->samplepos;
}

void SNDDMA_Shutdown(void)
{
}

void SNDDMA_Submit(void)
{
	SNDDMA_pump();
}

