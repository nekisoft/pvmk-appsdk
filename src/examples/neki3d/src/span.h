//span.h
//Span decomposition and rasterization
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef SPAN_H
#define SPAN_H

#include "common.h"

//Adds a textured triangle to the span buffer.
void span_add_tri(int tex,
	const fix24p8_t *va4, const int *ta2, 
	const fix24p8_t *vb4, const int *tb2,
	const fix24p8_t *vc4, const int *tc2);

//Rasterizes the span buffer into the given framebuffer, and clears the span buffer.
void span_finish(void);

#endif //SPAN_H
