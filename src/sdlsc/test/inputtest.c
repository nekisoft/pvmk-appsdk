//inputtest.c
//Checks that scripted input arrives, in order, through whichever path the shim is using:
//straight into the queue when headless, as key events through SDL when there is a window.
//Bryan E. Topp <betopp@betopp.com> 2024
//
//Run with a script that presses start, start+a, nothing, then down. With "strict" as the
//first argument every one of those states must be seen, in order - which is what a
//deterministic (headless, virtual clock) run has to deliver. Without it, states may be
//coalesced by a window that is being pumped at its own pace, but nothing unexpected may
//appear and the first and last must still turn up.

#include <sc.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define W 320
#define H 240

static unsigned short fb[2][H][W];

static int failures;

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
	if(!cond)
		failures++;
	report("TEST %-46s %s\n", name, cond ? "PASS" : "FAIL");
}

int app_main(int argc, const char **argv)
{
	int strict = (argc > 1 && strcmp(argv[1], "strict") == 0);

	//States the script asks for
	const unsigned int want_start = _SC_BTNBIT_START;
	const unsigned int want_start_a = _SC_BTNBIT_START | _SC_BTNBIT_A;
	const unsigned int want_none = 0;
	const unsigned int want_down = _SC_BTNBIT_DOWN;
	const unsigned int allowed[] = { want_start, want_start_a, want_none, want_down };
	const int allowed_count = 4;

	unsigned int seen[256];
	int seen_count = 0;

	for(int ff = 0; ff < 400; ff++)
	{
		//Spend a millisecond of the run's clock, so script frames come round
		_sc_pause();

		_sc_input_t ev[8];
		int got = _sc_input(ev, (int)sizeof(_sc_input_t), (int)sizeof(ev));
		for(int ee = 0; ee < got && seen_count < 256; ee++)
			seen[seen_count++] = (unsigned int)(unsigned short)ev[ee].buttons;

		unsigned short *buffer = &(fb[ff % 2][0][0]);
		for(int pp = 0; pp < (W * H); pp++)
			buffer[pp] = (unsigned short)(ff * 7);
		(void)_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, buffer);
	}

	//Nothing the script never asked for may turn up
	int unexpected = 0;
	for(int ss = 0; ss < seen_count; ss++)
	{
		int known = 0;
		for(int aa = 0; aa < allowed_count; aa++)
			if(seen[ss] == allowed[aa])
				known = 1;
		if(!known)
			unexpected++;
	}
	check(unexpected == 0, "input reports only states the script asked for");

	//start must come before down
	int start_at = -1;
	int down_at = -1;
	for(int ss = 0; ss < seen_count; ss++)
	{
		if(seen[ss] == want_start && start_at < 0)
			start_at = ss;
		if(seen[ss] == want_down && down_at < 0)
			down_at = ss;
	}
	check(start_at >= 0, "scripted start press arrives");
	check(down_at >= 0, "scripted down press arrives");
	check(start_at >= 0 && down_at >= 0 && start_at < down_at, "scripted input arrives in order");

	if(strict)
	{
		//Every state in turn, with nothing between them: only possible when scanout is
		//deterministic, which is what the headless path is for.
		int idx = 0;
		for(int ss = 0; ss < seen_count && idx < allowed_count; ss++)
			if(seen[ss] == allowed[idx])
				idx++;
		check(idx == allowed_count, "every scripted state arrives, in order");
	}

	report("RESULT failures=%d states=%d\n", failures, seen_count);
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
