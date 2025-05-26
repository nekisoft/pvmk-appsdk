/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Raven 3D Engine                                                         */
/* Copyright (C) 1996 by Softdisk Publishing                               */
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "d_global.h"
#include "d_video.h"
#include "d_ints.h"
#include "d_misc.h"
#include "timer.h"
#include "audio.h"
#include "protos.h"
#include "r_refdef.h"
#include "d_disk.h"
#include "svrdos4g.h"

#include <sc.h>


/**** CONSTANTS ****/

#define TIMERINT 8
#define KEYBOARDINT 9
#define VBLCOUNTER 16000
#define MOUSEINT 0x33
#define MOUSESENSE SC.mousesensitivity
#define JOYPORT 0x201
#define MOUSESIZE 16


/**** VARIABLES ****/

//void(interrupt* oldkeyboardisr)();
void (*timerhook)();  // called every other frame (player)
boolean timeractive;
longint timecount;           // current time index
boolean keyboard[NUMCODES];  // keyboard flags
boolean pausekey, capslock, newascii;
boolean mouseinstalled, joyinstalled;
int     in_button[NUMBUTTONS];  // frames the button has been down
byte    lastscan;
char    lastascii;

/* mouse data */
short mdx, mdy, b1, b2;
int   hiding = 1, busy = 1; /* internal flags */
int   mousex = 160, mousey = 100;
byte  back[MOUSESIZE * MOUSESIZE]; /* background for mouse */
byte  fore[MOUSESIZE * MOUSESIZE]; /* mouse foreground */


/* joystick data */
int  jx, jy, jdx, jdy, j1, j2;
word jcenx, jceny, xsense, ysense;

/* config data */
extern SoundCard SC;


byte ASCIINames[] =  // Unshifted ASCII for scan codes
    { 0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,   9,   'q', 'w', 'e',
      'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,  0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k',
      'l', ';', 39,  '`', 0,   92,  'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,
      ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '-', 0,
      0,   0,   '+', 0,   0,   0,   0,   127, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };

byte ShiftNames[] =  // Shifted ASCII for scan codes
    { 0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8,   9,   'Q', 'W', 'E',
      'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13,  0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K',
      'L', ':', 34,  '~', 0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,
      ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '-', 0,
      0,   0,   '+', 0,   0,   0,   0,   127, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };

byte SpecialNames[] =  // ASCII for 0xe0 prefixed codes
    { 0, 0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,   13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, '/', 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int scanbuttons[NUMBUTTONS] = {
    SC_UPARROW,     // bt_north
    SC_RIGHTARROW,  // bt_east
    SC_DOWNARROW,   // bt_south
    SC_LEFTARROW,   // bt_west
    SC_CONTROL,     // bt_fire
    SC_ALT,         // bt_straf
    SC_SPACE,       // bt_use
    SC_LSHIFT,      // bt_run
    SC_Z,           // bt_jump
    SC_X,           // bt_useitem
    SC_A,           // bt_asscam
    SC_PGUP,        // bt_lookup
    SC_PGDN,        // bt_lookdown
    SC_HOME,        // bt_centerview
    SC_COMMA,       // bt_slideleft
    SC_PERIOD,      // bt_slideright
    SC_INSERT,      // bt_invleft
    SC_DELETE,      // bt_invright
};


/**** FUNCTIONS ****/


void INT_KeyboardISR(void)
/* keyboard interrupt
    processes make/break codes
    sets key flags accordingly */
{
	
}


void INT_ReadControls(void)
/* read in input controls */
{
	//in_button[bt_...] = 0
	//in_button[bt_...] = 3
	
   
}

/*============================================================================*/

void INT_TimerISR(void)
/* process each timer tick */
{
    timecount++;  // increment timing variables
    if (MusicPresent)
        dPoll();  // have to poll a lot!
    if ((timecount & 1) && timerhook)
        timerhook();
    if (MusicPresent)
        dPoll();  // have to poll a lot!
}


void INT_TimerHook(void (*hook)(void)) { timerhook = hook; }

//PVMK doesn't support timer tick interrupts for user processes
//I guess I could make the kernel signal periodically but it's fucking ugly in general
//The actual use-case here seems to be "periodically we wait for the timer" anyway
static int PvmkTimerSimTicks = 0;
static uint16_t PvmkLastButtons = 0;
void PvmkTimerSim(void)
{	
	//Update inputs
	_sc_input_t input = {0};
	while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
	{
		if(input.format != 'A')
			continue;
		
		//Update player control variables
		in_button[bt_fire]     = (input.buttons & _SC_BTNBIT_A)     ? 1 : 0;
		in_button[bt_jump]     = (input.buttons & _SC_BTNBIT_B)     ? 1 : 0;
		in_button[bt_use]      = (input.buttons & _SC_BTNBIT_C)     ? 1 : 0;
		in_button[bt_north]    = (input.buttons & _SC_BTNBIT_UP)    ? 1 : 0;
		in_button[bt_east]     = (input.buttons & _SC_BTNBIT_RIGHT) ? 1 : 0;
		in_button[bt_south]    = (input.buttons & _SC_BTNBIT_DOWN)  ? 1 : 0;
		in_button[bt_west]     = (input.buttons & _SC_BTNBIT_LEFT)  ? 1 : 0;
		in_button[bt_invright] = (input.buttons & _SC_BTNBIT_Z)     ? 1 : 0;
		in_button[bt_useitem]  = (input.buttons & _SC_BTNBIT_Y)     ? 1 : 0;
		

/*
#define bt_straf 5
#define bt_run 7
#define bt_asscam 10
#define bt_lookup 11
#define bt_lookdown 12
#define bt_centerview 13
#define bt_slideleft 14
#define bt_slideright 15
*/

		
		//Hacks for keyboard input in different scenarios
		if( (input.buttons & ~PvmkLastButtons) & _SC_BTNBIT_MODE)
		{
			//Add a "escape" text input
			newascii = 1;
			lastascii = 27;
		}
		keyboard[SC_ESCAPE]     = (input.buttons & _SC_BTNBIT_MODE)  ? 1 : 0;
		keyboard[SC_ENTER]      = (input.buttons & _SC_BTNBIT_START) ? 1 : 0;
		keyboard[SC_LEFTARROW]  = (input.buttons & _SC_BTNBIT_LEFT)  ? 1 : 0;
		keyboard[SC_RIGHTARROW] = (input.buttons & _SC_BTNBIT_RIGHT) ? 1 : 0;
		keyboard[SC_UPARROW]    = (input.buttons & _SC_BTNBIT_UP)    ? 1 : 0;
		keyboard[SC_DOWNARROW]  = (input.buttons & _SC_BTNBIT_DOWN)  ? 1 : 0;
		keyboard[SC_1]          = (input.buttons & _SC_BTNBIT_X)     ? 1 : 0;
		
		//Set aside buttons for edge-detection next time
		PvmkLastButtons = input.buttons;
	}
	
	//Simulate appropriate number of timer ticks
	int vticks_desired = _sc_getticks() * 70 / 1000;
	if(vticks_desired > PvmkTimerSimTicks + 10)
	{
		//Running very far behind
		PvmkTimerSimTicks = vticks_desired - 1;
	}
	while(vticks_desired > PvmkTimerSimTicks)
	{
		INT_TimerISR();
		PvmkTimerSimTicks++;
	}
}

void KBstub() {}

/*============================================================================*/

void J_GetJoyInfo(word* x, word* y)
{    
    if(x != NULL)
	    *x = 0;
    if(y != NULL)
	    *y = 0;
}


void J_GetButtons(void)
{

}


boolean J_Detect(void)
{
	return true;
}


void J_Calibrate(void)
{

}


void J_Init(void)
{

}


/*============================================================================*/

void UpdateMouse(void)
{

}

void MouseShow(void)
{
	
}

void MouseHide(void)
{
	
}

int MouseGetClick(short* x, short* y)
{
	if(x != NULL)
		*x = 0;
	if(y != NULL)
		*y = 0;
	
	return 0;
}


void /*_loadds far      */ MouseMove(int x, int y)
//#pragma aux MouseMove parm[ecx][edx]
{
	(void)x;
	(void)y;
}


void ResetMouse(void)
{

}


void M_Init(void)
{

}


void M_Shutdown(void)
{

}


/***************************************************************************/

void lock_region(void* address, unsigned length)
{
	(void)address; (void)length;
}


void unlock_region(void* address, unsigned length)
{
	(void)address; (void)length;
}


void timerstub1(void);
void timerstub2(void);

void INT_Setup(void)
{
    memset(keyboard, 0, sizeof(keyboard));
    M_Init();
    dInitTimer();
    dStartTimer(INT_TimerISR, TICKS(70));
    timeractive = true;
    J_Init();
}


void INT_ShutdownKeyboard(void)
{

}


void INT_Shutdown(void)
{

}
