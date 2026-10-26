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

//When the application uses _sc_shim_main(), main() belongs to the application and calls us;
//SDL must not rename it out from under us.
#ifndef SDL_MAIN_HANDLED
	#define SDL_MAIN_HANDLED 1
#endif

//Libraries from host system
#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(_WIN32)
	#include <direct.h>
	#define _sdlsc_mkdir(path) _mkdir(path)
#else
	#include <sys/stat.h>
	#define _sdlsc_mkdir(path) mkdir((path), 0777)
#endif

//===
//How this works
//===
//
//The real PVMK system has a scanout engine that runs on its own, off in the video
//hardware: a game enqueues a buffer with _sc_gfx_flip() and carries on, and the
//kernel's vertical-blanking interrupt starts displaying that buffer some time later.
//The game learns which buffer is on screen from flip's return value, which is how
//double and triple buffering work.
//
//To do that here we have to draw with SDL, and SDL has a rule about threads:
//video is initialized on, and belongs to, whichever thread called
//SDL_InitSubSystem(SDL_INIT_VIDEO). SDL 3 says so in as many words
//(src/SDL.c: "We initialize video on the main thread. On Apple platforms this is a
//requirement. On other platforms, this is the definition"), and SDL 2 says the same
//thing for renderers in SDL_render.h ("These functions must be called from the main
//thread"). Win32 adds a second reason: a window belongs to the thread that created
//it, and only that thread can pump its messages.
//
//So there are two ways to run, and both keep every SDL call on one legal thread:
//
//Inline (default, no application change):
//	The application's main thread calls our system calls, and each call does its own
//	SDL work right there. That thread is the process main thread, so it is the thread
//	SDL wants. _sc_gfx_flip() enqueues the frame and then scans it out immediately
//	(copy to texture, present), returning the buffer that was on screen before this
//	flip - the same contract as the kernel, so double and triple buffering in the
//	application work unchanged. Presenting is vertical-synced, so the application is
//	paced by the display instead of running away from it.
//
//Pumped (application calls _sc_shim_main() from main):
//	The real main thread is handed to the SDL scanout/event pump, and the application
//	runs on a worker thread. This keeps the scanout asynchronous like the hardware:
//	flip enqueues, returns the displayed buffer, and does not block. There is a single
//	pending slot, exactly like the kernel's gfx_enq_*, so an application that flips
//	faster than the display simply drops frames, as it would on the console.
//
//Either way the application stays a plain single-threaded PVMK program: it never
//calls SDL, and never has to know which mode it is in.
//
//Headless mode (SDLSC_HEADLESS=1, or no display available) runs the same logic with
//no video at all: frames are counted, and optionally dumped, hashed and scripted, so
//the examples can be exercised on a build machine. See the environment variables in
//_sdlsc_cfg_init() below.
//

//===
//Configuration, from the environment only - an application stays a drop-in
//replacement, so nothing here needs a source change or an API call.
//===
static int _sdlsc_headless;                 //SDLSC_HEADLESS=1
static int _sdlsc_vsync = 1;                //SDLSC_VSYNC=0
static int _sdlsc_vclock;                   //SDLSC_CLOCK=virtual
static SDL_atomic_t _sdlsc_vtime;           //virtual-clock milliseconds seen so far
static int _sdlsc_trace;                    //SDLSC_TRACE=1
static long long _sdlsc_frame_limit;        //SDLSC_FRAMES=n
static const char *_sdlsc_dump_dir;         //SDLSC_DUMP=dir
static int _sdlsc_dump_every = 1;           //SDLSC_DUMP_EVERY=n
static const char *_sdlsc_input_path;       //SDLSC_INPUT=file
static const char *_sdlsc_nvm_path;         //SDLSC_NVM=file
static const char *_sdlsc_title;            //SDLSC_TITLE=text
static int _sdlsc_audio_off;                //SDLSC_AUDIO=off
static long long _sdlsc_quit_after;         //SDLSC_QUIT_AFTER=n frames (testing)
static long long _sdlsc_reset_after;        //SDLSC_RESET_AFTER=n frames (testing)

//Looks up the given environment variable, treating empty as unset.
static const char *_sdlsc_getenv(const char *name)
{
	const char *val = getenv(name);
	if(val == NULL || val[0] == '\0')
		return NULL;
	return val;
}

//Looks up the given boolean environment variable.
static int _sdlsc_getenv_flag(const char *name, int dflt)
{
	const char *val = _sdlsc_getenv(name);
	if(val == NULL)
		return dflt;
	if(!strcmp(val, "0") || !strcmp(val, "no") || !strcmp(val, "off") || !strcmp(val, "false"))
		return 0;
	return 1;
}

//Looks up the given integer environment variable.
static long long _sdlsc_getenv_num(const char *name, long long dflt)
{
	const char *val = _sdlsc_getenv(name);
	if(val == NULL)
		return dflt;
	return strtoll(val, NULL, 0);
}

//Buttons named in a scripted-input file.
static const struct { const char *name; unsigned int bits; } _sdlsc_btnnames[] =
{
	{ "up",    _SC_BTNBIT_UP    },
	{ "left",  _SC_BTNBIT_LEFT  },
	{ "down",  _SC_BTNBIT_DOWN  },
	{ "right", _SC_BTNBIT_RIGHT },
	{ "a",     _SC_BTNBIT_A     },
	{ "b",     _SC_BTNBIT_B     },
	{ "c",     _SC_BTNBIT_C     },
	{ "x",     _SC_BTNBIT_X     },
	{ "y",     _SC_BTNBIT_Y     },
	{ "z",     _SC_BTNBIT_Z     },
	{ "start", _SC_BTNBIT_START },
	{ "mode",  _SC_BTNBIT_MODE  },
};

//One scripted button state, applied at the given frame number.
struct _sdlsc_script_s { long long frame; unsigned int buttons; };
static struct _sdlsc_script_s *_sdlsc_script;
static int _sdlsc_script_len;
static int _sdlsc_script_pos;

//Turns a button name into bits. Returns 0 if it isn't one.
static unsigned int _sdlsc_button_bits(const char *name, int len)
{
	for(int nn = 0; nn < (int)(sizeof(_sdlsc_btnnames) / sizeof(_sdlsc_btnnames[0])); nn++)
	{
		if((int)strlen(_sdlsc_btnnames[nn].name) == len && !strncmp(_sdlsc_btnnames[nn].name, name, len))
			return _sdlsc_btnnames[nn].bits;
	}
	return 0;
}

//Reads the scripted-input file, if any. Format, one line per change:
//	frame buttons
//	30 a
//	45 a,up
//	60 -
static void _sdlsc_script_read(const char *path)
{
	FILE *file = fopen(path, "r");
	if(file == NULL)
	{
		fprintf(stderr, "sdlsc: failed to open scripted input \"%s\"\n", path);
		return;
	}

	static char line[1024];
	while(fgets(line, sizeof(line), file) != NULL)
	{
		//Strip comments
		char *hash = strchr(line, '#');
		if(hash != NULL)
			*hash = '\0';

		char *scan = line;
		while(*scan == ' ' || *scan == '\t')
			scan++;

		//Frame number
		if(*scan < '0' || *scan > '9')
			continue;
		long long frame = strtoll(scan, &scan, 10);

		//Button list
		unsigned int buttons = 0;
		while(*scan != '\0')
		{
			while(*scan == ' ' || *scan == '\t' || *scan == ',')
				scan++;
			if(*scan == '\0' || *scan == '\n' || *scan == '\r')
				break;
			if(*scan == '-')
			{
				scan++;
				continue;
			}

			char *start = scan;
			while(*scan != '\0' && *scan != ',' && *scan != ' ' && *scan != '\t' && *scan != '\n' && *scan != '\r')
				scan++;

			unsigned int bits = _sdlsc_button_bits(start, (int)(scan - start));
			if(bits == 0)
				fprintf(stderr, "sdlsc: scripted input: unknown button \"%.*s\"\n", (int)(scan - start), start);
			buttons |= bits;
		}

		//Grow the table and remember this state
		struct _sdlsc_script_s *grown = realloc(_sdlsc_script, (_sdlsc_script_len + 1) * sizeof(*grown));
		if(grown == NULL)
		{
			fprintf(stderr, "sdlsc: out of memory reading scripted input\n");
			break;
		}
		_sdlsc_script = grown;
		_sdlsc_script[_sdlsc_script_len].frame = frame;
		_sdlsc_script[_sdlsc_script_len].buttons = buttons;
		_sdlsc_script_len++;
	}

	fclose(file);
}

//===
//Shared state
//===

//Everything below is protected by this mutex: the application thread and the
//scanout thread talk to each other through it, and nothing else.
static SDL_mutex *_sdlsc_mtx;
static SDL_cond  *_sdlsc_cnd;

//Whether this process hands the real main thread to the pump (see _sc_shim_main)
static int _sdlsc_pumped;

//Set when the process should stop pumping (window closed, or we're done)
static volatile int _sdlsc_done;
static volatile int _sdlsc_pump_waiting;       //the pump is blocked, and wants nudging

//Whether the application entry point is still running
static int _sdlsc_app_running;

//The application's entry point, and the arguments to give it.
//A negative argc means the entry point takes no arguments (int f(void)).
//void(*)(void) is only a carrier type: C allows casts between function pointer types,
//and it is the one such type that does not provoke -Wcast-function-type.
static void (*_sdlsc_entry)(void);
static int _sdlsc_entry_argc;
static char **_sdlsc_entry_argv;

//Single enqueued flip, matching the kernel's gfx_enq_* fields
static int _sdlsc_enq_valid;
static int _sdlsc_enq_mode;
static const void *_sdlsc_enq_buf;
static unsigned long long _sdlsc_seq_enq;
static unsigned long long _sdlsc_seq_done;

//What the simulated scanout is displaying
static const void *_sdlsc_displayed;
static int _sdlsc_displayed_mode;
static long long _sdlsc_frames;

//Current gamepad state, and the queue of state changes for the application to read
static unsigned int _sdlsc_buttons;
#define _SDLSC_INQ_LEN 256
static unsigned short _sdlsc_inq[_SDLSC_INQ_LEN];
static int _sdlsc_inq_head;
static int _sdlsc_inq_tail;

//Video state - only ever touched by the SDL thread
struct _sdlsc_mode_s { int w; int h; SDL_Texture *tex; };
static struct _sdlsc_mode_s _sdlsc_modes[_SC_GFX_MODE_MAX] =
{
	[_SC_GFX_MODE_TEXT]          = { .w = 640, .h = 480 },
	[_SC_GFX_MODE_VGA_16BPP]     = { .w = 640, .h = 480 },
	[_SC_GFX_MODE_320X240_16BPP] = { .w = 320, .h = 240 },
};
static int _sdlsc_video_up;
static SDL_Window *_sdlsc_window;
static SDL_Renderer *_sdlsc_renderer;
static int _sdlsc_logical_mode = -1;

//Event type used to wake the pump when the application enqueues a flip. Without this
//the pump would only notice a new frame when its event wait times out, which costs a
//whole frame of latency on every flip.
static Uint32 _sdlsc_wake_event;

//Audio device, opened on first use
static SDL_AudioDeviceID _sdlsc_audio_dev;

//Says whether we've already complained about a framebuffer address
static int _sdlsc_fb_warned;

//Prints an SDL failure and gives up.
static int _sdlsc_ticks(int advance);

static void _sdlsc_die(const char *what)
{
	fprintf(stderr, "sdlsc: %s: %s\n", what, SDL_GetError());
	fflush(NULL);
	exit(-1);
}

//Reads the environment, once.
static void _sdlsc_cfg_init(void)
{
	static int done;
	if(done)
		return;
	done = 1;

	_sdlsc_headless    = _sdlsc_getenv_flag("SDLSC_HEADLESS", 0);
	_sdlsc_vsync       = _sdlsc_getenv_flag("SDLSC_VSYNC", 1);
	_sdlsc_trace       = _sdlsc_getenv_flag("SDLSC_TRACE", 0);
	_sdlsc_audio_off   = !_sdlsc_getenv_flag("SDLSC_AUDIO", 1);
	_sdlsc_frame_limit = _sdlsc_getenv_num("SDLSC_FRAMES", 0);
	_sdlsc_quit_after  = _sdlsc_getenv_num("SDLSC_QUIT_AFTER", 0);
	_sdlsc_reset_after = _sdlsc_getenv_num("SDLSC_RESET_AFTER", 0);
	_sdlsc_dump_dir    = _sdlsc_getenv("SDLSC_DUMP");
	_sdlsc_dump_every  = (int)_sdlsc_getenv_num("SDLSC_DUMP_EVERY", 1);
	_sdlsc_input_path  = _sdlsc_getenv("SDLSC_INPUT");
	_sdlsc_nvm_path    = _sdlsc_getenv("SDLSC_NVM");
	_sdlsc_title       = _sdlsc_getenv("SDLSC_TITLE");

	if(_sdlsc_dump_every < 1)
		_sdlsc_dump_every = 1;

	const char *clock = _sdlsc_getenv("SDLSC_CLOCK");
	_sdlsc_vclock = (clock != NULL && !strcmp(clock, "virtual"));

	//Headless runs shouldn't need an audio device; let SDL discard the samples at the
	//right rate instead, so the application's flow control still works. A hint rather
	//than setenv(), so this needs no POSIX environment calls - and SDL_setenv wins over
	//a hint anyway, so anyone who set SDL_AUDIODRIVER themselves still gets their driver.
	if(_sdlsc_headless)
		SDL_SetHint(SDL_HINT_AUDIODRIVER, "dummy");

	if(_sdlsc_input_path != NULL)
		_sdlsc_script_read(_sdlsc_input_path);

	_sdlsc_mtx = SDL_CreateMutex();
	_sdlsc_cnd = SDL_CreateCond();
	if(_sdlsc_mtx == NULL || _sdlsc_cnd == NULL)
		_sdlsc_die("SDL_CreateMutex/SDL_CreateCond");

	//Scripted input for frame zero is in force before the application ever runs
	if(_sdlsc_script_len > 0 && _sdlsc_script[0].frame <= 0)
	{
		_sdlsc_buttons = _sdlsc_script[0].buttons;
		_sdlsc_script_pos = 1;
	}
}

//===
//Input
//===

//Adds one gamepad state change to the queue the application reads from.
//Caller holds the mutex.
static void _sdlsc_input_push_locked(unsigned int buttons)
{
	if(buttons == _sdlsc_buttons)
		return;

	_sdlsc_buttons = buttons;
	_sdlsc_inq[_sdlsc_inq_head] = (unsigned short)buttons;
	_sdlsc_inq_head = (_sdlsc_inq_head + 1) % _SDLSC_INQ_LEN;
	if(_sdlsc_inq_head == _sdlsc_inq_tail)
	{
		//Full - drop the oldest, the application is not polling input at all
		_sdlsc_inq_tail = (_sdlsc_inq_tail + 1) % _SDLSC_INQ_LEN;
	}
}

//Takes one gamepad state change out of the queue, if there is one.
static int _sdlsc_input_pop(unsigned int *buttons)
{
	int got = 0;
	SDL_LockMutex(_sdlsc_mtx);
	if(_sdlsc_inq_tail != _sdlsc_inq_head)
	{
		*buttons = _sdlsc_inq[_sdlsc_inq_tail];
		_sdlsc_inq_tail = (_sdlsc_inq_tail + 1) % _SDLSC_INQ_LEN;
		got = 1;
	}
	SDL_UnlockMutex(_sdlsc_mtx);
	return got;
}

//The frame number scripted input is timed against: frames displayed when we are running
//at real speed, or frames of virtual time when the clock is virtual. The distinction
//matters because a program is allowed to sit in an input loop without drawing anything -
//"press start" - and then no frames are displayed, but its script still has to arrive.
static long long _sdlsc_input_frame(void)
{
	if(_sdlsc_vclock)
		return ((long long)_sdlsc_ticks(0) * 60) / 1000;
	return _sdlsc_frames;
}

//One key we can stand in for each button with, so that scripted input can be delivered
//as real key events and take the same path a player's fingers do. Must agree with the
//keymap in _sdlsc_event().
static const int _sdlsc_btnscancodes[12] =
{
	[0]  = SDL_SCANCODE_UP,
	[1]  = SDL_SCANCODE_LEFT,
	[2]  = SDL_SCANCODE_DOWN,
	[3]  = SDL_SCANCODE_RIGHT,
	[4]  = SDL_SCANCODE_J,
	[5]  = SDL_SCANCODE_K,
	[6]  = SDL_SCANCODE_L,
	[7]  = SDL_SCANCODE_U,
	[8]  = SDL_SCANCODE_I,
	[9]  = SDL_SCANCODE_O,
	[10] = SDL_SCANCODE_RETURN,
	[11] = SDL_SCANCODE_ESCAPE,
};

//Applies any scripted input that comes into force at the given frame.
//With a window, the change is offered as key presses and releases, so it goes through
//the same translation a real keypress does; without one, there is no event queue to put
//it in, so it goes straight into the queue the application reads.
static void _sdlsc_input_script(long long frame)
{
	unsigned int target = _sdlsc_buttons;
	int changed = 0;

	SDL_LockMutex(_sdlsc_mtx);
	while(_sdlsc_script_pos < _sdlsc_script_len && _sdlsc_script[_sdlsc_script_pos].frame <= frame)
	{
		target = _sdlsc_script[_sdlsc_script_pos].buttons;
		_sdlsc_script_pos++;
		changed = 1;
	}
	if(!changed)
	{
		SDL_UnlockMutex(_sdlsc_mtx);
		return;
	}
	unsigned int current = _sdlsc_buttons;
	if(_sdlsc_headless)
	{
		_sdlsc_input_push_locked(target);
		SDL_UnlockMutex(_sdlsc_mtx);
		return;
	}
	SDL_UnlockMutex(_sdlsc_mtx);

	//Let the event handler work out the new state from the keys, exactly as it would for
	//a player; pushing the state ourselves as well would swallow the change.
	for(int bb = 0; bb < 12; bb++)
	{
		unsigned int bit = 1u << bb;
		if((target & bit) == (current & bit))
			continue;

		SDL_Event event;
		memset(&event, 0, sizeof(event));
		event.type = (target & bit) ? SDL_KEYDOWN : SDL_KEYUP;
		event.key.state = (target & bit) ? SDL_PRESSED : SDL_RELEASED;
		event.key.keysym.scancode = (SDL_Scancode)_sdlsc_btnscancodes[bb];
		event.key.keysym.sym = SDL_GetKeyFromScancode((SDL_Scancode)_sdlsc_btnscancodes[bb]);
		SDL_PushEvent(&event);
	}
}

//Translates one SDL event into our gamepad state. Runs on the SDL thread.
static void _sdlsc_event(SDL_Event *event)
{
	static const int keymap[512] =
	{
		[SDL_SCANCODE_W]        = _SC_BTNBIT_UP,
		[SDL_SCANCODE_UP]       = _SC_BTNBIT_UP,
		[SDL_SCANCODE_A]        = _SC_BTNBIT_LEFT,
		[SDL_SCANCODE_LEFT]     = _SC_BTNBIT_LEFT,
		[SDL_SCANCODE_S]        = _SC_BTNBIT_DOWN,
		[SDL_SCANCODE_DOWN]     = _SC_BTNBIT_DOWN,
		[SDL_SCANCODE_D]        = _SC_BTNBIT_RIGHT,
		[SDL_SCANCODE_RIGHT]    = _SC_BTNBIT_RIGHT,

		[SDL_SCANCODE_U]        = _SC_BTNBIT_X,
		[SDL_SCANCODE_I]        = _SC_BTNBIT_Y,
		[SDL_SCANCODE_O]        = _SC_BTNBIT_Z,

		[SDL_SCANCODE_J]        = _SC_BTNBIT_A,
		[SDL_SCANCODE_K]        = _SC_BTNBIT_B,
		[SDL_SCANCODE_L]        = _SC_BTNBIT_C,

		[SDL_SCANCODE_SPACE]    = _SC_BTNBIT_A,
		[SDL_SCANCODE_LCTRL]    = _SC_BTNBIT_B,
		[SDL_SCANCODE_LALT]     = _SC_BTNBIT_C,

		[SDL_SCANCODE_RETURN]   = _SC_BTNBIT_START,
		[SDL_SCANCODE_BACKSPACE]= _SC_BTNBIT_MODE,
		[SDL_SCANCODE_ESCAPE]   = _SC_BTNBIT_MODE,
	};

	switch(event->type)
	{
		case SDL_QUIT:
		{
			_sdlsc_done = 1;
			break;
		}
		case SDL_RENDER_TARGETS_RESET:
		case SDL_RENDER_DEVICE_RESET:
		{
			//The renderer threw its textures away - a lost GL context or D3D device.
			//Streaming textures do not come back on their own, so drop ours and let
			//the next present make new ones.
			for(int mm = 0; mm < _SC_GFX_MODE_MAX; mm++)
			{
				if(_sdlsc_modes[mm].tex != NULL)
					SDL_DestroyTexture(_sdlsc_modes[mm].tex);
				_sdlsc_modes[mm].tex = NULL;
			}
			_sdlsc_logical_mode = -1;
			break;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			int scancode = event->key.keysym.scancode;
			if(scancode < 0 || scancode >= (int)(sizeof(keymap) / sizeof(keymap[0])))
				break;

			unsigned int bits = (unsigned int)keymap[scancode];
			if(bits == 0)
				break;

			SDL_LockMutex(_sdlsc_mtx);
			unsigned int buttons = _sdlsc_buttons;
			if(event->type == SDL_KEYDOWN)
				buttons |= bits;
			else
				buttons &= ~bits;
			_sdlsc_input_push_locked(buttons);
			SDL_UnlockMutex(_sdlsc_mtx);
			break;
		}
		default:
		{
			break;
		}
	}
}

//Takes everything the window system has for us. Runs on the SDL thread.
static void _sdlsc_pump_events(void)
{
	//Scripted input is offered to the event queue here, so that a windowed run sees it
	//whether or not the application is polling.
	if(!_sdlsc_headless && _sdlsc_script_len > 0)
		_sdlsc_input_script(_sdlsc_input_frame() + 1);

	SDL_Event event;
	while(SDL_PollEvent(&event))
		_sdlsc_event(&event);
}

//===
//Video
//===

//Makes sure we have a window, renderer and a texture for each graphics mode.
//Runs on the SDL thread, and only ever there: SDL wants video on one thread, and
//on Apple platforms that thread has to be the main thread.
static void _sdlsc_video_start(void)
{
	if(_sdlsc_headless)
		return;

	if(_sdlsc_video_up)
	{
		//Already running - but the textures can have been thrown away by a lost
		//renderer context, so put back any that are missing.
		for(int mm = 0; mm < _SC_GFX_MODE_MAX; mm++)
		{
			struct _sdlsc_mode_s *mode = &(_sdlsc_modes[mm]);
			if(mm == _SC_GFX_MODE_TEXT || mode->tex != NULL)
				continue;
			mode->tex = SDL_CreateTexture(_sdlsc_renderer, SDL_PIXELFORMAT_RGB565,
				SDL_TEXTUREACCESS_STREAMING, mode->w, mode->h);
			if(mode->tex == NULL)
				_sdlsc_die("SDL_CreateTexture");
		}
		return;
	}

	if(!SDL_WasInit(SDL_INIT_VIDEO))
	{
		if(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
		{
			//No display to talk to. Carry on headless rather than dying: this is what
			//lets an application run on a build machine with no X server.
			fprintf(stderr, "sdlsc: no video (%s); running headless\n", SDL_GetError());
			_sdlsc_headless = 1;
			return;
		}
	}

	if(_sdlsc_wake_event == 0)
		_sdlsc_wake_event = SDL_RegisterEvents(1);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

	const char *title = (_sdlsc_title != NULL) ? _sdlsc_title : "An awesome Neki32 game!";
	_sdlsc_window = SDL_CreateWindow(title,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		1280, 960, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	if(_sdlsc_window == NULL)
		_sdlsc_die("SDL_CreateWindow");

	_sdlsc_renderer = SDL_CreateRenderer(_sdlsc_window, -1, _sdlsc_vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
	if(_sdlsc_renderer == NULL && _sdlsc_vsync)
		_sdlsc_renderer = SDL_CreateRenderer(_sdlsc_window, -1, 0);
	if(_sdlsc_renderer == NULL)
		_sdlsc_die("SDL_CreateRenderer");

	//Keep the picture at a whole number of screen pixels per console pixel, so the
	//pixel art stays square when the window is resized.
	SDL_RenderSetIntegerScale(_sdlsc_renderer, SDL_TRUE);
	SDL_SetRenderDrawColor(_sdlsc_renderer, 0, 0, 0, 255);

	for(int mm = 0; mm < _SC_GFX_MODE_MAX; mm++)
	{
		struct _sdlsc_mode_s *mode = &(_sdlsc_modes[mm]);
		if(mm == _SC_GFX_MODE_TEXT)
			continue;

		mode->tex = SDL_CreateTexture(_sdlsc_renderer, SDL_PIXELFORMAT_RGB565,
			SDL_TEXTUREACCESS_STREAMING, mode->w, mode->h);
		if(mode->tex == NULL)
			_sdlsc_die("SDL_CreateTexture");
	}

	_sdlsc_video_up = 1;
}

//Puts the given frame on the screen, or just accounts for it in headless mode.
//Runs on the SDL thread.
static void _sdlsc_present(int mode, const void *buffer)
{
	if(_sdlsc_headless)
		return;

	_sdlsc_video_start();
	if(_sdlsc_headless)
		return;

	struct _sdlsc_mode_s *modeptr = &(_sdlsc_modes[mode]);

	//Follow the graphics mode, so aspect ratio is right and resizing works
	if(_sdlsc_logical_mode != mode)
	{
		SDL_RenderSetLogicalSize(_sdlsc_renderer, modeptr->w, modeptr->h);
		_sdlsc_logical_mode = mode;
	}

	if(mode != _SC_GFX_MODE_TEXT && modeptr->tex != NULL && buffer != NULL)
	{
		if(SDL_UpdateTexture(modeptr->tex, NULL, buffer, modeptr->w * 2) != 0)
			_sdlsc_die("SDL_UpdateTexture");
		if(SDL_RenderCopy(_sdlsc_renderer, modeptr->tex, NULL, NULL) != 0)
			_sdlsc_die("SDL_RenderCopy");
	}
	else
	{
		//Text mode - the kernel would draw its own text buffer here, which we don't have
		SDL_SetRenderDrawColor(_sdlsc_renderer, 0, 0, 0, 255);
		if(SDL_RenderClear(_sdlsc_renderer) != 0)
			_sdlsc_die("SDL_RenderClear");
	}

	SDL_RenderPresent(_sdlsc_renderer);
}

//Writes the given frame out as a binary PPM, for automated comparison.
static void _sdlsc_dump(int mode, const void *buffer, long long frame)
{
	if(_sdlsc_dump_dir == NULL || buffer == NULL || mode == _SC_GFX_MODE_TEXT)
		return;

	if(_sdlsc_dump_every > 1 && (frame % _sdlsc_dump_every) != 0)
		return;

	//Make the directory the first time we are asked to write into it, so an automated
	//run only has to say SDLSC_DUMP=out and not set the directory up first.
	static int made_dir;
	if(!made_dir)
	{
		made_dir = 1;
		_sdlsc_mkdir(_sdlsc_dump_dir);
	}

	char path[1024];
	snprintf(path, sizeof(path), "%s/frame%06lld.ppm", _sdlsc_dump_dir, frame);

	FILE *file = fopen(path, "wb");
	if(file == NULL)
	{
		static int complained;
		if(!complained)
		{
			complained = 1;
			fprintf(stderr, "sdlsc: failed to open \"%s\" for frame dump (further failures not reported)\n", path);
		}
		return;
	}

	const int w = _sdlsc_modes[mode].w;
	const int h = _sdlsc_modes[mode].h;
	const uint16_t *pixels = (const uint16_t *)buffer;

	fprintf(file, "P6\n%d %d\n255\n", w, h);
	unsigned char *row = (unsigned char *)malloc((size_t)w * 3u);
	if(row == NULL)
	{
		fclose(file);
		_sdlsc_die("malloc");
	}
	for(int yy = 0; yy < h; yy++)
	{
		for(int xx = 0; xx < w; xx++)
		{
			uint16_t pixel = pixels[(yy * w) + xx];
			row[(xx * 3) + 0] = (unsigned char)(((pixel >> 11) & 0x1F) << 3);
			row[(xx * 3) + 1] = (unsigned char)(((pixel >>  5) & 0x3F) << 2);
			row[(xx * 3) + 2] = (unsigned char)(((pixel >>  0) & 0x1F) << 3);
		}
		fwrite(row, 1, (size_t)w * 3u, file);
	}
	free(row);

	fclose(file);
}

//Cheap FNV-1a over a frame, so a trace log can be diffed without keeping images.
static unsigned int _sdlsc_hash(int mode, const void *buffer)
{
	if(buffer == NULL || mode == _SC_GFX_MODE_TEXT)
		return 0;

	const unsigned char *bytes = (const unsigned char *)buffer;
	size_t len = (size_t)_sdlsc_modes[mode].w * (size_t)_sdlsc_modes[mode].h * 2u;
	unsigned int hash = 2166136261u;
	for(size_t bb = 0; bb < len; bb++)
	{
		hash ^= bytes[bb];
		hash *= 16777619u;
	}
	return hash;
}

//===
//Scanout
//===

//Presents one pending flip, if there is one. Runs on the SDL thread.
//Returns nonzero if a frame was displayed.
static int _sdlsc_scanout(void)
{
	int mode;
	const void *buffer;
	unsigned long long seq;

	SDL_LockMutex(_sdlsc_mtx);
	if(!_sdlsc_enq_valid)
	{
		SDL_UnlockMutex(_sdlsc_mtx);
		return 0;
	}
	mode = _sdlsc_enq_mode;
	buffer = _sdlsc_enq_buf;
	seq = _sdlsc_seq_enq;
	_sdlsc_enq_valid = 0;

	//Taking the frame *is* the flip, so publish it as displayed right here, under the
	//same lock. The application is told that whatever flip reports as displayed is on
	//screen, and it avoids drawing into that buffer; if we published only after copying
	//the pixels out, an application running ahead of the scanout could reclaim this
	//buffer while we were still reading it, and the frame would be torn.
	_sdlsc_displayed = buffer;
	_sdlsc_displayed_mode = mode;
	_sdlsc_frames++;
	long long frame = _sdlsc_frames - 1;
	_sdlsc_seq_done = seq;
	SDL_CondBroadcast(_sdlsc_cnd);
	SDL_UnlockMutex(_sdlsc_mtx);

	//Now show it, if there's anything to show it on. The application will not draw into
	//this buffer again until flip tells it that something else is displayed.
	_sdlsc_present(mode, buffer);

	//Record it, if we're being watched
	_sdlsc_dump(mode, buffer, frame);
	if(_sdlsc_trace)
	{
		SDL_LockMutex(_sdlsc_mtx);
		unsigned int buttons = _sdlsc_buttons;
		SDL_UnlockMutex(_sdlsc_mtx);
		fprintf(stderr, "sdlsc: frame %lld mode %d ticks %d buttons 0x%04x hash 0x%08x\n",
			frame, mode, _sdlsc_ticks(0), buttons, _sdlsc_hash(mode, buffer));
	}

	//Input scripted for the frame we're about to start

	if(_sdlsc_frame_limit > 0 && _sdlsc_frames >= _sdlsc_frame_limit)
	{
		fflush(NULL);
		exit(0);
	}

	return 1;
}

//===
//The pump, when the application gave us the real main thread
//===

//Runs the application's entry point, on the worker thread.
static int _sdlsc_app_thread(void *unused)
{
	(void)unused;

	//A negative argc means the entry point takes no arguments (int f(void)),
	//which is common for PVMK applications that don't parse a command line.
	int result;
	if(_sdlsc_entry_argc < 0)
		result = ((int (*)(void))_sdlsc_entry)();
	else
		result = ((int (*)(int, char **))_sdlsc_entry)(_sdlsc_entry_argc, _sdlsc_entry_argv);

	SDL_LockMutex(_sdlsc_mtx);
	_sdlsc_app_running = 0;
	SDL_CondBroadcast(_sdlsc_cnd);
	SDL_UnlockMutex(_sdlsc_mtx);
	return result;
}

//The real main thread: scan out frames, and keep the window alive.
static void _sdlsc_pump(void)
{
	while(!_sdlsc_done)
	{
		if(!_sdlsc_headless)
			_sdlsc_pump_events();

		if(_sdlsc_done)
			break;

		//Testing hook: close the window from the inside, so the path where the user
		//clicks the close box can be exercised without a user.
		if(_sdlsc_quit_after > 0 && _sdlsc_frames >= _sdlsc_quit_after)
		{
			if(_sdlsc_headless)
				_sdlsc_done = 1;
			else
			{
				SDL_Event quit;
				memset(&quit, 0, sizeof(quit));
				quit.type = SDL_QUIT;
				SDL_PushEvent(&quit);
			}
			_sdlsc_quit_after = 0;
		}

		//Testing hook: pretend the renderer lost its context, which is what happens on
		//a real display when the GL context or D3D device goes away. The textures have
		//to come back and the picture has to keep going.
		if(_sdlsc_reset_after > 0 && _sdlsc_frames >= _sdlsc_reset_after)
		{
			_sdlsc_reset_after = 0;
			if(!_sdlsc_headless)
			{
				SDL_Event reset;
				memset(&reset, 0, sizeof(reset));
				reset.type = SDL_RENDER_DEVICE_RESET;
				SDL_PushEvent(&reset);
			}
		}

		if(_sdlsc_scanout())
			continue;

		SDL_LockMutex(_sdlsc_mtx);
		int app_running = _sdlsc_app_running;
		SDL_UnlockMutex(_sdlsc_mtx);
		if(!app_running)
			break;

		if(_sdlsc_headless)
		{
			//Nothing to show. Sleep until the application enqueues a flip.
			SDL_LockMutex(_sdlsc_mtx);
			while(!_sdlsc_enq_valid && _sdlsc_app_running && !_sdlsc_done)
			{
				if(SDL_CondWaitTimeout(_sdlsc_cnd, _sdlsc_mtx, 100) == SDL_MUTEX_TIMEDOUT)
					break;
			}
			SDL_UnlockMutex(_sdlsc_mtx);
		}
		else
		{
			//Keep the window alive: block until something happens, but not forever,
			//in case the application enqueued a flip without us noticing. The timeout
			//is what makes the wakeup below a hint rather than something to get right:
			//if a flip lands between the flag being set and the wait starting, the
			//timeout picks it up.
			_sdlsc_pump_waiting = 1;
			SDL_Event event;
			if(SDL_WaitEventTimeout(&event, 20))
			{
				_sdlsc_pump_waiting = 0;
				_sdlsc_event(&event);
				_sdlsc_pump_events();
			}
			_sdlsc_pump_waiting = 0;
		}
	}

	//If we stopped because the window was closed, there is no polite way to stop a
	//PVMK application, so the whole process goes. If we stopped because the
	//application finished, we return and let main() exit normally.
	if(_sdlsc_done)
	{
		fflush(NULL);
		exit(0);
	}
}


//===
//Entry point, for applications that hand us the real main thread
//===
int _sdlsc_run(void (*entry)(void), int argc, void *argv)
{
	_sdlsc_cfg_init();

	if(_sdlsc_pumped)
	{
		fprintf(stderr, "sdlsc: _sc_shim_main() called more than once\n");
		return 1;
	}
	_sdlsc_pumped = 1;
	_sdlsc_entry = entry;
	_sdlsc_entry_argc = argc;
	_sdlsc_entry_argv = (char **)argv;

	//Set the video up here, while we still own the main thread, so that a failure is
	//reported before the application starts rather than as a dead window later.
	if(!_sdlsc_headless)
		_sdlsc_video_start();

	_sdlsc_app_running = 1;
	SDL_Thread *thread = SDL_CreateThread(_sdlsc_app_thread, "sdlsc app", NULL);
	if(thread == NULL)
		_sdlsc_die("SDL_CreateThread");

	//From here on, the real main thread belongs to SDL.
	_sdlsc_pump();

	int result = 0;
	SDL_WaitThread(thread, &result);
	return result;
}

//===
//System calls
//===

int _sc_gfx_flip(int mode, const void *buffer)
{
	_sdlsc_cfg_init();

	//The validation the kernel does
	if(mode < 0 || mode >= _SC_GFX_MODE_MAX)
		return -_SC_EINVAL;

	if((mode == _SC_GFX_MODE_TEXT) != (buffer == NULL))
		return -_SC_EINVAL;

	//On a 64-bit host the address we have to return cannot fit in the int that the
	//system call returns, and the application will not recognise the buffer in the
	//return value. Complain once, rather than leaving that to be discovered by
	//wondering why the picture tears.
	if(!_sdlsc_fb_warned && sizeof(void *) > 4 && (((uintptr_t)buffer) >> 32) != 0)
	{
		_sdlsc_fb_warned = 1;
		fprintf(stderr, "sdlsc: framebuffer %p is above 4GB, but _sc_gfx_flip() returns a\n"
			"       32-bit address. The application cannot recognise this buffer in the\n"
			"       return value. Keep framebuffers in static memory, or map them below 4GB.\n",
			buffer);
	}

	//Enqueue, exactly like the kernel's gfx_flip()
	SDL_LockMutex(_sdlsc_mtx);
	const void *displayed = _sdlsc_displayed;
	_sdlsc_enq_mode = mode;
	_sdlsc_enq_buf = buffer;
	_sdlsc_enq_valid = 1;
	unsigned long long seq = ++_sdlsc_seq_enq;
	SDL_CondBroadcast(_sdlsc_cnd);
	SDL_UnlockMutex(_sdlsc_mtx);

	if(_sdlsc_pumped)
	{
		if(!_sdlsc_headless && _sdlsc_wake_event != 0 && _sdlsc_pump_waiting)
		{
			//Nudge the pump out of its event wait. Pushing an event is thread-safe.
			//Only worth doing when it is actually waiting: an application that flips in
			//a tight loop would otherwise fill SDL's event queue with wakeups.
			SDL_Event wake;
			memset(&wake, 0, sizeof(wake));
			wake.type = _sdlsc_wake_event;
			SDL_PushEvent(&wake);
		}

		if(_sdlsc_headless)
		{
			//Deterministic scanout: wait for this exact frame to be consumed, so that
			//one flip is exactly one frame of virtual time.
			SDL_LockMutex(_sdlsc_mtx);
			while(_sdlsc_seq_done < seq && !_sdlsc_done)
				SDL_CondWait(_sdlsc_cnd, _sdlsc_mtx);
			SDL_UnlockMutex(_sdlsc_mtx);
		}

		if(_sdlsc_done)
			exit(0);
	}
	else
	{
		//Inline scanout. We are on the SDL thread - the process main thread - so the
		//work can just happen here and now.
		_sdlsc_pump_events();
		if(_sdlsc_done)
			exit(0);
		_sdlsc_scanout();
		if(_sdlsc_done)
			exit(0);
	}

	//The kernel returns the address of the image displayed before this flip took
	//effect, so that the caller knows which buffer is safe to draw into next. PVMK
	//is a 32-bit system, so the kernel returns it in an int.
	return (int)((intptr_t)displayed & 0x7FFFFFFF);
}

int _sc_input(_sc_input_t *buffer_ptr, int bytes_per_event, int bytes_max)
{
	_sdlsc_cfg_init();

	if(buffer_ptr == NULL)
		return -_SC_EINVAL;
	if(bytes_per_event < (int)sizeof(_sc_input_t))
		return -_SC_EINVAL;
	if(bytes_max < bytes_per_event)
		return -_SC_EINVAL;

	if(!_sdlsc_pumped)
	{
		//Inline: we own SDL, so pump it here
		_sdlsc_pump_events();
		if(_sdlsc_done)
			exit(0);
	}

	//Scripted input is timed against the clock, so that a program polling for a button
	//before it draws anything still gets one. With a window the event pump delivers it,
	//as key events; here we only do it when there is no event queue at all.
	if(_sdlsc_headless && _sdlsc_script_len > 0)
		_sdlsc_input_script(_sdlsc_input_frame() + 1);

	unsigned char *out = (unsigned char *)buffer_ptr;
	int events = 0;
	while((events + 1) * bytes_per_event <= bytes_max)
	{
		unsigned int buttons = 0;
		if(!_sdlsc_input_pop(&buttons))
			break;

		_sc_input_t ev;
		memset(&ev, 0, sizeof(ev));
		ev.format = 'A';
		ev.buttons = (short)buttons;
		memcpy(out + (events * bytes_per_event), &ev, sizeof(ev));
		events++;
	}

	if(events <= 0)
		return -_SC_EAGAIN;

	return events;
}

int _sc_snd_play(int mode, const void *chunk, int chunkbytes, int maxbuf)
{
	_sdlsc_cfg_init();

	//The validation the kernel does
	if(mode < 0 || mode >= _SC_SND_MODE_MAX)
		return -_SC_EINVAL;

	if(mode == _SC_SND_MODE_SILENT)
	{
		//Silence - no chunk, no buffer
		if(chunk != NULL || chunkbytes != 0 || maxbuf != 0)
			return -_SC_EINVAL;

		if(_sdlsc_audio_dev != 0)
			SDL_ClearQueuedAudio(_sdlsc_audio_dev);

		return 0;
	}

	if(chunk == NULL || chunkbytes <= 0 || maxbuf <= 0)
		return -_SC_EINVAL;

	//16-bit stereo means whole frames of four bytes
	if(mode == _SC_SND_MODE_48K_16B_2C && ((chunkbytes & 3) || (maxbuf & 3)))
		return -_SC_EINVAL;

	if(_sdlsc_audio_off)
		return chunkbytes; //Pretend it all went out

	if(_sdlsc_audio_dev == 0)
	{
		if(!SDL_WasInit(SDL_INIT_AUDIO))
		{
			if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
				_sdlsc_die("SDL_InitSubSystem(SDL_INIT_AUDIO)");
		}

		SDL_AudioSpec desired;
		memset(&desired, 0, sizeof(desired));
		desired.freq = 48000;
		desired.format = AUDIO_S16LSB;
		desired.channels = 2;
		desired.samples = 2048;

		SDL_AudioSpec obtained;
		memset(&obtained, 0, sizeof(obtained));

		SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
		if(dev == 0)
			_sdlsc_die("SDL_OpenAudioDevice");

		_sdlsc_audio_dev = dev;
		SDL_PauseAudioDevice(dev, 0);
	}

	if(SDL_GetQueuedAudioSize(_sdlsc_audio_dev) + (size_t)chunkbytes > (size_t)maxbuf)
		return -_SC_EAGAIN;

	if(SDL_QueueAudio(_sdlsc_audio_dev, chunk, (Uint32)chunkbytes) != 0)
		_sdlsc_die("SDL_QueueAudio");

	//The kernel returns the number of bytes now enqueued
	return (int)SDL_GetQueuedAudioSize(_sdlsc_audio_dev);
}

void _sc_pause(void)
{
	_sdlsc_cfg_init();

	if(!_sdlsc_pumped)
	{
		_sdlsc_pump_events();
		if(_sdlsc_done)
			exit(0);
	}

	if(_sdlsc_vclock)
	{
		//Virtual clock: waiting is a millisecond of virtual time and no real time at
		//all, so that a program that pauses between frames still makes progress and
		//still runs as fast as the machine allows. Yield, so other threads get a look in.
		SDL_AtomicAdd(&_sdlsc_vtime, 1);
		SDL_Delay(0);
		return;
	}

	//The kernel blocks the caller until something happens to it, which in practice is
	//the next 1KHz tick. Sleeping one millisecond is the same thing, and keeps polling
	//loops (including fatal-error loops) off the CPU.
	SDL_Delay(1);
}

//Reads the clock the shim reports to the application.
//In virtual-clock mode, each read is one millisecond later than the last. The clock must
//not depend on frames being displayed: a program is entitled to wait for time to pass
//before it draws anything (Doom's tic loop does exactly that), and would then be waiting
//for a frame that only it could produce. Advancing on demand keeps a run deterministic and
//as fast as the machine can go; the cost is that idle time is charged at one millisecond
//per enquiry, so a program that waits five seconds polls five thousand times.
static int _sdlsc_ticks(int advance)
{
	if(_sdlsc_vclock)
	{
		if(advance)
			return SDL_AtomicAdd(&_sdlsc_vtime, 1) + 1;
		return SDL_AtomicGet(&_sdlsc_vtime);
	}

	return (int)SDL_GetTicks();
}

int _sc_getticks(void)
{
	_sdlsc_cfg_init();
	return _sdlsc_ticks(1);
}

int _sc_print(const char *buf_ptr)
{
	_sdlsc_cfg_init();

	if(buf_ptr == NULL)
		return -_SC_EINVAL;

	int len = (int)strlen(buf_ptr);
	if(len > 0)
	{
		fwrite(buf_ptr, 1, (size_t)len, stdout);
		fflush(stdout);
	}
	return len;
}

int _sc_nvm_save(const void *buf, int len)
{
	_sdlsc_cfg_init();

	if(buf == NULL || len < 0)
		return -_SC_EINVAL;

	const char *path = (_sdlsc_nvm_path != NULL) ? _sdlsc_nvm_path : ".nvm.bin";
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if(fd < 0)
	{
		fprintf(stderr, "sdlsc: failed to open NVM \"%s\"\n", path);
		return -_SC_ENOENT;
	}

	int written = 0;
	while(written < len)
	{
		int result = (int)write(fd, (const char *)buf + written, (size_t)(len - written));
		if(result <= 0)
		{
			fprintf(stderr, "sdlsc: failed to write NVM \"%s\"\n", path);
			close(fd);
			return -_SC_ENOSPC;
		}
		written += result;
	}

	close(fd);
	return 0;
}

int _sc_nvm_load(void *buf, int len)
{
	_sdlsc_cfg_init();

	if(buf == NULL || len < 0)
		return -_SC_EINVAL;

	const char *path = (_sdlsc_nvm_path != NULL) ? _sdlsc_nvm_path : ".nvm.bin";
	int fd = open(path, O_RDONLY);
	if(fd < 0)
		return -_SC_ENOENT;

	int got = 0;
	while(got < len)
	{
		int result = (int)read(fd, (char *)buf + got, (size_t)(len - got));
		if(result < 0)
		{
			close(fd);
			return -_SC_EINVAL;
		}
		if(result == 0)
			break;
		got += result;
	}

	close(fd);

	//The kernel copies as much as the record holds - asking for more than it has is
	//not an error - and returns the number of bytes it gave back. A short read is
	//reported as the bytes actually copied, and only a missing record is ENOENT.
	return got;
}

int _sc_nvm_ident(const char *name)
{
	(void)name;
	return -_SC_ENOSYS;
}

int _sc_nvm_delete(void)
{
	return -_SC_ENOSYS;
}

int _sc_nvm_enum(int idx, char *name_buf, int name_max)
{
	(void)idx;
	(void)name_buf;
	(void)name_max;
	return -_SC_ENOSYS;
}

int _sc_halt(void)
{
	fflush(NULL);
	exit(0);
}

int _sc_mount(int container_fd, int disk_number)
{
	(void)container_fd;
	(void)disk_number;
	return -_SC_ENOSYS;
}

int _sc_deny_syscall(int callnum)
{
	(void)callnum;
	return -_SC_ENOSYS;
}

int _sc_deny_filefmt(int mode)
{
	(void)mode;
	return -_SC_ENOSYS;
}

#endif //USE_SDLSC
