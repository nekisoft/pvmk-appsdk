#include "assets.h"
#include <stdio.h>
#include <stdlib.h>

// Use stb_image for PNG loading (public domain)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


Sprite* load_png_sprite(const char* filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);  // Force RGBA
    
    if (!data) {
        fprintf(stderr, "Failed to load image: %s\n", filename);
        return NULL;
    }
    
    Sprite* sprite = (Sprite*)malloc(sizeof(Sprite));
    if (!sprite) {
        stbi_image_free(data);
        return NULL;
    }
    
    sprite->width = width;
    sprite->height = height;
    sprite->pitch = width * sizeof(uint32_t);
    sprite->pixels = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    
    if (!sprite->pixels) {
        free(sprite);
        stbi_image_free(data);
        return NULL;
    }
    
    // Convert RGBA to ARGB format expected by renderer
    for (int i = 0; i < width * height; i++) {
        unsigned char r = data[i * 4 + 0];
        unsigned char g = data[i * 4 + 1];
        unsigned char b = data[i * 4 + 2];
        unsigned char a = data[i * 4 + 3];
        
        sprite->pixels[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | 
                           ((uint32_t)g << 8) | (uint32_t)b;
    }
    
    stbi_image_free(data);
    return sprite;
}

void free_sprite(Sprite* sprite) {
    if (sprite) {
        if (sprite->pixels) {
            free(sprite->pixels);
        }
        free(sprite);
    }
}

bool assets_init(Assets* assets) {
    memset(assets, 0, sizeof(Assets));
    
    // Load player sprites
    assets->player = load_png_sprite("png/player.png");
    assets->playerLeft = load_png_sprite("png/playerLeft.png");
    assets->playerRight = load_png_sprite("png/playerRight.png");
    assets->playerDamaged = load_png_sprite("png/playerDamaged.png");
    
    if (!assets->player || !assets->playerLeft || 
        !assets->playerRight || !assets->playerDamaged) {
        fprintf(stderr, "Failed to load player sprites\n");
        return false;
    }
    
    // Load enemy sprites
    assets->alien = load_png_sprite("png/enemyShip.png");
    assets->ufo = load_png_sprite("png/enemyUFO.png");
    
    if (!assets->alien || !assets->ufo) {
        fprintf(stderr, "Failed to load enemy sprites\n");
        return false;
    }
    
    // Load projectile sprites
    assets->laserGreen = load_png_sprite("png/laserGreen.png");
    assets->laserRed = load_png_sprite("png/laserRed.png");
    
    if (!assets->laserGreen || !assets->laserRed) {
        fprintf(stderr, "Failed to load projectile sprites\n");
        return false;
    }
    
    // Load other sprites
    assets->shield = load_png_sprite("png/shield.png");
    assets->life = load_png_sprite("png/life.png");
    assets->starBackground = load_png_sprite("png/Background/starBackground.png");
    
    if (!assets->shield || !assets->life) {
        fprintf(stderr, "Failed to load other sprites\n");
        return false;
    }
    
    // Generate sound effects
    assets->shootSound = audio_generate_shoot();
    assets->explosionSound = audio_generate_explosion();
    assets->ufoSound = audio_generate_ufo();
    assets->bgMusic = audio_generate_bgm();
    
    if (!assets->shootSound || !assets->explosionSound || 
        !assets->ufoSound || !assets->bgMusic) {
        fprintf(stderr, "Failed to generate sounds\n");
        return false;
    }
    
    printf("All assets loaded successfully\n");
    return true;
}

void assets_cleanup(Assets* assets) {
    // Free sprites
    free_sprite(assets->player);
    free_sprite(assets->playerLeft);
    free_sprite(assets->playerRight);
    free_sprite(assets->playerDamaged);
    free_sprite(assets->alien);
    free_sprite(assets->ufo);
    free_sprite(assets->laserGreen);
    free_sprite(assets->laserRed);
    free_sprite(assets->shield);
    free_sprite(assets->life);
    free_sprite(assets->starBackground);
    
    // Note: Don't free sounds here as they're managed by the audio system
    memset(assets, 0, sizeof(Assets));
}

