//prefs_headless.cpp
//Preferences for the console front end: there is no wxWidgets and no saved configuration.
//Bryan E. Topp <betopp@betopp.com> 2026
//
//The wx front end stores pads and the GDB port in a wxConfig, which is the only reason the
//emulation core ever needed wxWidgets at all. Here the defaults are returned and nothing is
//written, which is all a headless run can want: no pad input, no RSP listener.

#include "prefs.h"
#include <string.h>

void prefs_read(prefs_t *out)
{
	memset(out, 0, sizeof(*out));

	//No buttons bound, so the emulated console sees an untouched pad
	for(int pp = 0; pp < PREFS_PAD_MAX; pp++)
	{
		out->pads[pp].js_id = -1;
		for(int bb = 0; bb < PREFS_PAD_BTN_MAX; bb++)
		{
			out->pads[pp].btn_src[bb] = PREFS_PAD_SRC_NONE;
			out->pads[pp].btn_val[bb] = 0;
		}
	}

	//No GDB listener unless asked for
	out->rsp_enabled = false;
	out->rsp_port = 0;
}

void prefs_write(const prefs_t *in)
{
	(void)in; //Nothing to save to
}
