//rsp.h
//GDB RSP server for Neki32 emulator
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef RSP_H
#define RSP_H

#include "prefs.h"
#include "process.h"

//Initializes RSP listener
void rsp_init(const prefs_t *prefs);

//Polls TCP socket and acts on RSP commands
void rsp_poll(void);

//Sets a process to debug-stopped and notifies the debugger
void rsp_dbgstop(int pid, process_dbgstop_t reason);

//Returns whether a debugger is connected
bool rsp_present(void);

#endif //RSP_H
