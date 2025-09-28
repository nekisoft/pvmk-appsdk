#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>


typedef struct {
    uint32_t* pixels;
    int width;
    int height;
    int pitch;  // bytes per row
} Sprite;

// Renderer functions
bool renderer_init(const char* title, int width, int height);
void renderer_clear(uint32_t color);
void renderer_present(void);
void renderer_cleanup(void);

// Drawing functions
void draw_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_filled_rect(int x, int y, int w, int h, uint32_t color);
void draw_sprite(Sprite* sprite, int x, int y);
void draw_sprite_scaled(Sprite* sprite, int x, int y, float scale);
void draw_text(const char* text, int x, int y, uint32_t color);

// Color utilities
#define RGBA(r, g, b, a) ((uint32_t)((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define RGB(r, g, b) RGBA(r, g, b, 255)

// Common colors
#define COLOR_BLACK RGB(0, 0, 0)
#define COLOR_WHITE RGB(255, 255, 255)
#define COLOR_RED RGB(255, 0, 0)
#define COLOR_GREEN RGB(0, 255, 0)
#define COLOR_BLUE RGB(0, 0, 255)
#define COLOR_YELLOW RGB(255, 255, 0)

#endif // RENDERER_H
