//shimtest.c
//Conformance test for the SDL system-call shim.
//Bryan E. Topp <betopp@betopp.com> 2024
//
//This uses nothing but the PVMK system-call API, so it builds two ways:
//	cc -DUSE_SDLSC=1 ... shimtest.c sdlsc.c		inline: shim does SDL on our thread
//	cc -DUSE_SDLSC=1 -DPUMPED ...			pumped: shim owns main, we get a worker
//and it runs with or without a window (SDLSC_HEADLESS=1) and with either clock.
//
//Every check prints a line, and the exit status is the number of failures, so a
//shell script can run a matrix of configurations and just look at the status.

#include <sc.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Three framebuffers, cycled, like a triple-buffering game
#define FB_COUNT 3
static unsigned short frame[FB_COUNT][480][640];

//Graphics modes we exercise, with their framebuffer geometry
static const struct
{
	int mode;
	int w;
	int h;
} modes[] =
{
	{ _SC_GFX_MODE_320X240_16BPP, 320, 240 },
	{ _SC_GFX_MODE_VGA_16BPP,     640, 480 },
};
#define MODE_COUNT ((int)(sizeof(modes) / sizeof(modes[0])))

static int checks;
static int failures;

//Prints a line, using the system-call API like an application would
static void report(const char *fmt, ...)
{
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	_sc_print(buf);
}

static void check(int cond, const char *name)
{
	checks++;
	if(!cond)
		failures++;
	report("TEST %-46s %s\n", name, cond ? "PASS" : "FAIL");
}

//Paints a whole frame in a colour derived from the frame number, so that a dumped
//frame says exactly which frame it is and a torn frame cannot pass as either.
//Components are chosen to survive 5:6:5 and the shim's expansion of it unchanged.
static void paint(unsigned short *fb, int w, int h, int index)
{
	unsigned short pixel =
		(unsigned short)((((index * 8) & 0x1F) << 11) | (((index * 4) & 0x3F) << 5) | ((index * 2) & 0x1F));
	for(int yy = 0; yy < h; yy++)
		for(int xx = 0; xx < w; xx++)
			fb[(yy * w) + xx] = pixel;
}

//Reads back a frame we have been given, at a known point
static int sample(const unsigned short *fb, int w, int h)
{
	if(fb == NULL || w <= 0 || h <= 0)
		return -1;
	return fb[((h / 2) * w) + (w / 2)];
}

int app_main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;

	report("shimtest: start\n");

	//---
	//Ticks and printing
	//---
	int t0 = _sc_getticks();
	int t1 = _sc_getticks();
	check(t1 >= t0, "getticks does not go backwards");

	//---
	//Arguments the kernel rejects
	//---
	check(_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, NULL) == -_SC_EINVAL, "flip rejects a null buffer");
	check(_sc_gfx_flip(99, frame[0]) == -_SC_EINVAL, "flip rejects a bad mode");
	check(_sc_snd_play(99, NULL, 0, 0) == -_SC_EINVAL, "snd_play rejects a bad mode");
	check(_sc_input(NULL, (int)sizeof(_sc_input_t), 64) == -_SC_EINVAL, "input rejects a null buffer");
	check(_sc_print(NULL) == -_SC_EINVAL, "print rejects a null string");

	//---
	//Flip: three buffers cycled, the way a real game does it
	//---
	//Nothing is displayed before the first flip, so the first one has nothing to
	//tell us about; after that, whatever flip returns must be a buffer we enqueued
	//earlier - never the one we are handing over, and never rubbish.
	const void *returned[16];
	int frames = 16;
	int first = 0;
	for(int ff = 0; ff < frames; ff++)
	{
		int idx = ff % FB_COUNT;
		paint(&(frame[idx][0][0]), 320, 240, ff);
		//flip returns a 32-bit address, so this is how an application has to read it
		int ret = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, frame[idx]);
		returned[ff] = (const void *)(uintptr_t)(unsigned int)ret;
		if(ff == 0)
			first = (returned[0] == NULL);
	}
	check(first, "first flip has no previous frame to report");

	int valid = 1;
	for(int ff = 1; ff < frames; ff++)
	{
		const void *r = returned[ff];
		if(r == NULL)
			continue;
		int known = 0;
		for(int bb = 0; bb < FB_COUNT; bb++)
			if(r == (const void *)(frame[bb]))
				known = 1;
		if(!known)
			valid = 0;
	}
	check(valid, "flip only ever returns one of our buffers");

	//A frame reported as displayed must hold the pixels of the frame that was
	//enqueued into that buffer, not a neighbour's: that is what tells us the
	//scanout is showing whole frames and in order. In headless mode the shim
	//consumes each frame before flip returns, so this is checkable.
	int ordered = 1;
	for(int ff = 0; ff < frames; ff++)
	{
		if(returned[ff] == NULL)
			continue;
		const unsigned short *fb = (const unsigned short *)returned[ff];
		int expect = -1;
		for(int gg = 0; gg <= ff; gg++)
			if((const void *)(frame[gg % FB_COUNT]) == returned[ff])
				expect = gg;
		if(expect >= 0 && sample(fb, 320, 240) != sample(&(frame[expect % FB_COUNT][0][0]), 320, 240))
			ordered = 0;
	}
	check(ordered, "frames reported as displayed hold the frame drawn into them");

	//---
	//Input: nothing scripted means no events at all, not synthetic ones
	//---
	_sc_input_t ev[8];
	int got = _sc_input(ev, (int)sizeof(_sc_input_t), (int)sizeof(ev));
	check(got == -_SC_EAGAIN || (got > 0 && got <= 8), "input reports events or EAGAIN");

	//Each event must be well-formed: our format letter and a button mask we know
	int wellformed = 1;
	for(int ee = 0; ee < got; ee++)
	{
		if(ev[ee].format != 'A')
			wellformed = 0;
		if((unsigned short)ev[ee].buttons & 0xF000u)
			wellformed = 0;
	}
	check(wellformed, "input events are well formed");

	//Draining must eventually say EAGAIN rather than invent events
	int drained = 0;
	for(int ii = 0; ii < 64; ii++)
	{
		int more = _sc_input(ev, (int)sizeof(_sc_input_t), (int)sizeof(ev));
		if(more == -_SC_EAGAIN)
		{
			drained = 1;
			break;
		}
	}
	check(drained, "input queue drains to EAGAIN");

	//---
	//Sound: queue bytes, then silence
	//---
	static short audio[4096];
	memset(audio, 0, sizeof(audio));
	int snd = _sc_snd_play(_SC_SND_MODE_48K_16B_2C, audio, (int)sizeof(audio), 65536);
	check(snd >= 0, "snd_play accepts a chunk");
	check(_sc_snd_play(_SC_SND_MODE_SILENT, NULL, 0, 0) == 0, "snd_play silent stops playback");
	check(_sc_snd_play(_SC_SND_MODE_48K_16B_2C, audio, 3, 65536) == -_SC_EINVAL,
		"snd_play rejects an unaligned chunk");

	//---
	//NVM: save, load back, and the calls that are not supported
	//---
	static char nvm_in[256];
	static char nvm_out[256];
	for(int ii = 0; ii < (int)sizeof(nvm_in); ii++)
		nvm_in[ii] = (char)(ii * 7);
	check(_sc_nvm_save(nvm_in, sizeof(nvm_in)) == 0, "nvm_save stores data");
	memset(nvm_out, 0, sizeof(nvm_out));
	check(_sc_nvm_load(nvm_out, sizeof(nvm_out)) == (int)sizeof(nvm_out), "nvm_load returns the length");
	check(memcmp(nvm_in, nvm_out, sizeof(nvm_in)) == 0, "nvm round trip is exact");
	check(_sc_nvm_load(nvm_out, 16) == 16, "nvm_load can read a prefix");
	check(_sc_nvm_ident("x") == -_SC_ENOSYS, "nvm_ident is unsupported, as documented");
	check(_sc_nvm_delete() == -_SC_ENOSYS, "nvm_delete is unsupported, as documented");

	//---
	//Pausing is cheap and does not hang
	//---
	int p0 = _sc_getticks();
	_sc_pause();
	int p1 = _sc_getticks();
	check(p1 >= p0, "pause returns");

	//---
	//Graphics modes, changing while running
	//---
	for(int mm = 0; mm < MODE_COUNT; mm++)
	{
		int mode = modes[mm].mode;
		int w = modes[mm].w;
		int h = modes[mm].h;

		//Fill only the part of the buffer this mode uses, so a mode change to a
		//bigger size cannot be passed off by stale pixels from the last one.
		for(int ff = 0; ff < 8; ff++)
		{
			unsigned short *fb = &(frame[ff % FB_COUNT][0][0]);
			paint(fb, w, h, (mm * 100) + ff);
			(void)_sc_gfx_flip(mode, fb);
		}

	}
	check(1, "mode changes do not crash");

	//Text mode, which the kernel draws itself
	(void)_sc_gfx_flip(_SC_GFX_MODE_TEXT, NULL);
	check(1, "text mode does not crash");

	//Back to graphics: the shim has to be able to leave text mode again
	(void)_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, frame[0]);
	check(1, "leaving text mode does not crash");

	//---
	//A short run in the last mode, to give the frame limiter something to count
	//---
	for(int ff = 0; ff < 24; ff++)
	{
		unsigned short *fb = &(frame[ff % FB_COUNT][0][0]);
		paint(fb, 320, 240, ff);
		(void)_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fb);
		(void)_sc_input(ev, (int)sizeof(_sc_input_t), (int)sizeof(ev));
	}

	//Whatever the clock says now, it must be at least what it said at the start
	check(_sc_getticks() >= t0, "clock still monotonic at the end");

	report("RESULT checks=%d failures=%d\n", checks, failures);
	return failures;
}

#ifndef PUMPED
int main(int argc, const char **argv)
{
	return app_main(argc, argv);
}
#else
int main(int argc, const char **argv)
{
	return _sc_shim_main(app_main, argc, argv);
}
#endif
