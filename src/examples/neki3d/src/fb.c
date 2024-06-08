//fb.c
//Framebuffers
//Bryan E. Topp <betopp@betopp.com> 2024

#include "fb.h"

#include <sc.h>
#include <string.h>

//Framebuffers - triple buffered
fbpx_t fbs[3][FBY][FBX];

//Backbuffer currently being prepared
fbpx_t *fb_back = &(fbs[0][0][0]);

void fb_flip(void)
{
	//Enqueue the last-rendered image to flip onto the display.
	int flip_result = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fb_back);
	if(flip_result < 0)
	{
		//Failed to enqueue a flip
		_sc_pause();
		return;
	}
	
	//Enqueued a flip. One buffer is now displayed and one is enqueued.
	fbpx_t *enqueued = fb_back;
	fbpx_t *displayed = (fbpx_t*)(intptr_t)flip_result;
	
	//Find the remaining buffer.
	fb_back = &(fbs[0][0][0]);
	if(fb_back == displayed || fb_back == enqueued)
		fb_back = &(fbs[1][0][0]);
	if(fb_back == displayed || fb_back == enqueued)
		fb_back = &(fbs[2][0][0]);
	
	//Now ready to start rendering the next frame.
	
	//Clear frame buffer (can ditch this once we're covering the screen with spans)
	memset(fb_back, 0, sizeof(fbs[0]));
}

