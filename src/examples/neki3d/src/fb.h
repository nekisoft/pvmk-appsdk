//fb.h
//Framebuffers
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef FB_H
#define FB_H

#include "common.h"

//Dimensions of framebuffer
#define FBX 320
#define FBY 240

//Pixel type
typedef uint16_t fbpx_t;

//Backbuffer currently being prepared
extern fbpx_t *fb_back;

//Flips buffers - enqueues the backbuffer to be presented, finds a free buffer for new backbuffer
void fb_flip(void);

#endif //FB_H
