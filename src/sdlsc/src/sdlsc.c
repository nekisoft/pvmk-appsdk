//sdlsc.c
//Library to replace PVMK system calls with SDL usage
//Bryan E. Topp <betopp@betopp.com> 2024

//Only build this if explicitly requested - this allows dropping it into your PVMK application without issue.
//Define USE_SDLSC=1 on the build that will use this shim library.
#if !USE_SDLSC
int _use_sdlsc_dummy = 0;
#else //USE_SDLSC

//Include the phony sc.h header from the shim library as well, and make sure it's not pulling in the real system calls.
#include "sc.h"
#if (!defined(_SDLSC_H)) || defined(_SC_H)
#error "We tried to include sc.h but didn't seem to get the right one, for the SDL shim."
#endif

//Libraries from host system
#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

//Whether the simulated system-call layer wants to exit
static bool _sdlsc_done;

//Checks the given SDL error code; prints the supplied description and an error if it's not 0.
void _sdlsc_require_sdlok(int err, const char *desc)
{
	if(err == 0)
		return;
	
	fprintf(stderr, "sdlsc: %s: %d %s\n", desc, err, SDL_GetError());
	_sdlsc_done = 1;
	exit(-1);
}

//Makes sure the given SDL subsystem is initialized, printing an error and aborting otherwise.
#define _sdlsc_require_sdlinit(iflag) \
	_sdlsc_require_sdlok(SDL_WasInit(iflag) ? 0 : SDL_InitSubSystem(iflag), "SDL_InitSubSystem(" #iflag ")")

//Buffer from video syscall to video ISR
static volatile int         _sdlsc_video_enq_valid;
static volatile int         _sdlsc_video_enq_mode;
static volatile const void *_sdlsc_video_enq_buffer;

//Buffer from video ISR to video syscall
static volatile const void *_sdlsc_video_buffer_displayed;

//SDL thread simulating video scanout / ISRs
static SDL_Thread *_sdlsc_video_thread;
int _sdlsc_video_function(void *dummy)
{
	(void)dummy;
	
	//Init SDL and make a window and renderer for us
	_sdlsc_require_sdlinit(SDL_INIT_VIDEO);
	
	SDL_Window *sdl_window = SDL_CreateWindow("An awesome Neki32 game!", 
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		1280, 960, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	
	_sdlsc_require_sdlok((sdl_window == NULL), "SDL_CreateWindow");
	
	SDL_Renderer *sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);
	
	_sdlsc_require_sdlok((sdl_renderer == NULL), "SDL_CreateRenderer");
	
	//Make SDL textures representing one frame of each graphical video mode (i.e. not text mode)
	//Todo - maybe bring in the kernel's text-mode buffer and support text-mode somehow?
	struct mode_table_s { SDL_Texture *tex; SDL_Rect rect; uint32_t fmt; } 
	mode_table[_SC_GFX_MODE_MAX] =
	{
		[_SC_GFX_MODE_VGA_16BPP]     = { .rect = { .w = 640, .h = 480 }, .fmt = SDL_PIXELFORMAT_RGB565 },
		[_SC_GFX_MODE_320X240_16BPP] = { .rect = { .w = 320, .h = 240 }, .fmt = SDL_PIXELFORMAT_RGB565 },
	};
	for(int mm = 0; mm < _SC_GFX_MODE_MAX; mm++)
	{
		struct mode_table_s *mptr = &(mode_table[mm]);
		if(mptr->rect.w > 0 && mptr->rect.h > 0 && mptr->fmt != 0)
		{
			mptr->tex = SDL_CreateTexture(sdl_renderer, mptr->fmt, SDL_TEXTUREACCESS_STREAMING, mptr->rect.w, mptr->rect.h);
			_sdlsc_require_sdlok((mptr->tex == NULL), "SDL_CreateTexture");
		}
	};
	
	//Wait for the user to supply frames for the display
	while(!_sdlsc_done)
	{
		//Check if we have a new buffer to display
		if(_sdlsc_video_enq_valid)
		{
			//User enqueued a buffer for display.
			int newmode = _sdlsc_video_enq_mode;
			const void *newbuf = (const void*)_sdlsc_video_enq_buffer;
			_sdlsc_video_enq_valid = false;
			
			//Update the SDL texture with their data
			struct mode_table_s *mptr = &(mode_table[newmode]);
			if(mptr->tex != NULL)
			{
				int upd_result = SDL_UpdateTexture(mptr->tex, &(mptr->rect), newbuf, mptr->rect.w * 2);
				_sdlsc_require_sdlok(upd_result, "SDL_UpdateTexture");
			}
				
			//Copy the SDL texture to the screen
			if(mptr->tex != NULL)
			{
				int rc_result = SDL_RenderCopy(sdl_renderer, mptr->tex, NULL, NULL);
				_sdlsc_require_sdlok(rc_result, "SDL_RenderCopy");
			}
			else
			{
				SDL_RenderClear(sdl_renderer);
			}
			
			//Present the screen
			SDL_RenderPresent(sdl_renderer);
			
			//After SDL says the screen is presented, then, their buffer is the one displayed.
			_sdlsc_video_buffer_displayed = newbuf;
		}
		else
		{
			//No request from the user, just keep waiting
			SDL_Delay(1);
		}
	}
	
	return 0;
}

int _sc_gfx_flip(int mode, const void *buffer)
{
	//If we need to initialize our video scanout thread, do so
	if(_sdlsc_video_thread == NULL)
	{
		_sdlsc_video_thread = SDL_CreateThread(&_sdlsc_video_function, "sdlsc video", NULL);
		_sdlsc_require_sdlok((_sdlsc_video_thread == NULL), "SDL_CreateThread");
	}
	
	//Some of the validation the kernel could do
	if(mode < 0 || mode >= _SC_GFX_MODE_MAX)
		return -_SC_EINVAL;
	
	if( (buffer == NULL) ^ (mode == _SC_GFX_MODE_TEXT) )
		return -_SC_EINVAL;
	
	//Tell the video scanout about the new buffer
	_sdlsc_video_enq_mode   = mode;
	_sdlsc_video_enq_buffer = buffer;
	_sdlsc_video_enq_valid  = true; //last
	
	//Return the buffer currently displayed
	return ((int)_sdlsc_video_buffer_displayed) & 0x7FFFFFFF;
}

//SDL audio setting
SDL_AudioDeviceID _sdlsc_audio_dev = 0;

int _sc_snd_play(int mode, const void *chunk, int chunkbytes, int maxbuf)
{
	if(_sdlsc_audio_dev == 0)
	{
		//Init SDL and get audio going
		_sdlsc_require_sdlinit(SDL_INIT_AUDIO);
		
		SDL_AudioSpec desired = 
		{
			.freq = 48000,
			.format = AUDIO_S16LSB,
			.channels = 2,
			.samples = 2048,
		};
		
		SDL_AudioSpec obtained = {0};
		int open_result = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
		_sdlsc_require_sdlok((open_result==0), "SDL_OpenAudioDevice");
		
		_sdlsc_audio_dev = open_result;
		SDL_PauseAudioDevice(_sdlsc_audio_dev, 0);
	}
	
	//Some of the validation the kernel could do
	if(mode == _SC_SND_MODE_SILENT)
		return 0;
	
	if(mode != _SC_SND_MODE_48K_16B_2C)
		return -_SC_EINVAL;
	
	if(maxbuf < 0)
		return -_SC_EINVAL;
	
	if(chunkbytes + SDL_GetQueuedAudioSize(_sdlsc_audio_dev) > (size_t)maxbuf)
		return -_SC_EAGAIN;
		
	int queued = SDL_QueueAudio(_sdlsc_audio_dev, chunk, chunkbytes);
	_sdlsc_require_sdlok(queued, "SDL_QueueAudio");

	return 0;
}

//State of simulated gamepad
static int _sc_input_last = 0;
static int _sc_input_count = 0;

int _sc_input(_sc_input_t *buffer_ptr, int bytes_per_event, int bytes_max)
{
	if(bytes_per_event < (int)sizeof(_sc_input_t))
		return -_SC_EINVAL;
	if(bytes_max < bytes_per_event)
		return -_SC_EINVAL;
	
	//Pump SDL event queue and translate to input events
	_sdlsc_require_sdlinit(SDL_INIT_EVENTS);
	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		if(e.type == SDL_QUIT)
			exit(0);
		
		if(e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
		{
			//Keyup or keydown - see if the key is one that we respond to
			static const int keymap[512] = 
			{
				[SDL_SCANCODE_Q] = _SC_BTNBIT_L,
				[SDL_SCANCODE_W] = _SC_BTNBIT_UP,
				[SDL_SCANCODE_E] = _SC_BTNBIT_R,
				
				[SDL_SCANCODE_A] = _SC_BTNBIT_LEFT,				
				[SDL_SCANCODE_S] = _SC_BTNBIT_DOWN,
				[SDL_SCANCODE_D] = _SC_BTNBIT_RIGHT,
				
				[SDL_SCANCODE_U] = _SC_BTNBIT_X,
				[SDL_SCANCODE_I] = _SC_BTNBIT_Y,
				[SDL_SCANCODE_O] = _SC_BTNBIT_Z,
				
				[SDL_SCANCODE_J] = _SC_BTNBIT_A,				
				[SDL_SCANCODE_K] = _SC_BTNBIT_B,				
				[SDL_SCANCODE_L] = _SC_BTNBIT_C,

				[SDL_SCANCODE_RETURN] = _SC_BTNBIT_START,
			};
			
			int bbit = keymap[e.key.keysym.scancode % 512];
			if(bbit != 0)
			{
				//Okay, the key is mapped to a button. Change the gamepad state.
				if(e.type == SDL_KEYDOWN)
					_sc_input_last |= bbit;
				else
					_sc_input_last &= ~bbit;
				
				_sc_input_count++;
			}
		}
	}
	
	if(_sc_input_count > 0)
	{
		//Some input happened - make a new gamepad event for it.
		memset(buffer_ptr, 0, sizeof(*buffer_ptr));
		buffer_ptr->format = 'A';
		buffer_ptr->buttons = _sc_input_last;
		_sc_input_count = 0;
		return 1;
	}
	else
	{
		//No input polled from SDL
		return -_SC_EAGAIN;
	}
}

void _sc_pause(void)
{
	//Okay, I have a confession to make. The kernel is driven entirely by the 1KHz timer interrupt.
	//So there's no real situations in which _sc_pause would unblock, other than instantly or in 1ms.
	_sdlsc_require_sdlinit(SDL_INIT_TIMER);
	SDL_Delay(1);
}

int _sc_getticks(void)
{
	_sdlsc_require_sdlinit(SDL_INIT_TIMER);
	return SDL_GetTicks();
}

#endif //USE_SDLSC
