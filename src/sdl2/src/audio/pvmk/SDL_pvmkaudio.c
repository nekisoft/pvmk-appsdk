/*
	Neki32 Application SDK - This file placed in the public domain.
	Bryan Topp <betopp@betopp.com>, Nekisoft Pty Ltd (ACN 680 583 251) 2025
*/
#include "../../SDL_internal.h"

#ifdef SDL_AUDIO_DRIVER_PVMK

#include "SDL_audio.h"

/* PVMK Audio driver */

#include "../SDL_sysaudio.h"
#include "SDL_pvmkaudio.h"
#include "SDL_timer.h"

#define PVMKAUDIO_DRIVER_NAME "pvmk"

static const Uint8 *PVMKAUDIO_LastBufPtr = NULL;
static int PVMKAUDIO_LastBufLen = 0;
static SDL_AudioDevice *SDL_PVMK_AudioDevice = NULL;

static Uint8 *PVMKAUDIO_GetDeviceBuf(_THIS)
{
    return this->hidden->mixbuf;
}

//PVMK does its audio work on the main thread like a champ.
void PVMKAUDIO_Pump(void)
{
	//Go as long as we can keep submitting more data to the kernel
	while(1)
	{
		//First off - if we've already prepared a buffer and set it aside, try to submit it.
		if(PVMKAUDIO_LastBufPtr != NULL)
		{
			//Try submitting the already existing buffer
			int enqueued = _sc_snd_play(_SC_SND_MODE_48K_16B_2C, PVMKAUDIO_LastBufPtr, PVMKAUDIO_LastBufLen, PVMKAUDIO_LastBufLen * 3);
			if(enqueued < 0)
			{
				//It didn't submit.
				//Just return, we can't do any more.
				return;
			}
			
			//Okay, we got our existing buffer submitted.
			PVMKAUDIO_LastBufPtr = NULL;
			PVMKAUDIO_LastBufLen = 0;
		}

		//Try to build a new buffer to submit, based on what we know of SDL audio state...
		SDL_AudioDevice *device = SDL_PVMK_AudioDevice;
		if(device == NULL)
			return;
		
		//If SDL is doing resampling and already has data for us, we'll submit that first
		if((device->stream != NULL) && (SDL_AudioStreamAvailable(device->stream) >= ((int)device->spec.size)))
		{
			//Get results of SDL resampling
			void *data = PVMKAUDIO_GetDeviceBuf(device);
			int got = SDL_AudioStreamGet(device->stream, data, device->spec.size);
			
			SDL_assert((got <= 0) || (got == device->spec.size));
			if (got != device->spec.size) {
				SDL_memset(data, device->spec.silence, device->spec.size);
			}
			
			//That's the buffer we'll try to submit next
			PVMKAUDIO_LastBufPtr = data;
			PVMKAUDIO_LastBufLen = got;
			continue;
		}
		
		//If the device is supposed to be paused, try to submit chunks of silence
		if (SDL_AtomicGet(&device->paused))
		{
			//Fill buffer with silence
			void *data = PVMKAUDIO_GetDeviceBuf(device);
			SDL_memset(data, device->spec.silence, device->spec.size);
			
			//That's the buffer we'll try to submit next
			PVMKAUDIO_LastBufPtr = data;
			PVMKAUDIO_LastBufLen = device->spec.size;
			continue;
		}
	
		//Otherwise callback to the application for more audio data
		void *udata = device->callbackspec.userdata;
		SDL_AudioCallback callback = device->callbackspec.callback;
		
		if (device->stream)
		{
			//SDL will be resampling - application writes into SDL buffer
			callback(udata, device->work_buffer, device->callbackspec.size);
			SDL_AudioStreamPut(device->stream, device->work_buffer, device->callbackspec.size);
		}
		else
		{
			//No resampling - application can write directly into our buffer
			callback(udata, PVMKAUDIO_GetDeviceBuf(device), device->callbackspec.size);
			
			//That's the buffer we'll try to submit next
			PVMKAUDIO_LastBufPtr = PVMKAUDIO_GetDeviceBuf(device);
			PVMKAUDIO_LastBufLen = device->callbackspec.size;
			continue;
		}
	}
}

static int PVMKAUDIO_OpenDevice(_THIS, const char *devname)
{
	if(SDL_PVMK_AudioDevice != NULL)
		return SDL_SetError("Only support one audio device");

	this->hidden = (struct SDL_PrivateAudioData *)SDL_calloc(1, sizeof(*(this->hidden)));
	if (!this->hidden)
	{
		return SDL_SetError("Out of memory");
	}

	this->spec.channels = 2;
	this->spec.format = AUDIO_S16LSB;
	this->spec.freq = 48000;
	
	// Allocate mixing buffer
	if (this->spec.size >= SDL_MAX_UINT32 / 2)
	{
		return SDL_SetError("Mixing buffer is too large.");
	}

	this->hidden->mixbuf = (Uint8 *)SDL_malloc(this->spec.size);
	if (!this->hidden->mixbuf)
	{
		return SDL_SetError("Out of memory");
	}

	SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);

	SDL_PVMK_AudioDevice = this;
	
	return 0;
}

static void PVMKAUDIO_CloseDevice(_THIS)
{
	if (this->hidden)
	{
		if (this->hidden->mixbuf)
		{
			SDL_free(this->hidden->mixbuf);
			this->hidden->mixbuf = NULL;
		}	
		SDL_free(this->hidden);
		this->hidden = NULL;		
	}
	
	if(SDL_PVMK_AudioDevice == this)
		SDL_PVMK_AudioDevice = NULL;
}

static SDL_bool PVMKAUDIO_Init(SDL_AudioDriverImpl *impl)
{
	//We bypass SDL's usual audio-thread implementation.
	//So all we really care about is opendevice/closedevice.
	impl->OpenDevice = PVMKAUDIO_OpenDevice;
	impl->CloseDevice = PVMKAUDIO_CloseDevice;

	impl->OnlyHasDefaultOutputDevice = SDL_TRUE;
	impl->HasCaptureSupport = SDL_FALSE;
	impl->SupportsNonPow2Samples = SDL_FALSE;
	
	impl->ProvidesOwnCallbackThread = SDL_TRUE; //Not really, we do it on the main thread

	return SDL_TRUE;
}

AudioBootStrap PVMKAUDIO_bootstrap = {
    PVMKAUDIO_DRIVER_NAME,
    "SDL PVMK audio driver",
    PVMKAUDIO_Init,
    0
};

#endif /* SDL_AUDIO_DRIVER_PVMK */

/* vi: set sts=4 ts=4 sw=4 expandtab: */
