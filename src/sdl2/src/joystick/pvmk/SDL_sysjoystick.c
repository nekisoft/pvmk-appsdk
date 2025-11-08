/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#ifdef SDL_JOYSTICK_PVMK

/* This is the PVMK implementation of the SDL joystick API */

#include <sc.h>

#include "../SDL_sysjoystick.h"
#include "SDL_events.h"

#define NB_BUTTONS 12

static int PVMK_JoystickInit(void)
{
    return 0;
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

static SDL_JoystickGUID PVMK_JoystickGetDeviceGUID(int device_index)
{
    SDL_JoystickGUID guid = SDL_CreateJoystickGUIDForName(PVMK_JoystickGetDeviceName(device_index));
    return guid;
}

static SDL_JoystickID PVMK_JoystickGetDeviceInstanceID(int device_index)
{
    return device_index;
}

static int PVMK_JoystickOpen(SDL_Joystick *joystick, int device_index)
{
    joystick->nbuttons = NB_BUTTONS;
    joystick->naxes = 0;
    joystick->nhats = 0;
    joystick->instance_id = device_index;
    return 0;
}

static int PVMK_JoystickSetSensorsEnabled(SDL_Joystick *joystick, SDL_bool enabled)
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
					for(int bit = 0; bit < NB_BUTTONS; bit++)
					{
						if(presses & (1u << bit))
							SDL_PrivateJoystickButton(js, bit, SDL_PRESSED);
						if(releases & (1u << bit))
							SDL_PrivateJoystickButton(js, bit, SDL_RELEASED);
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
}

static void PVMK_JoystickQuit(void)
{
}

static SDL_bool PVMK_JoystickGetGamepadMapping(int device_index, SDL_GamepadMapping *out)
{
    /* There is only one possible mapping. */
    *out = (SDL_GamepadMapping){
       
        .dpup          = { EMappingKind_Button, _SC_BTNIDX_UP },
        .dpdown        = { EMappingKind_Button, _SC_BTNIDX_DOWN },
        .dpleft        = { EMappingKind_Button, _SC_BTNIDX_LEFT },
        .dpright       = { EMappingKind_Button, _SC_BTNIDX_RIGHT },
    
        .a             = { EMappingKind_Button, _SC_BTNIDX_A },
        .b             = { EMappingKind_Button, _SC_BTNIDX_B },
	.rightshoulder = { EMappingKind_Button, _SC_BTNIDX_C },
	
        .x             = { EMappingKind_Button, _SC_BTNIDX_X },
        .y             = { EMappingKind_Button, _SC_BTNIDX_Y },
	.leftshoulder  = { EMappingKind_Button, _SC_BTNIDX_Z },
	 
        .back          = { EMappingKind_Button, _SC_BTNIDX_MODE },
        .start         = { EMappingKind_Button, _SC_BTNIDX_START },
	
        .guide         = { EMappingKind_None, 255 },
        .leftstick     = { EMappingKind_None, 255 },
        .rightstick    = { EMappingKind_None, 255 },
	
        .righttrigger  = { EMappingKind_None, 255 },
	.lefttrigger   = { EMappingKind_None, 255 },
	
        .misc1         = { EMappingKind_None, 255 },
        .paddle1       = { EMappingKind_None, 255 },
        .paddle2       = { EMappingKind_None, 255 },
        .paddle3       = { EMappingKind_None, 255 },
        .paddle4       = { EMappingKind_None, 255 },
	
        .leftx         = { EMappingKind_None, 255 },
        .lefty         = { EMappingKind_None, 255 },
        .rightx        = { EMappingKind_None, 255 },
        .righty        = { EMappingKind_None, 255 },
       
        
    };
    return SDL_TRUE;
}

static void PVMK_JoystickDetect(void)
{
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
}

static Uint32 PVMK_JoystickGetCapabilities(SDL_Joystick *joystick)
{
    return 0;
}

static int PVMK_JoystickRumble(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    return SDL_Unsupported();
}

static int PVMK_JoystickRumbleTriggers(SDL_Joystick *joystick, Uint16 left_rumble, Uint16 right_rumble)
{
    return SDL_Unsupported();
}

static int PVMK_JoystickSetLED(SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue)
{
    return SDL_Unsupported();
}

static int PVMK_JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size)
{
    return SDL_Unsupported();
}

SDL_JoystickDriver SDL_PVMK_JoystickDriver = {
    .Init = PVMK_JoystickInit,
    .GetCount = PVMK_JoystickGetCount,
    .Detect = PVMK_JoystickDetect,
    .GetDeviceName = PVMK_JoystickGetDeviceName,
    .GetDevicePath = PVMK_JoystickGetDevicePath,
    .GetDeviceSteamVirtualGamepadSlot = PVMK_JoystickGetDeviceSteamVirtualGamepadSlot,
    .GetDevicePlayerIndex = PVMK_JoystickGetDevicePlayerIndex,
    .SetDevicePlayerIndex = PVMK_JoystickSetDevicePlayerIndex,
    .GetDeviceGUID = PVMK_JoystickGetDeviceGUID,
    .GetDeviceInstanceID = PVMK_JoystickGetDeviceInstanceID,
    .Open = PVMK_JoystickOpen,
    .Rumble = PVMK_JoystickRumble,
    .RumbleTriggers = PVMK_JoystickRumbleTriggers,
    .GetCapabilities = PVMK_JoystickGetCapabilities,
    .SetLED = PVMK_JoystickSetLED,
    .SendEffect = PVMK_JoystickSendEffect,
    .SetSensorsEnabled = PVMK_JoystickSetSensorsEnabled,
    .Update = PVMK_JoystickUpdate,
    .Close = PVMK_JoystickClose,
    .Quit = PVMK_JoystickQuit,
    .GetGamepadMapping = PVMK_JoystickGetGamepadMapping
};

#endif /* SDL_JOYSTICK_PVMK */

/* vi: set sts=4 ts=4 sw=4 expandtab: */
