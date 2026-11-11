//bench_work.c
//Workloads that behave like the things a game actually does, used to check the
//timing model against the real chip.
//
//This file is compiled twice: into the bare-metal benchmark image that runs on the
//board over JTAG (pvmk kernel/ports/f1c/bench), and into a PVMK application that
//runs under nemul. The two builds must contain the same instructions, which is
//checked by disassembling both and comparing, so that a difference in measured time
//is the model's error and not the compiler's.
//
//No libc, no floating point except where the workload is explicitly about it, and
//every function returns a checksum of the work it did so the two sides can be shown
//to have done identical work rather than merely similar amounts.

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;

//Fill the screen with halfword stores, one per pixel: what a game does when it
//clears the screen or draws a solid background. Consecutive halfwords pack into
//the same cache line, so the line is allocated once and written completely, which
//is the cheap path.
uint32_t work_screen_fill(volatile uint16_t *fb, uint32_t pixels, uint16_t colour)
{
	for(uint32_t i = 0; i < pixels; i++)
		fb[i] = colour;
	return pixels;
}

//The same fill with 32-bit stores, four pixels at a time. Compared with
//work_screen_fill over the same number of bytes it separates the cost of a halfword
//store from the cost of the loop: the ARM9's halfword store is not the same
//instruction in the pipeline as a word store, and the model has to know by how much.
uint32_t work_fill_word(volatile uint32_t *fb, uint32_t words, uint32_t colour)
{
	for(uint32_t i = 0; i < words; i++)
		fb[i] = colour;
	return words;
}

//The same screen, but every pixel's colour has to be computed: an LCG step and a
//couple of mixes per pixel. Same memory traffic, far more arithmetic, so the
//difference between this and work_screen_fill is the cost of the arithmetic.
uint32_t work_screen_gradient(volatile uint16_t *fb, uint32_t w, uint32_t h)
{
	uint32_t c = 0x12345678u;
	for(uint32_t y = 0; y < h; y++)
	{
		for(uint32_t x = 0; x < w; x++)
		{
			c = c * 1103515245u + 12345u;
			fb[y * w + x] = (uint16_t)((c >> 16) ^ (x + y));
		}
	}
	return c;
}

//Scrolling the screen up one line: read from one line and write to the one before
//it. Sequential in both directions but offset by a line, which is the access
//pattern where a write-allocate cache and the row buffer both get exercised.
uint32_t work_scroll(volatile uint16_t *fb, uint32_t w, uint32_t h)
{
	uint32_t sum = 0;
	for(uint32_t i = 0; i < w * (h - 1u); i++)
	{
		uint16_t v = fb[i + w];
		fb[i] = v;
		sum += v;
	}
	return sum;
}

//Fixed-point Mandelbrot, Q12, escaping at |z| > 1. Everything is 32-bit integer
//arithmetic because the chip has no FPU and neither do the games. Steps are
//precomputed so there is no division in the inner loop (the ARM926 has no divide
//instruction either; a divide there is a library call and would dominate).
//Returns the total iteration count, which is a strong check that both sides did
//exactly the same work.
uint32_t work_mandelbrot(volatile uint16_t *fb, uint32_t w, uint32_t h, uint32_t maxit)
{
	//x from -2.0 to 0.5, y from -1.0 to 1.0, in Q12.
	const int32_t x0 = -8192, x1 = 2048, y0 = -4096, y1 = 4096;
	int32_t dcx = (x1 - x0) / (int32_t)w;
	int32_t dcy = (y1 - y0) / (int32_t)h;
	uint32_t total = 0;

	for(uint32_t py = 0; py < h; py++)
	{
		int32_t ci = y0 + (int32_t)py * dcy;
		for(uint32_t px = 0; px < w; px++)
		{
			int32_t cr = x0 + (int32_t)px * dcx;
			int32_t zr = 0, zi = 0;
			uint32_t it = 0;

			while(it < maxit)
			{
				int32_t zr2 = (zr * zr) >> 12;
				int32_t zi2 = (zi * zi) >> 12;
				if(zr2 + zi2 > 4096)
					break;
				zi = ((zr * zi) >> 11) + ci;
				zr = zr2 - zi2 + cr;
				it++;
			}

			total += it;
			fb[py * w + px] = (uint16_t)(it * 8u);
		}
	}
	return total;
}

//Pure integer arithmetic, no memory traffic at all: multiply, shift, xor. This is
//the workload that says whether the issue-cost part of the model is right, with
//the caches out of the picture.
uint32_t work_int_math(uint32_t iters, uint32_t seed)
{
	uint32_t a = seed;
	uint32_t b = seed ^ 0x9E3779B9u;
	for(uint32_t i = 0; i < iters; i++)
	{
		a = a * 1103515245u + 12345u;
		b ^= a >> 7;
		b = (b << 3) | (b >> 29);
		a += b;
	}
	return a ^ b;
}

//The same shape in single-precision float. There is no FPU on this chip, so every
//one of these is a call into the compiler's software floating point: a workload
//that is entirely instruction cost and library code.
float work_float_math(uint32_t iters, float seed)
{
	float a = seed;
	for(uint32_t i = 0; i < iters; i++)
		a = a * 1.0000001f + 0.5f;
	return a;
}
