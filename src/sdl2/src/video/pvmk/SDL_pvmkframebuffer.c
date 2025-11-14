/*
	Neki32 Application SDK - This file placed in the public domain.
	Bryan Topp <betopp@betopp.com>, Nekisoft Pty Ltd (ACN 680 583 251) 2025
*/
#include "../../SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PVMK

#include "../SDL_sysvideo.h"
#include "SDL_pvmkframebuffer_c.h"
#include "SDL_pvmkvideo.h"

#define PVMK_SURFACE "_SDL_PVMKSurface"


static SDL_Window *Pvmk_Window = NULL;
static int Pvmk_Gfx_Mode = 0;
static uint16_t Pvmk_Framebuffers[3][640*480];

int SDL_PVMK_CreateWindowFramebuffer(_THIS, SDL_Window *window, Uint32 *format, void **pixels, int *pitch)
{
    SDL_PVMK_DestroyWindowFramebuffer(_this, window);

    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &mode);
    
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    
    int bpp = 0;
    uint32_t rmask = 0;
    uint32_t gmask = 0;
    uint32_t bmask = 0;
    uint32_t amask = 0;
    SDL_PixelFormatEnumToMasks(mode.format, &bpp, &rmask, &gmask, &bmask, &amask);
    
    SDL_Surface *framebuffer = SDL_CreateRGBSurfaceFrom(Pvmk_Framebuffers[0], w, h, bpp, bpp*w, rmask, gmask, bmask, amask);
    if (!framebuffer) {
        return SDL_OutOfMemory();
    }

    SDL_SetWindowData(window, PVMK_SURFACE, framebuffer);
    *format = mode.format;
    *pixels = framebuffer->pixels;
    *pitch = framebuffer->pitch;
    return 0;
}

int SDL_PVMK_UpdateWindowFramebuffer(_THIS, SDL_Window *window, const SDL_Rect *rects, int numrects)
{
    SDL_WindowData *drv_data = (SDL_WindowData *)window->driverdata;
    SDL_Surface *surface;
    void *framebuffer;

    surface = (SDL_Surface *)SDL_GetWindowData(window, PVMK_SURFACE);
    if (!surface) {
        return SDL_SetError("%s: Unable to get the window surface.", __func__);
    }

    //Display the buffer we rendered to
    int current = _sc_gfx_flip(drv_data->fmt, surface->pixels);

    //Pick the next framebuffer to use
    for(int bb = 0; bb < 3; bb++)
    {
	if((uintptr_t)(Pvmk_Framebuffers[bb]) == (uintptr_t)(surface->pixels))
		continue; //Just enqueued this one
	if((uintptr_t)(Pvmk_Framebuffers[bb]) == (uintptr_t)(current))
		continue; //This one being displayed already
		
	//Found the next one we'll use
	surface->pixels = (void*)&(Pvmk_Framebuffers[bb][0]);
	break;
    }
    
    if(window->surface != NULL)
	window->surface->pixels = surface->pixels;

    return 0;
}

void SDL_PVMK_DestroyWindowFramebuffer(_THIS, SDL_Window *window)
{
    SDL_Surface *surface;
    surface = (SDL_Surface *)SDL_SetWindowData(window, PVMK_SURFACE, NULL);
    SDL_FreeSurface(surface);
}

#endif /* SDL_VIDEO_DRIVER_PVMK */

/* vi: set sts=4 ts=4 sw=4 expandtab: */
