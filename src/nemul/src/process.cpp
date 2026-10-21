//process.cpp
//Process table of emulated processes
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#define FILE_TRACE_CAT TRACE_CAT_PROCESS
#include "trace.h"

#include "process.h"
#include "interp.h"
#include "sysc.h"
#include "rsp.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

//PID1 process image from actual kernel build system
#include "init.inc"

//Storage for process table, as declared in process.h.
process_t process_table[PROCESS_MAX];

//Ring of the last instructions each process ran, so that when the interpreter gives up it can
//say what led there. A wild branch is much easier to believe with the preceding twenty
//instructions in front of you.
#define HIST_LEN 1024
static uint32_t hist_pc[PROCESS_MAX][HIST_LEN];
static uint32_t hist_ir[PROCESS_MAX][HIST_LEN];
static int hist_pos[PROCESS_MAX];

//Breakpoint state, see emul_set_break below
static uint32_t emul_bp_addr;
static int emul_bp_steps;
static int emul_bp_hit;
static int emul_bp_left;
int emul_stop_requested;

void emul_set_break(uint32_t addr, int steps)
{
	emul_bp_addr = addr;
	emul_bp_steps = steps;
	emul_bp_hit = 0;
	emul_bp_left = 0;
	emul_stop_requested = 0;
}

//Set NEMUL_LOG_INSTR to have the interpreter explain itself when a process runs into memory
//that is not code. Off by default because it is noise in a normal run.
static int hist_logging = -1;
static uint32_t hist_max = 0; //Anything executing above this is not code, if it is set
static int hist_walked[PROCESS_MAX];

static void hist_record(int idx, process_t *pptr)
{
	if(hist_logging < 0)
	{
		hist_logging = (getenv("NEMUL_LOG_INSTR") != NULL) ? 1 : 0;
		const char *maxstr = getenv("NEMUL_LOG_INSTR_MAX");
		if(maxstr != NULL)
			hist_max = (uint32_t)strtoul(maxstr, NULL, 16);
	}

	uint32_t pc = pptr->regs[15];
	uint32_t ir = 0;
	if(pc + 4 <= pptr->size)
		ir = pptr->mem[pc / 4];

	//Two zero instructions in a row means the process is walking an empty page rather than
	//running code, and by the time the interpreter gives up the ring has long since been
	//overwritten by that walk. Show what led into it, once.
	int beyond = (hist_max != 0 && pc > hist_max);
	int walking = (ir == 0 && hist_ir[idx][(hist_pos[idx] + HIST_LEN - 1) % HIST_LEN] == 0);

	if(hist_logging && (beyond || walking) && !hist_walked[idx])
	{
		//Walking a run of data means the code fell through or branched into it. Find the last
		//step that was not the next instruction and show from there: that is the transfer that
		//went wrong.
		hist_walked[idx] = 1;
		int from = 0;
		for(int nn = 1; nn < 200; nn++)
		{
			int here = (hist_pos[idx] + HIST_LEN - nn) % HIST_LEN;
			int prev = (hist_pos[idx] + HIST_LEN - nn - 1) % HIST_LEN;
			if(hist_pc[idx][here] != hist_pc[idx][prev] + 4)
			{
				from = nn;
				break;
			}
		}

		fprintf(stderr, "pid %d: at %8.8X (not code); the transfer that took it there:\n",
			pptr->pid, pc);
		fprintf(stderr, "   r0=%8.8X r1=%8.8X r12=%8.8X r13=%8.8X r14=%8.8X cpsr=%8.8X\n",
			pptr->regs[0], pptr->regs[1], pptr->regs[12], pptr->regs[13], pptr->regs[14],
			pptr->cpsr);
		for(int nn = from + 6; nn > 0; nn--)
		{
			int at = (hist_pos[idx] + HIST_LEN - nn) % HIST_LEN;
			fprintf(stderr, "   %8.8X  %8.8X%s\n", hist_pc[idx][at], hist_ir[idx][at],
				(nn == from) ? "   <-- branch" : "");
		}
	}

	hist_pc[idx][hist_pos[idx]] = pc;
	hist_ir[idx][hist_pos[idx]] = ir;
	hist_pos[idx] = (hist_pos[idx] + 1) % HIST_LEN;
}

static void hist_dump(int idx, process_t *pptr)
{
	fprintf(stderr, "pid %d: last %d instructions before stopping at %8.8X\n",
		pptr->pid, HIST_LEN, pptr->regs[15]);
	for(int nn = 0; nn < HIST_LEN; nn++)
	{
		int at = (hist_pos[idx] + nn) % HIST_LEN;
		if(hist_pc[idx][at] == 0 && hist_ir[idx][at] == 0)
			continue;
		fprintf(stderr, "   %8.8X  %8.8X\n", hist_pc[idx][at], hist_ir[idx][at]);
	}
	for(int rr = 0; rr < 16; rr++)
		fprintf(stderr, "   r%d=%8.8X%s", rr, pptr->regs[rr], (rr % 4 == 3) ? "\n" : " ");
	fprintf(stderr, "   cpsr=%8.8X\n", pptr->cpsr);
}

void process_reset(void)
{
	TINFO("%s", "Resetting process table...\n");
	
	//Free all the dynamically allocated parts of the process table
	//This is just the user memory for the moment
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].mem != NULL)
		{
			TDEBUG("Freeing memory from process %d\n", process_table[pp].pid);
			free(process_table[pp].mem); process_table[pp].mem = NULL;
		}
	}
	
	//Clear the process table
	memset(process_table, 0, sizeof(process_table));
	TINFO("%s", "Process table reset.\n");
	
	//Make the initial process
	process_table[1].pid = 1;
	
	//The initial process gets the machine's whole user address space, not just the size of the
	//image: crt0 puts its stack near the top of that space.
	#define PROCESS_SPACE (24 * 1024 * 1024)
	assert(process_table[1].mem == NULL);
	process_table[1].mem = (uint32_t*)calloc(1, PROCESS_SPACE);
	if(process_table[1].mem == NULL)
	{
		TFATAL("Failed to allocate %u bytes for initial process\n", PROCESS_SPACE);
		exit(-1);
	}
	
	process_table[1].size = PROCESS_SPACE;
	memcpy(process_table[1].mem, init, sizeof(init));
	process_table[1].regs[15] = 0x1000;
	
	process_table[1].state = PROCESS_STATE_ALIVE;
	
	TDEBUG("PID1 set up, %lu bytes in init process.\n", sizeof(init));
}

void process_step(void)
{
	TDEBUG("%s", "=== PROCESS STEP ===\n");
	
	//Pick a process to run
	process_t *pptr = NULL;
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state != PROCESS_STATE_ALIVE)
			continue; //Not an alive process - dead or nonexistant
		
		if(process_table[pp].paused && !process_table[pp].unpaused)
			continue; //Called _sc_pause and nobody's unpaused them yet
		
		if(process_table[pp].dbgstop)
			continue; //Stopped for debugging
		
		//Likely candidate - todo, replicate scheduling algo from real kernel
		pptr = &(process_table[pp]);
		break;
	}
	
	if(pptr == NULL)
	{
		TDEBUG("%s", "No runnable processes.\n");
		return;
	}
	
	TDEBUG("Scheduled process %d\n", pptr->pid);
	
	//A process that was paused and then unpaused can be paused again
	if(pptr->paused && pptr->unpaused)
	{
		pptr->paused = 0;
		pptr->unpaused = 0;
	}
	
	//This is super approximate but whatever
	//The Nuvoton chip runs 300MHz so one millisecond is about 300K instructions (dev version)
	//The Allwinner chip is a bit faster, 500MHz or so (consumer version prototype)
	int procidx = (int)(pptr - process_table);

	interp_result_t result = INTERP_RESULT_OK;
	for(int instr = 0; instr < 300 * 1000; instr++)
	{
		hist_record(procidx, pptr);

		//Breakpoint: stop here, then watch the next few instructions of this same process one
		//at a time. Stepping is done inside this loop so the process keeps the CPU instead of
		//the schedule moving on between instructions.
		if(emul_bp_addr != 0 && pptr->regs[15] == emul_bp_addr && !emul_bp_hit)
		{
			emul_bp_hit = 1;
			emul_bp_left = emul_bp_steps;
			fprintf(stderr, "break: pid %d reaches %8.8X, watching %d instructions\n",
				pptr->pid, emul_bp_addr, emul_bp_steps);
			for(int rr = 0; rr < 16; rr++)
				fprintf(stderr, "   r%-2d %8.8X%s", rr, pptr->regs[rr], (rr % 4 == 3) ? "\n" : " ");
			fprintf(stderr, "   cpsr %8.8X\n", pptr->cpsr);
		}
		else if(emul_bp_left > 0)
		{
			uint32_t at = pptr->regs[15];
			uint32_t ir = 0;
			if(at + 4 <= pptr->size)
				ir = pptr->mem[at / 4];
			fprintf(stderr, "   %8.8X  %8.8X  sp=%8.8X lr=%8.8X r0=%8.8X\n",
				at, ir, pptr->regs[13], pptr->regs[14], pptr->regs[0]);
			emul_bp_left--;
			if(emul_bp_left == 0)
			{
				emul_stop_requested = 1;
				return;
			}
		}

		result = interp_step(pptr->regs, &(pptr->cpsr), pptr->mem, pptr->size);
		if(result != INTERP_RESULT_OK)
		{
			//Something happened that would have caused a CPU exception/interrupt
			break;
		}
	}
	
	//Anything that is not a syscall or a clean timeslice end is a surprise worth explaining
	if(result != INTERP_RESULT_OK && result != INTERP_RESULT_SYSCALL && hist_logging)
		hist_dump(procidx, pptr);

	//See what happened to the process
	switch(result)
	{
		case INTERP_RESULT_OK:
		{
			//The process just ran out its timeslice in user mode.
			//No action is needed, we can keep running it more later.
			break;
		}
		case INTERP_RESULT_SYSCALL:
		{
			//User code triggered a system-call, handle it before continuing
			sysc(pptr);

			//A call like exit kills the process, and a killed process does not get to run the
			//instruction after the trap. Without this the caller carries on and the program
			//restarts itself instead of stopping.
			if(pptr->state == PROCESS_STATE_DEAD)
				return;
			break;
		}
		case INTERP_RESULT_BKPT:
		{
			//Hit a GDB breakpoint instruction
			pptr->regs[15] -= 4; //Back off, stay at the instruction we tried to run
			rsp_dbgstop(pptr->pid, PROCESS_DBGSTOP_BKPT);
			break;
		}
		case INTERP_RESULT_FATAL:
		{
			//Interpreter failure
			pptr->regs[15] -= 4; //Back off, stay at the instruction we tried to run
			rsp_dbgstop(pptr->pid, PROCESS_DBGSTOP_FATAL);
			break;
		}
		case INTERP_RESULT_AC:
		{
			//Alignment check
			pptr->regs[15] -= 4; //Back off, stay at the instruction we tried to run
			rsp_dbgstop(pptr->pid, PROCESS_DBGSTOP_AC);
			break;
		}
		case INTERP_RESULT_ABT:
		{
			//Data abort
			pptr->regs[15] -= 4; //Back off, stay at the instruction we tried to run
			rsp_dbgstop(pptr->pid, PROCESS_DBGSTOP_ABT);
			break;
		}
		case INTERP_RESULT_PF:
		{
			//Prefetch abort
			pptr->regs[15] -= 4; //Back off, stay at the instruction we tried to run
			rsp_dbgstop(pptr->pid, PROCESS_DBGSTOP_PF);
			break;
		}
		default:
		{
			TFATAL("%s", "Unhandled result from ARM code interpreter\n");
			exit(-1);
		}
	}
	
	return;
}

int process_fork(int parent)
{
	//Find parent process
	process_t *parent_pptr = NULL;
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].pid == parent)
		{
			parent_pptr = &(process_table[pp]);
			break;
		}
	}
	if(parent_pptr == NULL)
	{
		//Didn't find the process requesting a fork...?
		TFATAL("%s", "bad call to process_fork, parent not found\n");
		exit(-1);
	}
	
	//Find free spot for child
	int child_idx = -1;
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state == PROCESS_STATE_NONE)
		{
			//This spot is free, use it
			child_idx = pp;
			break;
		}
	}
	if(child_idx == -1)
	{
		//No free space for child
		TWARNING("%s", "No space for another process in process table.\n");
		return -PVMK_ENOSPC;
	}
	process_t *child_pptr = &(process_table[child_idx]);
	
	//Make sure we can allocate space for its memory
	if(child_pptr->mem != NULL)
	{
		free(child_pptr->mem);
		child_pptr->mem = NULL;
		child_pptr->size = 0;
	}
	
	child_pptr->mem = (uint32_t*)malloc(parent_pptr->size);
	if(child_pptr->mem == NULL)
	{
		//No space for memory for this child
		TERROR("%s", "No memory on the host for a copy of the process.\n");
		return -PVMK_ENOMEM;
	}
	child_pptr->size = parent_pptr->size;
	
	//Child process starts alive and runnable
	child_pptr->state = PROCESS_STATE_ALIVE;
	child_pptr->paused = false;
	child_pptr->unpaused = false;
	
	//Child gets a PID corresponding to its index in the process table
	child_pptr->pid += PROCESS_MAX;
	if( (child_pptr->pid <= 0) || ((child_pptr->pid % PROCESS_MAX) != child_idx) )
		child_pptr->pid = child_idx;
	
	child_pptr->ppid = parent_pptr->pid;
	
	//Memory starts as copy of parent memory
	memcpy(child_pptr->mem, parent_pptr->mem, parent_pptr->size);
	
	//Register state starts as copy of parent registers
	memcpy(child_pptr->regs, parent_pptr->regs, sizeof(child_pptr->regs));
	child_pptr->cpsr = parent_pptr->cpsr;
	
	//Environment copied across
	memcpy(child_pptr->env_buf, parent_pptr->env_buf, sizeof(child_pptr->env_buf));
	child_pptr->env_len = parent_pptr->env_len;
	
	//Fudge fudge... make sure the child sees a return from fork system-call with "0" in return value
	child_pptr->regs[0] = 0;
	
	//Done, return ID of new child
	return child_pptr->pid;
}

process_t *process_find(int pid)
{
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state == PROCESS_STATE_NONE)
			continue;
		if(process_table[pp].pid != pid)
			continue;
		
		return &(process_table[pp]);
	}
	return NULL;
}