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
	//Interpolands at beginning and end
	fix24p8_t x0[4]; //X, W, S, T
	fix24p8_t x1[4]; //X, W, S, T
	
	//Properties of the span
	uint8_t tex; //Texture index
	uint8_t mip; //MIP level to use
	
	uint8_t dummy[2];
} span_t;
#define SPAN_MAX 128
static span_t spans[FBY][SPAN_MAX]; //Span 0 of each line is unused and functions as the head/tail pointer
static int span_used[FBY];

//Single span being constructed - intermediate results of DDA at each videoline
static int span_l[FBY][4];
static int span_r[FBY][4];

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

//Inserts a span into the span buffer
static void span_add(int y, int tex, int x0[4], int x1[4])
{
	if(y < 0 || y >= FBY)
		return; //Off screen vertically
	
	if(x0[0] == x1[0])
		return; //Zero length horizontally
	
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
	
	if(spans[y][ss].x0[0] < 0)
	{
		span_clip_beginning(spans[y][ss].x0, spans[y][ss].x1, 0);
	}
	
	if(spans[y][ss].x1[0] > FBX)
	{
		span_clip_end(spans[y][ss].x0, spans[y][ss].x1, FBX);
	}
	
	
	spans[y][ss].tex = tex;
	spans[y][ss].mip = 0;
	
	span_used[y]++;
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
	
	//Compute NDC
	fix24p8_t va3[5];
	fix24p8_t vb3[5];
	fix24p8_t vc3[5];
	
	//Copy texture coords over so they stay with the right vert
	va3[3] = ta2[0]; va3[4] = ta2[1];
	vb3[3] = tb2[0]; vb3[4] = tb2[1];
	vc3[3] = tc2[0]; vc3[4] = tc2[1];

	//Dehomogenize
	va3[2] = FV(256) * va4[2] / va4[3];
	vb3[2] = FV(256) * vb4[2] / vb4[3];
	vc3[2] = FV(256) * vc4[2] / vc4[3];
	
	va3[1] = FV(256) * va4[0] / va4[3];
	vb3[1] = FV(256) * vb4[0] / vb4[3];
	vc3[1] = FV(256) * vc4[0] / vc4[3];

	va3[0] = FV(256) * va4[1] / va4[3];
	vb3[0] = FV(256) * vb4[1] / vb4[3];
	vc3[0] = FV(256) * vc4[1] / vc4[3];
		
	//Turn to pixel coordinates
	va3[1] = ((va3[1] * FBX / 256) + FV(FBX / 2)) / FV(1);
	vb3[1] = ((vb3[1] * FBX / 256) + FV(FBX / 2)) / FV(1);
	vc3[1] = ((vc3[1] * FBX / 256) + FV(FBX / 2)) / FV(1);

	va3[0] = ((va3[0] * FBY / 256) + FV(FBY / 2)) / FV(1);
	vb3[0] = ((vb3[0] * FBY / 256) + FV(FBY / 2)) / FV(1);
	vc3[0] = ((vc3[0] * FBY / 256) + FV(FBY / 2)) / FV(1);
	
	va3[2] = va3[2] / FV(1);
	vb3[2] = vb3[2] / FV(1);
	vc3[2] = vc3[2] / FV(1);

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
	//Clip spans on each line
	for(int ll = 0; ll < FBY; ll++)
	{
		span_t *ls = spans[ll];
		for(int aa = 0; aa < span_used[ll]; aa++)
		{
			for(int bb = aa+1; bb < span_used[ll]; bb++)
			{				
				const int aa_beg = ls[aa].x0[0];
				const int aa_end = ls[aa].x1[0];
				
				const int bb_beg = ls[bb].x0[0];
				const int bb_end = ls[bb].x1[0];
				
				const int aa_far = ls[aa].x0[1] + ls[aa].x1[1];
				const int bb_far = ls[bb].x0[1] + ls[bb].x1[1];
				
				if(aa_end <= bb_beg)
				{
					//Disjoint
					continue;
				}
				else if(bb_end <= aa_beg)
				{
					//Disjoint
					continue;
				}
				else if(aa_beg <= bb_beg && aa_end >= bb_end)
				{
					//span A is all around span B.
					if(aa_far > bb_far)
					{
						//A is behind.
						//Span B chops span A into two spans.
						int ss = span_used[ll];
						if(ss < SPAN_MAX)
						{
							//Add a copy of span A, after span B
							memcpy(&ls[ss], &ls[aa], sizeof(ls[ss]));
							
							//Clip the later half after span B
							span_clip_beginning(ls[ss].x0, ls[ss].x1, bb_end);
							
							span_used[ll]++;
						}
						
						//Clip the earlier half before span B
						span_clip_end(ls[aa].x0, ls[aa].x1, bb_beg);
					}
					else
					{
						//B is behind.
						//It is totally eliminated by span A.
						memset(&(ls[bb]), 0, sizeof(ls[bb]));
					}
				}
				else if(bb_beg <= aa_beg && bb_end >= aa_end)
				{
					//span B is all around span A.
					if(aa_far > bb_far)
					{
						//A is behind.
						//It is totally eliminated by span B.
						memset(&(ls[aa]), 0, sizeof(ls[aa]));
					}
					else
					{
						//B is behind.
						//Span A chops span B into two spans.
						int ss = span_used[ll];
						if(ss < SPAN_MAX)
						{
							//Add a copy of span B, before span A
							memcpy(&ls[ss], &ls[bb], sizeof(ls[ss]));
							
							//Clip the earlier half before span A
							span_clip_end(ls[ss].x0, ls[ss].x1, aa_beg);
							
							span_used[ll]++;
						}
						
						//Clip the later half after span A
						span_clip_beginning(ls[bb].x0, ls[bb].x1, aa_end);
						
					}
				}
				else if(aa_beg < bb_beg)
				{
					//span A is earlier but they overlap
					if(aa_far > bb_far)
					{
						//A is in back - clip its end so they don't overlap.
						span_clip_end(ls[aa].x0, ls[aa].x1, bb_beg);
					}
					else
					{
						//B is in back - clip its beginning so they don't overlap.
						span_clip_beginning(ls[bb].x0, ls[bb].x1, aa_end);
					}
				}
				else if(bb_beg < aa_beg)
				{
					//span B is earlier but they overlap
					if(aa_far > bb_far)
					{
						//A is in back - clip its beginning so they don't overlap.
						span_clip_beginning(ls[aa].x0, ls[aa].x1, bb_end);
					}
					else
					{
						//B is in back - clip its end so they don't overlap.
						span_clip_end(ls[bb].x0, ls[bb].x1, aa_beg);
					}
				}
			}
		}
	}
	
	//Draw the contents of each span
	//This is where the actual rasterization / texture-mapping happens
	uint16_t *fbline = fb_back;
	for(int ll = 0; ll < FBY; ll++)
	{		
		//Run through the spans and rasterize them
		//Todo - maybe sort by texture or something for cache locality
		span_t *sptr = spans[ll];
		for(int ss = 0; ss < span_used[ll]; ss++)
		{
			if(sptr->x0[0] == sptr->x1[0])
			{
				//Empty
			}
			else if(sptr->tex == 0)
			{
				//Performance-critical pixel rasterization loop!	
				for(int pp = sptr->x0[0]; pp < sptr->x1[0]; pp++)
				{
					fbline[pp] = 0;
				}
			}
			else
			{
				const fbpx_t *texdata = tex_data(sptr->tex);
				const int px_n = (sptr->x1[0] - sptr->x0[0]);
				
				uint32_t d_ts = (sptr->x1[2] - sptr->x0[2]) / px_n; //0.16
				uint32_t d_tt = (sptr->x1[3] - sptr->x0[3]) / px_n; //0.16
				
				uint32_t ts = sptr->x0[2]; //0.16
				uint32_t tt = sptr->x0[3]; //0.16
			
				//Performance-critical pixel rasterization loop!	
				//Convoluted a bit to make ARM compile better.
				uint16_t *pstart = &(fbline[sptr->x0[0]]);
				for(int pp = 0; pp < px_n; pp++)
				{
					pstart[pp] = texdata[ ((ts >> 11) % 32) + (((tt >> 11)%32) << 5) ];
					//if(pp == 0 || pp == px_n - 1)
					//	pstart[pp] = 0xFFFF;
					
					ts += d_ts;
					tt += d_tt;
				}
			}
			
			sptr++;
		}
		fbline += FBX;
	}
	
	//Clear the span-buffers, now that they're drained out to the framebuffer
	for(int ll = 0; ll < FBY; ll++)
	{
		span_used[ll] = 0;
		span_add(ll, 0, (int[4]){0,(1<<29),0,0}, (int[4]){FBX,(1<<29),65536,65536});		
	}
}
