/****************************************************************************
 *
 *                   Digital Sound Interface Kit (DSIK)
 *                            Version 2.00
 *
 *                           by Carlos Hasan
 *
 * Filename:     timer.c
 * Version:      Revision 1.1
 *
 * Language:     WATCOM C
 * Environment:  IBM PC (DOS/4GW)
 *
 * Description:  Timer interrupt services.
 *
 * Revision History:
 * ----------------
 *
 * Revision 1.1  94/11/16  10:48:42  chv
 * Added VGA vertical retrace synchronization code
 *
 * Revision 1.0  94/10/28  22:45:47  chv
 * Initial revision
 *
 ****************************************************************************/

#include <stdlib.h>
#include "timer.h"


//static void(interrupt far* BiosTimer)(void) = NULL;
//static void (*UserTimer)(void);
//static long TimerCount, TimerSpeed, TimerSync;

void        timerstub1(void) {}


void dInitTimer(void)
{

}


void dDoneTimer(void)
{

}


void dSetTimerSpeed(int Speed)
{
	(void)Speed;
}


void dStartTimer(TimerProc Timer, int Speed)
{
	(void)Timer;
	(void)Speed;
}


void dStopTimer(void)
{

}

void timerstub2(void) {}
