// Modified by Bryan E. Topp, 2021
//
// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"

#include "m_argv.h"
#include "d_main.h"

#include <sc.h>

int
app_main
( int		argc,
  char**	argv ) 
{ 
    myargc = argc; 
    myargv = argv; 
 
    D_DoomMain (); 

    return 0;
} 

//Entry point: on PVMK this just calls app_main(). The desktop SDL shim uses it to
//take the real main thread for SDL and runs app_main() on a worker thread instead -
//see sdlsc/include/sc.h.
int
main
( int		argc,
  char**	argv )
{
    return _sc_shim_main(app_main, argc, argv);
}
