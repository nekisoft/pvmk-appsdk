//trace.h
//Debug tracing for simulation
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef _TRACE_H
#define _TRACE_H

#include <stdio.h>

extern int tracing;

//Execution tracing macro
#define TRACE(x, ...) do{if(tracing){fprintf(stderr, x, __VA_ARGS__);}}while(0)
//#define TRACE(x, ...) fprintf(stderr, x, __VA_ARGS__)
//#define TRACE(x, ...) do{if(0){fprintf(stderr, x, __VA_ARGS__);}}while(0)

#endif //_TRACE_H

