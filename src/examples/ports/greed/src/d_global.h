/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Raven 3D Engine                                                         */
/* Copyright (C) 1995 by Softdisk Publishing                               */
/*                                                                         */
/* Original Design:                                                        */
/*  John Carmack of id Software                                            */
/*                                                                         */
/* Enhancements by:                                                        */
/*  Robert Morgan of Channel 7............................Main Engine Code */
/*  Todd Lewis of Softdisk Publishing......Tools,Utilities,Special Effects */
/*  John Bianca of Softdisk Publishing..............Low-level Optimization */
/*  Carlos Hasan..........................................Music/Sound Code */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#ifndef DGLOBAL_H
#define DGLOBAL_H

#include <stdint.h>

// #define PARMCHECK
// #define VALIDATE

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t  longint; //Fuck me, at least they didn't type "long" all over the code like the Cylindrix guys. -betopp
typedef enum
{
    false,
    true
} boolean;

//Checks system time and runs phony "timer ISR" calls
void PvmkTimerSim(void);

//Copies from phony 8-bit front-buffer to next of truecolor buffers, and enqueues video flip
void PvmkPresent(void);

#endif
