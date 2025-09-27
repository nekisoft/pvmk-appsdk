//font.c
//Text drawing
//Bryan E. Topp <betopp@betopp.com> 2025

#include "font.h"
#include "fbs.h"

//Font bitmap data for each font supported
#include "font_scifi.xbm"

//Information about each font supported
typedef struct font_style_data_s
{
	int width;
	int height; //of all 128 characters
	const unsigned char *bits;
} font_style_data_t;
font_style_data_t font_style_table[FS_MAX] = 
{
	[FS_SCIFI] = { .width = font_scifi_width, .height = font_scifi_height, .bits = font_scifi_bits },
};

static int font_draw_char(font_style_t style, char ch, uint16_t color, int x, int y)
{
	const unsigned char *font_bits = font_style_table[style].bits;
	int font_width = font_style_table[style].width;
	int font_height = font_style_table[style].height / 128;
	
	uint32_t fillmap = 0;
	const unsigned char *pattern = font_bits + (((uint8_t)ch) * ((font_width+7)/8) * (font_height));
	for(int rr = 0; rr < font_height; rr++)
	{
		if(y+rr < 0)
			continue;
		if(y+rr >= SCREENY)
			break;
		
		for(int cc = 0; cc < font_width; cc++)
		{
			if(x+cc >= SCREENX)
				break;
			
			if(!(pattern[cc/8] & (1u << (cc%8))))
			{
				if(x+cc >= 0)
					BACKBUF[y+rr][x+cc-16] = color;
				
				fillmap |= 1u << cc;
			}
		}
		pattern += (font_width+7)/8;
	}
	
	int charwidth = 0;
	while(fillmap > 0)
	{
		charwidth++;
		fillmap >>= 1;
	}
	return charwidth+1;
}

int font_draw(font_style_t style, const char *str, uint16_t color, int x, int y)
{
	int allwidth = 0;
	while(*str != '\0')
	{
		int ww = font_draw_char(style, *str, color, x, y);
		if(str[0] == ' ' && str[1] != '\0')
			ww = font_style_table[style].width * 3 / 8;
		
		str++;
		x += ww;
		allwidth += ww;
	}
	return allwidth;
}
