//fb.h
//Framebuffers
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef FB_H
#define FB_H

#include "common.h"

//Dimensions of framebuffer
#define HIRES 0
#if HIRES
	#define FBX 640
	#define FBY 480
	#define FBMODE _SC_GFX_MODE_VGA_16BPP
#else
	#define FBX 320
	#define FBY 240
	#define FBMODE _SC_GFX_MODE_320X240_16BPP
#endif

//Pixel type
typedef uint16_t fbpx_t;

//Backbuffer currently being prepared
extern fbpx_t *fb_back;

//Flips buffers - enqueues the backbuffer to be presented, finds a free buffer for new backbuffer
void fb_flip(void);

#endif //FB_H
