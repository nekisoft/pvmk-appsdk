/*
	Neki32 Application SDK - This file placed in the public domain.
	Bryan Topp <betopp@betopp.com>, Nekisoft Pty Ltd (ACN 680 583 251) 2025
*/
#include "../../SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PVMK

#include <sc.h>

#include "../../events/SDL_events_c.h"
#include "SDL_pvmkevents_c.h"

void PVMK_PumpEvents(_THIS)
{

	//SDL_Event ev;
	//ev.type = SDL_QUIT;
	//SDL_PushEvent(&ev);

#ifdef SDL_AUDIO_DRIVER_PVMK
	extern void PVMKAUDIO_Pump(void);
	PVMKAUDIO_Pump();
#endif /* SDL_AUDIO_DRIVER_PVMK */

}

#endif /* SDL_VIDEO_DRIVER_PVMK */

/* vi: set sts=4 ts=4 sw=4 expandtab: */
