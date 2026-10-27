//disasm.h
//ARM instruction decoder for the simulator's debugger
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _DISASM_H
#define _DISASM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//Longest text this decoder will produce for one instruction
#define DISASM_MAX 96

//Decodes one ARM-state instruction, spelled the way GNU objdump spells it so that anyone who
//has read a listing before can read this.
//
//addr is where the instruction sits, which matters for PC-relative branch targets.
//Returns 0 when the word decodes, or -1 when this decoder does not know it - the caller should
//then show the raw word.
//When the instruction goes somewhere by itself (b, bl, blx) *flow is set to where.
int disasm_arm(uint32_t addr, uint32_t ir, char *out, size_t outsz, uint32_t *flow);

//Human-readable name of the register, "r0".."pc", for register views
const char *disasm_regname(int reg);

#endif //_DISASM_H
