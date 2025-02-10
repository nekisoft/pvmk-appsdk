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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#if 0

/**** CONSTANTS ****/

#define PATHS 2

char* paths_to_check[PATHS] = { "DOS4GPATH", "PATH" };
char* loader                = "DOS4GW.EXE";


/**** VARIABLES ****/

char fullpath[64];


/**** FUNCTIONS ****/

char* dos4g_path(void)
{
    int i;

    for (i = 0; i < PATHS; i++)
    {
        _searchenv(loader, paths_to_check[i], fullpath);
        if (fullpath[0])
            return fullpath;
    }
    return loader;
}


void main(int argc, char* argv[])
{
    char* av[4];
    char  cmdline[128];

    av[0] = dos4g_path();    /* Locate the DOS/4G loader */
    av[1] = argv[0];         /* name of executable to run */
    av[2] = getcmd(cmdline); /* command line */
    av[3] = NULL;            /* end of list */
    setbuf(stdout, NULL);
    putenv("DOS4GVM=MAXMEM#16384 VIRTUALSIZE#16384 SWAPNAME#GREED.SWP");
    // putenv("DOS4GVM=MAXMEM#8192 VIRTUALSIZE#12288 SWAPNAME#GREED.SWP");
    // putenv("DOS16M=^0x02");
    execvp(av[0], av);
    puts("Stub Failure:"); /* error */
    puts(av[0]);
    puts(strerror(errno));
    exit(255); /* indicate error to dos*/
}

#endif
