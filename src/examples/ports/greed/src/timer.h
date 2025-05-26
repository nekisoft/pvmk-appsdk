/****************************************************************************
 *
 *                   Digital Sound Interface Kit (DSIK)
 *                            Version 2.00
 *
 *                           by Carlos Hasan
 *
 * Filename:     timer.h
 * Version:      Revision 1.0
 *
 * Language:     WATCOM C
 * Environment:  IBM PC (DOS/4GW)
 *
 * Description:  Timer interrupt services header file.
 *
 * Revision History:
 * ----------------
 *
 * Revision 1.0  94/10/28  22:45:47  chv
 * Initial revision
 *
 ****************************************************************************/

#ifndef __TIMER_H
#define __TIMER_H

#define TICKS(hz) ((int) (1193182L / (hz)))

#ifdef __cplusplus
extern "C"
{
#endif

    /* Timer services API prototypes */

    typedef void (*TimerProc)(void);

    void dInitTimer(void);
    void dDoneTimer(void);
    void dSetTimerSpeed(int Speed);
    void dStartTimer(TimerProc Timer, int Speed);
    void dStopTimer(void);

#ifdef __cplusplus
}
#endif

/* Register calling conventions used by the API routines */
/*
#pragma aux dInitTimer "_*" parm[];
#pragma aux dDoneTimer "_*" parm[];
#pragma aux dSetTimerSpeed "_*" parm[eax];
#pragma aux dStartTimer "_*" parm[eax][edx];
#pragma aux dStopTimer "_*" parm[];
*/

#endif
