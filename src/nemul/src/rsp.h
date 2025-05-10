//rsp.h
//GDB RSP server for Neki32 emulator
//Bryan E. Topp <betopp@betopp.com> 2025
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

#endif //RSP_H
