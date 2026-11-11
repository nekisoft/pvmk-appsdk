//membench.c
//Runs one of the shared benchmark workloads inside a real PVMK program, so nemul's
//timing model can be compared with the same code measured on the board.
//
//The workload is chosen at build time with -DWORKLOAD=<n>, and WORKLOAD=0 does
//nothing: the difference in modelled cycles between 0 and n is the cost of the
//workload, with the kernel boot common to both and therefore cancelled out.
//
//The frame is flipped before the work starts, because the board's benchmark runs
//with the display engine scanning 320x240 and the model charges bus contention for
//it. Without the flip the two would not be comparing the same machine state.

#include <sc.h>
#include <stdint.h>
#include <stdio.h>

#ifndef WORKLOAD
#define WORKLOAD 0
#endif

uint32_t work_screen_fill(volatile uint16_t *fb, uint32_t pixels, uint16_t colour);
uint32_t work_screen_gradient(volatile uint16_t *fb, uint32_t w, uint32_t h);
uint32_t work_scroll(volatile uint16_t *fb, uint32_t w, uint32_t h);
uint32_t work_mandelbrot(volatile uint16_t *fb, uint32_t w, uint32_t h, uint32_t maxit);
uint32_t work_int_math(uint32_t iters, uint32_t seed);
float work_float_math(uint32_t iters, float seed);
uint32_t work_fill_word(volatile uint32_t *fb, uint32_t words, uint32_t colour);
uint32_t work_call(uint32_t iters, uint32_t *sink);

//The same counts the board's work_suite uses, so the two runs do identical work.
#define WORK_FILL_REPS 20u
#define WORK_GRAD_REPS 5u
#define WORK_SCROLL_REPS 10u
#define WORK_INT_ITERS 2000000u
#define WORK_FLT_ITERS 200000u

static uint16_t fb[240][320];
static volatile uint32_t checksum;

int app_main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;

	//Put the frame on the display before doing anything: from here on the engine is
	//reading it out of DRAM every frame and the CPU shares the bus with it.
	_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, &fb[0][0]);

#if WORKLOAD == 1
	for(uint32_t i = 0; i < WORK_FILL_REPS; i++)
		checksum += work_screen_fill(&fb[0][0], 320u * 240u, (uint16_t)i);
#elif WORKLOAD == 2
	for(uint32_t i = 0; i < WORK_GRAD_REPS; i++)
		checksum ^= work_screen_gradient(&fb[0][0], 320u, 240u);
#elif WORKLOAD == 3
	for(uint32_t i = 0; i < WORK_SCROLL_REPS; i++)
		checksum += work_scroll(&fb[0][0], 320u, 240u);
#elif WORKLOAD == 4
	checksum = work_mandelbrot(&fb[0][0], 320u, 240u, 64u);
#elif WORKLOAD == 5
	checksum = work_int_math(WORK_INT_ITERS, 1u);
#elif WORKLOAD == 7
	//Same loop over a 64x64 patch, which fits in the data cache: instructions without
	//memory traffic, to tell an issue-model error apart from a memory-model error.
	for(uint32_t i = 0; i < 100u; i++)
		checksum ^= work_screen_gradient(&fb[0][0], 64u, 64u);
#elif WORKLOAD == 8
	for(uint32_t i = 0; i < 100u; i++)
		checksum += work_screen_fill(&fb[0][0], 64u * 64u, (uint16_t)i);
#elif WORKLOAD == 9
	//Word stores over the same 8KByte patch, against WORKLOAD 8's halfword stores.
	for(uint32_t i = 0; i < 100u; i++)
		checksum += work_fill_word((volatile uint32_t *)&fb[0][0], 64u * 64u / 2u, i);
#elif WORKLOAD == 10
	checksum = work_call(200000u, (uint32_t *)&fb[0][0]);
#elif WORKLOAD == 6
	{
		union { float f; uint32_t u; } cv;
		cv.f = work_float_math(WORK_FLT_ITERS, 1.0f);
		checksum = cv.u;
	}
#endif

	//The checksum is how the two sides prove they did the same work: the board's
	//benchmark records the same value, so matching them rules out the possibility
	//that a timing comparison is really comparing different amounts of work.
#if WORKLOAD != 0
	printf("CHECK %08X\n", (unsigned)checksum);
#endif

	//Keep flipping so the emulator has frames to run and the program looks like a
	//game that has finished its work for the frame.
	for(;;)
		_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, &fb[0][0]);
}

int main(int argc, const char **argv)
{
	return _sc_shim_main(app_main, argc, argv);
}
