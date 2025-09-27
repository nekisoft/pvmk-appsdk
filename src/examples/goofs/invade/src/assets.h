#ifndef ASSETS_H
#define ASSETS_H

#include "renderer.h"
#include "audio.h"
#include <stdbool.h>

// Asset loading functions
Sprite* load_png_sprite(const char* filename);
void free_sprite(Sprite* sprite);

// Asset management structure
typedef struct {
    // Player sprites
    Sprite* player;
    Sprite* playerLeft;
    Sprite* playerRight;
    Sprite* playerDamaged;
    
    // Enemy sprites
    Sprite* alien;
    Sprite* ufo;
    
    // Projectile sprites
    Sprite* laserGreen;
    Sprite* laserRed;
    
    // Other sprites
    Sprite* shield;
    Sprite* life;
    
    // Background sprites
    Sprite* starBackground;
    
    // Sound effects
    Sound* shootSound;
    Sound* explosionSound;
    Sound* ufoSound;
    Sound* bgMusic;
} Assets;

// Global asset management
bool assets_init(Assets* assets);
void assets_cleanup(Assets* assets);

#endif // ASSETS_H
