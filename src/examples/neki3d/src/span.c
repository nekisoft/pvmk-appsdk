//span.c
//Span decomposition and rasterization
//Bryan E. Topp <betopp@betopp.com> 2024

#include "span.h"
#include "fb.h"
#include "tex.h"

#include <string.h>

//Spans in span buffer - linked list on each line
typedef struct span_s
{
	//Properties of the span
	uint8_t next; //Index of next span when sorted
	uint8_t prev; //Index of previous span when sorted
	uint8_t tex; //Texture index
	uint8_t mip; //MIP level to use
	
	//Interpolands at beginning and end
	fix24p8_t x0[4]; //X, W, S, T
	fix24p8_t x1[4]; //X, W, S, T
} span_t;
#define SPAN_MAX 128
static span_t spans[FBY][SPAN_MAX]; //Span 0 of each line is unused and functions as the head/tail pointer
static int span_used[FBY];

//Single span being constructed - intermediate results of DDA at each videoline
static int span_l[FBY][4];
static int span_r[FBY][4];

//Inserts a span into the span buffer
static void span_add(int y, int tex, int x0[4], int x1[4])
{
	if(y < 0 || y >= FBY)
		return; //Off screen vertically
	
	if(x0[0] == x1[0])
		return; //Zero length horizontally
	
	span_used[y]++;
	int ss = span_used[y];
	if(ss >= SPAN_MAX)
		return;	

	//Flip so the x0 side is always on the left in screen-X
	if(x0[0] < x1[0])
	{
		memcpy(spans[y][ss].x0, x0, sizeof(spans[y][ss].x0));
		memcpy(spans[y][ss].x1, x1, sizeof(spans[y][ss].x1));
	}
	else
	{
		memcpy(spans[y][ss].x0, x1, sizeof(spans[y][ss].x0));
		memcpy(spans[y][ss].x1, x0, sizeof(spans[y][ss].x1));
	}
	
	spans[y][ss].tex = tex;
	
	spans[y][spans[y][0].prev].next = ss;
	spans[y][ss].prev = spans[y][0].prev;
	spans[y][ss].next = 0;
	spans[y][0].prev = ss;
}

//Clips the beginning of a span
static void span_clip_beginning(int x0[4], int x1[4], int newbegin)
{
	int oldlen = x1[0] - x0[0];
	int newlen = x1[0] - newbegin;
	
	if(newlen <= 0)
		newlen = 0;
	if(newlen > oldlen)
		return;
	if(oldlen <= 0)
		return;
	
	for(int dd = 0; dd < 4; dd++)
	{
		x0[dd] = x1[dd] + ((int64_t)(x0[dd] - x1[dd]) * newlen / oldlen);
	}
}

//Clips the end of a span
static void span_clip_end(int x0[4], int x1[4], int newend)
{
	int oldlen = x1[0] - x0[0];
	int newlen = newend - x0[0];
	
	if(newlen <= 0)
		newlen = 0;
	if(newlen > oldlen)
		return;
	if(oldlen <= 0)
		return;
	
	for(int dd = 0; dd < 4; dd++)
	{
		x1[dd] = x0[dd] + ((int64_t)(x1[dd] - x0[dd]) * newlen / oldlen);
	}	
}

//DDA (Bresenham lines) algorithm for working down triangle edges
//The "X" is multidimensional as we interpolate screen X, depth, and two texture coordinates
static void span_dda(int y0, const int x0[4], int y1, const int x1[4], int dest[FBY][4])
{
	int den; //Denominator
	den = y1 - y0;
	if(den <= 0)
		return;

	for(int dd = 0; dd < 4; dd++)
	{
		int cur = x0[dd]; //Current interpolated value
		int rem = 0; //Remainder, fraction of the way to the next increment
		int num = x1[dd] - x0[dd]; //Numerator, amount added to remainder
		int whole = num / den; //Whole steps of interpolated value that we take every time
		num = num % den; //Remainder accumulates only those between whole steps
		
		if(num < 0)
		{
			int min = (x0[dd] < x1[dd]) ? x0[dd] : x1[dd];
			for(int yy = y0; yy < y1; yy += 1)
			{
				if(cur < min)
					cur = min;
				
				if(yy >= 0 && yy < FBY)
					dest[yy][dd] = cur;
				
				cur += whole;
				rem += num;
				if(rem <= -den)
				{
					rem += den;
					cur--;
				}
			}
		}
		else
		{
			
			int max = (x0[dd] > x1[dd]) ? x0[dd] : x1[dd];
			for(int yy = y0; yy < y1; yy += 1)
			{
				if(cur > max)
					cur = max;
				
				if(yy >= 0 && yy < FBY)
					dest[yy][dd] = cur;
				
				cur += whole;
				rem += num;
				if(rem >= den)
				{
					rem -= den;
					cur++;
				}
			}
		}		
	}
}

//Decomposes a triangle into spans, inserting them into the span buffer
void span_add_tri(int tex,
	const fix24p8_t *va4, const int *ta2, 
	const fix24p8_t *vb4, const int *tb2,
	const fix24p8_t *vc4, const int *tc2)
{
	if(va4[3] <= 0 || vb4[3] <= 0 || vc4[3] <= 0)
		return; //Behind the camera
	
	//Dehomogenize, turn to screen-space
	fix24p8_t va3[5];
	fix24p8_t vb3[5];
	fix24p8_t vc3[5];
	
	va3[3] = ta2[0]; va3[4] = ta2[1];
	vb3[3] = tb2[0]; vb3[4] = tb2[1];
	vc3[3] = tc2[0]; vc3[4] = tc2[1];
	
	//Todo - we're doing GL-style NDC as the output of the MVP transform
	//Then we have to do this additional step to turn it into integer pixel-coords at reasonable precision
	//We could combine this with the MVP stuff to save some time, once debugged like this

	va3[2] = (int64_t)FV(65536) * va4[3] / va4[2]; //inverse Z for sorting/texturing
	vb3[2] = (int64_t)FV(65536) * vb4[3] / vb4[2];
	vc3[2] = (int64_t)FV(65536) * vc4[3] / vc4[2];
	
	va3[1] = (int64_t)FBX * FV(1) * va4[0] / va4[3];
	vb3[1] = (int64_t)FBX * FV(1) * vb4[0] / vb4[3];
	vc3[1] = (int64_t)FBX * FV(1) * vc4[0] / vc4[3];

	va3[0] = (int64_t)FBY * FV(1) * va4[1] / va4[3];
	vb3[0] = (int64_t)FBY * FV(1) * vb4[1] / vb4[3];
	vc3[0] = (int64_t)FBY * FV(1) * vc4[1] / vc4[3];
	
	va3[1] += FBX * FV(0.5);
	vb3[1] += FBX * FV(0.5);
	vc3[1] += FBX * FV(0.5);

	va3[0] += FBY * FV(0.5);
	vb3[0] += FBY * FV(0.5);
	vc3[0] += FBY * FV(0.5);
	
	for(int dd = 0; dd < 3; dd++)
	{
		va3[dd] /= FV(1);
		vb3[dd] /= FV(1);
		vc3[dd] /= FV(1);
	}
	
	//Figure out top, middle, bottom verts in Y
	fix24p8_t *top = va3;
	if(vb3[0] < top[0])
		top = vb3;
	if(vc3[0] < top[0])
		top = vc3;
	
	fix24p8_t *bot = va3;
	if(vb3[0] > bot[0])
		bot = vb3;
	if(vc3[0] > bot[0])
		bot = vc3;
	
	fix24p8_t *mid = va3;
	if(mid == top || mid == bot)
		mid = vb3;
	if(mid == top || mid == bot)
		mid = vc3;

	//Do DDA to interpolate the X, depth, and texture coordinates, as we move down in Y
	span_dda(top[0], top + 1, mid[0], mid + 1, span_l);
	span_dda(mid[0], mid + 1, bot[0], bot + 1, span_l);
	
	span_dda(top[0], top + 1, bot[0], bot + 1, span_r);
	
	//Insert the result into the span buffer at each row we touched
	for(int yy = top[0]; yy < bot[0]; yy++)
	{
		span_add(yy, tex, span_l[yy], span_r[yy]);
	}
}

void span_finish(void)
{
	//This is where the actual rasterization / texture-mapping happens
	uint16_t *fbline = fb_back;
	for(int ll = 0; ll < FBY; ll++)
	{		
		span_t *ls = spans[ll];
		
		//Sort spans by starting X coordinate
		for(int ss = 0; ss < span_used[ll]; ss++)
		{
			int spanidx = 0;
			while(1)
			{
				spanidx = ls[spanidx].next;
				if(spanidx == 0)
					break;
				
				int successor = ls[spanidx].next;
				if(successor == 0)
					break;
				
				if(ls[spanidx].x0[0] > ls[successor].x0[0])
				{
					ls[spanidx].next = ls[successor].next;
					ls[successor].prev = ls[spanidx].prev;
					ls[successor].next = spanidx;
					ls[spanidx].prev = successor;
					ls[ls[successor].prev].next = successor;
					ls[ls[spanidx].next].prev = spanidx;
					spanidx = 0;
				}
			}
		}
		
		//Clip spans by X coordinate
		for(int ss = 0; ss < span_used[ll]; ss++)
		{
			int spanidx = 0;
			while(1)
			{
				spanidx = ls[spanidx].next;
				if(spanidx == 0)
					break;
				
				int successor = ls[spanidx].next;
				if(successor == 0)
					break;
				
				int span_end = ls[spanidx].x1[0];
				int next_begin = ls[successor].x0[0];
				int next_end = ls[successor].x1[0];
				if(span_end <= next_begin)
				{
					//Spans don't overlap
					//(This is bad because it means there's a gap in the rendering...)
					continue;
				}
				
				if(span_end > next_end)
				{
					//The spanidx span begins before the successor (as they're sorted)
					//It also ends after the successor.
					//The spanidx span needs to be split, therefore, around the successor.
					//Add a copy of the current span, beginning right where the successor does.
					//Then the existing one can get chopped.
					span_used[ll]++;
					int ss = span_used[ll];
					if(ss < SPAN_MAX)
					{
						memcpy(&ls[ss], &ls[spanidx], sizeof(ls[ss]));
						ls[ss].next = ls[successor].next;
						ls[ls[successor].next].prev = ss;
						ls[ss].prev = successor;
						ls[successor].next = ss;
					}
				}
								
				//Need to clip these two spans.
				//Figure out which one is in front.
				int avgw_spanidx = ls[spanidx].x0[1] + ls[spanidx].x1[1];
				int avgw_successor = ls[successor].x0[1] + ls[successor].x1[1];
				
				if(avgw_spanidx > avgw_successor)
				{
					//spanidx is in front of successor
					//The beginning of successor needs to get shaved.
					span_clip_beginning(
						ls[successor].x0,
						ls[successor].x1, 
						ls[spanidx].x1[0]
					);
				}
				else
				{
					//successor is in front of spanidx
					//The end of spanidx needs to get shaved.
					span_clip_end(
						ls[spanidx].x0,
						ls[spanidx].x1, 
						ls[successor].x0[0]
					);
				}
				
				//Check if we've annihilated either of these spans
				if(ls[spanidx].x0[0] == ls[spanidx].x1[0])
				{
					//spanidx span has been eliminated
					ls[ls[spanidx].prev].next = ls[spanidx].next;
					ls[ls[spanidx].next].prev = ls[spanidx].prev;
				}
				else if(ls[successor].x0[0] == ls[successor].x1[0])
				{
					//successor span has been eliminated
					ls[ls[successor].prev].next = ls[successor].next;
					ls[ls[successor].next].prev = ls[successor].prev;
				}
			}
		}
		
		//Run through the spans in order and rasterize them
		const span_t *sptr = &(ls[0]);
		for(int ss = 0; ss < span_used[ll]; ss++)
		{
			sptr = &(ls[sptr->next]);
			
			const fbpx_t *texdata = tex_data(sptr->tex);
			
			uint16_t ts = sptr->x0[2]; //0.16
			uint16_t tt = sptr->x0[3]; //0.16
			
			int px_n = (sptr->x1[0] - sptr->x0[0]);
			
			uint16_t d_ts = (sptr->x1[2] - sptr->x0[2]) / px_n; //0.16
			uint16_t d_tt = (sptr->x1[3] - sptr->x0[3]) / px_n; //0.16
			
			//Performance-critical pixel rasterization loop!
			for(int pp = sptr->x0[0]; pp < sptr->x1[0]; pp++)
			{
				fbpx_t sample = texdata[ (ts >> 8) + (tt & 0xFF00) ];
				
				fbline[pp] = sample;
				
				//Show wireframe outlines
				//if(pp == sptr->x0[0] || pp == sptr->x1[0] -1 )
				//	fbline[pp] = 0xFFFF;
				
				//Show how many times a pixel is touched (should be 1)
				//(void)sample;
				//fbline[pp] += 0x1082;
				
				ts += d_ts;
				tt += d_tt;
			}
		}
		
		fbline += FBX;
	}
	
	//Clear the span-buffers, now that they're drained out to the framebuffer
	for(int ll = 0; ll < FBY; ll++)
	{
		//Item 0 is just a head-pointer
		spans[ll][0].next = 1;
		spans[ll][0].prev = 0;
		span_used[ll] = 0;
		
		span_add(ll, 0, (int[4]){0,0,0,0}, (int[4]){FBX,0,65536,65536});		
	}
}
