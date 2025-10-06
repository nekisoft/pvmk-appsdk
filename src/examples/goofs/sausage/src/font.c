//font.c
//Text drawing for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "font.h"
#include "fbs.h"

//Font bitmap data for each font supported
#include "font_fat.xbm"

//Information about each font supported
typedef struct font_style_data_s
{
	int width;
	int height; //of all 128 characters
	const char *bits;
} font_style_data_t;
font_style_data_t font_style_table[FS_MAX] = 
{
	[FS_FAT] = { .width = font_fat_width, .height = font_fat_height, .bits = font_fat_bits },
};

static int font_draw_char(font_style_t style, char ch, uint16_t color, int x, int y)
{
	const char *font_bits = font_style_table[style].bits;
	int font_width = font_style_table[style].width;
	int font_height = font_style_table[style].height / 128;
	
	uint32_t fillmap = 0;
	const char *pattern = font_bits + (((uint8_t)ch) * ((font_width+7)/8) * (font_height));
	for(int rr = 0; rr < font_height; rr++)
	{
		for(int cc = 0; cc < font_width; cc++)
		{
			if(!(pattern[cc/8] & (1u << (cc%8))))
			{
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
	return charwidth;
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
