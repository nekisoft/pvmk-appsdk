//pads.c
//Player gamepad buttons
//Bryan E. Topp <betopp@betopp.com> 2025

#include "pads.h"
#include <sc.h>

uint16_t pads_states[4];

void pads_update(void)
{
	_sc_input_t ii = {0};
	while(_sc_input(&ii, sizeof(ii), sizeof(ii)) > 0)
	{
		if(ii.format >= 'A' && ii.format <= 'D')
			pads_states[ii.format - 'A'] = ii.buttons;
	}
}

uint16_t pads_get(int player)
{
	return pads_states[player];
}
