//ui_debug.h
//Debugging window for the Neki32 simulator front end
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//The window draws from a snapshot that the simulation thread takes while nothing else is
//touching the machine, and reports what the user asked for. It never touches the emulator's
//state itself: the machine belongs to the simulation thread.

#ifndef _UI_DEBUG_H
#define _UI_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include "process.h"
#include "symbols.h"

//Instructions published around the program counter for the disassembly view
#define UI_DEBUG_CODE_MAX 192

//Recent instructions published for the trace view
#define UI_DEBUG_TRACE_MAX 128

//One process as the debugger sees it
typedef struct ui_debug_proc_s
{
	int pid;
	int state;
	int dbgstop;
	bool paused;
	uint32_t pc;
	uint32_t cpsr;
	uint32_t regs[16];
} ui_debug_proc_t;

//Everything the window shows, filled in by the simulation thread
typedef struct ui_debug_state_s
{
	bool valid;              //has ever been filled
	bool machine_paused;     //the whole simulation is stopped, not just one process
	int npids;
	ui_debug_proc_t proc[PROCESS_MAX];

	uint32_t emul_ms;        //emulated milliseconds since the machine was reset

	//Breakpoints the machine is watching
	int bp_count;
	uint32_t bp[PROCESS_BP_MAX];
	int bp_hit_pid;
	uint32_t bp_hit_addr;

	//Instructions around the followed process's program counter
	int code_pid;
	uint32_t code_base;
	int code_count;
	uint32_t code[UI_DEBUG_CODE_MAX];

	//What the followed process ran lately
	int trace_pid;
	int trace_count;
	uint32_t trace_pc[UI_DEBUG_TRACE_MAX];
	uint32_t trace_ir[UI_DEBUG_TRACE_MAX];

} ui_debug_state_t;

//What the window wants done about it
typedef struct ui_debug_req_s
{
	int step;                //instructions to step, 0 for none
	int pause;               //-1 no change, 0 run, 1 stop
	int bp_add;              //address to break at, 0 for none
	int bp_del;              //address to stop breaking at, 0 for none
	int bp_clear;
} ui_debug_req_t;

//State of the window itself, which belongs to the interface thread
typedef struct ui_debug_s
{
	bool open;

	//Process being examined, 0 for whichever is running
	int pid;

	//Whether the disassembly follows the program counter
	bool follow_pc;

	//Where the disassembly starts when it is not following the program counter
	uint32_t look_at;

	//Address typed into the breakpoint box
	char bp_text[16];

	//Whether to show the instruction trace
	bool show_trace;

	//Symbols, if any were loaded
	symbols_t *syms;
	char syms_path[512];
	char syms_status[128];

	//Requests built up during the frame, for the caller to carry out
	ui_debug_req_t req;

} ui_debug_t;

//Opens the window
void ui_debug_open(ui_debug_t *st);

//Draws the window from a snapshot. Requests are written into st->req for the caller to carry out.
void ui_debug_draw(ui_debug_t *st, const ui_debug_state_t *state);

//Loads symbols from a file, replacing any already loaded
bool ui_debug_load_symbols(ui_debug_t *st, const char *path);

//Frees anything the window owns
void ui_debug_close(ui_debug_t *st);

#endif //_UI_DEBUG_H
