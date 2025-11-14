/*
	Neki32 Application SDK - This file placed in the public domain.
	Bryan Topp <betopp@betopp.com>, Nekisoft Pty Ltd (ACN 680 583 251) 2025
*/
#ifndef SDL_pvmkframebuffer_c_h_
#define SDL_pvmkframebuffer_c_h_

#include "../../SDL_internal.h"

int SDL_PVMK_CreateWindowFramebuffer(_THIS, SDL_Window *window, Uint32 *format, void **pixels, int *pitch);
int SDL_PVMK_UpdateWindowFramebuffer(_THIS, SDL_Window *window, const SDL_Rect *rects, int numrects);
void SDL_PVMK_DestroyWindowFramebuffer(_THIS, SDL_Window *window);

#endif /* SDL_pvmkframebuffer_c_h_ */

/* vi: set sts=4 ts=4 sw=4 expandtab: */
