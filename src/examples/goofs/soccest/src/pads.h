//pads.h
//Player gamepad buttons
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef PADS_H
#define PADS_H

#include <stdint.h>

//Updates input from system to generate new pad state
void pads_update(void);

//Returns latest pad state
uint16_t pads_get(int player);

#endif //PADS_H
