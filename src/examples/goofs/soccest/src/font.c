//font.c
//Text drawing for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "font.h"
#include "fbs.h"

#include "font.xbm"

static int font_draw_char(char ch, uint16_t color, int x, int y)
{
	uint32_t fillmap = 0;
	char *pattern = font_bits + (((uint8_t)ch) * 4 * 32);
	for(int rr = 0; rr < 32; rr++)
	{
		for(int cc = 0; cc < 32; cc++)
		{
			if(!(pattern[cc/8] & (1u << (cc%8))))
			{
				BACKBUF[y+rr][x+cc-16] = color;
				fillmap |= 1u << cc;
			}
		}
		pattern += 4;
	}
	
	int charwidth = 0;
	while(fillmap > 0)
	{
		charwidth++;
		fillmap >>= 1;
	}
	return charwidth;
}

int font_draw(const char *str, uint16_t color, int x, int y)
{
	int allwidth = 0;
	while(*str != '\0')
	{
		int ww = font_draw_char(*str, color, x, y);
		if(str[0] == ' ' && str[1] != '\0')
			ww = 12;
		
		str++;
		x += ww;
		allwidth += ww;
	}
	return allwidth;
}
