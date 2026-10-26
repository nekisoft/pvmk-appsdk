//nemul_headless.cpp
//Console front end for Nemul, the Neki32 Simulator.
//Bryan E. Topp <betopp@betopp.com> 2026
//
//Same emulation core as the wxWidgets front end - interp, process table, syscalls, the real
//init image - with no window, no audio and no X. Exists so a card image can be booted and the
//whole boot path exercised on a machine with no display, which is where the card scheme gets
//tested: nemul answers _sc_cardprobe from a recorded card, so bootmenu derives exactly what
//mkanticopy built for.
//
//  nemul-headless --disk <card image> --records <record sector or card image>
//                 [--ticks <n>] [--fb <prefix>] [--quiet]
//
//The framebuffer is written out as PPM files so what the console would have shown can be
//looked at. One file per emulated second, plus a final frame.

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "process.h"
#include "sysc.h"
#include "rsp.h"
#include "prefs.h"
#include "trace.h"
#include "prof.h"

//Milliseconds of emulated time, which is what _sc_getticks returns. The wx front end keeps
//this in its timer; here the loop below owns it.
uint32_t EmulTimerTicks = 0;

//Framebuffer sizes by mode, matching sysc.cpp's sysc_fb_sizes
static const int fb_widths[]  = { 0, 640, 320 };
static const int fb_heights[] = { 0, 480, 240 };

//Writes a frontbuffer as a binary PPM. Returns 0 on success.
//Creates the directories a dump path needs, one level at a time
static void make_dirs(const char *path)
{
	char scratch[4096];
	snprintf(scratch, sizeof(scratch), "%s", path);
	for(char *cc = scratch + 1; *cc != '\0'; cc++)
	{
		if(*cc != '/')
			continue;
		*cc = '\0';
		mkdir(scratch, 0777);
		*cc = '/';
	}

	//And the last component itself, which has no slash after it
	mkdir(scratch, 0777);
}

static int dumpfb(const char *prefix, int index, uint16_t *fb, int mode)
{
	if(fb == NULL || mode <= 0 || mode > 2)
		return -1;

	int width = fb_widths[mode];
	int height = fb_heights[mode];

	char path[4096];
	if(index < 0)
		snprintf(path, sizeof(path), "%s-final.ppm", prefix);
	else
		snprintf(path, sizeof(path), "%s-%05d.ppm", prefix, index);

	FILE *out = fopen(path, "wb");
	if(out == NULL)
	{
		static int complained = 0;
		if(!complained)
			fprintf(stderr, "cannot write framebuffer dump to %s\n", path);
		complained = 1;
		return -1;
	}

	fprintf(out, "P6\n%d %d\n255\n", width, height);
	for(int yy = 0; yy < height; yy++)
	{
		for(int xx = 0; xx < width; xx++)
		{
			uint16_t pixel = fb[(yy * width) + xx];
			unsigned char rgb[3];
			rgb[0] = (unsigned char)(((pixel >> 11) & 0x1F) << 3); //R
			rgb[1] = (unsigned char)(((pixel >>  5) & 0x3F) << 2); //G
			rgb[2] = (unsigned char)(((pixel >>  0) & 0x1F) << 3); //B
			if(fwrite(rgb, 1, 3, out) != 3)
			{
				fclose(out);
				return -1;
			}
		}
	}

	fclose(out);
	return 0;
}

//Whether anything is left that could still run
static int all_stopped(void)
{
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state == PROCESS_STATE_ALIVE && !process_table[pp].dbgstop)
			return 0;
	}
	return 1;
}

int main(int argc, char **argv)
{
	const char *diskpath = NULL;
	const char *recspath = NULL;
	const char *fbprefix = NULL;
	uint32_t breakat = 0;
	int breaksteps = 24;
	const char *nnepath = NULL;
	long ticks = 30000; //Thirty seconds of emulated time
	int quiet = 0;
	int tracing = 0;
	int tracesysc = 0;

	for(int aa = 1; aa < argc; aa++)
	{
		if(strcmp(argv[aa], "--disk") == 0 && aa + 1 < argc)
			diskpath = argv[++aa];
		else if(strcmp(argv[aa], "--records") == 0 && aa + 1 < argc)
			recspath = argv[++aa];
		else if(strcmp(argv[aa], "--ticks") == 0 && aa + 1 < argc)
			ticks = atol(argv[++aa]);
		else if(strcmp(argv[aa], "--bp") == 0 && aa + 1 < argc)
			breakat = (uint32_t)strtoul(argv[++aa], NULL, 16);
		else if(strcmp(argv[aa], "--step") == 0 && aa + 1 < argc)
			breaksteps = atoi(argv[++aa]);
		else if(strcmp(argv[aa], "--nne") == 0 && aa + 1 < argc)
			nnepath = argv[++aa];
		else if(strcmp(argv[aa], "--fb") == 0 && aa + 1 < argc)
			fbprefix = argv[++aa];
		else if(strcmp(argv[aa], "--quiet") == 0)
			quiet = 1;
		else if(strcmp(argv[aa], "--trace") == 0)
			tracing = 1;
		else if(strcmp(argv[aa], "--trace-sysc") == 0)
			tracesysc = 1;
		else
		{
			fprintf(stderr, "usage: %s --disk <card image> [--records <record sector>] "
				"[--nne <image>] [--bp <hex addr>] [--step <n>] [--ticks <n>] [--fb <prefix>] [--quiet] [--trace] [--trace-sysc]\n", argv[0]);
			return 2;
		}
	}

	if(recspath != NULL)
	{
		if(sysc_setcardrecords(recspath) != 0)
		{
			fprintf(stderr, "card records %s: not a PVMKINSP record sector\n", recspath);
			return 2;
		}
		if(!quiet)
			printf("card: answering from records in %s\n", recspath);
	}

	if(diskpath != NULL)
	{
		int diskfd = open(diskpath, O_RDWR);
		if(diskfd < 0)
		{
			fprintf(stderr, "cannot open disk image %s\n", diskpath);
			return 2;
		}
		sysc_setdiskfd(diskfd);
		if(!quiet)
			printf("disk: %s\n", diskpath);
	}

	//The trace buffer is a ring the GUI shows in a window. Here it can be dumped on the way
	//out, which is how an interpreter failure explains itself without a debugger.
	if(tracing)
	{
		//Fatal and error messages only. At DEBUG the interpreter logs every instruction, which
		//is three hundred thousand lines per emulated millisecond and overruns the ring buffer
		//before it can be read back.
		for(int cc = 0; cc < TRACE_CAT_MAX; cc++)
			trace_sev_limit[cc] = TRACE_SEV_ERROR;
		trace_also_stderr = 1;
	}

	//System calls and the card probes behind them, which is how the emulated card can be
	//watched answering the boot path
	if(tracesysc)
	{
		trace_sev_limit[TRACE_CAT_SYSC] = TRACE_SEV_DEBUG;
		trace_sev_limit[TRACE_CAT_PROCESS] = TRACE_SEV_INFO;
		trace_also_stderr = 1;
	}

	prefs_t prefs;
	prefs_read(&prefs);

	if(fbprefix != NULL)
	{
		//Make the directory the dumps go in, if the path has one
		char dirof[4096];
		snprintf(dirof, sizeof(dirof), "%s", fbprefix);
		char *slash = strrchr(dirof, '/');
		if(slash != NULL)
		{
			*slash = '\0';
			if(dirof[0] != '\0')
				make_dirs(dirof);
		}
	}

	process_reset();

	//A single program instead of the boot menu, for testing one thing at a time
	if(nnepath != NULL)
	{
		if(sysc_bootimage(nnepath) != 0)
		{
			fprintf(stderr, "cannot boot %s as an NNE image\n", nnepath);
			return 2;
		}
		if(!quiet)
			printf("nne: %s\n", nnepath);
	}

	//The console's vertical blank: sixty a second, as the timer in the wx front end does it.
	//Popping the frontbuffer is what unblocks a process waiting on _sc_gfx_flip, so the rate
	//here is part of the emulation and not just how often the picture is looked at.
	uint16_t *fb = NULL;
	int fbmode = 0;
	long ms_till_vsync = 16;
	int vsync_logging = (getenv("NEMUL_LOG_VSYNC") != NULL) ? 1 : 0;
	long vsyncs = 0;

	if(breakat != 0)
		emul_set_break(breakat, breaksteps);

	for(long tick = 0; tick < ticks; tick++)
	{
		rsp_poll();
		prof_tick((long)EmulTimerTicks, quiet);
		process_step();
		EmulTimerTicks++;

		ms_till_vsync--;
		if(ms_till_vsync <= 0)
		{
			ms_till_vsync = 16;
			sysc_popfbptr(&fb, &fbmode);
			vsyncs++;
			if(vsync_logging && vsyncs <= 5)
				fprintf(stderr, "vsync %ld: buffer %p mode %d\n", vsyncs, (void*)fb, fbmode);
			if(fbprefix != NULL && (vsyncs % 60) == 0)
				dumpfb(fbprefix, (int)(vsyncs / 60), fb, fbmode);
		}

		if(all_stopped() || emul_stop_requested)
			break;
	}

	if(fbprefix != NULL)
		dumpfb(fbprefix, -1, fb, fbmode);

	prof_dump();

	if(tracing || tracesysc)
	{
		//Newest first out of trace_recall, so collect and then print oldest first
		#define TRACE_SHOW_MAX 400
		static trace_cat_e tcat[TRACE_SHOW_MAX];
		static trace_sev_e tsev[TRACE_SHOW_MAX];
		static char *tmsg[TRACE_SHOW_MAX];
		int shown = 0;
		for(; shown < TRACE_SHOW_MAX; shown++)
		{
			if(!trace_recall(shown, &tcat[shown], &tsev[shown], &tmsg[shown]))
				break;
		}
		for(int ii = shown - 1; ii >= 0; ii--)
			fprintf(stderr, "[%d/%d] %s", (int)tcat[ii], (int)tsev[ii], tmsg[ii]);
	}

	if(!quiet)
	{
		printf("stopped after %ld ticks\n", ticks);
		for(int pp = 0; pp < PROCESS_MAX; pp++)
		{
			if(process_table[pp].state == PROCESS_STATE_NONE)
				continue;
			printf("  pid %d state %d pc %8.8X dbgstop %d\n",
				process_table[pp].pid, (int)process_table[pp].state,
				process_table[pp].regs[15], (int)process_table[pp].dbgstop);
		}
	}

	return 0;
}
