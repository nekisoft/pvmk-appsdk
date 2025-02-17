//SDL_pvmk.c
//SDL implementations for PVMK
//Bryan E. Topp <betopp@betopp.com> 2025

#include "SDL_internal.h"
#include "audio/SDL_sysaudio.h"
#include "audio/SDL_audiodev_c.h"
#include "video/SDL_sysvideo.h"
#include "video/SDL_pixels_c.h"
#include "events/SDL_events_c.h"
#include "joystick/SDL_sysjoystick.h"

#if SDL_PLATFORM_PVMK

//PVMK system calls
#include <sc.h>

SDL_AudioDevice *SDL_PVMK_AudioDevice = NULL;

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
		{
			if(SDL_PVMK_AudioDevice != NULL)
				SDL_PlaybackAudioThreadIterate(SDL_PVMK_AudioDevice);
				
			_sc_pause();
		}
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

    mode.w = 640;
    mode.h = 480;
    mode.refresh_rate = 60.0f;
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.internal = modedata;
    modedata->gfx_mode = _SC_GFX_MODE_VGA_16BPP;
    Pvmk_Gfx_Mode = _SC_GFX_MODE_VGA_16BPP;

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
	
	if(SDL_PVMK_AudioDevice != NULL)
	{
		SDL_PlaybackAudioThreadIterate(SDL_PVMK_AudioDevice);
	}
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
    
	if(SDL_PVMK_AudioDevice != NULL)
	{
		SDL_PlaybackAudioThreadIterate(SDL_PVMK_AudioDevice);
	}
    
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

static bool PVMK_JoystickInit(void)
{
    SDL_PrivateJoystickAdded(1);
    SDL_PrivateJoystickAdded(2);
    SDL_PrivateJoystickAdded(3);
    SDL_PrivateJoystickAdded(4);
    return true;
}

static const char *PVMK_JoystickGetDeviceName(int device_index)
{
    static const char *names[] =
    {
	"Neki32 Pad A",
	"Neki32 Pad B",
	"Neki32 Pad C",
	"Neki32 Pad D",
    };
    if(device_index < 0 || device_index >= (int)SDL_arraysize(names))
	return "";
    else
	return names[device_index];
	
}

static int PVMK_JoystickGetCount(void)
{
    return 4;
}

static SDL_GUID PVMK_JoystickGetDeviceGUID(int device_index)
{
    return SDL_CreateJoystickGUIDForName(PVMK_JoystickGetDeviceName(device_index));
}

static SDL_JoystickID PVMK_JoystickGetDeviceInstanceID(int device_index)
{
    return device_index + 1;
}

static bool PVMK_JoystickOpen(SDL_Joystick *joystick, int device_index)
{
    (void)device_index;
    
    joystick->nbuttons = 12;
    joystick->naxes = 0;
    joystick->nhats = 0;

    return true;
}

static bool PVMK_JoystickSetSensorsEnabled(SDL_Joystick *joystick, bool enabled)
{
    return SDL_Unsupported();
}

static void PVMK_JoystickUpdate(SDL_Joystick *joystick)
{
	(void)joystick;
	static uint16_t last_buttons[4] = {0};
	
	int ticks = _sc_getticks();
	_sc_input_t input = {0};
	while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
	{
		switch(input.format)
		{
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			{
				int player = input.format - 'A';
				SDL_Joystick *js = SDL_GetJoystickFromID(1 + player);
				if(js)
				{
					int presses = input.buttons & ~last_buttons[player];
					int releases = last_buttons[player] & ~input.buttons;
					for(int bit = 0; bit < 16; bit++)
					{
						if(presses & (1u << bit))
							SDL_SendJoystickButton(ticks, js, bit, 1);
						if(releases & (1u << bit))
							SDL_SendJoystickButton(ticks, js, bit, 0);
					}
				}
				last_buttons[player] = input.buttons;
			}
			break;
			
			default:
			break;
		}
	}
}

static void PVMK_JoystickClose(SDL_Joystick *joystick)
{
	(void)joystick;
}

static void PVMK_JoystickQuit(void)
{

}

static bool PVMK_JoystickGetGamepadMapping(int device_index, SDL_GamepadMapping *out)
{
    // There is only one possible mapping.
    *out = (SDL_GamepadMapping){
        .a             = { EMappingKind_Button, _SC_BTNIDX_A },
        .b             = { EMappingKind_Button, _SC_BTNIDX_B },
        .x             = { EMappingKind_Button, _SC_BTNIDX_X },
        .y             = { EMappingKind_Button, _SC_BTNIDX_Y },
        .back          = { EMappingKind_Button, _SC_BTNIDX_MODE },
        .guide         = { EMappingKind_None,   255 },
        .start         = { EMappingKind_Button, _SC_BTNIDX_START },
        .leftstick     = { EMappingKind_None,   255 },
        .rightstick    = { EMappingKind_None,   255 },
        .leftshoulder  = { EMappingKind_Button, _SC_BTNIDX_Z },
        .rightshoulder = { EMappingKind_Button, _SC_BTNIDX_C },
        .dpup          = { EMappingKind_Button, _SC_BTNIDX_UP },
        .dpdown        = { EMappingKind_Button, _SC_BTNIDX_DOWN },
        .dpleft        = { EMappingKind_Button, _SC_BTNIDX_LEFT },
        .dpright       = { EMappingKind_Button, _SC_BTNIDX_RIGHT },
        .misc1         = { EMappingKind_None,   255 },
        .right_paddle1 = { EMappingKind_None,   255 },
        .left_paddle1  = { EMappingKind_None,   255 },
        .right_paddle2 = { EMappingKind_None,   255 },
        .left_paddle2  = { EMappingKind_None,   255 },
        .leftx         = { EMappingKind_None,   255 },
        .lefty         = { EMappingKind_None,   255 },
        .rightx        = { EMappingKind_None,   255 },
        .righty        = { EMappingKind_None,   255 },
        .lefttrigger   = { EMappingKind_None,   255 },
        .righttrigger  = { EMappingKind_None,   255 },
    };
    return true;
}

static void PVMK_JoystickDetect(void)
{
}

static bool PVMK_JoystickIsDevicePresent(Uint16 vendor_id, Uint16 product_id, Uint16 version, const char *name)
{
    // We don't override any other drivers
    (void)vendor_id;
    (void)product_id;
    (void)version;
    (void)name;
    return false;
}

static const char *PVMK_JoystickGetDevicePath(int device_index)
{
    return NULL;
}

static int PVMK_JoystickGetDeviceSteamVirtualGamepadSlot(int device_index)
{
    return -1;
}

static int PVMK_JoystickGetDevicePlayerIndex(int device_index)
{
    return -1;
}

static void PVMK_JoystickSetDevicePlayerIndex(int device_index, int player_index)
{
    (void)device_index;
    (void)player_index;
}

static bool PVMK_JoystickRumble(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    (void)joystick; (void)low_frequency_rumble; (void)high_frequency_rumble;
    return SDL_Unsupported();
}

static bool PVMK_JoystickRumbleTriggers(SDL_Joystick *joystick, Uint16 left_rumble, Uint16 right_rumble)
{
    (void)joystick; (void)left_rumble; (void)right_rumble;
    return SDL_Unsupported();
}

static bool PVMK_JoystickSetLED(SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue)
{
    (void)joystick; (void)red; (void)green; (void)blue;
    return SDL_Unsupported();
}

static bool PVMK_JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size)
{
    (void)joystick;
    (void)data;
    (void)size;
    return SDL_Unsupported();
}

SDL_JoystickDriver SDL_PVMK_JoystickDriver = {
    PVMK_JoystickInit,
    PVMK_JoystickGetCount,
    PVMK_JoystickDetect,
    PVMK_JoystickIsDevicePresent,
    PVMK_JoystickGetDeviceName,
    PVMK_JoystickGetDevicePath,
    PVMK_JoystickGetDeviceSteamVirtualGamepadSlot,
    PVMK_JoystickGetDevicePlayerIndex,
    PVMK_JoystickSetDevicePlayerIndex,
    PVMK_JoystickGetDeviceGUID,
    PVMK_JoystickGetDeviceInstanceID,
    PVMK_JoystickOpen,
    PVMK_JoystickRumble,
    PVMK_JoystickRumbleTriggers,
    PVMK_JoystickSetLED,
    PVMK_JoystickSendEffect,
    PVMK_JoystickSetSensorsEnabled,
    PVMK_JoystickUpdate,
    PVMK_JoystickClose,
    PVMK_JoystickQuit,
    PVMK_JoystickGetGamepadMapping
};

struct SDL_PrivateAudioData
{
	int dummy;
	void *mixbuf;
};

static bool PVMKAUDIO_OpenDevice(SDL_AudioDevice *device)
{
	if(SDL_PVMK_AudioDevice != NULL)
		return SDL_SetError("Only support one audio device");

    device->hidden = (struct SDL_PrivateAudioData *)SDL_calloc(1, sizeof(*device->hidden));
    if (!device->hidden) {
        return false;
    }

    device->spec.channels = 2;
    device->spec.format = SDL_AUDIO_S16LE;
    device->spec.freq = 48000;
    SDL_UpdatedAudioDeviceFormat(device);

    // Allocate mixing buffer
    if (device->buffer_size >= SDL_MAX_UINT32 / 2) {
        return SDL_SetError("Mixing buffer is too large.");
    }

    device->hidden->mixbuf = (Uint8 *)SDL_malloc(device->buffer_size);
    if (!device->hidden->mixbuf) {
        return false;
    }

    SDL_memset(device->hidden->mixbuf, device->silence_value, device->buffer_size);
    
   
SDL_PVMK_AudioDevice = device;
    SDL_PlaybackAudioThreadSetup(device);
    
    return true;
}

static const Uint8 *PVMKAUDIO_LastBufPtr = NULL;
int PVMKAUDIO_LastBufLen = 0;

static bool PVMKAUDIO_PlayDevice(SDL_AudioDevice *device, const Uint8 *buffer, int buflen)
{
	if(device != SDL_PVMK_AudioDevice)
	{
		//Wrong device? We only allow one open at a time
		return false;
	}
	
	if(PVMKAUDIO_LastBufPtr != NULL)
	{
		//Already have one buffer waiting to go, can't keep going!
		return false;
	}
			
	int enqueued = _sc_snd_play(_SC_SND_MODE_48K_16B_2C, buffer, buflen, buflen * 3);
	if(enqueued == -_SC_EAGAIN)
	{
		//Kernel's buffer is already full
		//We'll retry this one later
		PVMKAUDIO_LastBufPtr = buffer;
		PVMKAUDIO_LastBufLen = buflen;
	}
	else
	{
		//Enqueued it OK
		PVMKAUDIO_LastBufPtr = NULL;
		PVMKAUDIO_LastBufLen = 0;
	}
	
	return true;
}

static bool PVMKAUDIO_WaitDevice(SDL_AudioDevice *device)
{
	//Make sure we're not holding a buffer to play
	while(PVMKAUDIO_LastBufPtr != NULL)
	{
		int enqueued = _sc_snd_play(_SC_SND_MODE_48K_16B_2C, PVMKAUDIO_LastBufPtr, PVMKAUDIO_LastBufLen, PVMKAUDIO_LastBufLen * 3);
		if(enqueued == -_SC_EAGAIN)
		{
			//Keep waiting for it to finish
			_sc_pause();
			continue;
		}
		if(enqueued < 0)
		{
			//Some other error happened
			return false;
		}
		
		//Okay, flushed out the buffer we had
		PVMKAUDIO_LastBufPtr = NULL;
		PVMKAUDIO_LastBufLen = 0;
	}
	return true;
}

static Uint8 *PVMKAUDIO_GetDeviceBuf(SDL_AudioDevice *device, int *buffer_size)
{
	if(PVMKAUDIO_LastBufPtr != NULL)
	{
		//Already have audio filled in a buffer waiting to play...
		int enqueued = _sc_snd_play(_SC_SND_MODE_48K_16B_2C, PVMKAUDIO_LastBufPtr, PVMKAUDIO_LastBufLen, PVMKAUDIO_LastBufLen * 3);
		if(enqueued == -_SC_EAGAIN)
		{
			//Haven't yet played the buffer we already took, can't give any more room
			*buffer_size = 0;
			return NULL;
		}
		if(enqueued < 0)
		{
			//Failed in some way...
			return false;
		}
	
		//Successfully played the buffer we took last time, we have room now
		PVMKAUDIO_LastBufPtr = NULL;
		PVMKAUDIO_LastBufLen = 0;
	}

	*buffer_size = device->buffer_size;
	return device->hidden->mixbuf;
}

static void PVMKAUDIO_CloseDevice(SDL_AudioDevice *device)
{
	SDL_PlaybackAudioThreadShutdown(device);
	
	if(SDL_PVMK_AudioDevice == device)
		SDL_PVMK_AudioDevice = NULL;
	
	if(device->hidden != NULL)
	{
		if(device->hidden->mixbuf != NULL)
		{
			SDL_free(device->hidden->mixbuf);
			device->hidden->mixbuf = NULL;
		}
		
		SDL_free(device->hidden);
		device->hidden = NULL;
	}
}

static bool PVMKAUDIO_Init(SDL_AudioDriverImpl *impl)
{
    impl->OpenDevice = PVMKAUDIO_OpenDevice;
    impl->PlayDevice = PVMKAUDIO_PlayDevice;
    impl->WaitDevice = PVMKAUDIO_WaitDevice;
    impl->GetDeviceBuf = PVMKAUDIO_GetDeviceBuf;
    impl->CloseDevice = PVMKAUDIO_CloseDevice;
    impl->OnlyHasDefaultPlaybackDevice = true;
    impl->HasRecordingSupport = false;
    impl->ProvidesOwnCallbackThread = true; //I mean not really, but whatever

    return true;
}

AudioBootStrap PVMKAUDIO_bootstrap = {
    "pvmkaudio",
    "PVMK Audio Output",
    PVMKAUDIO_Init,
    0
};


#endif //SDL_PLATFORM_PVMK
