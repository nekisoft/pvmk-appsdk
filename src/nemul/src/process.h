//process.h
//Process table of emulated processes
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _PROCESS_H
#define _PROCESS_H

#include <stdint.h>

//States a process can be in
typedef enum process_state_e
{
	PROCESS_STATE_NONE = 0,  //No process in this process control block
	PROCESS_STATE_ALIVE,     //Process is alive and well, might run if unpaused
	PROCESS_STATE_DEAD,      //Process has died but status hasn't been delivered to parent
	PROCESS_STATE_MAX        //Number of process states
} process_state_t;

//Reasons a process can be stopped by the debugger
typedef enum process_dbgstop_e
{
	PROCESS_DBGSTOP_NONE = 0, //Can run
	PROCESS_DBGSTOP_CTRLC, //Interrupted by user / debugger just connected
	PROCESS_DBGSTOP_SIGNAL, //Stopped instead of taking a signal
	PROCESS_DBGSTOP_BKPT, //Hit a GDB breakpoint instruction
	PROCESS_DBGSTOP_ABT, //Data abort exception
	PROCESS_DBGSTOP_AC, //Alignment check exception
	PROCESS_DBGSTOP_PF, //Prefetch abort exception
	PROCESS_DBGSTOP_FATAL, //Interpreter failure
	PROCESS_DBGSTOP_MAX
} process_dbgstop_t;

//Process control block + saved context + reference to memory space
//Everything about each emulated process
typedef struct process_s
{
	//State of this process table entry
	process_state_t state;
	
	//Identity of process and its parent
	int pid;
	int ppid;
	
	//Memory space of the process, dynamically allocated by calloc/realloc.
	//Represents the user-mode virtual memory space given by the machine-layer in the real kernel.
	uint32_t *mem;
	
	//Size of memory allocated to this process
	uint32_t size;
	
	//Pending image from mexec calls
	char *mexec_mem;
	
	//Size of pending image from mexec calls
	uint32_t mexec_size;
	
	//Registers and status of the saved CPU context (for the interpreter)
	//This represents the context we'd switch to in the real kernel.
	uint32_t regs[16];
	uint32_t cpsr;
	
	//Whether the process has been paused/unpaused (blocking on system calls)
	bool paused;
	bool unpaused;
	
	//Environment buffer
	char env_buf[4096];
	int env_len;
	
	//If the process is stopped by the debugger, and why
	process_dbgstop_t dbgstop;
	
} process_t;

//Table of emulated processes - fixed number like the real machine (8 as of kernel r0u3)
#define PROCESS_MAX 8
extern process_t process_table[PROCESS_MAX];

//Finds process by PID
process_t *process_find(int pid);

//Resets process table for new run of the emulator
void process_reset(void);

//Runs approximately 1ms of simulation on a runnable process, if any
void process_step(void);

//Stops to look at a process when it reaches the given address, printing the next "steps"
//instructions one at a time and then ending the run. Zero address disables it.
void emul_set_break(uint32_t addr, int steps);
extern int emul_stop_requested;

//Tries to make a copy of the given process
int process_fork(int parent);

//Runs exactly one instruction of one process, for the debugger's step
int process_run_insn(int pid);

//Instructions each process has run lately, back=0 being the one that ran most recently: the
//program counter stands on the instruction after it. Returns -1 when there is nothing that far
//back.
int process_hist(int pid, int back, uint32_t *pc, uint32_t *ir);

//Lets the instruction under a breakpoint run once, so that resuming does not stop on it again
void process_bp_skip_once(int pid, uint32_t addr);

//Breakpoints the debugger window sets and clears while the machine runs. A hit leaves the machine
//standing on the instruction, sets emul_stop_requested, and records where it happened.
#define PROCESS_BP_MAX 32
extern uint32_t process_bp[PROCESS_BP_MAX];
extern int process_bp_count;
extern int process_bp_hit_pid;
extern uint32_t process_bp_hit_addr;
int process_bp_add(uint32_t addr);
int process_bp_del(uint32_t addr);
void process_bp_clear(void);

//Breakpoints belonging to one process, for a debugger that keeps its own set per program. A pid of
//zero means every process, which is what the window and a plain address mean.
int process_bp_add_owner(uint32_t addr, int pid);
int process_bp_del_owner(uint32_t addr, int pid);
extern int process_bp_pid[];

#endif //_PROCESS_H
