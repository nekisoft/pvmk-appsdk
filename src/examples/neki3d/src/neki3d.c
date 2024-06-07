//neki3d.c
//Demo 3D texture-mapper for Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

#include <sc.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

//temp
#include <math.h>

#define FBX 320
#define FBY 240

//Framebuffers
uint16_t fbs[3][FBY][FBX];
uint16_t *fb_next;

//Fixed-point math type
typedef int32_t fix24p8_t;

//Triangle vertex
typedef struct vert_s
{
	fix24p8_t pos[3];
	uint8_t   rgb[3];
	uint8_t   zzz[1];
	fix24p8_t tex[2];
	fix24p8_t ign[2];
} vert_t;

//Spans in span buffer - linked list on each line
typedef struct span_s
{
	uint8_t next; //Index of next span when sorted
	uint8_t prev; //Index of previous span when sorted
	uint8_t tex; //Texture index
	uint8_t mip; //MIP level to use
	
	uint16_t x; //Starting pixel on line
	uint16_t n; //Width in pixels
	
	fix24p8_t sw; //Texture coordinate S * W
	fix24p8_t tw; //Texture coordinate T * W
	fix24p8_t w; //W (depth)
	
	fix24p8_t dsw; //Step in S*W
	fix24p8_t dtw; //Step in T*W
	fix24p8_t dw; //Step in W
} span_t;
#define SPAN_MAX 128
span_t spans[FBY][SPAN_MAX]; //Span 0 of each line is unused and functions as the head/tail pointer
int spanused[FBY];

//Texture memory
uint8_t textures[64][256][256][4];

//Inserts a span into the span buffer
void addspan(int y, int x0, int x1)
{
	if(y < 0 || y >= FBY)
		return;
	
	spanused[y]++;
	int ss = spanused[y];
	if(ss >= SPAN_MAX)
		return;	

	if(x0 < x1)
	{
		spans[y][ss].x = x0;
		spans[y][ss].n = x1 - x0;
	}
	else
	{
		spans[y][ss].x = x1;
		spans[y][ss].n = x0 - x1;
	}
	
	spans[y][spans[y][0].prev].next = ss;
	spans[y][ss].prev = spans[y][0].prev;
	spans[y][ss].next = 0;
	spans[y][0].prev = ss;
}

//Decomposes a triangle into spans, inserting them into the span buffer
void makespans(const fix24p8_t *va4, const fix24p8_t *vb4, const fix24p8_t *vc4)
{
	//Dehomogenize
	fix24p8_t va3[3];
	fix24p8_t vb3[3];
	fix24p8_t vc3[3];
	for(int dd = 0; dd < 3; dd++)
	{
		va3[dd] = va4[dd] / va4[3];
		vb3[dd] = vb4[dd] / vb4[3];
		vc3[dd] = vc4[dd] / vc4[3];
	}
	
	//Figure out top, middle, bottom verts in Y
	fix24p8_t *top = va3;
	if(vb3[1] < top[1])
		top = vb3;
	if(vc3[1] < top[1])
		top = vc3;
	
	fix24p8_t *bot = va3;
	if(vb3[1] > bot[1])
		bot = vb3;
	if(vc3[1] > bot[1])
		bot = vc3;
	
	fix24p8_t *mid = va3;
	if(mid == top || mid == bot)
		mid = vb3;
	if(mid == top || mid == bot)
		mid = vc3;
	
	//Bresenham lines
	
	int xx_long = top[0]; //Long side X coordinate (top to bottom)
	int xx_long_r = 0; //Remainder for Bresenham algorithm
	int xx_long_n = bot[0] - top[0]; //Numerator
	int xx_long_d = bot[1] - top[1]; //Denominator
	int xx_long_i = 1; //Increment when remainder >= denominator
	int xx_long_min = (bot[0] < top[0]) ? bot[0] : top[0];
	int xx_long_max = (bot[0] > top[0]) ? bot[0] : top[0];
	if(xx_long_n < 0 && xx_long_d < 0)
	{
		xx_long_n *= -1;
		xx_long_d *= -1;
	}
	else if(xx_long_n < 0)
	{
		xx_long_n *= -1;
		xx_long_i = -1;
	}
	else if(xx_long_d < 0)
	{
		xx_long_d *= -1;
		xx_long_i = -1;
	}
	
	int xx_first = top[0]; //Top short side X coordinate (top to middle)
	int xx_first_r = 0;
	int xx_first_n = mid[0] - top[0];
	int xx_first_d = mid[1] - top[1];
	int xx_first_i = 1;
	int xx_first_min = (mid[0] < top[0]) ? mid[0] : top[0];
	int xx_first_max = (mid[0] > top[0]) ? mid[0] : top[0];
	if(xx_first_n < 0 && xx_first_d < 0)
	{
		xx_first_n *= -1;
		xx_first_d *= -1;
	}
	else if(xx_first_n < 0)
	{
		xx_first_n *= -1;
		xx_first_i = -1;
	}
	else if(xx_first_d < 0)
	{
		xx_first_d *= -1;
		xx_first_i = -1;
	}	
	
	int xx_second = mid[0]; //Bottom short side X coordinate (middle to bottom)
	int xx_second_r = 0;
	int xx_second_n = bot[0] - mid[0];
	int xx_second_d = bot[1] - mid[1];
	int xx_second_i = 1;
	int xx_second_min = (mid[0] < bot[0]) ? mid[0] : bot[0];
	int xx_second_max = (mid[0] > bot[0]) ? mid[0] : bot[0];
	if(xx_second_n < 0 && xx_second_d < 0)
	{
		xx_second_n *= -1;
		xx_second_d *= -1;
	}
	else if(xx_second_n < 0)
	{
		xx_second_n *= -1;
		xx_second_i = -1;
	}
	else if(xx_second_d < 0)
	{
		xx_second_d *= -1;
		xx_second_i = -1;
	}
	
	int yy = top[1];
	
	//Work from top to middle
	for(; yy < mid[1]; yy += 1)
	{
		xx_long_r += xx_long_n;
		while(xx_long_r >= xx_long_d)
		{
			xx_long_r -= xx_long_d;
			xx_long += xx_long_i;
		}
		
		xx_first_r += xx_first_n;
		while(xx_first_r >= xx_first_d)
		{
			xx_first_r -= xx_first_d;
			xx_first += xx_first_i;
		}
		
		if(xx_long < xx_long_min)
			xx_long = xx_long_min;
		if(xx_long > xx_long_max)
			xx_long = xx_long_max;
		
		if(xx_first < xx_first_min)
			xx_first = xx_first_min;
		if(xx_first > xx_first_max)
			xx_first = xx_first_max;
		
		addspan(yy, xx_long, xx_first);
	}
	
	//Work from middle to bottom
	for(; yy < bot[1]; yy += 1)
	{
		xx_long_r += xx_long_n;
		while(xx_long_r >= xx_long_d)
		{
			xx_long_r -= xx_long_d;
			xx_long += xx_long_i;
		}
		
		xx_second_r += xx_second_n;
		while(xx_second_r >= xx_second_d)
		{
			xx_second_r -= xx_second_d;
			xx_second += xx_second_i;
		}
		
		if(xx_long < xx_long_min)
			xx_long = xx_long_min;
		if(xx_long > xx_long_max)
			xx_long = xx_long_max;
		
		if(xx_second < xx_second_min)
			xx_second = xx_second_min;
		if(xx_second > xx_second_max)
			xx_second = xx_second_max;
		
		addspan(yy, xx_long, xx_second);
	}
}

int main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	
	fb_next = &(fbs[0][0][0]);
	int loop = 0;
	
	while(1)
	{
		loop += 4;
		//usleep(1000);
		#define M_PI 3.1415926535f
		
		//Enqueue the last-rendered image to flip onto the display.
		int flip_result = _sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fb_next);
		if(flip_result < 0)
		{
			//Failed to enqueue a flip
			_sc_pause();
			continue;
		}
		
		//Enqueued a flip. One buffer is now displayed and one is enqueued.
		uint16_t *enqueued = fb_next;
		uint16_t *displayed = (uint16_t*)(intptr_t)flip_result;
		
		//Find the remaining buffer.
		fb_next = &(fbs[0][0][0]);
		if(fb_next == displayed || fb_next == enqueued)
			fb_next = &(fbs[1][0][0]);
		if(fb_next == displayed || fb_next == enqueued)
			fb_next = &(fbs[2][0][0]);
		
		//Now ready to start rendering the next frame.
		
		//Clear span buffers
		for(int ll = 0; ll < FBY; ll++)
		{
			spans[ll][0].next = 0;
			spans[ll][0].prev = 0;
			spanused[ll] = 0;
		}
		
		//Clear frame buffer
		memset(fb_next, 0, sizeof(fbs[0]));
	
		//Insert some test triangles into the span buffers
		fix24p8_t va[4] = 
		{
			(256 * 160) + (256 * 64 * sinf(loop * M_PI / 1000.0f)),
			(256 * 120) + (256 * 64 * cosf(loop * M_PI / 1000.0f)),
			0,
			256,
		};
		
		fix24p8_t vb[4] = 
		{
			(256 * 160) + (256 * 64 * sinf((loop+256) * M_PI / 1000.0f)),
			(256 * 120) + (256 * 64 * cosf((loop+256) * M_PI / 1000.0f)),
			0,
			256,
		};
		
		fix24p8_t vc[4] = 
		{
			(256 * 160) + (256 * 64 * sinf((loop+512) * M_PI / 1000.0f)),
			(256 * 120) + (256 * 64 * cosf((loop+512) * M_PI / 1000.0f)),
			0,
			256,
		};
		
		fix24p8_t vd[4] = 
		{
			(256 * 160) + (256 * 64 * sinf((loop+1024) * M_PI / 1000.0f)),
			(256 * 120) + (256 * 64 * cosf((loop+1024) * M_PI / 1000.0f)),
			0,
			256,
		};
		
		fix24p8_t ve[4] = 
		{
			(256 * 160) + (256 * 64 * sinf((loop+1536) * M_PI / 1000.0f)),
			(256 * 120) + (256 * 64 * cosf((loop+1536) * M_PI / 1000.0f)),
			0,
			256,
		};
		
		makespans(va, vc, vb);
		makespans(va, vc, vd);
		makespans(ve, va, vd);
		
		//Rasterize spans
		uint16_t spancolor = 0x4208;
		uint16_t *fbline = fb_next;
		for(int ll = 0; ll < FBY; ll++)
		{
			int spanidx = 0;
			while(1)
			{
				spanidx = spans[ll][spanidx].next;
				if(spanidx == 0)
					break;
				
				for(int pp = 0; pp < spans[ll][spanidx].n; pp++)
				{
					fbline[spans[ll][spanidx].x] += spancolor;
					spans[ll][spanidx].x++;
				}
			}
			
			fbline += FBX;
		}
		
		//Get input
		_sc_input_t input = {0};
		while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
		{
			
		}
	}
}
