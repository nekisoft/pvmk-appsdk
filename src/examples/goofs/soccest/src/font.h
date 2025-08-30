//font.h
//Text drawing for soccer game
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef FONT_H
#define FONT_H

#include <stdint.h>

//Draws string of text, no wrapping. Returns pixel width of whole thing.
int font_draw(const char *str, uint16_t color, int x, int y);

#endif //FONT_H
