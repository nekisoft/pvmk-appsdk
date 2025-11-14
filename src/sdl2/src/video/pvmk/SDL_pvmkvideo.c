/*
	Neki32 Application SDK - This file placed in the public domain.
	Bryan Topp <betopp@betopp.com>, Nekisoft Pty Ltd (ACN 680 583 251) 2025
*/
#include "../../SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PVMK

#include "../SDL_sysvideo.h"
#include "SDL_pvmkevents_c.h"
#include "SDL_pvmkframebuffer_c.h"
#include "SDL_pvmkvideo.h"

#define PVMKVID_DRIVER_NAME "pvmk"

static int PVMK_VideoInit(_THIS);
static void PVMK_VideoQuit(_THIS);
static void PVMK_GetDisplayModes(_THIS, SDL_VideoDisplay *display);
static int PVMK_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode);
static int PVMK_GetDisplayBounds(_THIS, SDL_VideoDisplay *display, SDL_Rect *rect);
static int PVMK_CreateWindow(_THIS, SDL_Window *window);
static void PVMK_DestroyWindow(_THIS, SDL_Window *window);

typedef struct
{
    int fmt;
} DisplayDriverData;

typedef struct
{
    int fmt;
} ModeDriverData;

static const struct
{
    SDL_PixelFormatEnum pixfmt;
    int width, height, scfmt;
} format_map[] = {
    { 320, 240, SDL_PIXELFORMAT_RGB565, _SC_GFX_MODE_320X240_16BPP },
    { 640, 480, SDL_PIXELFORMAT_RGB565, _SC_GFX_MODE_VGA_16BPP },
};

/* PVMK driver bootstrap functions */

static void PVMK_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device->displays);
    SDL_free(device->driverdata);
    SDL_free(device);
}

static SDL_VideoDevice *PVMK_CreateDevice(void)
{
    SDL_VideoDevice *device;
    SDL_VideoData *phdata;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return 0;
    }

    /* Initialize internal data */
    phdata = (SDL_VideoData *)SDL_calloc(1, sizeof(SDL_VideoData));
    if (!phdata) {
        SDL_OutOfMemory();
        SDL_free(device);
        return NULL;
    }

    device->driverdata = phdata;

    device->VideoInit = PVMK_VideoInit;
    device->VideoQuit = PVMK_VideoQuit;

    device->GetDisplayModes = PVMK_GetDisplayModes;
    device->SetDisplayMode = PVMK_SetDisplayMode;
    device->GetDisplayBounds = PVMK_GetDisplayBounds;

    device->CreateSDLWindow = PVMK_CreateWindow;
    device->DestroyWindow = PVMK_DestroyWindow;

    device->PumpEvents = PVMK_PumpEvents;

    device->CreateWindowFramebuffer = SDL_PVMK_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = SDL_PVMK_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = SDL_PVMK_DestroyWindowFramebuffer;

    device->free = PVMK_DeleteDevice;

    device->quirk_flags = VIDEO_DEVICE_QUIRK_FULLSCREEN_ONLY;

    return device;
}

VideoBootStrap PVMK_bootstrap = { PVMKVID_DRIVER_NAME, "Neki32 Video Driver", PVMK_CreateDevice, NULL /* no ShowMessageBox implementation */ };

static int PVMK_VideoInit(_THIS)
{
    SDL_DisplayMode mode;
    ModeDriverData *modedata;
    SDL_VideoDisplay display;
    DisplayDriverData *display_driver_data = SDL_calloc(1, sizeof(DisplayDriverData));
    if (!display_driver_data) {
        return SDL_OutOfMemory();
    }

    SDL_zero(mode);
    SDL_zero(display);

    display_driver_data->fmt = format_map[0].scfmt;

    modedata = SDL_malloc(sizeof(ModeDriverData));
    if (!modedata) {
        return SDL_OutOfMemory();
    }

    mode.w = format_map[0].width;
    mode.h = format_map[0].height;
    mode.refresh_rate = 60;
    mode.format = format_map[0].pixfmt;
    mode.driverdata = modedata;
    modedata->fmt = format_map[0].scfmt;
    
    display.name = "Neki32 TV";
    display.desktop_mode = mode;
    display.current_mode = mode;
    display.driverdata = display_driver_data;

    return SDL_AddVideoDisplay(&display, SDL_FALSE);
}

static void PVMK_VideoQuit(_THIS)
{
}

static void PVMK_GetDisplayModes(_THIS, SDL_VideoDisplay *display)
{
    for (int i = 0; i < SDL_arraysize(format_map); i++) {
    
        ModeDriverData *modedata = SDL_malloc(sizeof(ModeDriverData));
        if (!modedata)
            continue;

	SDL_DisplayMode mode;
        SDL_zero(mode);
	
        mode.w = format_map[i].width;
        mode.h = format_map[i].height;
        mode.refresh_rate = 60;
        mode.format = format_map[i].pixfmt;
        mode.driverdata = modedata;
        modedata->fmt = format_map[i].scfmt;
	
        if (!SDL_AddDisplayMode(display, &mode)) {
            SDL_free(modedata);
        }
    }
}

static int PVMK_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    DisplayDriverData *displaydata = display->driverdata;
    ModeDriverData *modedata = mode->driverdata;
    
    displaydata->fmt = modedata->fmt;
    return 0;
}

static int PVMK_GetDisplayBounds(_THIS, SDL_VideoDisplay *display, SDL_Rect *rect)
{
    DisplayDriverData *driver_data = (DisplayDriverData *)display->driverdata;
    if (!driver_data) {
        return -1;
    }
    
    rect->x = 0;
    rect->y = 0;
    
    for (int i = 0; i < SDL_arraysize(format_map); i++) {
    
	if(driver_data->fmt == format_map[i].scfmt)
	{
		rect->w = format_map[i].width;
		rect->h = format_map[i].height;
		return 0;
	}
    }

    return -1;
}

static int PVMK_CreateWindow(_THIS, SDL_Window *window)
{
    DisplayDriverData *display_data;
    SDL_WindowData *window_data = (SDL_WindowData *)SDL_calloc(1, sizeof(SDL_WindowData));
    if (!window_data) {
        return SDL_OutOfMemory();
    }
    display_data = (DisplayDriverData *)SDL_GetDisplayDriverData(window->display_index);
    window_data->fmt = display_data->fmt;
    window->driverdata = window_data;
    SDL_SetKeyboardFocus(window);
    return 0;
}

static void PVMK_DestroyWindow(_THIS, SDL_Window *window)
{
    if (!window) {
        return;
    }
    SDL_free(window->driverdata);
}

#endif /* SDL_VIDEO_DRIVER_PVMK */

/* vi: set sts=4 ts=4 sw=4 expandtab: */
