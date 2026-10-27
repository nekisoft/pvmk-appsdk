//symbols.h
//Function names and source line positions read from an emulated program's ELF file
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//The card carries a flat image with no names in it, so a debugger looking at the machine can
//only show addresses. The ELF that the image was made from still has both: its symbol table
//names functions, and its DWARF line table says which line of which file each address came from.
//This reads those two out of the file with no outside help, which is what makes source-level
//debugging possible without pulling in a library.

#ifndef _SYMBOLS_H
#define _SYMBOLS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct symbols_s symbols_t;

//Loads symbols from an ELF file. Returns NULL if it is not one this understands.
symbols_t *symbols_load(const char *path);
void symbols_free(symbols_t *syms);

//Finds the function containing an address. Returns its name, or NULL. The start and size of the
//function are written to the out pointers when they are given. When several symbols share an
//address the largest one wins, so that a jump into a loop label still names the routine.
const char *symbols_func(symbols_t *syms, uint32_t addr, uint32_t *start_out, uint32_t *size_out);

//Finds the source position of an address. Returns false when the file has no line for it.
bool symbols_line(symbols_t *syms, uint32_t addr, const char **file_out, int *line_out);

//What the file gave up, for the debugger to report
void symbols_stats(symbols_t *syms, int *functions, int *lines);

#endif //_SYMBOLS_H
