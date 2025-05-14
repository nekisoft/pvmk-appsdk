//trace.h
//Debug tracing for simulation
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _TRACE_H
#define _TRACE_H

#include <stdio.h>

extern int tracing;

//Execution tracing macro
#define TRACE(x, ...) do{if(tracing){fprintf(stderr, x, __VA_ARGS__);}}while(0)
//#define TRACE(x, ...) fprintf(stderr, x, __VA_ARGS__)
//#define TRACE(x, ...) do{if(0){fprintf(stderr, x, __VA_ARGS__);}}while(0)

#endif //_TRACE_H

