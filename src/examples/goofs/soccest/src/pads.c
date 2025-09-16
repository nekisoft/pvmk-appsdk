//pads.c
//Player gamepad buttons
//Bryan E. Topp <betopp@betopp.com> 2025

#include "pads.h"
#include <sc.h>

uint16_t pads[5];
static uint16_t pads_edge_last[5];
static uint16_t pads_detect_press[5];
static uint16_t pads_detect_taken[5];

void pads_update(void)
{
	_sc_input_t ii = {0};
	while(_sc_input(&ii, sizeof(ii), sizeof(ii)) > 0)
	{
		if(ii.format == 'A')
			pads[PAD_A] = ii.buttons;
		if(ii.format == 'B')
			pads[PAD_B] = ii.buttons;
		if(ii.format == 'C')
			pads[PAD_C] = ii.buttons;
		if(ii.format == 'D')
			pads[PAD_D] = ii.buttons;
	}
	
	pads[PAD_ANY] = pads[PAD_A] | pads[PAD_B] | pads[PAD_C] | pads[PAD_D];
	
	for(int pp = 0; pp < 5; pp++)
	{
		pads_detect_press[pp] = pads[pp];
		pads_detect_taken[pp] &= pads[pp];
	}
}

uint16_t pads_edge(int player)
{
	uint16_t now = pads[player];
	uint16_t then = pads_edge_last[player];
	
	pads_edge_last[player] = pads[player];
	
	return now & ~then;
}

bool pads_detect(int player, int button)
{
	if( (pads_detect_press[player] & ~pads_detect_taken[player]) & button)
	{
		pads_detect_taken[player] |= button;
		return true;
	}
	else
	{
		return false;
	}
}
