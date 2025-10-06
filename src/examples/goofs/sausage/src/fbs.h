//fbs.h
//Framebuffers in soccer game
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef FBS_H
#define FBS_H

#include <stdint.h>

//Screen resolution
#define SCREENX 640
#define SCREENY 480

//Framebuffers
extern uint16_t fbs[3][SCREENY][SCREENX];
extern int fbs_next;
#define BACKBUF fbs[fbs_next]

//Flips buffers
void fbs_flip(void);

#endif //FBS_H
