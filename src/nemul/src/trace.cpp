//trace.cpp
//Debug tracing for simulation
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#include "trace.h"
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

//Currently configured verbosity
trace_sev_e trace_sev_limit[TRACE_CAT_MAX];

//Buffer for trace data
#define TRACE_BUF_LEN (4*1024*1024)
static char trace_buf[TRACE_BUF_LEN];
static int trace_pos;

//Each entry in the trace buffer has the following format:
//1 byte - category
//1 byte - severity
//4 bytes - severity limits enabled at this time, 3 bits per each category
//x bytes - NUL-terminated message string

void trace_write(trace_cat_e cat, trace_sev_e sev, const char *fmt, ...)
{
	//If we're obviously off the end of the buffer, blank it and go back
	if(trace_pos + 5 >= TRACE_BUF_LEN)
	{
		memset(trace_buf + trace_pos, 0, TRACE_BUF_LEN - trace_pos);
		trace_pos = 0;
	}
	
	//Try to append the new message to see if it fits, including the terminating NUL
	va_list ap;
	va_start(ap, fmt);
	int msglen = vsnprintf(trace_buf + trace_pos + 6, TRACE_BUF_LEN - (trace_pos + 6), fmt, ap);
	va_end(ap);
	
	if(msglen + 7 >= TRACE_BUF_LEN)
	{
		//Ran off the end of the buffer
		memset(trace_buf + trace_pos, 0, TRACE_BUF_LEN - trace_pos);
		trace_pos = 0;
		
		//Try again
		va_start(ap, fmt);
		msglen = vsnprintf(trace_buf + trace_pos + 6, TRACE_BUF_LEN - (trace_pos + 6), fmt, ap);
		va_end(ap);
	}
	
	//Write the "header" information before the message
	trace_buf[trace_pos + 0] = cat;
	trace_buf[trace_pos + 1] = sev;
	
	uint32_t enabled_bits = 0;
	for(int cc = 0; cc < TRACE_CAT_MAX; cc++)
	{
		enabled_bits <<= 3;
		enabled_bits |= trace_sev_limit[cc] & 0x7;
	}
	memcpy(trace_buf + trace_pos + 2, &enabled_bits, 4);
	
	//Move past the header, message string, terminating NUL
	trace_pos += 6 + msglen + 1;
}

bool trace_recall(int back, trace_cat_e *cat_out, trace_sev_e *sev_out, char **msg_out)
{
	int starting_pos = trace_pos;
	for(int bb = 0; bb <= back; bb++)
	{
		while(trace_buf[starting_pos] == '\0')
		{
			starting_pos--;
			if(starting_pos < 0)
				starting_pos = TRACE_BUF_LEN-1;
			if(starting_pos == trace_pos)
				return false;
		}
		while(trace_buf[starting_pos] != '\0')
		{
			starting_pos--;
			if(starting_pos < 0)
				starting_pos = TRACE_BUF_LEN-1;
			if(starting_pos == trace_pos)
				return false;
		}
	}
	
	starting_pos++;
	if(starting_pos >= TRACE_BUF_LEN)
		starting_pos = 0;
	
	*cat_out = (trace_cat_e)trace_buf[starting_pos + 0];
	*sev_out = (trace_sev_e)trace_buf[starting_pos + 1];
	*msg_out = trace_buf + starting_pos + 6;
	return true;
}
