//font.h
//Text drawing for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef FONT_H
#define FONT_H

#include <stdint.h>

//Fonts we can draw
typedef enum font_style_e
{
	FS_NONE = 0,
	FS_REGULAR,
	FS_SMALL,
	FS_MAX
} font_style_t;

//Draws string of text, no wrapping. Returns pixel width of whole thing.
int font_draw(font_style_t style, const char *str, uint16_t color, int x, int y);

#endif //FONT_H
