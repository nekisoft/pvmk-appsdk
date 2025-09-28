#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "renderer.h"
#include "audio.h"

// Game configuration
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000 / TARGET_FPS)

// Game constants
#define PLAYER_SPEED 4
#define BULLET_SPEED 4
#define ALIEN_SPEED_BASE 1
#define ALIEN_DROP_DISTANCE 20
#define MAX_BULLETS 10
#define MAX_ALIEN_BULLETS 2
#define MAX_EXPLOSIONS 10
#define PLAYER_LIVES 3
// Wave-specific configurations
#define MAX_ALIEN_ROWS 5
#define MAX_ALIEN_COLS 7

// Wave 1: 6 enemies, slow speed
#define WAVE1_ROWS 2
#define WAVE1_COLS 3
#define WAVE1_SPEED 0.3f

// Wave 2: 9 enemies, medium speed  
#define WAVE2_ROWS 3
#define WAVE2_COLS 3
#define WAVE2_SPEED 0.5f

// Wave 3: 10 enemies, faster speed but slower dropping
#define WAVE3_ROWS 2
#define WAVE3_COLS 5
#define WAVE3_SPEED 0.7f
#define SHIELD_COUNT 4

// UFO speeds for each wave
#define UFO_SPEED_WAVE1 1.0f   // Slow - easy to hit
#define UFO_SPEED_WAVE2 2.0f   // Medium speed
#define UFO_SPEED_WAVE3 3.0f   // Fast speed (original)
#define UFO_SPAWN_CHANCE 600  // 1 in 600 frames (~10 seconds at 60fps)

// Enemy bullet speeds for each wave (25% slower for wave 1)
#define ENEMY_BULLET_SPEED_WAVE1 1.5f  // 25% slower than normal
#define ENEMY_BULLET_SPEED_WAVE2 2.0f  // Normal speed
#define ENEMY_BULLET_SPEED_WAVE3 2.0f  // Normal speed

// Sprite sizes
#define PLAYER_WIDTH 12
#define PLAYER_HEIGHT 12
#define ALIEN_WIDTH 8    // Scaled down to 50% (was 16)
#define ALIEN_HEIGHT 8   // Scaled down to 50% (was 16)
#define BULLET_WIDTH 8
#define BULLET_HEIGHT 16
#define SHIELD_WIDTH 32
#define SHIELD_HEIGHT 24
#define UFO_WIDTH 16  // Much smaller - same as player size
#define UFO_HEIGHT 8  // Much smaller - same as alien height

// Points
#define ALIEN_POINTS_TOP 30
#define ALIEN_POINTS_MID 20
#define ALIEN_POINTS_BOT 10
#define UFO_POINTS 200


// Function declarations
void game_init(void);
void game_update(uint32_t deltaTime);
void game_handle_input(int key, bool pressed);
void game_render(void);
void game_cleanup(void);

bool game_isgameover(void);

// Wave configuration functions
int get_wave_rows(int wave);
int get_wave_cols(int wave);
float get_wave_speed(int wave);
float get_ufo_speed(int wave);
float get_enemy_bullet_speed(int wave);

#endif // GAME_H
