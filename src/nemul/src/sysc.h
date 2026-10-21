//sysc.h
//Emulated system-calls in Neki32 simulator
//Bryan E. Topp <betopp@betopp.com> 2024

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#ifndef _SYSC_H
#define _SYSC_H

#include <stdint.h>
#include "process.h"

//Error numbers as defined by Neki32 system-call interface
#define PVMK_EPERM  1
#define PVMK_ENOENT 2
#define PVMK_ENXIO  6
#define PVMK_EAGAIN 11
#define PVMK_ENOMEM 12
#define PVMK_EFAULT 14
#define PVMK_EINVAL 22
#define PVMK_ENOSPC 28
#define PVMK_ENOSYS 38

//Performs a system-call
void sysc(process_t *pptr);

//Sets the host file used to service disk reads/writes
void sysc_setdiskfd(int fd);

//Sets the file the virtual card answers _sc_cardprobe from: either a 2048-byte record sector
//as the inspector writes it, or a whole card image with the records at LBA 18.
//Returns 0 if it looks like a valid record sector.
int sysc_setcardrecords(const char *path);

//Boots the given NNE image as PID1, instead of the embedded init. Returns 0 on success.
int sysc_bootimage(const char *path);

//Gets the latest frontbuffer to be enqueued by a process, and marks it active
//(Simulates entry to vertical blanking in the game console.)
void sysc_popfbptr(uint16_t **bufptr_out, int *mode_out);

//Enqueues input events for _sc_input to return
void sysc_pushpads(const uint16_t *pads);

#endif //_SYSC_H