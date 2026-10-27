//nemul.cpp
//Entry point for Neki32 emulator
//Bryan E. Topp <betopp@betopp.com> 2025
//
//Front end: one SDL2 window, the console picture in it, and Dear ImGui for the menu bar,
//the status line and the two dialogs. The emulation core (interp, process, sysc, rsp, trace,
//prof) knows nothing about any of this and did not change when the toolkit did.
//
//Why this and not wxWidgets: the front end used to be a wxFrame with a wxMenu, a wxPanel it
//blitted a wxImage onto, a wxTimer, a wxFileDialog and a wxConfig for the bindings. That is a
//large toolkit, it needs a native port per platform, and the static build of it in this SDK is
//a tarball that has to be configured and compiled before nemul can be built at all. SDL2 plus
//Dear ImGui is two small libraries: a window, a 2D renderer and an immediate-mode UI drawn by
//the renderer we already have. No OpenGL, no native widgets, no per-platform window code.

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//SDL's own main() wrapper is for platforms where a GUI program does not start at main;
//plain main() plus SDL_SetMainReady() keeps the link free of SDL2main everywhere
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "process.h"
#include "prefs.h"
#include "sysc.h"
#include "rsp.h"
#include "prof.h"
#include "ui_prefs.h"
#include "ui_debug.h"
#include "ui_files.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

//Milliseconds of emulated time, which is what _sc_getticks returns
uint32_t EmulTimerTicks = 0;

//Vertical blanks entered so far; the picture is presented on these, sixty a second
static uint32_t EmulVsyncs = 0;

//Open disk image, or -1 when the console has no game in it
static int EmulDisk = -1;

//Pad state being reported to the emulation
static uint16_t EmulPadState[PREFS_PAD_MAX];

//Set once the user has been told about a crash, so one fault is not a hundred popups
static int YelledAboutCrash = 0;

//Whether the simulation is stepping. Only the simulation thread sets RunMode.
static int RunMode = 0;
static int SimStoppedAtBreak = 0;
static int SimWasPaused = 0;
static int DbgOpened = 0;           //the debugger window has been given its defaults   //the machine came to rest on a breakpoint, worth saying so

//Whether the user has paused it. Read by both threads, hence the atomic.
static SDL_atomic_t Paused;

//Preferences, as loaded at startup and as edited in the window
static prefs_t EmulPrefs;

//Latest picture out of the emulation, copied at the vertical blank.
//Copied rather than held as a pointer because sysc_popfbptr() hands back a pointer into the
//displaying process's memory: if that process exits before the next frame is drawn, the old
//front end painted freed memory.
static uint16_t FrameBuf[640 * 480];
static int FrameModeShared = 0;
static unsigned FrameSeq = 0;

//The window's own copy of the picture, and the mode and sequence number the texture holds
static uint16_t FrameCopy[640 * 480];
static int FrameMode = 0;
static unsigned FrameSeqSeen = 0;

//SDL texture holding FrameBuf on the GPU (or in the software renderer)
static SDL_Texture *FrameTex = NULL;
static int FrameTexW = 0;
static int FrameTexH = 0;

//Status line at the bottom of the window. Both threads write it, so it lives under SimLock.
static char StatusText[1024] = "Neki32 Simulator - No Image Loaded";

//Crash popup, raised by the vertical blank when a process stops without a debugger attached.
//The simulation thread sets the pending flag and the text; the window turns it into a popup.
static SDL_atomic_t CrashPending;
static char CrashPendingText[1024];
static int CrashPopup = 0;
static char CrashText[1024];

//About popup
static int AboutPopup = 0;

//Commands from the window to the simulation: a reset, or a new card image to put in. One of
//each is plenty - a second press before the first is served means the same thing.
static int WantReset = 0;
static int WantInsert = 0;
static char WantPath[1024];

//What the machine is running, as a path the debugger connection can be told about and reload.
//Written on the simulation thread by whoever loads it: the window, or a client asking for a run.
static char LoadedImage[1024];

//------------------------------------------------------------------------------------------------
//Threads
//
//The simulation runs on its own thread and the window on the main one. What they share is small
//and listed above: the pads going one way, the picture, the status line and the crash box coming
//back, and a one-slot command channel for reset and card changes. Everything else - the process
//table, the interpreter, the emulated console's memory - belongs to the simulation thread alone,
//so this is a change to the front end only, not to the emulator.

static SDL_mutex *SimLock = NULL;
static SDL_Thread *SimThread = NULL;
static SDL_atomic_t SimQuit;

//Puts a message on the status line. Both threads do this.
static void status_set(const char *text)
{
	SDL_LockMutex(SimLock);
	snprintf(StatusText, sizeof(StatusText), "%s", text);
	SDL_UnlockMutex(SimLock);
}

//Copies the status line out for drawing, so the lock is not held while the window is drawn
static void status_get(char *out, size_t size)
{
	SDL_LockMutex(SimLock);
	snprintf(out, size, "%s", StatusText);
	SDL_UnlockMutex(SimLock);
}

//Asks the simulation to start over with whatever card is in it, and to do nothing else
static void sim_request_reset(void)
{
	SDL_LockMutex(SimLock);
	WantReset = 1;
	SDL_UnlockMutex(SimLock);
}

//Asks the simulation to put a card image in, or to take it out when given NULL
static void sim_request_insert(const char *path)
{
	SDL_LockMutex(SimLock);
	WantInsert = 1;
	snprintf(WantPath, sizeof(WantPath), "%s", path != NULL ? path : "");
	SDL_UnlockMutex(SimLock);
}

//Frame timing, reported at exit when NEMUL_LOG_TIMING is set. The emulation figure is written by
//the simulation thread and read after it has been joined, so it needs no lock.
static Uint64 StatEmulTicks = 0;
static Uint64 StatRenderTicks = 0;
static Uint64 StatFrames = 0;
static long StatSteps = 0;


//Scripted keypresses, for running the front end on a machine with no keyboard to press.
//A key goes down at a given moment of *emulated* time and comes back up a while later, which is
//what makes a scripted run repeatable: the window draws as fast as it likes while the console
//runs at its own speed, and only the console's clock decides when the button is pressed.
#define PRESS_MAX 32
typedef struct press_s
{
	SDL_Keycode key;
	long at_ms;	//Emulated milliseconds after reset when the key goes down
	long hold;	//Emulated milliseconds it stays down
	int down;	//Whether the window has sent the press and not the release
} press_t;
static press_t Presses[PRESS_MAX];
static int NPresses = 0;

//Keys fed to the interface itself at a given frame, so that menus and the debugger can be driven
//without a person in front of the window
#define UIKEY_MAX 16
typedef struct uikey_s
{
	SDL_Keycode key;
	long at;     //Frame number it goes down on
	long hold;   //Frames it stays down
	int down;    //Whether the press has been sent and not the release
} uikey_t;
static uikey_t UiKeys[UIKEY_MAX];
static int NUiKeys = 0;

//Emulated milliseconds reached so far, published by the simulation for the window to read:
//this is the clock the scripted keypresses run on
static SDL_atomic_t EmulMs;

//Framebuffer sizes by mode, matching sysc.cpp's sysc_fb_sizes
static const int fb_widths[]  = { 0, 640, 320 };
static const int fb_heights[] = { 0, 480, 240 };

//------------------------------------------------------------------------------------------------
//Emulation

static void dbg_publish(void);
static ui_debug_t Dbg;

//Enters the vertical blank: hands the newest frontbuffer to the display and reports the pads
static void emul_vsync(void)
{
	uint16_t *fb = NULL;
	int mode = 0;
	sysc_popfbptr(&fb, &mode);

	SDL_LockMutex(SimLock);

	//Copy while the pointer is known good, then let the caller carry on
	if(fb != NULL && mode > 0 && mode <= 2)
	{
		memcpy(FrameBuf, fb, (size_t)fb_widths[mode] * fb_heights[mode] * 2);
		FrameModeShared = mode;
	}
	else
	{
		FrameModeShared = 0;
	}
	FrameSeq++;

	//Input is sampled once a frame, as the hardware does
	uint16_t pads[PREFS_PAD_MAX];
	memcpy(pads, EmulPadState, sizeof(pads));

	SDL_UnlockMutex(SimLock);

	sysc_pushpads(pads);

	EmulVsyncs++;

	//Keep the debugger's view of the machine current while it is being watched
	if(Dbg.open)
		dbg_publish();

	//Check if there's a debug-stopped program with no debugger attached
	if(!rsp_present())
	{
		int nstopped = 0;
		process_t *pptr = NULL;
		for(int pp = 0; pp < PROCESS_MAX; pp++)
		{
			if(process_table[pp].dbgstop)
			{
				nstopped++;
				pptr = &(process_table[pp]);
			}
		}

		if(nstopped && !YelledAboutCrash)
		{
			static const char *reasons[PROCESS_DBGSTOP_MAX] = {0};
			reasons[PROCESS_DBGSTOP_CTRLC] = "Interrupted by User";
			reasons[PROCESS_DBGSTOP_SIGNAL] = "Signal Sent";
			reasons[PROCESS_DBGSTOP_BKPT] = "Breakpoint Hit";
			reasons[PROCESS_DBGSTOP_ABT] = "Data Abort";
			reasons[PROCESS_DBGSTOP_AC] = "Alignment Check";
			reasons[PROCESS_DBGSTOP_PF] = "Prefetch Abort";
			reasons[PROCESS_DBGSTOP_FATAL] = "Neki32 Interpreter Bug";

			char msg[1024];
			snprintf(msg, sizeof(msg), "PID %d stopped: %s at program location 0x%8.8X.",
				pptr->pid, reasons[pptr->dbgstop], (unsigned)pptr->regs[15]);
			status_set(msg);

			//The popup belongs to the window: hand it the text and let it raise the box
			SDL_LockMutex(SimLock);
			snprintf(CrashPendingText, sizeof(CrashPendingText),
				"The process with PID %d has stopped due to a crash.\n\n"
				"The cause is: %s, at program location 0x%8.8X.\n\n"
				"Attach a debugger or restart the simulation to continue.",
				pptr->pid, reasons[pptr->dbgstop], (unsigned)pptr->regs[15]);
			SDL_UnlockMutex(SimLock);
			SDL_AtomicSet(&CrashPending, 1);
			YelledAboutCrash = 1;
		}
		else if(YelledAboutCrash && (nstopped == 0))
		{
			//Simulation has been reset
			YelledAboutCrash = 0;
		}
	}
}

//Runs approximately one millisecond of emulated time
static void emul_tick(void)
{
	prof_tick((long)EmulTimerTicks, 0);
	process_step();
	EmulTimerTicks++;
	if(EmulTimerTicks * 60ull > EmulVsyncs * 1000ull)
		emul_vsync();
}

//Called from the debugger connection's thread when a client sets the machine going: a machine
//that the window paused to show a breakpoint has to start again for the client to make progress
static void emul_unpause(void)
{
	SDL_AtomicSet(&Paused, 0);
}

//Starts a fresh simulation with whatever disk is inserted. Simulation thread only, after the
//window has stopped holding anything it drew from the old one.
static void sim_reset(void)
{
	RunMode = 0;
	EmulTimerTicks = 0;
	EmulVsyncs = 0;
	YelledAboutCrash = 0;
	SDL_AtomicSet(&Paused, 0);
	SDL_AtomicSet(&CrashPending, 0);

	sysc_setdiskfd(EmulDisk);
	process_reset();

	RunMode = 1;
}

//Inserts an image, or ejects whatever is in there when given NULL. Simulation thread only.
static int sim_insert(const char *path)
{
	if(path == NULL)
	{
		if(EmulDisk >= 0)
		{
			close(EmulDisk);
			EmulDisk = -1;
		}
		status_set("Neki32 Simulator - No Image Loaded");
		LoadedImage[0] = '\0';
		sim_reset();
		return 0;
	}

	int newfd = open(path, O_RDWR);
	if(newfd < 0)
	{
		//Some cards are read-only media, and a run that only reads should not need a
		//writable image, so try again read-only before giving up
		newfd = open(path, O_RDONLY);
	}

	if(newfd < 0)
	{
		char msg[1024];
		snprintf(msg, sizeof(msg), "Cannot open %s: %s", path, strerror(errno));
		status_set(msg);
		return -1;
	}

	if(EmulDisk >= 0)
	{
		close(EmulDisk);
		EmulDisk = -1;
	}

	EmulDisk = newfd;
	snprintf(LoadedImage, sizeof(LoadedImage), "%s", path);
	char msg[1024];
	snprintf(msg, sizeof(msg), "Opened %s", path);
	status_set(msg);
	sim_reset();
	return 0;
}

//Serves whatever the window has asked for. Simulation thread only; the window keeps its hands
//off the emulator and puts requests in these slots instead.
static void sim_serve_requests(void)
{
	int insert = 0;
	int reset = 0;
	char path[1024];

	SDL_LockMutex(SimLock);
	insert = WantInsert;
	reset = WantReset;
	snprintf(path, sizeof(path), "%s", WantPath);
	WantInsert = 0;
	WantReset = 0;
	SDL_UnlockMutex(SimLock);

	if(insert)
		sim_insert(path[0] ? path : NULL);
	if(reset)
	{
		sim_reset();
		rsp_after_reset();
		status_set("Restarted simulation.");
	}
}

//Loads a program because a debugger connection asked for one: an image to boot, or an .nne to run
//on its own. Simulation thread only, from the poll between instructions, so the machine is not
//stepping while its process table is replaced. An empty name means "the same image again", which
//is what a client rebuilding between runs wants.
static int rsp_load_program(const char *path, char *err, size_t errsz)
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

	if(is_nne)
	{
		//A single program with the boot menu out of the way, which is how a game is tested on
		//its own. The card image stays in: a program that reads its data still needs one.
		sim_reset();
		if(sysc_bootimage(wanted) != 0)
		{
			snprintf(err, errsz, "Cannot boot %s as an NNE image", wanted);
			return -1;
		}
		snprintf(LoadedImage, sizeof(LoadedImage), "%s", wanted);
		rsp_after_reset();
		return 0;
	}

	if(sim_insert(wanted) != 0)
	{
		snprintf(err, errsz, "Cannot open %s", wanted);
		return -1;
	}

	rsp_after_reset();
	return 0;
}

//Says which image is loaded, for a client that asks what it is debugging
static const char *rsp_loaded_image(void)
{
	return LoadedImage;
}

//------------------------------------------------------------------------------------------------
//Debugger
//
//The window never touches the machine. The simulation thread fills in a snapshot of it, the
//window draws from a copy of that snapshot, and anything the user asks for comes back as a
//request carried out between instructions.

static ui_debug_state_t DbgShared;    //filled by the simulation thread
static ui_debug_state_t DbgShown;     //copied for the window to draw
static SDL_atomic_t DbgPid;           //process the window is following, 0 for any
static SDL_atomic_t DbgStep;          //instructions left to single-step
static SDL_atomic_t DbgBpAdd;
static SDL_atomic_t DbgBpDel;
static SDL_atomic_t DbgBpClear;

//Fills in the snapshot the window draws from. Simulation thread only: the machine is not to be
//read while it is stepping.
static void dbg_publish(void)
{
	ui_debug_state_t snap;
	memset(&snap, 0, sizeof(snap));
	snap.valid = true;
	snap.machine_paused = (SDL_AtomicGet(&Paused) != 0);
	snap.emul_ms = (uint32_t)EmulTimerTicks;

	//Every process, with the registers it would resume with
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		const process_t *proc = &(process_table[pp]);
		if(proc->state == PROCESS_STATE_NONE)
			continue;

		ui_debug_proc_t *out = &(snap.proc[snap.npids]);
		out->pid = proc->pid;
		out->state = (int)proc->state;
		out->dbgstop = (int)proc->dbgstop;
		out->paused = proc->paused;
		out->pc = proc->regs[15];
		out->cpsr = proc->cpsr;
		memcpy(out->regs, proc->regs, sizeof(out->regs));
		snap.npids++;
	}

	//Breakpoints and the last one the machine stopped at
	snap.bp_count = process_bp_count;
	memcpy(snap.bp, process_bp, sizeof(snap.bp));
	snap.bp_hit_pid = process_bp_hit_pid;
	snap.bp_hit_addr = process_bp_hit_addr;

	//The process on show: the one the window picked, or else the first that is alive
	int pid = SDL_AtomicGet(&DbgPid);
	const process_t *shown = process_find(pid);
	if(shown == NULL || shown->state == PROCESS_STATE_NONE)
	{
		for(int pp = 0; pp < PROCESS_MAX; pp++)
		{
			if(process_table[pp].state == PROCESS_STATE_ALIVE)
			{
				shown = &(process_table[pp]);
				break;
			}
		}
	}

	if(shown != NULL)
	{
		snap.code_pid = shown->pid;
		snap.trace_pid = shown->pid;

		//A window of instructions around the program counter, so that the disassembly can show
		//what comes next as well as where the program has been
		const uint32_t window = (uint32_t)(UI_DEBUG_CODE_MAX / 2) * 4;
		const uint32_t pc = shown->regs[15];
		snap.code_base = (pc > window) ? (pc - window) : 0;
		for(int ii = 0; ii < UI_DEBUG_CODE_MAX; ii++)
		{
			const uint32_t addr = snap.code_base + (uint32_t)ii * 4;
			if(addr / 4 < shown->size / 4)
				snap.code[ii] = shown->mem[addr / 4];
			else
				break;
			snap.code_count = ii + 1;
		}

		//What it ran lately, newest first
		for(int bb = 0; bb < UI_DEBUG_TRACE_MAX; bb++)
		{
			uint32_t hist_pc = 0, hist_ir = 0;
			if(process_hist(shown->pid, bb, &hist_pc, &hist_ir) < 0)
				break;
			snap.trace_pc[bb] = hist_pc;
			snap.trace_ir[bb] = hist_ir;
			snap.trace_count = bb + 1;
		}
	}

	if(getenv("NEMUL_LOG_DEBUG"))
	{
		fprintf(stderr, "dbg: paused %d pids %d shown %d pc %8.8x bp %d hit %8.8x trace %d\n",
			(int)snap.machine_paused, snap.npids, snap.code_pid,
			(unsigned)((snap.npids > 0) ? snap.proc[0].pc : 0),
			snap.bp_count, (unsigned)snap.bp_hit_addr, snap.trace_count);
	}

	SDL_LockMutex(SimLock);
	DbgShared = snap;
	SDL_UnlockMutex(SimLock);
}

//Carries out whatever the window asked for. Called on the simulation thread, between instructions.
static void sim_serve_debug(void)
{
	const int bp_add = SDL_AtomicGet(&DbgBpAdd);
	const int bp_del = SDL_AtomicGet(&DbgBpDel);
	const int bp_clear = SDL_AtomicGet(&DbgBpClear);
	SDL_AtomicSet(&DbgBpAdd, 0);
	SDL_AtomicSet(&DbgBpDel, 0);
	SDL_AtomicSet(&DbgBpClear, 0);

	if(bp_clear)
		process_bp_clear();
	if(bp_del)
		process_bp_del((uint32_t)bp_del);
	if(bp_add)
		process_bp_add((uint32_t)bp_add);

	const int want = SDL_AtomicGet(&DbgStep);
	if(want <= 0)
		return;
	SDL_AtomicSet(&DbgStep, 0);

	//Step the process on show, or whatever is runnable if it has gone
	int pid = SDL_AtomicGet(&DbgPid);
	for(int ss = 0; ss < want; ss++)
	{
		const process_t *proc = process_find(pid);
		if(proc == NULL || proc->state != PROCESS_STATE_ALIVE)
		{
			proc = NULL;
			for(int pp = 0; pp < PROCESS_MAX; pp++)
			{
				if(process_table[pp].state == PROCESS_STATE_ALIVE)
				{
					proc = &(process_table[pp]);
					break;
				}
			}
		}
		if(proc == NULL)
			break;

		pid = proc->pid;
		SDL_AtomicSet(&DbgPid, pid);
		process_run_insn(pid);

		//Stepping onto a breakpoint is how a step over a call stops inside it: hold there, and
		//clear the request so that the next resume is not refused at once
		if(emul_stop_requested)
		{
			emul_stop_requested = 0;
			break;
		}
	}

	dbg_publish();
}

//The simulation thread. Runs emulated time as fast as the host will allow, keeping it in step
//with the wall clock so that a game plays at the right speed; when it cannot keep up it drops
//the backlog rather than running further behind, which is what the old two-millisecond timer did.
static int sim_thread(void *arg)
{
	(void)arg;

	Uint64 freq = SDL_GetPerformanceFrequency();
	Uint64 last = SDL_GetPerformanceCounter();
	double owed_ms = 0.0;

	while(!SDL_AtomicGet(&SimQuit))
	{
		sim_serve_requests();

		//Breakpoints can be set or cleared at any time; while stopped, this is also where a
		//single-step is carried out
		sim_serve_debug();

		//Serve the debugger connection here rather than in the emulated tick, so that a client
		//is answered even when the machine is standing still
		rsp_poll();

		//Coming back from a stop at a breakpoint, the instruction under it has to be allowed to
		//run, or the machine would stop on the same address for ever
		{
			const int now_paused = (SDL_AtomicGet(&Paused) != 0);
			if(SimWasPaused && !now_paused && process_bp_hit_pid != 0)
				process_bp_skip_once(process_bp_hit_pid, process_bp_hit_addr);
			SimWasPaused = now_paused;
		}

		if(SDL_AtomicGet(&Paused) || !RunMode)
		{

			//Nothing to do: sleep rather than spin, and do not count the time spent asleep
			//as time the emulation owes
			SDL_Delay(4);
			last = SDL_GetPerformanceCounter();
			owed_ms = 0.0;
			continue;
		}

		Uint64 now = SDL_GetPerformanceCounter();
		double elapsed_ms = (double)(now - last) * 1000.0 / (double)freq;
		last = now;
		if(elapsed_ms > 250.0)
			elapsed_ms = 250.0;

		owed_ms += elapsed_ms;
		int steps = (int)owed_ms;
		owed_ms -= (double)steps;
		if(steps > 250)
		{
			steps = 250;
			owed_ms = 0.0;
		}

		if(steps <= 0)
		{
			//Caught up with the clock: let the host do something else for a moment
			SDL_Delay(1);
			continue;
		}

		Uint64 before = SDL_GetPerformanceCounter();
		for(int ss = 0; ss < steps; ss++)
		{
			//Serving the debugger once per emulated millisecond keeps a client's packets from
			//waiting for the whole batch, which is long enough for it to give up and resend
			rsp_poll();

			if(emul_stop_requested)
			{
				//A breakpoint the debugger set: the machine stands on the instruction and the
				//window takes over
				emul_stop_requested = 0;
				SDL_AtomicSet(&Paused, 1);
				SimStoppedAtBreak = 1;
				break;
			}
			emul_tick();
		}
		StatEmulTicks += SDL_GetPerformanceCounter() - before;
		StatSteps += steps;

		//Let the window know how far the console has got, for its scripted keypresses
		SDL_AtomicSet(&EmulMs, (int)EmulTimerTicks);

		if(SimStoppedAtBreak)
		{
			SimStoppedAtBreak = 0;
			dbg_publish();
			if(process_bp_hit_pid != 0)
			{
				char msg[128];
				snprintf(msg, sizeof(msg), "Stopped at breakpoint %8.8x (pid %d).",
					(unsigned)process_bp_hit_addr, process_bp_hit_pid);
				status_set(msg);
			}
		}
	}

	return 0;
}

//------------------------------------------------------------------------------------------------
//Input

//Whether a window has the keyboard, in which case keys are not pad input
static int ui_has_keyboard(const ui_prefs_t *prefs_ui, const ui_files_t *files_ui)
{
	if(prefs_ui->open || files_ui->open || CrashPopup || AboutPopup)
		return 1;
	if(ImGui::GetIO().WantTextInput)
		return 1;
	return 0;
}

//Applies a key to the emulated pads, if it is bound to anything
static void pad_key(int keycode, int down)
{
	if(getenv("NEMUL_LOG_INPUT"))
		fprintf(stderr, "pad_key: key %d down %d\n", keycode, down);

	for(int pp = 0; pp < PREFS_PAD_MAX; pp++)
	{
		for(int bb = 0; bb < PREFS_PAD_BTN_MAX; bb++)
		{
			if(EmulPrefs.pads[pp].btn_src[bb] != PREFS_PAD_SRC_KEY)
				continue;

			if(EmulPrefs.pads[pp].btn_val[bb] != keycode)
				continue;

			if(getenv("NEMUL_LOG_INPUT"))
				fprintf(stderr, "pad_key: pad %d button %d -> %d\n", pp, bb, down);

			if(down)
			{
				SDL_LockMutex(SimLock);
				EmulPadState[pp] |= (uint16_t)(1u << bb);
				SDL_UnlockMutex(SimLock);
			}
			else
			{
				SDL_LockMutex(SimLock);
				EmulPadState[pp] &= (uint16_t)~(1u << bb);
				SDL_UnlockMutex(SimLock);
			}
		}
	}
}

//------------------------------------------------------------------------------------------------
//Picture

//Pulls the newest picture out of the simulation and puts it in the texture, or throws the
//texture away when the mode changed size. Window thread only: it owns all the drawing.
static void frame_texture_sync(SDL_Renderer *renderer)
{
	int mode = 0;

	SDL_LockMutex(SimLock);
	if(FrameSeq != FrameSeqSeen)
	{
		mode = FrameModeShared;
		if(mode > 0 && mode <= 2)
			memcpy(FrameCopy, FrameBuf, (size_t)fb_widths[mode] * fb_heights[mode] * 2);
		FrameSeqSeen = FrameSeq;
	}
	SDL_UnlockMutex(SimLock);

	if(mode <= 0 || mode > 2)
	{
		//Nothing has been flipped yet, or the console gave the picture up
		FrameMode = 0;
		return;
	}

	int want_w = fb_widths[mode];
	int want_h = fb_heights[mode];

	if(FrameTex != NULL && (FrameTexW != want_w || FrameTexH != want_h))
	{
		SDL_DestroyTexture(FrameTex);
		FrameTex = NULL;
	}

	if(FrameTex == NULL)
	{
		//RGB565 straight from the emulation: the renderer converts it, so no per-pixel work
		//on the host at all - the old front end turned every pixel into three bytes first
		FrameTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, want_w, want_h);
		FrameTexW = want_w;
		FrameTexH = want_h;
		SDL_SetTextureScaleMode(FrameTex, SDL_ScaleModeNearest);
		SDL_SetTextureBlendMode(FrameTex, SDL_BLENDMODE_NONE);
	}

	if(FrameTex != NULL)
	{
		SDL_UpdateTexture(FrameTex, NULL, FrameCopy, want_w * 2);
		FrameMode = mode;
	}
}

//------------------------------------------------------------------------------------------------
//UI

//The console picture, fitted into the space the window has for it
static void draw_screen(ImDrawList *dl, ImVec2 origin, ImVec2 avail)
{
	//Black behind everything, so the letterboxing is part of the picture
	dl->AddRectFilled(origin, ImVec2(origin.x + avail.x, origin.y + avail.y), IM_COL32(0, 0, 0, 255));

	if(FrameMode == 0 || FrameTex == NULL)
	{
		//No signal: the console is running but nothing has flipped a buffer yet
		const char *msg = "No Signal";
		ImVec2 tsz = ImGui::CalcTextSize(msg);
		dl->AddText(ImVec2(origin.x + (avail.x - tsz.x) * 0.5f, origin.y + (avail.y - tsz.y) * 0.5f),
			IM_COL32(90, 90, 90, 255), msg);
		return;
	}

	//Four by three, as the console outputs, scaled to the smaller of the two directions
	float scale_x = avail.x / 640.0f;
	float scale_y = avail.y / 480.0f;
	float scale = (scale_x < scale_y) ? scale_x : scale_y;
	if(scale <= 0.0f)
		return;

	ImVec2 size(640.0f * scale, 480.0f * scale);
	ImVec2 pos(origin.x + (avail.x - size.x) * 0.5f, origin.y + (avail.y - size.y) * 0.5f);

	dl->AddImage((ImTextureID)(intptr_t)FrameTex, pos, ImVec2(pos.x + size.x, pos.y + size.y));
}

//One frame of the front end
static void draw_ui(int *want_open_files, int *want_open_prefs, int *want_quit)
{
	ImGuiViewport *vp = ImGui::GetMainViewport();

	//The host window covers the whole viewport and is not a window the user can move: what
	//moves and resizes is the OS window behind it
	ImGui::SetNextWindowPos(vp->WorkPos);
	ImGui::SetNextWindowSize(vp->WorkSize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("##nemul", NULL,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_MenuBar);
	ImGui::PopStyleVar(3);

	if(ImGui::BeginMenuBar())
	{
		if(ImGui::BeginMenu("File"))
		{
			if(ImGui::MenuItem("Open Image...", "Ctrl+O"))
				*want_open_files = 1;

			//No host card reader: say so rather than pretending the item does something
			if(ImGui::MenuItem("Open Card", NULL, false, false))
			{
			}
			if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("Reading a card from a host card reader is not implemented.\nUse File > Open Image on a card image instead.");

			if(ImGui::MenuItem("Open Menu (no card)", "Ctrl+M"))
				sim_request_insert(NULL);

			ImGui::Separator();

			if(ImGui::MenuItem("Restart", "Ctrl+R"))
				sim_request_reset();

			ImGui::Separator();

			if(ImGui::MenuItem("Exit", "Ctrl+Q"))
				*want_quit = 1;

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Edit"))
		{
			if(ImGui::MenuItem("Preferences...", "Ctrl+P"))
				*want_open_prefs = 1;

			ImGui::Separator();

			//A paused simulation is still stepped by nothing at all, which is what a debugger
			//wants: the RSP listener keeps polling while the targets stay put
			bool paused = (SDL_AtomicGet(&Paused) != 0);
			if(ImGui::MenuItem("Pause", "F5", &paused))
			{
				SDL_AtomicSet(&Paused, paused ? 1 : 0);
				status_set(paused ? "Paused." : "Running.");
			}

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Debug"))
		{
			bool open = Dbg.open;
			if(ImGui::MenuItem("Debugger...", "F7", &open))
				Dbg.open = open;

			bool paused = (SDL_AtomicGet(&Paused) != 0);
			if(!paused)
				ImGui::BeginDisabled();
			if(ImGui::MenuItem("Step One Instruction", "F10"))
				SDL_AtomicAdd(&DbgStep, 1);
			ImGui::EndDisabled();

			if(ImGui::MenuItem("Break at Program Counter"))
			{
				SDL_LockMutex(SimLock);
				const uint32_t pc = (DbgShown.npids > 0) ? DbgShown.proc[0].pc : 0;
				SDL_UnlockMutex(SimLock);
				if(pc != 0)
					SDL_AtomicSet(&DbgBpAdd, (int)pc);
			}

			ImGui::Separator();
			if(ImGui::MenuItem("Load Symbols from ELF..."))
				Dbg.open = true;

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Help"))
		{
			if(ImGui::MenuItem("About"))
				AboutPopup = 1;
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	//Picture on top, status line underneath
	ImVec2 avail = ImGui::GetContentRegionAvail();
	float status_h = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y * 2.0f;
	float screen_h = avail.y - status_h;
	if(screen_h < 0.0f)
		screen_h = 0.0f;

	ImVec2 origin = ImGui::GetCursorScreenPos();
	draw_screen(ImGui::GetWindowDrawList(), origin, ImVec2(avail.x, screen_h));

	char status[1024];
	status_get(status, sizeof(status));

	ImGui::SetCursorScreenPos(ImVec2(origin.x + 6.0f, origin.y + screen_h + ImGui::GetStyle().ItemSpacing.y));
	if(SDL_AtomicGet(&Paused))
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "PAUSED  %s", status);
	else
		ImGui::TextUnformatted(status);

	//A fault in the simulation arrives here as a pending message rather than a popup, because
	//the popup has to be opened by the thread that owns the ImGui context
	if(SDL_AtomicGet(&CrashPending))
	{
		SDL_LockMutex(SimLock);
		snprintf(CrashText, sizeof(CrashText), "%s", CrashPendingText);
		SDL_UnlockMutex(SimLock);
		SDL_AtomicSet(&CrashPending, 0);
		CrashPopup = 1;
	}

	//Dialogs live inside this window's ID stack so their popups do too
	if(CrashPopup)
	{
		ImGui::OpenPopup("Simulated Crash");
		CrashPopup = 0;
	}

	if(AboutPopup)
	{
		ImGui::OpenPopup("About");
		AboutPopup = 0;
	}

	if(ImGui::BeginPopupModal("Simulated Crash", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 34.0f);
		ImGui::TextUnformatted(CrashText);
		ImGui::PopTextWrapPos();
		ImGui::Spacing();

		if(ImGui::Button("Restart", ImVec2(120, 0)))
		{
			sim_request_reset();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if(ImGui::Button("Dismiss", ImVec2(120, 0)))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	if(ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Neki32 Game Console Simulator");
		ImGui::Spacing();
		ImGui::TextUnformatted(
			"This program performs instruction-level simulation of\n"
			"a Neki32 program in userland. It intercepts syscalls\n"
			"and executes them using devices from the host system.");
		ImGui::Spacing();
		ImGui::TextUnformatted("Copyright 2025 Nekisoft Pty Ltd (ACN 680 583 251)");
		ImGui::TextUnformatted("http://www.nekisoft.com.au");
		ImGui::Spacing();

		if(ImGui::Button("Close", ImVec2(120, 0)))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	ImGui::End();
}

//------------------------------------------------------------------------------------------------
//Screenshot, for running the front end where there is no display to look at

static int write_bmp(const char *path, const unsigned char *rgba, int width, int height)
{
	FILE *out = fopen(path, "wb");
	if(out == NULL)
		return -1;

	int row = width * 3;
	int pad = (4 - (row % 4)) % 4;
	int data = (row + pad) * height;
	unsigned char head[54] = {0};

	head[0] = 'B';
	head[1] = 'M';
	head[2] = (unsigned char)((54 + data) & 0xFF);
	head[3] = (unsigned char)(((54 + data) >> 8) & 0xFF);
	head[4] = (unsigned char)(((54 + data) >> 16) & 0xFF);
	head[5] = (unsigned char)(((54 + data) >> 24) & 0xFF);
	head[10] = 54;
	head[14] = 40;
	head[18] = (unsigned char)(width & 0xFF);
	head[19] = (unsigned char)((width >> 8) & 0xFF);
	head[20] = (unsigned char)((width >> 16) & 0xFF);
	head[21] = (unsigned char)((width >> 24) & 0xFF);
	head[22] = (unsigned char)(height & 0xFF);
	head[23] = (unsigned char)((height >> 8) & 0xFF);
	head[24] = (unsigned char)((height >> 16) & 0xFF);
	head[25] = (unsigned char)((height >> 24) & 0xFF);
	head[26] = 1;
	head[28] = 24;

	if(fwrite(head, 1, sizeof(head), out) != sizeof(head))
	{
		fclose(out);
		return -1;
	}

	//BMP rows run bottom to top, and pixels are stored BGR. The readback is ARGB8888, which on
	//a little-endian machine is the bytes B, G, R, A in memory - the same order BMP wants.
	unsigned char line[8192];
	for(int yy = height - 1; yy >= 0; yy--)
	{
		int len = 0;
		for(int xx = 0; xx < width; xx++)
		{
			const unsigned char *px = rgba + ((yy * width) + xx) * 4;
			line[len++] = px[0];
			line[len++] = px[1];
			line[len++] = px[2];
		}
		for(int pp = 0; pp < pad; pp++)
			line[len++] = 0;

		if(fwrite(line, 1, (size_t)len, out) != (size_t)len)
		{
			fclose(out);
			return -1;
		}
	}

	fclose(out);
	return 0;
}

//Reads the window back and writes it out
static int screenshot(SDL_Renderer *renderer, SDL_Window *window, const char *path)
{
	int width = 0;
	int height = 0;
	if(SDL_GetRendererOutputSize(renderer, &width, &height) != 0)
		return -1;
	if(width <= 0 || height <= 0 || width > 8192)
		return -1;

	size_t sz = (size_t)width * (size_t)height * 4;
	unsigned char *rgba = (unsigned char *)malloc(sz);
	if(rgba == NULL)
		return -1;

	int result = -1;
	if(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, rgba, width * 4) == 0)
		result = write_bmp(path, rgba, width, height);

	free(rgba);
	(void)window;
	return result;
}

//------------------------------------------------------------------------------------------------
//Startup

int main(int argc, char **argv)
{
	const char *image = NULL;
	const char *shot = NULL;
	const char *open_ui = NULL;
	const char *symbols_path = NULL;
	const char *step_text = NULL;
	int open_debugger = 0;
	uint32_t break_at = 0;
	long frames = -1;
	int width = 960;
	int height = 760;

	for(int aa = 1; aa < argc; aa++)
	{
		if(strcmp(argv[aa], "--image") == 0 && aa + 1 < argc)
			image = argv[++aa];
		else if(strcmp(argv[aa], "--frames") == 0 && aa + 1 < argc)
			frames = atol(argv[++aa]);
		else if(strcmp(argv[aa], "--shot") == 0 && aa + 1 < argc)
			shot = argv[++aa];
		else if(strcmp(argv[aa], "--ui") == 0 && aa + 1 < argc)
			open_ui = argv[++aa];
		else if(strcmp(argv[aa], "--ui-key") == 0 && aa + 1 < argc)
		{
			//--ui-key <key name>:<frame>[:<frames held>], repeatable
			char spec[128];
			snprintf(spec, sizeof(spec), "%s", argv[++aa]);
			char *colon = strchr(spec, ':');
			if(colon == NULL || NUiKeys >= UIKEY_MAX)
			{
				fprintf(stderr, "--ui-key wants <key name>:<frame>[:<frames held>]\n");
				return 2;
			}
			*colon = 0;
			SDL_Keycode key = SDL_GetKeyFromName(spec);
			if(key == SDLK_UNKNOWN)
			{
				fprintf(stderr, "--ui-key: no key called %s\n", spec);
				return 2;
			}
			UiKeys[NUiKeys].key = key;
			UiKeys[NUiKeys].at = atol(colon + 1);
			UiKeys[NUiKeys].hold = 1;
			char *held = strchr(colon + 1, ':');
			if(held != NULL)
				UiKeys[NUiKeys].hold = atol(held + 1);
			NUiKeys++;
		}
		else if(strcmp(argv[aa], "--debug") == 0)
			open_debugger = 1;
		else if(strcmp(argv[aa], "--symbols") == 0 && aa + 1 < argc)
		{
			open_debugger = 1;
			symbols_path = argv[++aa];
		}
		else if(strcmp(argv[aa], "--break") == 0 && aa + 1 < argc)
		{
			open_debugger = 1;
			break_at = (uint32_t)strtoul(argv[++aa], NULL, 16);
		}
		else if(strcmp(argv[aa], "--step") == 0 && aa + 1 < argc)
		{
			open_debugger = 1;
			step_text = argv[++aa];
		}
		else if(strcmp(argv[aa], "--size") == 0 && aa + 1 < argc)
			sscanf(argv[++aa], "%dx%d", &width, &height);
		else if(strcmp(argv[aa], "--press") == 0 && aa + 1 < argc)
		{
			//--press <key name>:<emulated ms>[:<emulated ms held>], repeatable
			char spec[128];
			snprintf(spec, sizeof(spec), "%s", argv[++aa]);
			char *colon = strchr(spec, ':');
			if(colon == NULL || NPresses >= PRESS_MAX)
			{
				fprintf(stderr, "--press wants <key name>:<emulated ms>[:<ms held>]\n");
				return 2;
			}
			*colon = '\0';

			SDL_Keycode key = SDL_GetKeyFromName(spec);
			if(key == SDLK_UNKNOWN)
			{
				fprintf(stderr, "--press: no key called %s\n", spec);
				return 2;
			}

			Presses[NPresses].key = key;
			Presses[NPresses].at_ms = atol(colon + 1);
			Presses[NPresses].hold = 100;
			Presses[NPresses].down = 0;
			char *hold = strchr(colon + 1, ':');
			if(hold != NULL)
				Presses[NPresses].hold = atol(hold + 1);
			NPresses++;
		}
		else if(argv[aa][0] != '-')
		{
			//A plain file name is the card image to run, which is how a tool in the SDK gets
			//used: pvmk-nemul game.iso
			image = argv[aa];
		}
		else
		{
			fprintf(stderr,
				"usage: %s [<card image>] [--image <iso>] [--frames <n>] [--shot <file.bmp>] "
				"[--ui prefs|crash|files]"
				" [--press <key>:<emulated ms>[:<ms held>]] [--size <w>x<h>]\n"
				"       [--debug] [--symbols <elf>] [--break <hex address>] [--step <instructions>]\n"
			"       [--ui-key <key>:<frame>[:<frames held>]]\n",
				argv[0]);
			return 2;
		}
	}

	SDL_SetMainReady();
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow("Neki32 Simulator",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if(window == NULL)
	{
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	//A renderer that can do accelerated work if the machine has it, and the software one if
	//not: the UI is a menu bar, some text and one streaming texture, so either is plenty
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(renderer == NULL)
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if(renderer == NULL)
	{
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	//Dear ImGui, with the SDL2 platform backend and the SDL_Renderer backend, so the window
	//system and the drawing both come from SDL2 and nothing else
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = NULL; //Window layout is fixed, so there is nothing worth saving
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();

	//ProggyClean at a size that reads on a high-density display, still built into the library
	ImFontConfig fontcfg;
	fontcfg.SizePixels = 16.0f;
	io.Fonts->AddFontDefault(&fontcfg);

	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);

	//Preferences, process table and debugger listener
	prefs_read(&EmulPrefs);
	rsp_init(&EmulPrefs);
	rsp_set_continue_hook(emul_unpause);
	rsp_set_image_hooks(rsp_load_program, rsp_loaded_image);
	process_reset();

	//The lock the window and the simulation share, and the flags they exchange
	SimLock = SDL_CreateMutex();
	if(SimLock == NULL)
	{
		fprintf(stderr, "cannot create mutex: %s\n", SDL_GetError());
		return 1;
	}
	SDL_AtomicSet(&SimQuit, 0);
	SDL_AtomicSet(&Paused, 0);
	SDL_AtomicSet(&CrashPending, 0);

	ui_prefs_t prefs_ui;
	memset(&prefs_ui, 0, sizeof(prefs_ui));
	prefs_ui.bind_pad = -1;
	prefs_ui.bind_btn = -1;

	ui_files_t files_ui;
	memset(&files_ui, 0, sizeof(files_ui));

	if(open_ui != NULL && strcmp(open_ui, "prefs") == 0)
		ui_prefs_open(&prefs_ui, &EmulPrefs);
	else if(open_ui != NULL && strcmp(open_ui, "files") == 0)
		ui_files_open(&files_ui, ".", true);
	else if(open_ui != NULL && strcmp(open_ui, "crash") == 0)
	{
		//Shows the box a fault raises, without having to produce a fault
		snprintf(CrashText, sizeof(CrashText),
			"The process with PID %d has stopped due to a crash.\n\n"
			"The cause is: %s, at program location 0x%8.8X.\n\n"
			"Attach a debugger or restart the simulation to continue.",
			1, "Data Abort", 0x00001234u);
		CrashPopup = 1;
	}

	if(image != NULL)
		sim_insert(image);
	else
		sim_insert(NULL);

	//The simulation gets a thread of its own; this one draws. Two cores where there are two,
	//which is the difference between a window that answers and one that is redrawn four times
	//a second because the interpreter is using its time.
	//A debugger asked for on the command line: open it, load what it was given, and start held
	//where it was told to
	if(open_debugger)
	{
		Dbg.open = true;
		if(symbols_path != NULL)
			ui_debug_load_symbols(&Dbg, symbols_path);
		if(break_at != 0)
			SDL_AtomicSet(&DbgBpAdd, (int)break_at);
		if(step_text != NULL)
		{
			SDL_AtomicSet(&Paused, 1);
			SDL_AtomicSet(&DbgStep, atoi(step_text));
		}
	}

	SimThread = SDL_CreateThread(sim_thread, "nemul-sim", NULL);
	if(SimThread == NULL)
	{
		fprintf(stderr, "cannot create simulation thread: %s\n", SDL_GetError());
		return 1;
	}

	//Main loop: draw at about sixty frames a second, from whatever the simulation has published
	Uint64 freq = SDL_GetPerformanceFrequency();
	Uint64 WallStart = SDL_GetPerformanceCounter();
	int running = 1;
	long framecount = 0;

	while(running)
	{
		Uint64 framestart = SDL_GetPerformanceCounter();

		//Scripted keypresses go in as ordinary SDL events, so what is being tested is the
		//event path the keyboard uses and not a shortcut around it. Their timing is emulated
		//time, so a run takes the same shape whether the host is fast or slow.
		long emulms = SDL_AtomicGet(&EmulMs);
		for(int pp = 0; pp < NPresses; pp++)
		{
			int want = (emulms >= Presses[pp].at_ms && emulms < Presses[pp].at_ms + Presses[pp].hold);
			if(want == Presses[pp].down)
				continue;
			Presses[pp].down = want;

			SDL_Event scripted;
			memset(&scripted, 0, sizeof(scripted));
			scripted.type = want ? SDL_KEYDOWN : SDL_KEYUP;
			scripted.key.type = scripted.type;
			scripted.key.state = want ? SDL_PRESSED : SDL_RELEASED;
			scripted.key.keysym.sym = Presses[pp].key;
			scripted.key.keysym.scancode = SDL_GetScancodeFromKey(Presses[pp].key);
			SDL_PushEvent(&scripted);
		}

		//Keys the test harness asked for, so that what the window does can be checked without
		//anybody clicking on it
		for(int kk = 0; kk < NUiKeys; kk++)
		{
			if(UiKeys[kk].at >= 0 && !UiKeys[kk].down && framecount >= UiKeys[kk].at)
			{
				SDL_Event press;
				memset(&press, 0, sizeof(press));
				press.type = SDL_KEYDOWN;
				press.key.state = SDL_PRESSED;
				press.key.keysym.sym = UiKeys[kk].key;
				press.key.keysym.scancode = SDL_GetScancodeFromKey(UiKeys[kk].key);
				SDL_PushEvent(&press);
				UiKeys[kk].down = 1;
			}
			else if(UiKeys[kk].down && framecount >= UiKeys[kk].at + UiKeys[kk].hold)
			{
				SDL_Event release;
				memset(&release, 0, sizeof(release));
				release.type = SDL_KEYUP;
				release.key.state = SDL_RELEASED;
				release.key.keysym.sym = UiKeys[kk].key;
				release.key.keysym.scancode = SDL_GetScancodeFromKey(UiKeys[kk].key);
				SDL_PushEvent(&release);
				UiKeys[kk].down = 0;
				UiKeys[kk].at = -1;
			}
		}

		//Events
		int any_event = 0;
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			any_event = 1;
			ImGui_ImplSDL2_ProcessEvent(&event);

			switch(event.type)
			{
				case SDL_QUIT:
				{
					running = 0;
					break;
				}

				case SDL_KEYDOWN:
				case SDL_KEYUP:
				{
					int key = (int)event.key.keysym.sym;
					int down = (event.type == SDL_KEYDOWN);

					//A pending binding in the preferences window eats the keypress
					if(down && ui_prefs_take_key(&prefs_ui, key))
						break;

					if(down && (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI)))
					{
						switch(key)
						{
							case SDLK_o: ui_files_open(&files_ui, files_ui.dir[0] ? files_ui.dir : ".", true); break;
							case SDLK_p: ui_prefs_open(&prefs_ui, &EmulPrefs); break;
							case SDLK_r: sim_request_reset(); break;
							case SDLK_m: sim_request_insert(NULL); break;
							case SDLK_q: running = 0; break;
							default: break;
						}
						break;
					}

					if(down && key == SDLK_F5)
					{
						int paused = (SDL_AtomicGet(&Paused) == 0);
						SDL_AtomicSet(&Paused, paused);
						status_set(paused ? "Paused." : "Running.");
						break;
					}

					if(down && key == SDLK_F7)
					{
						Dbg.open = !Dbg.open;
						break;
					}

					if(down && key == SDLK_F10 && SDL_AtomicGet(&Paused))
					{
						//One instruction, which is the shortcut a person reaches for when stopped
						SDL_AtomicAdd(&DbgStep, 1);
						status_set("Stepping one instruction.");
						break;
					}

					if(down && key == SDLK_F11)
					{
						Uint32 flags = SDL_GetWindowFlags(window);
						SDL_SetWindowFullscreen(window, (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
						break;
					}

					if(ui_has_keyboard(&prefs_ui, &files_ui))
						break;

					pad_key(key, down);
					break;
				}

				case SDL_DROPFILE:
				{
					//Dragging a card image onto the window inserts it
					sim_request_insert(event.drop.file);
					SDL_free(event.drop.file);
					break;
				}

				default:
					break;
			}
		}

		//Nothing here steps the simulation: it has its own thread and its own clock. This
		//thread only takes the newest picture and draws it.
		//
		//And it only draws when there is something new to show: a fresh picture, an input
		//event, or a screenshot to take. Redrawing an unchanged picture sixty times a second
		//keeps a core warm and takes memory bandwidth away from the simulation, and this
		//emulator's picture often changes only a handful of times a second.
		int have_new_frame = 0;
		SDL_LockMutex(SimLock);
		have_new_frame = (FrameSeq != FrameSeqSeen);
		SDL_UnlockMutex(SimLock);

		int redraw = have_new_frame || any_event || (shot != NULL);
		if(redraw)
		{
		frame_texture_sync(renderer);

		Uint64 renderstart = SDL_GetPerformanceCounter();

		//Frame
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		int want_open_files = 0;
		int want_open_prefs = 0;
		int want_quit = 0;

		draw_ui(&want_open_files, &want_open_prefs, &want_quit);

		//The dialogs are separate windows, drawn after the host so they sit on top of it
		ui_files_draw(&files_ui);
		ui_prefs_draw(&prefs_ui);

		//The debugger draws from a copy taken under the lock, so that the simulation thread can
		//keep filling in its own copy while the frame is being built
		if(Dbg.open)
		{
			//However the window came to be open, it starts out following the program counter
			if(!DbgOpened)
			{
				ui_debug_open(&Dbg);
				DbgOpened = 1;
			}

			SDL_LockMutex(SimLock);
			DbgShown = DbgShared;
			SDL_UnlockMutex(SimLock);

			if(!DbgShown.valid)
				DbgShown.machine_paused = (SDL_AtomicGet(&Paused) != 0);

			ui_debug_draw(&Dbg, &DbgShown);

			if(Dbg.req.pause >= 0)
			{
				SDL_AtomicSet(&Paused, Dbg.req.pause);
				status_set(Dbg.req.pause ? "Paused." : "Running.");
			}
			if(Dbg.req.step > 0)
				SDL_AtomicAdd(&DbgStep, Dbg.req.step);
			if(Dbg.req.bp_add != 0)
				SDL_AtomicSet(&DbgBpAdd, Dbg.req.bp_add);
			if(Dbg.req.bp_del != 0)
				SDL_AtomicSet(&DbgBpDel, Dbg.req.bp_del);
			if(Dbg.req.bp_clear)
				SDL_AtomicSet(&DbgBpClear, 1);
		}

		ImGui::Render();

		//A file was chosen: put it in and start over
		if(files_ui.done)
		{
			files_ui.done = false;
			sim_request_insert(files_ui.result);
		}

		//Preferences were accepted: keep them, save them, and forget any held buttons
		if(prefs_ui.apply)
		{
			prefs_ui.apply = false;
			EmulPrefs = prefs_ui.working;
			prefs_write(&EmulPrefs);
			SDL_LockMutex(SimLock);
			memset(EmulPadState, 0, sizeof(EmulPadState));
			SDL_UnlockMutex(SimLock);
			prefs_ui.saved_frames = 90;
			status_set("Saved control bindings.");
		}

		if(want_open_files)
			ui_files_open(&files_ui, files_ui.dir[0] ? files_ui.dir : ".", true);
		if(want_open_prefs)
			ui_prefs_open(&prefs_ui, &EmulPrefs);
		if(want_quit)
			running = 0;

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

		//A screenshot is read back after the frame is submitted but before the buffer swap,
		//which is the only order that works with a single-buffered software renderer
		if(shot != NULL && frames > 0 && framecount + 1 >= frames)
		{
			if(screenshot(renderer, window, shot) != 0)
				fprintf(stderr, "cannot write screenshot to %s\n", shot);
		}

		SDL_RenderPresent(renderer);
		StatRenderTicks += SDL_GetPerformanceCounter() - renderstart;
		StatFrames++;

		framecount++;
		if(frames > 0 && framecount >= frames)
			running = 0;
		}

		if(running)
		{
			//Sixty frames a second at most, and no delay if the host cannot keep up
			Uint64 spent = SDL_GetPerformanceCounter() - framestart;
			double spent_ms = (double)spent * 1000.0 / (double)freq;
			if(spent_ms < 16.0)
				SDL_Delay((Uint32)(16.0 - spent_ms));
		}
	}

	//Stop the simulation before anything it touches goes away, so that no step is running
	//while the process table, the disk fd or the debugger listener are torn down
	SDL_AtomicSet(&SimQuit, 1);
	if(SimThread != NULL)
	{
		int status = 0;
		SDL_WaitThread(SimThread, &status);
		SimThread = NULL;
	}

	prof_dump();

	//Where the time went, for when the window feels slow: is the emulation eating the frame, or
	//is the drawing itself slow? NEMUL_LOG_TIMING=1
	//The two figures are CPU time on separate threads, so they are compared against the wall
	//clock rather than against each other - they can add up to more than it.
	if(getenv("NEMUL_LOG_TIMING"))
	{
		double wall_s = (double)(SDL_GetPerformanceCounter() - WallStart) / (double)freq;
		double emul_s = (double)StatEmulTicks / (double)freq;
		double rend_s = (double)StatRenderTicks / (double)freq;
		fprintf(stderr,
			"timing: %llu frames in %.3f s of wall clock\n"
			"timing: emulation %.3f s of cpu (%.0f%% of a core), drawing %.3f s (%.0f%% of a core)\n"
			"timing: %ld steps, %.3f ms per emulated ms (%.0f emulated ms per host second)\n",
			(unsigned long long)StatFrames, wall_s,
			emul_s, wall_s > 0.0 ? 100.0 * emul_s / wall_s : 0.0,
			rend_s, wall_s > 0.0 ? 100.0 * rend_s / wall_s : 0.0,
			StatSteps,
			StatSteps > 0 ? 1000.0 * emul_s / (double)StatSteps : 0.0,
			emul_s > 0.0 ? (double)StatSteps / emul_s : 0.0);
	}

	if(FrameTex != NULL)
		SDL_DestroyTexture(FrameTex);

	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	if(EmulDisk >= 0)
		close(EmulDisk);

	return 0;
}
