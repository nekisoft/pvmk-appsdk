//compare_interp.cpp
//Compares ARM interpreter with real instruction traces
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#if COMPARE_INTERP
//Input to this program is a GDB log file, containing test-cases of the form:
//test_begin n
//(register dump)
//ir 0xXXXXXXXX
//(register dump)
//test_end

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "interp.h"
#include "trace.h"

int tracing = 1;

int main(int argc, const char **argv)
{
	FILE *infile = stdin;
	if(argc > 1)
	{
		infile = fopen(argv[1], "rb");
		if(infile == NULL)
			infile = stdin;
	}
	
	uint32_t input_regs[17] = {0};
	uint32_t output_regs[17] = {0};
	uint32_t sim_regs[17] = {0};
	uint32_t ir = 0;
	int output_phase = 0;
	int testn = 0;
	
	while(!feof(infile))
	{
		char linebuf[1024] = {0};
		char *lineptr = fgets(linebuf, sizeof(linebuf)-1, infile);
		if(lineptr == NULL)
			break;
		
		uint32_t parm = 0;
		uint32_t *regs = (output_phase) ? output_regs : input_regs;
		if(sscanf(linebuf, " r10 %X ", &parm) == 1)
			regs[10] = parm;
		else if(sscanf(linebuf, " r11 %X ", &parm) == 1)
			regs[11] = parm;
		else if(sscanf(linebuf, " r12 %X ", &parm) == 1)
			regs[12] = parm;
		else if(sscanf(linebuf, " r13 %X ", &parm) == 1)
			regs[13] = parm;
		else if(sscanf(linebuf, " r14 %X ", &parm) == 1)
			regs[14] = parm;
		else if(sscanf(linebuf, " r15 %X ", &parm) == 1)
			regs[15] = parm;
		else if(sscanf(linebuf, " sp %X ", &parm) == 1)
			regs[13] = parm;
		else if(sscanf(linebuf, " lr %X ", &parm) == 1)
			regs[14] = parm;
		else if(sscanf(linebuf, " pc %X ", &parm) == 1)
			regs[15] = parm;
		else if(sscanf(linebuf, " cpsr %X ", &parm) == 1)
			regs[16] = parm;
		else if(sscanf(linebuf, " r0 %X ", &parm) == 1)
			regs[0] = parm;
		else if(sscanf(linebuf, " r1 %X ", &parm) == 1)
			regs[1] = parm;
		else if(sscanf(linebuf, " r2 %X ", &parm) == 1)
			regs[2] = parm;
		else if(sscanf(linebuf, " r3 %X ", &parm) == 1)
			regs[3] = parm;
		else if(sscanf(linebuf, " r4 %X ", &parm) == 1)
			regs[4] = parm;
		else if(sscanf(linebuf, " r5 %X ", &parm) == 1)
			regs[5] = parm;
		else if(sscanf(linebuf, " r6 %X ", &parm) == 1)
			regs[6] = parm;
		else if(sscanf(linebuf, " r7 %X ", &parm) == 1)
			regs[7] = parm;
		else if(sscanf(linebuf, " r8 %X ", &parm) == 1)
			regs[8] = parm;
		else if(sscanf(linebuf, " r9 %X ", &parm) == 1)
			regs[9] = parm;
		else if(sscanf(linebuf, " ir %X ", &parm) == 1)
		{
			ir = parm;
			output_phase = 1;
		}
		else if(sscanf(linebuf, " test_begin %d ", &testn) == 1)
		{
			output_phase = 0;
			memset(input_regs, 0, sizeof(input_regs));
			memset(output_regs, 0, sizeof(output_regs));
			memset(sim_regs, 0, sizeof(sim_regs));
		}
		else if(strstr(linebuf, "test_end") != NULL)
		{
			output_phase = 0;
			
			memcpy(sim_regs, input_regs, sizeof(sim_regs));
			interp_result_t r = interp_step_force(sim_regs, sim_regs + 16, NULL, 0, ir);
			if(r == INTERP_RESULT_FATAL)
			{
				TRACE("%s", "Fatal result\n");
				exit(-1);
			}
			else if(r == INTERP_RESULT_OK)
			{
				TRACE("%s", "Interpreted OK, checking result regs\n");
				if(memcmp(sim_regs, output_regs, sizeof(sim_regs)) != 0)
				{
					TRACE("Mismatch in registers!  Test=%d IR=0x%8.8X\n", testn, ir);
					TRACE("%s", "reg\ttestcase\texpected\tsimulated\n");
					for(int rr = 0; rr < 17; rr++)
					{
						if(rr == 16)
							TRACE("%s", "CPSR=\t");
						else
							TRACE("r%d=\t", rr);
						
						TRACE("%8.8X\t%8.8X\t%8.8X\n", 
							input_regs[rr], output_regs[rr], sim_regs[rr]);
					}
					exit(-1);
				}
				else
				{
					TRACE("%s", "Regs OK\n");
				}
			}
			else if(r == INTERP_RESULT_ABT)
			{
				TRACE("%s", "Memory access attempted, trying to ignore loads\n");
				if(ir & (1u << 20))
				{
					TRACE("%s", "L-bit set, not comparing\n");
				}
				else
				{
					TRACE("%s", "L-bit clear, comparing, resetting SP to correct value\n");
					sim_regs[13] = output_regs[13];
					TRACE("%s", "checking result regs\n");
					if(memcmp(sim_regs, output_regs, sizeof(sim_regs)) != 0)
					{
						TRACE("Mismatch in registers!  Test=%d IR=0x%8.8X\n", testn, ir);
						TRACE("%s", "reg\ttestcase\texpected\tsimulated\n");
						for(int rr = 0; rr < 17; rr++)
						{
							if(rr == 16)
								TRACE("%s", "CPSR=\t");
							else
								TRACE("r%d=\t", rr);
							
							TRACE("%8.8X\t%8.8X\t%8.8X\n", 
								input_regs[rr], output_regs[rr], sim_regs[rr]);
						}
						exit(-1);
					}
					else
					{
						TRACE("%s", "Regs OK\n");
					}
					
				
				}
				
				
				
			}
			else if(r == INTERP_RESULT_SYSCALL)
			{
				TRACE("%s", "Syscall attempted, ignoring\n");
			}
			else
			{
				TRACE("%s", "Other result from interpreting\n");
				exit(-1);
			}			
		}
	}
	
	TRACE("%s", "All tests passed.\n");
	return 0;
}

#endif //COMPARE_INTERP
