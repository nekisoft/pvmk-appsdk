//SDL_pvmk.c
//SDL implementations for PVMK
//Bryan E. Topp <betopp@betopp.com> 2025

#include "SDL_internal.h"
#include "video/SDL_sysvideo.h"
#include "video/SDL_pixels_c.h"
#include "events/SDL_events_c.h"

#if SDL_PLATFORM_PVMK

//PVMK system calls
#include <sc.h>

char *SDL_SYS_GetBasePath(void)
{
	return strdup("/");
}
char *SDL_SYS_GetPrefPath(const char *org, const char *app)
{
	(void)org;
	(void)app;
	return strdup("/");
}

char *SDL_SYS_GetUserFolder(SDL_Folder folder)
{
	(void)folder;
	return strdup("/");
}

char *SDL_SYS_GetCurrentDirectory(void)
{
	return strdup("/");
}

void SDL_SYS_DelayNS(Uint64 ns)
{
	Uint64 start_ns = (Uint64)_sc_getticks() * (Uint64)1000000u;
	while(1)
	{
		Uint64 now_ns = (Uint64)_sc_getticks() * (Uint64)1000000u;
		if(now_ns >= start_ns + ns)
			return;
		else
			_sc_pause();
	}
}

//Todo - we could spare another timer on the chip and get a better-than-ms count here.
//Needs extra kernel work + syscall.
Uint64 SDLCALL SDL_GetPerformanceFrequency(void)
{
	return 1000;
}

Uint64 SDLCALL SDL_GetPerformanceCounter(void)
{
	return _sc_getticks();
}

struct SDL_WindowData
{
	//Triple buffering behind the scenes
	SDL_Surface *fbs[3];
	int fbnext;
};

struct SDL_VideoData
{
	int dummy; //PVMK tracks no state here
};

struct SDL_DisplayData
{
	int dummy; //PVMK tracks no state here
};

struct SDL_DisplayModeData
{
	int gfx_mode; //Value given to _sc_gfx_flip
};

static SDL_Window *Pvmk_Window = NULL;
static int Pvmk_Gfx_Mode = 0;
static uint16_t Pvmk_Framebuffers[3][640*480];

static bool PVMK_VideoInit(SDL_VideoDevice *_this)
{
    SDL_DisplayMode mode;
    SDL_DisplayModeData *modedata;
    SDL_VideoDisplay display;
    SDL_DisplayData *display_driver_data = SDL_calloc(1, sizeof(SDL_DisplayData));
    if (!display_driver_data) {
        return false;
    }

    SDL_zero(mode);
    SDL_zero(display);

    modedata = SDL_malloc(sizeof(SDL_DisplayModeData));
    if (!modedata) {
        return false;
    }

    mode.w = 320;
    mode.h = 240;
    mode.refresh_rate = 60.0f;
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.internal = modedata;
    modedata->gfx_mode = _SC_GFX_MODE_320X240_16BPP;
    Pvmk_Gfx_Mode = _SC_GFX_MODE_320X240_16BPP;

    display.name = "pvmk";
    display.desktop_mode = mode;
    display.internal = display_driver_data;

    if (SDL_AddVideoDisplay(&display, false) == 0) {
        return false;
    }
    return true;
}

static void PVMK_VideoQuit(SDL_VideoDevice *_this)
{
    (void)_this;
}

static bool PVMK_GetDisplayModes(SDL_VideoDevice *_this, SDL_VideoDisplay *display)
{
    (void)display;
    (void)_this;
    
    SDL_DisplayModeData *modedata;
    SDL_DisplayMode mode;
    
    static const struct { int h; int w; int px; int hz; } modetab[_SC_GFX_MODE_MAX] = 
    {
	[_SC_GFX_MODE_VGA_16BPP]     = { .h = 480, .w = 640, .px = SDL_PIXELFORMAT_RGB565, .hz = 60 },
	[_SC_GFX_MODE_320X240_16BPP] = { .h = 240, .w = 320, .px = SDL_PIXELFORMAT_RGB565, .hz = 60 },
    };
    
    for (int i = 0; i < _SC_GFX_MODE_MAX; i++) {
	if(modetab[i].w == 0 || modetab[i].h == 0)
		continue;
		
        modedata = SDL_malloc(sizeof(SDL_DisplayModeData));
        if (!modedata)
            continue;

        SDL_zero(mode);
        mode.w = modetab[i].w;
        mode.h = modetab[i].h;
        mode.refresh_rate = modetab[i].hz;
        mode.format = modetab[i].px;
        mode.internal = modedata;
	modedata->gfx_mode = i;

        if (!SDL_AddFullscreenDisplayMode(display, &mode)) {
            SDL_free(modedata);
        }
    }

    return true;
}

static bool PVMK_SetDisplayMode(SDL_VideoDevice *_this, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    (void)_this;
    (void)display;
    SDL_DisplayModeData *modedata = mode->internal;
    Pvmk_Gfx_Mode = modedata->gfx_mode;
    return true;
}

static bool PVMK_GetDisplayBounds(SDL_VideoDevice *_this, SDL_VideoDisplay *display, SDL_Rect *rect)
{
    (void)_this;
    rect->x = 0;
    rect->y = 0;
    rect->w = display->current_mode->w;
    rect->h = display->current_mode->h;
    return true;
}

static bool PVMK_CreateWindow(SDL_VideoDevice *_this, SDL_Window *window, SDL_PropertiesID create_props)
{
    (void)_this;
    if(Pvmk_Window != NULL) {
        return SDL_SetError("%s: Only one window supported.", __func__);
    }
    
    SDL_DisplayData *display_data;
    SDL_WindowData *window_data = (SDL_WindowData *)SDL_calloc(1, sizeof(SDL_WindowData));
    if (!window_data) {
        return false;
    }
    display_data = SDL_GetDisplayDriverDataForWindow(window);
    window->internal = window_data;
    SDL_SetKeyboardFocus(window);
    Pvmk_Window = window;
    return true;
}

static void PVMK_DestroyWindow(SDL_VideoDevice *_this, SDL_Window *window)
{
    (void)_this;
    if (!window) {
        return;
    }
    SDL_free(window->internal);
    
    if(Pvmk_Window == window)
	Pvmk_Window = NULL;
}

static void PVMK_PumpEvents(SDL_VideoDevice *_this)
{
(void)_this;
/*
        SDL_Event ev;
        ev.type = SDL_EVENT_QUIT;
        ev.common.timestamp = 0;
        SDL_PushEvent(&ev);
        return;
    */
}

#define PVMK_SURFACE "SDL.internal.window.surface"

static void PVMK_DestroyWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_ClearProperty(SDL_GetWindowProperties(window), PVMK_SURFACE);
}

static bool PVMK_CreateWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window, SDL_PixelFormat *format, void **pixels, int *pitch)
{
    SDL_WindowData *drv_data = window->internal;
    const SDL_DisplayMode *mode;
    SDL_Surface *framebuffer;
    int w, h;

    PVMK_DestroyWindowFramebuffer(_this, window);

    mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
    SDL_GetWindowSizeInPixels(window, &w, &h);
    
    framebuffer = SDL_CreateSurfaceFrom(w, h, mode->format, Pvmk_Framebuffers[0], w*2);
    if(framebuffer == NULL)
        return false;
	
    SDL_SetSurfaceProperty(SDL_GetWindowProperties(window), PVMK_SURFACE, framebuffer);
    *format = mode->format;
    *pixels = framebuffer->pixels;
    *pitch = framebuffer->pitch;
    return true;
}

static bool PVMK_UpdateWindowFramebuffer(SDL_VideoDevice *_this, SDL_Window *window, const SDL_Rect *rects, int numrects)
{
    SDL_WindowData *drv_data = window->internal;
    SDL_Surface *surface;
    surface = (SDL_Surface *)SDL_GetPointerProperty(SDL_GetWindowProperties(window), PVMK_SURFACE, NULL);
    if (!surface) {
        return SDL_SetError("%s: Unable to get the window surface.", __func__);
    }

    //Display the buffer we rendered to
    int current = _sc_gfx_flip(Pvmk_Gfx_Mode, surface->pixels);
    
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
    
    return true;
}




static void PVMK_Destroy(SDL_VideoDevice *device)
{
    SDL_free(device->internal);
    SDL_free(device);
}

static SDL_VideoDevice *PVMK_Create(void)
{
    SDL_VideoDevice *device;
    SDL_VideoData *phdata;

    // Initialize SDL_VideoDevice structure
    device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        return NULL;
    }

    // Initialize internal PVMK specific data
    phdata = (SDL_VideoData *)SDL_calloc(1, sizeof(SDL_VideoData));
    if (!phdata) {
        SDL_free(device);
        return NULL;
    }
    device->internal = phdata;

    // Setup amount of available displays and current display
    device->num_displays = 0;

    // Set device free function
    device->free = PVMK_Destroy;

    // Setup all functions which we can handle
    device->VideoInit = PVMK_VideoInit;
    device->VideoQuit = PVMK_VideoQuit;
    device->GetDisplayModes = PVMK_GetDisplayModes;
    device->SetDisplayMode = PVMK_SetDisplayMode;
    device->GetDisplayBounds = PVMK_GetDisplayBounds;
    device->CreateSDLWindow = PVMK_CreateWindow;
    device->DestroyWindow = PVMK_DestroyWindow;
    device->PumpEvents = PVMK_PumpEvents;
    device->CreateWindowFramebuffer = PVMK_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = PVMK_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = PVMK_DestroyWindowFramebuffer;
    device->device_caps = VIDEO_DEVICE_CAPS_FULLSCREEN_ONLY;
    
    return device;
}



VideoBootStrap PVMK_bootstrap = {
    "pvmk",
    "PVMK Video Driver",
    PVMK_Create,
    NULL // no ShowMessageBox implementation
};

#endif //SDL_PLATFORM_PVMK
