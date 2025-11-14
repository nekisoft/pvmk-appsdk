/*
	Neki32 Application SDK - This file placed in the public domain.
	Bryan Topp <betopp@betopp.com>, Nekisoft Pty Ltd (ACN 680 583 251) 2025
*/
#include "../../SDL_internal.h"

#ifndef SDL_pvmkvideo_h_
#define SDL_pvmkvideo_h_

#include <sc.h>

#include "../SDL_sysvideo.h"

typedef struct SDL_VideoData
{
    int fmt;
} SDL_VideoData;

typedef struct SDL_WindowData
{
    int fmt;
} SDL_WindowData;

#endif /* SDL_pvmkvideo_h_ */

/* vi: set sts=4 ts=4 sw=4 expandtab: */
