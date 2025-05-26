//subdiv.h
//Dynamic subdivision and clipping
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef SUBDIV_H
#define SUBDIV_H

#include "common.h"

//Takes a transformed triangle, subdivides, clips it, and feeds it to the span code
void subdiv_tri(int tex,
	const fix24p8_t *va4, const int *ta2, 
	const fix24p8_t *vb4, const int *tb2,
	const fix24p8_t *vc4, const int *tc2);

#endif //SUBDIV_H
