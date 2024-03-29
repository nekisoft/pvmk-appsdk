/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include <stdio.h>
#include "pvmkport.h"
#include "../version.h"
#include "control.h"
#undef MIN
#undef MAX
#include "../openbor.h"

#define	PAD_START			1
#define	MAX_PADS			4
#define	PAD_END				(MAX_BUTTONS*MAX_PADS)

static int hwbutton = 0;
static int lastkey[MAX_PADS];
static int port[MAX_PADS];
static const char *padnames[PAD_END+1+1] = {
	"...",
#define CONTROLNAMES(x)    \
	x" L",             \
	x" Up",            \
	x" R",             \
	x" Left",          \
	x" Down",          \
	x" Right",         \
	x" X",             \
	x" Y",             \
	x" Z",             \
	x" A",             \
	x" B",             \
	x" C",             \
	x" Start",         \
	x" Select",        \
	x" Power",        	
	CONTROLNAMES("P1")
	CONTROLNAMES("P2")
	CONTROLNAMES("P3")
	CONTROLNAMES("P4")
	"undefined"
};

static int flag_to_index(unsigned long flag)
{
	int index = 0;
	unsigned long bit = 1;
	while(!((bit<<index)&flag) && index<31) ++index;
	return index;
}

void control_exit()
{

}

void control_init(int joy_enable)
{
	(void)joy_enable;
}

int control_usejoy(int enable)
{
	(void)enable;
	return 0;
}

int control_getjoyenabled()
{
	return 1;
}

int keyboard_getlastkey(void)
{
	int i, ret=0;
	for(i=0; i<MAX_PADS; i++)
	{
		ret |= lastkey[i];
		lastkey[i] = 0;
	}
	return ret;
}

void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key)
{
	if(!pcontrols) return;
	pcontrols->settings[flag_to_index(flag)] = key;
	pcontrols->keyflags = pcontrols->newkeyflags = 0;
}

// Scan input for newly-pressed keys.
// Return value:
// 0  = no key was pressed
// >0 = key code for pressed key
// <0 = error
int control_scankey()
{
	static unsigned ready = 0;
	unsigned i, k=0;

	for(i=0; i<MAX_PADS; i++)
	{
		if(lastkey[i])
		{
			k = 1 + i*MAX_BUTTONS + flag_to_index(lastkey[i]);
			break;
		}
	}

	if(ready && k)
	{
		ready = 0;
		return k;
	}
	ready = (!k);
	return 0;
}

char * control_getkeyname(unsigned keycode)
{
	if(keycode >= PAD_START && keycode <= PAD_END) return (char*)padnames[keycode];
	return "...";
}

void control_update(s_playercontrols ** playercontrols, int numplayers)
{
	_sc_input_t input = {0};
	while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
	{
		int pad = input.format - 'A';
		if(pad < 0 || pad >= MAX_PADS)
			continue;
		
		port[pad] = input.buttons;
		lastkey[pad] |= input.buttons;
	}
	
	for(int player=0; player<numplayers; player++)
	{
		s_playercontrols *pcontrols = playercontrols[player];
		int k = 0;
		for(int i=0; i<32; i++)
		{
			int t = pcontrols->settings[i];
			if(t >= PAD_START && t <= PAD_END)
			{
				int portnum = (t-1) / MAX_BUTTONS;
				int shiftby = (t-1) % MAX_BUTTONS;
				if(portnum >= 0 && portnum <= 3)
				{
					if((port[portnum] >> shiftby) & 1) k |= (1<<i);
				}
			}
		}
		pcontrols->kb_break = 0;
		pcontrols->newkeyflags = k & (~pcontrols->keyflags);
		pcontrols->keyflags = k;
	}
}

void control_rumble(int port, int ratio, int msec)
{
	(void)port;
	(void)ratio;
	(void)msec;
}

