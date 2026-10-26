//prof.h
//Instruction-attribution profiler for the Neki32 simulator.
//
//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _PROF_H
#define _PROF_H

#include <stdint.h>

//Off unless NEMUL_PROF names an output prefix. NEMUL_PROF_FROM and NEMUL_PROF_TO are
//milliseconds of emulated time, so a run can skip the boot and the loading and measure the
//thing that is actually slow.
extern int prof_enabled;

//Called once per emulated millisecond by the front end, with the time it is emulated to.
//Turns sampling on and off at the window edges.
void prof_tick(long ms, int quiet);

//Counts one execution at a program counter. The whole cost in the instruction loop is a load
//and a branch when the profiler is off, which is why it is inlined.
void prof_count(int idx, uint32_t pc);

static inline void prof_hit(int idx, uint32_t pc)
{
	if(prof_enabled)
		prof_count(idx, pc);
}

//Counts a system-call by number, within the recording window, so the picture includes what
//leaves emulated code - and so a frame count can be divided into the instruction count.
void prof_sysc_hit(uint32_t nr);

//Writes the histogram out. Called on the way out of the run.
void prof_dump(void);

#endif //_PROF_H
