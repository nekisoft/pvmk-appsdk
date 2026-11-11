//nemul_headless.cpp
//Console front end for Nemul, the Neki32 Simulator.
//Bryan E. Topp <betopp@betopp.com> 2026
//
//Same emulation core as the GUI front end - interp, process table, syscalls, the real init
//image - with no window, no audio and no X. This is the front end an automated harness or an
//agent drives: every run is deterministic, every displayed frame has a hash, and the exit
//status says whether the program finished, crashed, or stopped at a breakpoint.
//
//  nemul-headless --disk <card image> [--nne <program>] [--records <record sector>]
//                 [--ticks <ms>] [--frames <n>] [--input <script>]
//                 [--fb <prefix> [--fb-every <n>]] [--shot <file.ppm>] [--hash]
//                 [--json <file>] [--symbols <elf>] [--gdb [port]]
//                 [--bp <hex addr>] [--step <n>] [--quiet] [--trace] [--trace-sysc]
//
//--frames counts displayed (vertical blank) frames and is the harness's main stop condition;
//--ticks is emulated milliseconds. Whichever comes first ends the run.
//
//--hash prints one line per displayed frame on stdout:
//
//    FRAME 12 TICKS 208 MODE 2 HASH 89ABCDEF
//
//so a regression is a changed hash, without keeping images. --shot writes the last displayed
//frame as a PPM. --json writes a machine-readable summary of the run.
//
//Exit status: 0 the stop condition was reached, 2 bad usage or I/O, 3 the program faulted
//(data abort, prefetch abort, alignment check or interpreter failure), 4 it stopped at a
//breakpoint (a --bp you asked for is a normal stop, 0), 5 it stopped for any other reason.
//
//--input feeds scripted controller input to whatever calls _sc_input. Each line is
//"<frame> <buttons>" and lands on that displayed frame. Buttons are a mask (decimal or 0x)
//or comma-separated names: up down left right a b c x y z start mode. Prefix a group with a
//pad letter to drive another port ("2 a,start 3 b:up"); no prefix means pad A. A pad holds
//its buttons until a later line changes them:
//
//    120 a
//    128 -
//
//The framebuffer is written out as PPM files so what the console would have shown can be
//looked at. One file per --fb-every displayed frames (60 by default), plus a final frame.

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "process.h"
#include "memmodel.h"
#include "sysc.h"
#include "rsp.h"
#include "prefs.h"
#include "trace.h"
#include "prof.h"
#include "symbols.h"

// --- scripted input --------------------------------------------------------
// The console has no keyboard, so a headless run has nothing to press. This
// lets a caller hand the emulator a list of (frame, pads) events and have
// them turn up in _sc_input exactly when a real pad would have produced them.

struct ScriptedInput
{
	long frame;
	uint16_t pads[4]; //complete pad state from this frame on
};

static ScriptedInput *in_script = NULL;
static int in_script_len = 0;
static int in_script_pos = 0;

static int button_by_name(const char *name, size_t len)
{
	static const struct { const char *n; int bit; } NAMES[] =
	{
		{ "up",   0 }, { "left", 1 }, { "down",  2 }, { "right", 3 },
		{ "a",    4 }, { "b",    5 }, { "c",     6 }, { "x",     7 },
		{ "y",    8 }, { "z",    9 }, { "start", 10 }, { "mode", 11 },
	};

	for(size_t nn = 0; nn < sizeof(NAMES) / sizeof(NAMES[0]); nn++)
	{
		if(strlen(NAMES[nn].n) == len && strncmp(NAMES[nn].n, name, len) == 0)
			return NAMES[nn].bit;
	}
	return -1;
}

//Parses one token into a pad index and a mask. Returns 0 on success.
//Tokens look like "a,start" (pad A), "3:b,up" (pad D), or "-" (no buttons).
static int parse_input_token(const char *tok, size_t len, int *pad_out, uint16_t *mask_out)
{
	int pad = 0;
	size_t at = 0;

	//A leading digit or letter selects the port: "2:" or "b:"
	if(len >= 2 && tok[1] == ':')
	{
		char c = tok[0];
		if(c >= '1' && c <= '4')
			pad = c - '1';
		else if(c >= 'a' && c <= 'd')
			pad = c - 'a';
		else if(c >= 'A' && c <= 'D')
			pad = c - 'A';
		else
			return -1;
		at = 2;
	}

	uint16_t mask = 0;
	if(len - at == 1 && tok[at] == '-')
	{
		*pad_out = pad;
		*mask_out = 0;
		return 0;
	}

	while(at < len)
	{
		const char *name = tok + at;
		size_t nlen = 0;
		while(at + nlen < len && tok[at + nlen] != ',')
			nlen++;
		if(nlen == 0)
			return -1;

		int bit = button_by_name(name, nlen);
		if(bit >= 0)
		{
			mask = (uint16_t)(mask | (1u << bit));
			at += nlen + 1;
			continue;
		}

		char tmp[32];
		if(nlen >= sizeof(tmp))
			return -1;
		memcpy(tmp, name, nlen);
		tmp[nlen] = '\0';

		char *e2 = NULL;
		long val = strtol(tmp, &e2, 0);
		if(e2 == tmp || *e2 != '\0')
			return -1;
		mask = (uint16_t)(mask | (uint16_t)val);
		at += nlen + 1;
	}

	*pad_out = pad;
	*mask_out = mask;
	return 0;
}

static int load_input_script(const char *path)
{
	FILE *ff = NULL;
	if(strcmp(path, "-") == 0)
		ff = stdin;
	else
		ff = fopen(path, "r");
	if(ff == NULL)
	{
		fprintf(stderr, "cannot open input script %s\n", path);
		return -1;
	}

	int cap = 64;
	in_script = (ScriptedInput*)malloc(sizeof(ScriptedInput) * (size_t)cap);
	in_script_len = 0;
	in_script_pos = 0;
	uint16_t pads[4] = { 0, 0, 0, 0 };

	char line[512];
	while(fgets(line, sizeof(line), ff) != NULL)
	{
		char *comment = strchr(line, '#');
		if(comment != NULL)
			*comment = '\0';

		char *pp = line;
		while(*pp == ' ' || *pp == '\t')
			pp++;
		if(*pp == '\0' || *pp == '\n' || *pp == '\r')
			continue;

		char *end = NULL;
		long frame = strtol(pp, &end, 10);
		if(end == pp)
		{
			fprintf(stderr, "input script %s: expected a frame number\n", path);
			if(ff != stdin) fclose(ff);
			return -1;
		}
		pp = end;

		//Apply every token on this line to the running pad state
		for(;;)
		{
			while(*pp == ' ' || *pp == '\t' || *pp == ',')
				pp++;
			if(*pp == '\0' || *pp == '\n' || *pp == '\r')
				break;

			char *tok = pp;
			while(*pp != '\0' && *pp != ' ' && *pp != '\t' &&
			      *pp != '\n' && *pp != '\r')
				pp++;
			size_t len = (size_t)(pp - tok);

			int pad = 0;
			uint16_t mask = 0;
			if(parse_input_token(tok, len, &pad, &mask) != 0)
			{
				fprintf(stderr, "input script %s: bad button group '%.*s'\n",
					path, (int)len, tok);
				if(ff != stdin) fclose(ff);
				return -1;
			}
			pads[pad] = mask;
		}

		if(in_script_len >= cap)
		{
			cap *= 2;
			in_script = (ScriptedInput*)realloc(in_script, sizeof(ScriptedInput) * (size_t)cap);
		}
		in_script[in_script_len].frame = frame;
		memcpy(in_script[in_script_len].pads, pads, sizeof(pads));
		in_script_len++;
	}
	if(ff != stdin)
		fclose(ff);

	//Keep them in time order; scripts are short and written by hand
	for(int aa = 1; aa < in_script_len; aa++)
	{
		ScriptedInput key = in_script[aa];
		int bb = aa - 1;
		while(bb >= 0 && in_script[bb].frame > key.frame)
		{
			in_script[bb + 1] = in_script[bb];
			bb--;
		}
		in_script[bb + 1] = key;
	}

	fprintf(stderr, "input script %s: %d event(s)\n", path, in_script_len);
	return 0;
}

//Milliseconds of emulated time, which is what _sc_getticks returns. The GUI front end keeps
//this in its timer; here the loop below owns it.
uint32_t EmulTimerTicks = 0;

//Framebuffer sizes by mode, matching sysc.cpp's sysc_fb_sizes
static const int fb_widths[]  = { 0, 640, 320 };
static const int fb_heights[] = { 0, 480, 240 };

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

//Writes a frontbuffer as a binary PPM. Returns 0 on success.
static int dumpfb_path(const char *path, uint16_t *fb, int mode)
{
	if(fb == NULL || mode <= 0 || mode > 2)
		return -1;

	int width = fb_widths[mode];
	int height = fb_heights[mode];

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

static int dumpfb(const char *prefix, long index, uint16_t *fb, int mode)
{
	char path[4096];
	snprintf(path, sizeof(path), "%s-%05ld.ppm", prefix, index);
	return dumpfb_path(path, fb, mode);
}

static uint32_t fbfnv(uint16_t *fb, int mode)
{
	if(fb == NULL || mode <= 0 || mode > 2)
		return 0;
	uint32_t hash = 2166136261u;
	const unsigned char *bb = (const unsigned char *)fb;
	size_t len = (size_t)fb_widths[mode] * (size_t)fb_heights[mode] * 2u;
	for(size_t ii = 0; ii < len; ii++)
	{
		hash ^= bb[ii];
		hash *= 16777619u;
	}
	return hash;
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

static const char *dbgstop_name(process_dbgstop_t why)
{
	switch(why)
	{
		case PROCESS_DBGSTOP_NONE:   return "none";
		case PROCESS_DBGSTOP_CTRLC:  return "interrupted";
		case PROCESS_DBGSTOP_SIGNAL: return "signal";
		case PROCESS_DBGSTOP_BKPT:   return "breakpoint";
		case PROCESS_DBGSTOP_ABT:    return "data-abort";
		case PROCESS_DBGSTOP_AC:     return "alignment-check";
		case PROCESS_DBGSTOP_PF:     return "prefetch-abort";
		case PROCESS_DBGSTOP_FATAL:  return "interpreter-failure";
		default:                     return "unknown";
	}
}

static int dbgstop_is_fault(process_dbgstop_t why)
{
	return why == PROCESS_DBGSTOP_ABT || why == PROCESS_DBGSTOP_AC ||
	       why == PROCESS_DBGSTOP_PF  || why == PROCESS_DBGSTOP_FATAL;
}

//--- GDB server image loading, same contract as the GUI front end -------------

static char LoadedImage[1024];

static int headless_load_program(const char *path, char *err, size_t errsz)
{
	char wanted[1024];
	snprintf(wanted, sizeof(wanted), "%s", (path != NULL) ? path : "");
	if(wanted[0] == '\0')
		snprintf(wanted, sizeof(wanted), "%s", LoadedImage);
	if(wanted[0] == '\0')
	{
		snprintf(err, errsz, "No image is loaded to run again");
		return -1;
	}

	const int len = (int)strlen(wanted);
	const int is_nne = (len > 4 && !strcmp(wanted + len - 4, ".nne"));

	process_reset();
	if(is_nne)
	{
		if(sysc_bootimage(wanted) != 0)
		{
			snprintf(err, errsz, "Cannot boot %s as an NNE image", wanted);
			return -1;
		}
	}
	else
	{
		int fd = open(wanted, O_RDWR);
		if(fd < 0)
		{
			snprintf(err, errsz, "Cannot open %s", wanted);
			return -1;
		}
		sysc_setdiskfd(fd);
	}

	snprintf(LoadedImage, sizeof(LoadedImage), "%s", wanted);
	rsp_after_reset();
	return 0;
}

static const char *headless_loaded_image(void)
{
	return LoadedImage;
}

#ifdef NEMUL_TIMING
//Per-frame accounting for the timing model: the running total is a cycle counter,
//so the useful figure is the difference across a frame.
static uint64_t model_prev_cycles = 0;
static uint64_t model_frame_us_sum = 0;
static uint64_t model_frame_us_max = 0;
static uint64_t model_frame_us_first = 0;
#endif

#ifdef NEMUL_TIMING
//Top bins of the model's per-PC accounting, so a hot loop can be read off and checked
//against the instruction stream.
static void dump_hist(void)
{
	uint32_t n = mem_model_hist_size();
	uint32_t top[16];
	for(uint32_t t = 0; t < 16; t++)
		top[t] = n;

	for(uint32_t i = 0; i < n; i++)
	{
		uint32_t pc; uint64_t cy, ct;
		mem_model_hist(i, &pc, &cy, &ct);
		if(!ct)
			continue;
		for(uint32_t t = 0; t < 16; t++)
		{
			uint64_t bcy = 0, bct = 0;
			uint32_t bpc = 0;
			if(top[t] != n)
				mem_model_hist(top[t], &bpc, &bcy, &bct);
			if(top[t] == n || cy > bcy)
			{
				for(uint32_t s = 15; s > t; s--)
					top[s] = top[s - 1];
				top[t] = i;
				break;
			}
		}
	}

	printf("model: hottest PCs (cycles, count, cycles per instruction)\n");
	for(uint32_t t = 0; t < 16 && top[t] != n; t++)
	{
		uint32_t pc; uint64_t cy, ct;
		mem_model_hist(top[t], &pc, &cy, &ct);
		printf("model:   %08X %12llu %10llu  %6.2f\n", pc,
			(unsigned long long)cy, (unsigned long long)ct,
			ct ? (double)cy / (double)ct : 0.0);
	}
}
#endif

int main(int argc, char **argv)
{
	//Line buffered: the guest's _sc_print writes straight to fd 1, and a fully buffered
	//stdout would let a FRAME line be emitted in the middle of the guest's text.
	setvbuf(stdout, NULL, _IOLBF, 0);

	const char *diskpath = NULL;
	const char *recspath = NULL;
	const char *fbprefix = NULL;
	uint32_t breakat = 0;
	int breaksteps = 24;
	const char *nnepath = NULL;
	const char *inputpath = NULL;
	const char *shotpath = NULL;
	const char *jsonpath = NULL;
	const char *hashpath = NULL;
	const char *symbolspath = NULL;
	long ticks = 30000; //Thirty seconds of emulated time
	long wantframes = -1; //Or a number of displayed frames
	long fbevery = 60;
	int dohash = 0;
	int gdbport = 0;
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
		else if(strcmp(argv[aa], "--frames") == 0 && aa + 1 < argc)
			wantframes = atol(argv[++aa]);
		else if(strcmp(argv[aa], "--bp") == 0 && aa + 1 < argc)
			breakat = (uint32_t)strtoul(argv[++aa], NULL, 16);
		else if(strcmp(argv[aa], "--step") == 0 && aa + 1 < argc)
			breaksteps = atoi(argv[++aa]);
		else if(strcmp(argv[aa], "--nne") == 0 && aa + 1 < argc)
			nnepath = argv[++aa];
		else if(strcmp(argv[aa], "--fb") == 0 && aa + 1 < argc)
			fbprefix = argv[++aa];
		else if(strcmp(argv[aa], "--fb-every") == 0 && aa + 1 < argc)
			fbevery = atol(argv[++aa]);
		else if(strcmp(argv[aa], "--shot") == 0 && aa + 1 < argc)
			shotpath = argv[++aa];
		else if(strcmp(argv[aa], "--json") == 0 && aa + 1 < argc)
			jsonpath = argv[++aa];
		else if(strcmp(argv[aa], "--symbols") == 0 && aa + 1 < argc)
			symbolspath = argv[++aa];
		else if(strcmp(argv[aa], "--input") == 0 && aa + 1 < argc)
			inputpath = argv[++aa];
		else if(strcmp(argv[aa], "--hash") == 0)
			dohash = 1;
		else if(strcmp(argv[aa], "--hash-file") == 0 && aa + 1 < argc)
			hashpath = argv[++aa];
		else if(strcmp(argv[aa], "--gdb") == 0)
		{
			gdbport = 14292;
			if(aa + 1 < argc && argv[aa + 1][0] >= '0' && argv[aa + 1][0] <= '9')
				gdbport = atoi(argv[++aa]);
		}
		else if(strncmp(argv[aa], "--gdb=", 6) == 0)
			gdbport = atoi(argv[aa] + 6);
		else if(strcmp(argv[aa], "--quiet") == 0)
			quiet = 1;
		else if(strcmp(argv[aa], "--trace") == 0)
			tracing = 1;
		else if(strcmp(argv[aa], "--trace-sysc") == 0)
			tracesysc = 1;
		else
		{
			fprintf(stderr,
				"usage: %s --disk <card image> [--records <record sector>] "
				"[--nne <image>] [--ticks <ms>] [--frames <n>] [--input <script>] "
				"[--fb <prefix>] [--fb-every <n>] [--shot <file.ppm>] [--hash] "
				"[--json <file>] [--hash-file <file>] [--symbols <elf>] [--gdb [port]] "
				"[--bp <hex addr>] [--step <n>] [--quiet] [--trace] [--trace-sysc]\n",
				argv[0]);
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
		snprintf(LoadedImage, sizeof(LoadedImage), "%s", diskpath);
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

	//Source-level names for a fault report, when the caller has an ELF to hand
	symbols_t *syms = NULL;
	if(symbolspath != NULL)
	{
		syms = symbols_load(symbolspath);
		if(syms == NULL)
			fprintf(stderr, "symbols: cannot read %s as an ELF file\n", symbolspath);
		else if(!quiet)
		{
			int nfunc = 0, nline = 0;
			symbols_stats(syms, &nfunc, &nline);
			printf("symbols: %d function(s), %d line(s) from %s\n", nfunc, nline, symbolspath);
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
		snprintf(LoadedImage, sizeof(LoadedImage), "%s", nnepath);
		if(!quiet)
			printf("nne: %s\n", nnepath);
	}

	if(gdbport > 0)
	{
		prefs.rsp_enabled = true;
		prefs.rsp_port = gdbport;
		rsp_set_image_hooks(headless_load_program, headless_loaded_image);
		rsp_init(&prefs);
		if(!quiet)
			printf("gdb: listening on port %d\n", gdbport);
	}

	//The console's vertical blank: sixty a second, as the GUI timer does it.
	//Popping the frontbuffer is what unblocks a process waiting on _sc_gfx_flip, so the rate
	//here is part of the emulation and not just how often the picture is looked at.
	uint16_t *fb = NULL;
	int fbmode = 0;
	long ms_till_vsync = 16;
	int vsync_logging = (getenv("NEMUL_LOG_VSYNC") != NULL) ? 1 : 0;
	long vsyncs = 0;
	long frames = 0;
	int stop_reason = 0; //0 limit, 1 fault, 2 breakpoint/other stop

	//Hashes can go to a file of their own: the guest is writing to stdout too, and its
	//line buffering is not ours to control, so mixing the two makes parsing fragile.
	FILE *hashfp = NULL;
	if(hashpath != NULL)
	{
		hashfp = fopen(hashpath, "w");
		if(hashfp == NULL)
		{
			fprintf(stderr, "cannot write %s\n", hashpath);
			return 2;
		}
	}

	if(breakat != 0)
		emul_set_break(breakat, breaksteps);

	if(inputpath != NULL && load_input_script(inputpath) != 0)
		return 1;

	for(long tick = 0; tick < ticks; tick++)
	{
		rsp_poll();

		//A debugger that attached stopped every process; that is not the run being over.
		//Stand still (emulated time included) until it resumes or goes away.
		if(all_stopped() && rsp_present())
		{
			struct timespec nap = { 0, 1000000 };
			nanosleep(&nap, NULL);
			tick--;
			continue;
		}

		prof_tick((long)EmulTimerTicks, quiet);
		process_step();
		EmulTimerTicks++;

		ms_till_vsync--;
		if(ms_till_vsync <= 0)
		{
			ms_till_vsync = 16;
			sysc_popfbptr(&fb, &fbmode);
			vsyncs++;

			//Scripted input lands on the frame it names, the way a real pad would.
			//Several lines for the same frame apply in order, then the pads are pushed once.
			if(in_script_pos < in_script_len && in_script[in_script_pos].frame <= frames)
			{
				while(in_script_pos < in_script_len && in_script[in_script_pos].frame <= frames)
				{
					if(!quiet)
						fprintf(stderr, "input: frame %ld pads %04X %04X %04X %04X\n",
							frames,
							(unsigned)in_script[in_script_pos].pads[0],
							(unsigned)in_script[in_script_pos].pads[1],
							(unsigned)in_script[in_script_pos].pads[2],
							(unsigned)in_script[in_script_pos].pads[3]);
					sysc_pushpads(in_script[in_script_pos].pads);
					in_script_pos++;
				}
			}

			if(vsync_logging && vsyncs <= 5)
				fprintf(stderr, "vsync %ld: buffer %p mode %d\n", vsyncs, (void*)fb, fbmode);

#ifdef NEMUL_TIMING
			//What the memory model says the CPU spent on this frame. This is the
			//number to compare against the 16.7ms a 60Hz frame allows: a game at
			//3ms here has plenty of room, one at 14ms does not.
			{
				uint64_t now = mem_model_cycles();
				model_frame_us_sum += mem_model_cycles_to_us(now - model_prev_cycles);
				if(frames > 0)
				{
					uint64_t this_us = mem_model_cycles_to_us(now - model_prev_cycles);
					if(this_us > model_frame_us_max)
						model_frame_us_max = this_us;
				}
				else
				{
					model_frame_us_first = mem_model_cycles_to_us(now);
				}
				model_prev_cycles = now;
			}
#endif

			if(hashfp != NULL)
				fprintf(hashfp, "FRAME %ld TICKS %ld MODE %d HASH %8.8X\n",
					frames, (long)EmulTimerTicks, fbmode, fbfnv(fb, fbmode));
			else if(dohash)
				printf("FRAME %ld TICKS %ld MODE %d HASH %8.8X\n",
					frames, (long)EmulTimerTicks, fbmode, fbfnv(fb, fbmode));

			if(fbprefix != NULL && fbevery > 0 && (frames % fbevery) == 0)
				dumpfb(fbprefix, frames, fb, fbmode);

			frames++;

			if(wantframes >= 0 && frames >= wantframes)
				break;
		}

		//A breakpoint hit or a debugger stop is not the end of the run while a client is
		//attached: stand still and let rsp_poll serve it until it resumes or leaves.
		if(all_stopped() && rsp_present())
		{
			emul_stop_requested = 0;
			continue;
		}
		if(all_stopped() || emul_stop_requested)
			break;
	}

	//The frame counter has already advanced past the one on screen
	long lastframe = (frames > 0) ? frames - 1 : 0;
	if(fbprefix != NULL && fbevery > 0 && (lastframe % fbevery) != 0)
		dumpfb(fbprefix, lastframe, fb, fbmode);
	if(shotpath != NULL)
		dumpfb_path(shotpath, fb, fbmode);

	if(hashfp != NULL)
		fclose(hashfp);

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

	//The GUI shows a crash box. A harness needs a line it can parse and a status it can test.
	int fault_pid = 0;
	process_dbgstop_t fault_why = PROCESS_DBGSTOP_NONE;
	uint32_t fault_pc = 0;
	int stopped_pid = 0;
	process_dbgstop_t stopped_why = PROCESS_DBGSTOP_NONE;
	uint32_t stopped_pc = 0;
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state == PROCESS_STATE_NONE)
			continue;
		if(process_table[pp].dbgstop == PROCESS_DBGSTOP_NONE)
			continue;
		if(dbgstop_is_fault(process_table[pp].dbgstop) && fault_pid == 0)
		{
			fault_pid = process_table[pp].pid;
			fault_why = process_table[pp].dbgstop;
			fault_pc = process_table[pp].regs[15];
		}
		else if(stopped_pid == 0)
		{
			stopped_pid = process_table[pp].pid;
			stopped_why = process_table[pp].dbgstop;
			stopped_pc = process_table[pp].regs[15];
		}
	}

	if(fault_pid != 0)
	{
		stop_reason = 1;
		const char *func = NULL;
		const char *file = NULL;
		int line = 0;
		if(syms != NULL)
			func = symbols_func(syms, fault_pc, NULL, NULL);
		if(syms != NULL)
			symbols_line(syms, fault_pc, &file, &line);
		const char *fname = (file != NULL && file[0] != '\0') ? file : NULL;
		printf("FAULT pid %d reason %s pc %8.8X%s%s%s%s\n",
			fault_pid, dbgstop_name(fault_why), fault_pc,
			func ? " in " : "", func ? func : "",
			fname ? " at " : "", fname ? fname : "");
		if(fname != NULL && line > 0)
			printf("FAULT-LINE %s:%d\n", fname, line);
	}
	else if(stopped_pid != 0 && breakat == 0)
	{
		stop_reason = 2;
		printf("STOP pid %d reason %s pc %8.8X\n",
			stopped_pid, dbgstop_name(stopped_why), stopped_pc);
	}

	symbols_free(syms);

	if(jsonpath != NULL)
	{
		FILE *jf = fopen(jsonpath, "w");
		if(jf == NULL)
			fprintf(stderr, "cannot write %s\n", jsonpath);
		else
		{
			fprintf(jf, "{\n");
			fprintf(jf, "  \"ticks\": %ld,\n", (long)EmulTimerTicks);
			fprintf(jf, "  \"frames\": %ld,\n", frames < 0 ? 0 : frames);
			fprintf(jf, "  \"vsyncs\": %ld,\n", vsyncs);
			fprintf(jf, "  \"stopped\": %s,\n", stop_reason == 0 ? "\"limit\"" : (stop_reason == 1 ? "\"fault\"" : "\"stop\""));
			if(fault_pid != 0)
				fprintf(jf, "  \"fault\": {\"pid\": %d, \"reason\": \"%s\", \"pc\": %u},\n",
					fault_pid, dbgstop_name(fault_why), (unsigned)fault_pc);
			else
				fprintf(jf, "  \"fault\": null,\n");
#ifdef NEMUL_TIMING
			fprintf(jf, "  \"model_cycles\": %llu,\n", (unsigned long long)mem_model_cycles());
			fprintf(jf, "  \"model_us\": %llu,\n", (unsigned long long)mem_model_cycles_to_us(mem_model_cycles()));
#endif
			fprintf(jf, "  \"image\": \"%s\"\n", LoadedImage);
			fprintf(jf, "}\n");
			fclose(jf);
		}
	}

	if(!quiet)
	{
		printf("stopped after %ld ticks, %ld frames\n", (long)EmulTimerTicks, frames < 0 ? 0 : frames);
#ifdef NEMUL_TIMING
		//What the memory model says this run would have taken on the real machine,
		//alongside the emulator's own instruction-count clock for comparison.
		{
			uint64_t total_us = mem_model_cycles_to_us(mem_model_cycles());
			printf("model: %llu cycles, %llu us of F1C time\n",
				(unsigned long long)mem_model_cycles(), (unsigned long long)total_us);
			if(frames > 0)
			{
				//The frame budget at 60Hz is 16667us. Anything under it means the
				//program is waiting for the display rather than holding it up.
				printf("model: %.0f us per frame average, %llu us worst, %.0f%% of a 60Hz frame\n",
					(double)total_us / (double)frames,
					(unsigned long long)model_frame_us_max,
					100.0 * (double)total_us / ((double)frames * 16667.0));

				//The breakdown, so the figure above can be checked rather than
				//believed: how many fills at what row-miss rate produced that time.
				mem_model_stats_t ms;
				mem_model_stats(&ms);
				printf("model: of issue, branches %llu in %llu taken, interlocks %llu\n",
					(unsigned long long)ms.branch_cycles,
					(unsigned long long)ms.taken_branches,
					(unsigned long long)ms.interlock_cycles);
				if(getenv("NEMUL_TIMING_HIST") != NULL)
					dump_hist();
				printf("model: issue %llu, fills %llu, stalls %llu of %llu cycles\n",
					(unsigned long long)ms.issue_cycles, (unsigned long long)ms.fill_cycles,
					(unsigned long long)ms.stall_cycles,
					(unsigned long long)mem_model_cycles());
				printf("model: %.2f cycles per instruction over %llu instructions\n",
					ms.instructions ? (double)mem_model_cycles() / (double)ms.instructions : 0.0,
					(unsigned long long)ms.instructions);
				printf("model: %llu fills (%llu row misses), %llu writebacks, "
				       "%llu I-misses, %llu D-read misses, %llu D-write misses\n",
					(unsigned long long)ms.fills, (unsigned long long)ms.row_misses,
					(unsigned long long)ms.writebacks, (unsigned long long)ms.icache_misses,
					(unsigned long long)ms.dcache_read_misses,
					(unsigned long long)ms.dcache_write_misses);
			}
		}
#endif
		for(int pp = 0; pp < PROCESS_MAX; pp++)
		{
			if(process_table[pp].state == PROCESS_STATE_NONE)
				continue;
			printf("  pid %d state %d pc %8.8X dbgstop %d\n",
				process_table[pp].pid, (int)process_table[pp].state,
				process_table[pp].regs[15], (int)process_table[pp].dbgstop);
		}
	}

	if(stop_reason == 1)
		return 3;
	if(stop_reason == 2)
		return 5;
	return 0;
}
