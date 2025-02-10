/****************************************************************************
 *
 *                   Digital Sound Interface Kit (DSIK)
 *                            Version 2.00
 *
 *                           by Carlos Hasan
 *
 * Filename:     detect.c
 * Version:      Revision 1.1
 *
 * Language:     WATCOM C
 * Environment:  IBM PC (DOS/4GW)
 *
 * Description:  Soundcards autodetection routines.
 *
 * Revision History:
 * ----------------
 *
 * Revision 1.1  94/12/28  13:02:27  chv
 * New Windows Sound System detection routine.
 *
 * Revision 1.0  94/11/20  18:02:36  chv
 * Initial revision
 *
 ****************************************************************************/
/* Destroyed by betopp 2025 - pvmk needs none of this crap! */

#include <stdio.h>
#include <string.h>
#include "d_global.h"
#include "audio.h"

/****************************************************************************
 *
 * Function:     dAutoDetect
 * Parameters:   SC  - Soundcard structure
 *
 * Description:  Autodetect soundcard device.
 *
 ****************************************************************************/

int dAutoDetect(SoundCard* SC)
{
	//pvmk
	SC->Port = SC->IrqLine = SC->DmaChannel = 0;
	SC->SampleRate = 48000;
	SC->Modes      = AF_16BITS | AF_STEREO;
	SC->ID         = ID_PVMK;
	return 0;
}
