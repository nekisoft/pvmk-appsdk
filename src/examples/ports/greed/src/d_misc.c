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
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "d_global.h"
#include "d_ints.h"
#include "d_video.h"
#include "d_misc.h"
#include "protos.h"

//pvmk - use posixy name
#include <strings.h>
#define stricmp strcasecmp

/**** CONSTANTS ****/

#define DPMIINT 0x31
#define DPMILOCKMEM 0x600
#define DPMIUNLOCKMEM 0x601
#define DOSGETMEM 0x100
#define DOSFREEMEM 0x101


/**** VARIABLES ****/

int    my_argc;
char** my_argv;
int    rndofs;
byte   rndtable[512] = {
    0,   8,   109, 220, 222, 241, 149, 107, 75,  248, 254, 140, 16,  66,  74,  21,  211, 47,  80,
    242, 154, 27,  205, 128, 161, 89,  77,  36,  95,  110, 85,  48,  212, 140, 211, 249, 22,  79,
    200, 50,  28,  188, 52,  140, 202, 120, 68,  145, 62,  70,  184, 190, 91,  197, 152, 224, 149,
    104, 25,  178, 252, 182, 202, 182, 141, 197, 4,   81,  181, 242, 145, 42,  39,  227, 156, 198,
    225, 193, 219, 93,  122, 175, 249, 0,   175, 143, 70,  239, 46,  246, 163, 53,  163, 109, 168,
    135, 2,   235, 25,  92,  20,  145, 138, 77,  69,  166, 78,  176, 173, 212, 166, 113, 94,  161,
    41,  50,  239, 49,  111, 164, 70,  60,  2,   37,  171, 75,  136, 156, 11,  56,  42,  146, 138,
    229, 73,  146, 77,  61,  98,  196, 135, 106, 63,  197, 195, 86,  96,  203, 113, 101, 170, 247,
    181, 113, 80,  250, 108, 7,   255, 237, 129, 226, 79,  107, 112, 166, 103, 241, 24,  223, 239,
    120, 198, 58,  60,  82,  128, 3,   184, 66,  143, 224, 145, 224, 81,  206, 163, 45,  63,  90,
    168, 114, 59,  33,  159, 95,  28,  139, 123, 98,  125, 196, 15,  70,  194, 253, 54,  14,  109,
    226, 71,  17,  161, 93,  186, 87,  244, 138, 20,  52,  123, 251, 26,  36,  17,  46,  52,  231,
    232, 76,  31,  221, 84,  37,  216, 165, 212, 106, 197, 242, 98,  43,  39,  175, 254, 145, 190,
    84,  118, 222, 187, 136, 120, 163, 236, 249, 74,  21,  211, 47,  80,  242, 154, 27,  205, 128,
    161, 89,  77,  36,  149, 104, 25,  178, 252, 182, 202, 182, 141, 197, 4,   81,  181, 242, 135,
    106, 63,  197, 195, 86,  96,  203, 113, 101, 170, 247, 181, 113, 71,  17,  161, 93,  186, 87,
    244, 138, 20,  52,  123, 251, 26,  36,  197, 242, 98,  43,  39,  175, 254, 145, 190, 84,  118,
    222, 187, 136, 25,  92,  20,  145, 138, 77,  69,  166, 78,  176, 173, 212, 166, 113, 94,  161,
    41,  50,  239, 49,  111, 164, 70,  60,  2,   37,  171, 75,  175, 143, 70,  239, 46,  246, 163,
    53,  163, 109, 168, 135, 2,   235, 52,  140, 202, 120, 68,  145, 62,  70,  184, 190, 91,  197,
    152, 224, 145, 224, 81,  206, 163, 45,  63,  90,  168, 114, 59,  33,  159, 95,  17,  46,  52,
    231, 232, 76,  31,  221, 84,  37,  216, 165, 212, 106, 80,  250, 108, 7,   255, 237, 129, 226,
    79,  107, 112, 166, 103, 241, 145, 42,  39,  227, 156, 198, 225, 193, 219, 93,  122, 175, 249,
    0,   95,  110, 85,  48,  212, 140, 211, 249, 22,  79,  200, 50,  28,  188, 0,   8,   109, 220,
    222, 241, 149, 107, 75,  248, 254, 140, 16,  66,  136, 156, 11,  56,  42,  146, 138, 229, 73,
    146, 77,  61,  98,  196, 24,  223, 239, 120, 198, 58,  60,  82,  128, 3,   184, 66,  143, 224,
    28,  139, 123, 98,  125, 196, 15,  70,  194, 253, 54,  14,  109, 226, 120, 163, 236, 249
};

/**** FUNCTIONS ****/

int MS_CheckParm(char* check)
/* checks for a parameter on the command line
   returns argument number if found */
{
    int   i;
    char* parm;

    for (i = 1; i < my_argc; i++)
    {
        parm = my_argv[i];
        while (!isalpha(*parm))  // skip - / \ etc.. in front of parm
            if (!*parm++)
                break;  // hit end of string without an alphanum
        if (!stricmp(check, parm))
            return i;
    }
    return 0;
}



void MS_Error(char* error, ...)
/* exit with an error message
   shuts everything down */
{
    va_list argptr;
//    byte*   vidmode;

    INT_Shutdown();
    //vidmode = (byte*) (0x40 * 16 + 0x49);
    //if (*vidmode != 3)
        VI_SetTextMode();
    va_start(argptr, error);
    vprintf(error, argptr);
    va_end(argptr);
    //while (kbhit())
    //    getch();
    //exit(1);
    while(1) { sleep(1); }
}


void MS_ExitClean(void)
/* exits without error message */
{
    INT_Shutdown();
    VI_SetTextMode();
    exit(0);
}
