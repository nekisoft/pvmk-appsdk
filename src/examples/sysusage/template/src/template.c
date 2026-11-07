//template.c
//A complete, tested starting point for a Neki32 game.
//Bryan E. Topp <betopp@betopp.com> 2026
//
//It is deliberately small but it does every thing a real game does, the way that does not
//have to be relearned later:
//
//  - a 320x240 RGB565 game at 60Hz, triple buffered, paced by _sc_gfx_flip
//  - input for four pads, with per-frame "held" and "pressed" state
//  - a fixed-timestep simulation (1/60s per frame), so a recorded run is reproducible
//  - sound effects queued without ever blocking the frame
//  - a save region in NVM, with a magic number, a version and a boot counter
//  - [EVENT] lines on stdout, so a shell script can test the game without a screen
//  - main() and app_main() split by _sc_shim_main, so the same source builds for the
//    desktop SDL shim (gmake -f Makefile.dev)
//
//The game itself: steer a square around and touch the blinking target. It exists to show
//the frame loop and the harness, not to be fun.

#include <sc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FB_W 320
#define FB_H 240
#define NFB  3

static uint16_t fbs[NFB][FB_H][FB_W];   //The three framebuffers, in BSS: 450KB total
static uint16_t background[FB_H][FB_W]; //Painted once, copied into each frame

//--- 16.16 fixed point -------------------------------------------------------
//The machine has no FPU. Positions and velocities are 16.16 integers: 65536 is 1.0.
typedef int32_t fix_t;
#define FIX(n) ((fix_t)((n) * 65536))
#define FIX_MUL(a, b) ((fix_t)(((int64_t)(a) * (int64_t)(b)) >> 16))
#define FIX_INT(a) ((a) >> 16)

//--- save data --------------------------------------------------------------
#define SAVE_MAGIC 0x544D504C //"TMPL"
#define SAVE_VERSION 1
#define SAVE_MAX 128

typedef struct save_s
{
	uint32_t magic;
	uint32_t version;
	uint32_t boots;
	uint32_t best_score;
	uint32_t checksum;
	uint8_t  reserved[SAVE_MAX - 20];
} save_t;

//--- sound ------------------------------------------------------------------
//0.25s of a square wave, stereo 16-bit. Made once, replayed as often as needed; the kernel
//copies it, so the buffer may be reused while the old copy plays.
#define TONE_FRAMES 12000
static int16_t tone[TONE_FRAMES * 2];

static void make_tone(int hz)
{
	for(int ii = 0; ii < TONE_FRAMES; ii++)
	{
		int phase = (ii * hz * 2) / 48000; //0 or 1
		int16_t sample = (phase & 1) ? 4000 : -4000;
		tone[(ii * 2) + 0] = sample;
		tone[(ii * 2) + 1] = sample;
	}
}

static void play_tone(void)
{
	int queued = _sc_snd_play(_SC_SND_MODE_48K_16B_2C, tone, sizeof(tone), sizeof(tone) * 2);
	if(queued < 0)
	{
		//-_SC_EAGAIN means the kernel queue is full; skipping a blip is better than
		//stalling the frame. Any other error is worth saying once.
		static int warned = 0;
		if(queued != -_SC_EAGAIN && !warned)
		{
			printf("[EVENT] SND_ERROR %d\n", queued);
			warned = 1;
		}
	}
}

//--- drawing ----------------------------------------------------------------
static inline uint16_t rgb565(int r, int g, int b)
{
	return (uint16_t)(((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F));
}

static void paint_background(void)
{
	for(int yy = 0; yy < FB_H; yy++)
	{
		for(int xx = 0; xx < FB_W; xx++)
		{
			int r = 6 + ((xx * 12) / FB_W);
			int g = 3 + ((yy * 12) / FB_H);
			int b = 16 - ((yy * 10) / FB_H);
			background[yy][xx] = rgb565(r, g, b);
		}
	}
}

//Fills a rectangle. Rows are written 32 bits at a time after the first pixel, because two
//RGB565 pixels fit one word and the framebuffer is 4-byte aligned by contract.
static void fill_rect(uint16_t *fb, int x, int y, int w, int h, uint16_t colour)
{
	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(x + w > FB_W) w = FB_W - x;
	if(y + h > FB_H) h = FB_H - y;
	if(w <= 0 || h <= 0)
		return;

	for(int yy = y; yy < y + h; yy++)
	{
		uint16_t *row = fb + (yy * FB_W) + x;
		int left = w;
		if(x & 1)
		{
			*row++ = colour;
			left--;
		}
		uint32_t pair = ((uint32_t)colour << 16) | colour;
		uint32_t *word = (uint32_t *)row;
		while(left >= 2)
		{
			*word++ = pair;
			left -= 2;
		}
		if(left)
			*(uint16_t *)word = colour;
	}
}

//--- input ------------------------------------------------------------------
static uint16_t held[4];
static uint16_t pressed[4];

static void poll_input(void)
{
	uint16_t before[4];
	memcpy(before, held, sizeof(before));

	_sc_input_t ev;
	while(_sc_input(&ev, sizeof(ev), sizeof(ev)) > 0)
	{
		int pad = -1;
		if(ev.format >= 'A' && ev.format <= 'D')
			pad = ev.format - 'A';
		if(pad >= 0)
			held[pad] = ev.buttons;
	}

	for(int pp = 0; pp < 4; pp++)
		pressed[pp] = (uint16_t)(held[pp] & ~before[pp]);

	//Say so when a pad changes. A test can then assert that its scripted press arrived,
	//and a person debugging a controller sees the same line.
	for(int pp = 0; pp < 4; pp++)
		if(held[pp] != before[pp])
			printf("[EVENT] PAD %c %04X\n", 'A' + pp, (unsigned)held[pp]);
}

//--- simulation -------------------------------------------------------------
typedef struct game_s
{
	fix_t px, py;   //player position
	fix_t vx, vy;   //player velocity
	int tx, ty;     //target position, pixels
	int score;
	uint32_t rng;
} game_t;

static uint32_t rng_next(uint32_t *state)
{
	//A small deterministic PRNG. Do not use rand() in a game you want to test: its
	//sequence depends on the C library and on whether anything else called it first.
	*state = (*state * 1664525u) + 1013904223u;
	return *state;
}

static void step_game(game_t *g)
{
	//Accelerate with the d-pad, with friction. 1/60s per call, always.
	const fix_t accel = FIX(0.35);
	const fix_t drag = FIX(0.88);
	const fix_t maxv = FIX(3.0);

	if(held[0] & _SC_BTNBIT_LEFT)  g->vx -= accel;
	if(held[0] & _SC_BTNBIT_RIGHT) g->vx += accel;
	if(held[0] & _SC_BTNBIT_UP)    g->vy -= accel;
	if(held[0] & _SC_BTNBIT_DOWN)  g->vy += accel;

	g->vx = FIX_MUL(g->vx, drag);
	g->vy = FIX_MUL(g->vy, drag);
	if(g->vx >  maxv) g->vx =  maxv;
	if(g->vx < -maxv) g->vx = -maxv;
	if(g->vy >  maxv) g->vy =  maxv;
	if(g->vy < -maxv) g->vy = -maxv;

	g->px += g->vx;
	g->py += g->vy;

	const fix_t margin = FIX(8);
	if(g->px < margin) { g->px = margin; g->vx = 0; }
	if(g->py < margin) { g->py = margin; g->vy = 0; }
	if(g->px > FIX(FB_W - 8)) { g->px = FIX(FB_W - 8); g->vx = 0; }
	if(g->py > FIX(FB_H - 8)) { g->py = FIX(FB_H - 8); g->vy = 0; }

	//Touch the target: score, say so, and move it somewhere else
	int dx = FIX_INT(g->px) - g->tx;
	int dy = FIX_INT(g->py) - g->ty;
	if((dx * dx) + (dy * dy) < 16 * 16)
	{
		g->score++;
		printf("[EVENT] COLLECT %d\n", g->score);
		g->tx = 16 + (int)(rng_next(&g->rng) % (FB_W - 32));
		g->ty = 16 + (int)(rng_next(&g->rng) % (FB_H - 32));
		play_tone();
	}
}

//--- entry point ------------------------------------------------------------
int app_main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;

	//Line-buffered, so a run killed by a harness still has its log.
	setvbuf(stdout, NULL, _IOLBF, 0);

	//Load the save. A freshly allocated record reads back zero bytes, and the magic tells
	//the two apart. NVM is always available; the kernel gives a temporary record to a game
	//whose card has no usable volume name.
	save_t save;
	memset(&save, 0, sizeof(save));
	int loaded = _sc_nvm_load(&save, sizeof(save));
	if(loaded < (int)sizeof(save) || save.magic != SAVE_MAGIC || save.version != SAVE_VERSION)
	{
		memset(&save, 0, sizeof(save));
		save.magic = SAVE_MAGIC;
		save.version = SAVE_VERSION;
	}

	save.boots++;
	printf("[EVENT] BOOT %u\n", (unsigned)save.boots);

	paint_background();
	make_tone(440);

	game_t game;
	memset(&game, 0, sizeof(game));
	game.px = FIX(FB_W / 2);
	game.py = FIX(FB_H / 2);
	game.tx = FB_W / 3;
	game.ty = FB_H / 3;
	game.rng = 0x12345678u;

	int back = 0;
	int frame = 0;
	int saved = 0;

	for(;;)
	{
		poll_input();

		if(pressed[0] & _SC_BTNBIT_A)
			play_tone();

		//Leave the game with START; save and say so first, because PVMK restarts a
		//program that exits and would otherwise lose the score.
		if(pressed[0] & _SC_BTNBIT_START)
		{
			if(game.score > (int)save.best_score)
				save.best_score = (uint32_t)game.score;
			int wrote = _sc_nvm_save(&save, sizeof(save));
			printf("[EVENT] SAVE result %d best %u\n", wrote, (unsigned)save.best_score);
			saved = 1;
			break;
		}

		step_game(&game);

		//Draw: copy the background, then the target and the player
		memcpy(fbs[back], background, sizeof(background));
		int blink = (frame / 15) & 1;
		fill_rect(fbs[back][0], game.tx - 6, game.ty - 6, 12, 12,
			blink ? rgb565(31, 31, 0) : rgb565(31, 16, 0));
		fill_rect(fbs[back][0], FIX_INT(game.px) - 8, FIX_INT(game.py) - 8, 16, 16,
			rgb565(8, 31, 16));

		//Enqueue this frame and stay in step with the display. The call returns the buffer
		//being displayed now; waiting until that is the buffer we just enqueued is what
		//makes one iteration one frame (60 per second), and therefore what makes the
		//simulation speed independent of how fast the CPU happens to be. A game that would
		//rather run ahead of the display can keep three buffers in flight and derive its
		//timestep from flip counts instead - but then a recorded run depends on CPU speed,
		//which is exactly what a test cannot afford.
		int shown = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[back]);
		while((void *)(intptr_t)shown != (void *)fbs[back])
		{
			_sc_pause();
			shown = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[back]);
		}

		//The buffer we enqueued is on screen; the next one may be drawn into.
		back = (back + 1) % NFB;

		frame++;
		if((frame % 60) == 0)
			printf("[EVENT] SECOND %d\n", frame / 60);
	}

	(void)saved;
	return 0;
}

//On PVMK this calls app_main. On the desktop shim it hands the real main thread to SDL and
//runs app_main on a worker thread, which is what SDL requires.
int main(int argc, const char **argv)
{
	return _sc_shim_main(app_main, argc, argv);
}
