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

//Front ends tell the server how to set the machine going again, because a client that resumes a
//thread expects the machine to run even if the window had paused it
void rsp_set_continue_hook(void (*hook)(void));

//Front ends tell the server how the machine can be given a program to run, so that a client can
//ask for one itself ("run" in GDB) and get a restarted machine back. The loader returns 0 when
//the machine is now running that image, or -1 with a message for the client; the second hook
//says which image is loaded, or NULL when there is none. Both are called from the simulation
//thread, between instructions, so the loader may reset the machine itself.
void rsp_set_image_hooks(int (*load)(const char *path, char *err, size_t errsz), const char *(*current)(void));

//Front ends tell the server when the machine has been restarted, so that the client's breakpoints
//can be put back and it does not have to know that they went away
void rsp_after_reset(void);

//Tells the debugger that a process has ended
void rsp_notify_exit(int pid);

//Returns whether a debugger is connected
bool rsp_present(void);

#endif //RSP_H
