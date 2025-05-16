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

//Tracing categories
enum trace_cat_e
{
	TRACE_CAT_NONE = 0,
	TRACE_CAT_UNKNOWN,
	TRACE_CAT_INTERP,
	TRACE_CAT_PROCESS,
	TRACE_CAT_RSP,
	TRACE_CAT_SYSC,
	TRACE_CAT_MAX
};

//Tracing severities
enum trace_sev_e
{
	TRACE_SEV_NONE = 0,
	TRACE_SEV_FATAL,
	TRACE_SEV_ERROR,
	TRACE_SEV_WARNING,
	TRACE_SEV_INFO,
	TRACE_SEV_DEBUG,
	TRACE_SEV_MAX
};

//Verbosity levels for trace categories
extern trace_sev_e trace_sev_limit[TRACE_CAT_MAX];

//cpp file should define FILE_TRACE_CAT before including trace.h, so macros can use their category
#ifndef FILE_TRACE_CAT
	#define FILE_TRACE_CAT TRACE_CAT_UNKNOWN
#endif

//Macro to conditionally log a trace if enabled (avoids function-call before bailing-out)
#define TRACE(cat, sev, x, ...) \
	do{ \
		if(sev <= trace_sev_limit[cat]) \
			trace_write(cat, sev, x, __VA_ARGS__); \
	}while(0)

//Macros to be used by others for each severity of trace
#define TFATAL(x, ...)   TRACE(FILE_TRACE_CAT, TRACE_SEV_FATAL,   x, __VA_ARGS__)
#define TERROR(x, ...)   TRACE(FILE_TRACE_CAT, TRACE_SEV_ERROR,   x, __VA_ARGS__)
#define TWARNING(x, ...) TRACE(FILE_TRACE_CAT, TRACE_SEV_WARNING, x, __VA_ARGS__)
#define TINFO(x, ...)    TRACE(FILE_TRACE_CAT, TRACE_SEV_INFO,    x, __VA_ARGS__)
#define TDEBUG(x, ...)   TRACE(FILE_TRACE_CAT, TRACE_SEV_DEBUG,   x, __VA_ARGS__)

//Function that logs to the internal tracing buffer
void trace_write(trace_cat_e cat, trace_sev_e sev, const char *fmt, ...);

//Works backwards to find the nth latest trace message
bool trace_recall(int back, trace_cat_e *cat_out, trace_sev_e *sev_out, char **msg_out);

#endif //_TRACE_H

