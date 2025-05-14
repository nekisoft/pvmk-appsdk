//process.cpp
//Process table of emulated processes
//Bryan E. Topp <betopp@betopp.com> 2025

#include "process.h"
#include "interp.h"
#include "trace.h"
#include "sysc.h"
#include "rsp.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

//PID1 process image from actual kernel build system
#include "init.inc"

//Storage for process table, as declared in process.h.
process_t process_table[PROCESS_MAX];

void process_reset(void)
{
	TRACE("%s", "Resetting process table...\n");
	
	//Free all the dynamically allocated parts of the process table
	//This is just the user memory for the moment
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].mem != NULL)
		{
			TRACE("Freeing memory from process %d\n", process_table[pp].pid);
			free(process_table[pp].mem); process_table[pp].mem = NULL;
		}
	}
	
	//Clear the process table
	memset(process_table, 0, sizeof(process_table));
	TRACE("%s", "Process table reset.\n");
	
	//Make the initial process
	process_table[1].pid = 1;
	
	assert(process_table[1].mem == NULL);
	process_table[1].mem = (uint32_t*)calloc(1, sizeof(init));
	if(process_table[1].mem == NULL)
	{
		TRACE("Failed to allocate %lu bytes for initial process\n", sizeof(init));
		exit(-1);
	}
	
	process_table[1].size = sizeof(init);
	memcpy(process_table[1].mem, init, sizeof(init));
	process_table[1].regs[15] = 0x1000;
	
	process_table[1].state = PROCESS_STATE_ALIVE;
	
	TRACE("PID1 set up, %lu bytes in init process.\n", sizeof(init));
}

void process_step(void)
{
	TRACE("%s", "=== PROCESS STEP ===\n");
	
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
		TRACE("%s", "No runnable processes.\n");
		return;
	}
	
	TRACE("Scheduled process %d\n", pptr->pid);
	
	//A process that was paused and then unpaused can be paused again
	if(pptr->paused && pptr->unpaused)
	{
		pptr->paused = 0;
		pptr->unpaused = 0;
	}
	
	//This is super approximate but whatever
	//The Nuvoton chip runs 300MHz so one millisecond is about 300K instructions (dev version)
	//The Allwinner chip is a bit faster, 500MHz or so (consumer version prototype)
	interp_result_t result = INTERP_RESULT_OK;
	for(int instr = 0; instr < 300 * 1000; instr++)
	{
		result = interp_step(pptr->regs, &(pptr->cpsr), pptr->mem, pptr->size);
		if(result != INTERP_RESULT_OK)
		{
			//Something happened that would have caused a CPU exception/interrupt
			break;
		}
	}
	
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
			TRACE("%s", "Unhandled result from ARM code interpreter\n");
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
		TRACE("%s", "bad call to process_fork, parent not found\n");
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
		TRACE("%s", "No space for another process in process table.\n");
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
		TRACE("%s", "No memory on the host for a copy of the process.\n");
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