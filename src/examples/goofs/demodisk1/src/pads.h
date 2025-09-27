//pads.h
//Player gamepad buttons
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef PADS_H
#define PADS_H

#include <stdint.h>
#include <stdbool.h>
#include <sc.h>

//Pad states
#define PAD_A 0
#define PAD_B 1
#define PAD_C 2
#define PAD_D 3
#define PAD_ANY 4
extern uint16_t pads[5];

//Updates input from system to generate new pad state
void pads_update(void);

//Returns buttons newly pressed (since the last call)
uint16_t pads_edge(int player);

//Consumes a single button press
bool pads_detect(int player, int button);

//Buttons
#define BTNBIT_A _SC_BTNBIT_A
#define BTNBIT_B _SC_BTNBIT_B
#define BTNBIT_C _SC_BTNBIT_C
#define BTNBIT_X _SC_BTNBIT_X
#define BTNBIT_Y _SC_BTNBIT_Y
#define BTNBIT_Z _SC_BTNBIT_Z
#define BTNBIT_START _SC_BTNBIT_START
#define BTNBIT_MODE _SC_BTNBIT_MODE
#define BTNBIT_LEFT _SC_BTNBIT_LEFT
#define BTNBIT_RIGHT _SC_BTNBIT_RIGHT
#define BTNBIT_UP _SC_BTNBIT_UP
#define BTNBIT_DOWN _SC_BTNBIT_DOWN

#endif //PADS_H
