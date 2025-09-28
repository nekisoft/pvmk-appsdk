#include "game.h"
#include "renderer.h"
#include "assets.h"
#include "audio.h"
#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Game entities
typedef struct {
    float x, y;
    int width, height;
    bool active;
    Sprite* sprite;
} Entity;

typedef struct {
    Entity base;
    int lives;
    int moveDir;     // -1 left, 0 stop, 1 right
    int moveDirY;    // -1 up, 0 stop, 1 down
    bool canFire;
    uint32_t lastFireTime;
    bool isInvincible;           // True during invincibility period
    uint32_t invincibilityStart; // When invincibility started
    uint32_t invincibilityDuration; // How long invincibility lasts (3000ms)
} Player;

typedef struct {
    Entity base;
    int points;
    int animFrame;
    uint32_t lastAnimTime;
} Alien;

typedef struct {
    Entity base;
    int direction;  // 1 for player bullet (up), -1 for alien bullet (down)
} Bullet;

typedef struct {
    Entity base;
    uint8_t* damageMap;  // Per-pixel damage tracking
    int health;
} Shield;

typedef struct {
    Entity base;
    int direction;  // -1 or 1 for left/right movement
    uint32_t spawnTime;
} UFO;

typedef struct {
    Entity base;
    uint32_t startTime;
    uint32_t duration;  // How long explosion lasts in milliseconds
    int animFrame;      // Current animation frame
    uint32_t lastFrameTime;
} Explosion;

// Game phases
typedef enum {
    PHASE_INTRO,      // Story/intro sequence
    PHASE_PLAYING,    // Main gameplay
    PHASE_GAME_OVER   // Game over state
} GamePhase;

// Game state
typedef struct {
    Player player;
    Alien aliens[MAX_ALIEN_ROWS][MAX_ALIEN_COLS];
    Bullet playerBullets[MAX_BULLETS];
    Bullet alienBullets[MAX_ALIEN_BULLETS];
    Shield shields[SHIELD_COUNT];
    UFO ufo;
    Explosion explosions[MAX_EXPLOSIONS];
    
    int score;
    int highScore;
    int wave;
    int alienCount;
    int alienDirection;
    float alienSpeed;
    uint32_t lastAlienMove;
    uint32_t lastAlienFire;
    
    // Smooth edge drop state
    bool alienDropping;      // true when aliens are in dropping animation
    float dropProgress;      // 0.0 to 1.0 progress of current drop
    uint32_t dropStartTime;  // when the current drop started
    
    GamePhase phase;
    uint32_t phaseStartTime;
    GamePhase phase_last;
    bool gameOver;
    bool victory;  // true if game ended due to victory, false if defeat
    bool paused;
    uint32_t gameStartTime;
    
    // Bonus point display
    bool showBonus;
    int bonusPoints;
    float bonusX, bonusY;
    uint32_t bonusStartTime;
} GameState;
static GameState g_gameState;
static GameState * const game = &g_gameState;


// Helper functions
bool check_collision(Entity* a, Entity* b);
void spawn_alien_bullet(void);
void spawn_player_bullet(void);
void spawn_ufo(void);
void spawn_explosion(float x, float y);
void damage_shield(Shield* shield, int x, int y, int radius);
void damage_shield_visual(Shield* shield, int x, int y, int radius);
void damage_shield_health(Shield* shield, int x, int y, int radius);
void next_wave(void);

void game_init(void) {
    memset(game, 0, sizeof(GameState));
    
    // Start with intro phase
    game->phase = PHASE_INTRO;
game->phase_last = PHASE_INTRO;
    game->phaseStartTime = SDL_GetTicks();
    
    // Initialize player (starts off-screen during intro)
    game->player.base.x = SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2;
    game->player.base.y = SCREEN_HEIGHT + PLAYER_HEIGHT;  // Below screen
    game->player.base.width = PLAYER_WIDTH;
    game->player.base.height = PLAYER_HEIGHT;
    game->player.base.active = true;
    game->player.base.sprite = g_assets.player;
    game->player.lives = PLAYER_LIVES;
    game->player.moveDir = 0;      // No horizontal movement initially
    game->player.moveDirY = 0;     // No vertical movement initially
    game->player.canFire = false;  // Can't fire during intro
    game->player.isInvincible = false;        // Not invincible initially
    game->player.invincibilityStart = 0;      // No invincibility timer
    game->player.invincibilityDuration = 3000; // 3 seconds invincibility
    
    // Initialize aliens (but inactive during intro) - start with wave 1 configuration
    int initial_rows = get_wave_rows(1);  // Start with wave 1
    int initial_cols = get_wave_cols(1);
    game->alienCount = initial_rows * initial_cols;  // Set correct count for wave 1
    
    // Initialize all alien slots
    for (int row = 0; row < MAX_ALIEN_ROWS; row++) {
        for (int col = 0; col < MAX_ALIEN_COLS; col++) {
            Alien* alien = &game->aliens[row][col];
            alien->base.active = false;  // All inactive initially
            alien->base.width = ALIEN_WIDTH;
            alien->base.height = ALIEN_HEIGHT;
            alien->base.sprite = g_assets.alien;
            
            // Different point values for different rows
            if (row == 0) alien->points = ALIEN_POINTS_TOP;
            else if (row < 3) alien->points = ALIEN_POINTS_MID;
            else alien->points = ALIEN_POINTS_BOT;
        }
    }
    
    // Set initial positions for wave 1 aliens only
    for (int row = 0; row < initial_rows; row++) {
        for (int col = 0; col < initial_cols; col++) {
            Alien* alien = &game->aliens[row][col];
            alien->base.x = 50 + col * (ALIEN_WIDTH + 15);
            alien->base.y = -ALIEN_HEIGHT - 50 - row * (ALIEN_HEIGHT + 15);  // Start above screen
        }
    }
    
    // Initialize shields
    int shieldSpacing = 120;  // Extra wide spacing for 4 shields with easy bullet passage
    int totalShieldSpace = SHIELD_COUNT * SHIELD_WIDTH + (SHIELD_COUNT - 1) * shieldSpacing;
    int startX = (SCREEN_WIDTH - totalShieldSpace) / 2;
    
    for (int i = 0; i < SHIELD_COUNT; i++) {
        Shield* shield = &game->shields[i];
        shield->base.x = startX + i * (SHIELD_WIDTH + shieldSpacing);
        shield->base.y = SCREEN_HEIGHT - 120;  // Position above player (adjusted for smaller sizes)
        shield->base.width = SHIELD_WIDTH;
        shield->base.height = SHIELD_HEIGHT;
        shield->base.active = true;  // Shields are always active
        shield->base.sprite = g_assets.shield;
        shield->health = 20;  // Now only 2 hits needed (10 damage each)
        
        // Allocate damage map (0 = undamaged, 255 = destroyed)
        int mapSize = SHIELD_WIDTH * SHIELD_HEIGHT;
        shield->damageMap = (uint8_t*)calloc(mapSize, sizeof(uint8_t));
    }
    
    // Initialize game state
    game->alienDirection = 1;
    game->alienSpeed = get_wave_speed(1);  // Start with wave 1 speed
    game->wave = 1;
    game->victory = false;  // Initialize victory flag
    game->highScore = 0;
    game->gameStartTime = SDL_GetTicks();
    
    // Initialize smooth drop state
    game->alienDropping = false;
    game->dropProgress = 0.0f;
    game->dropStartTime = 0;
    
    // Initialize bonus display
    game->showBonus = false;
    game->bonusPoints = 0;
    game->bonusX = 0;
    game->bonusY = 0;
    game->bonusStartTime = 0;
    
    // Initialize explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        game->explosions[i].base.active = false;
    }
}

void game_cleanup(void) {
    // Free shield damage maps
    for (int i = 0; i < SHIELD_COUNT; i++) {
        if (game->shields[i].damageMap) {
            free(game->shields[i].damageMap);
        }
    }
}

bool check_collision(Entity* a, Entity* b) {
    // Check if both entities are active
    if (!a->active || !b->active) return false;
    
    // Calculate center points of both entities
    float a_center_x = a->x + a->width / 2.0f;
    float a_center_y = a->y + a->height / 2.0f;
    float b_center_x = b->x + b->width / 2.0f;
    float b_center_y = b->y + b->height / 2.0f;
    
    // Calculate radii (use larger dimension + generous padding for 100% detection)
    float a_radius = (a->width > a->height ? a->width : a->height) / 2.0f + 8.0f;
    float b_radius = (b->width > b->height ? b->width : b->height) / 2.0f + 8.0f;
    
    // Calculate distance between centers
    float dx = a_center_x - b_center_x;
    float dy = a_center_y - b_center_y;
    float distance_squared = dx * dx + dy * dy;  // Skip sqrt for performance
    float combined_radius = a_radius + b_radius;
    float combined_radius_squared = combined_radius * combined_radius;
    
    // Check circular collision first
    bool circular_hit = distance_squared < combined_radius_squared;
    
    // Also check expanded rectangular collision as backup for 100% coverage
    float padding = 6.0f;  // Extra padding for edge cases
    bool rect_hit = (a->x - padding < b->x + b->width + padding &&
                     a->x + a->width + padding > b->x - padding &&
                     a->y - padding < b->y + b->height + padding &&
                     a->y + a->height + padding > b->y - padding);
    
    // Return true if EITHER collision method detects a hit
    return circular_hit || rect_hit;
}


void spawn_player_bullet(void) {
    if (!game->player.canFire) return;
    
    // Find an inactive bullet slot
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet* bullet = &game->playerBullets[i];
        if (!bullet->base.active) {
            // Center bullet horizontally on player cannon
            float playerCenterX = game->player.base.x + (PLAYER_WIDTH / 2.0f);
            bullet->base.x = playerCenterX - (BULLET_WIDTH / 2.0f);
            bullet->base.y = game->player.base.y - 4;  // Spawn from top of cannon
            bullet->base.width = BULLET_WIDTH;
            bullet->base.height = BULLET_HEIGHT;
            bullet->base.active = true;
            bullet->base.sprite = g_assets.laserGreen;
            bullet->direction = 1;  // Up
            
            game->player.canFire = false;
            game->player.lastFireTime = SDL_GetTicks();
            
            // Play shoot sound
            audio_play_sound(g_assets.shootSound);
            break;
        }
    }
}

void spawn_alien_bullet(void) {
    // Pick a random active alien to fire
    int wave_rows = get_wave_rows(game->wave);
    int wave_cols = get_wave_cols(game->wave);
    int activeAliens[MAX_ALIEN_ROWS * MAX_ALIEN_COLS];
    int activeCount = 0;
    
    for (int row = 0; row < wave_rows; row++) {
        for (int col = 0; col < wave_cols; col++) {
            if (game->aliens[row][col].base.active) {
                activeAliens[activeCount++] = row * wave_cols + col;
            }
        }
    }
    
    if (activeCount == 0) return;
    
    int shooterIndex = activeAliens[rand() % activeCount];
    int row = shooterIndex / wave_cols;
    int col = shooterIndex % wave_cols;
    Alien* shooter = &game->aliens[row][col];
    
    // Find an inactive alien bullet slot
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        Bullet* bullet = &game->alienBullets[i];
        if (!bullet->base.active) {
            bullet->base.x = shooter->base.x + ALIEN_WIDTH / 2 - BULLET_WIDTH / 2;
            bullet->base.y = shooter->base.y + ALIEN_HEIGHT;
            bullet->base.width = BULLET_WIDTH;
            bullet->base.height = BULLET_HEIGHT;
            bullet->base.active = true;
            bullet->base.sprite = g_assets.laserRed;
            bullet->direction = -1;  // Down
            break;
        }
    }
}

void spawn_ufo(void) {
    if (!game->ufo.base.active && rand() % UFO_SPAWN_CHANCE == 0) {
        game->ufo.base.width = UFO_WIDTH;
        game->ufo.base.height = UFO_HEIGHT;
        game->ufo.base.y = 20;
        game->ufo.base.active = true;
        game->ufo.base.sprite = g_assets.ufo;
        game->ufo.spawnTime = SDL_GetTicks();
        
        // Randomly spawn from left or right
        if (rand() % 2 == 0) {
            game->ufo.base.x = -UFO_WIDTH;
            game->ufo.direction = 1;
        } else {
            game->ufo.base.x = SCREEN_WIDTH;
            game->ufo.direction = -1;
        }
        
        // Debug: Print UFO speed for current wave
        float ufoSpeed = get_ufo_speed(game->wave);
        printf("UFO spawned in Wave %d with speed %.1f\n", game->wave, ufoSpeed);
        fflush(stdout);
        
        // Play UFO sound
        audio_play_sound(g_assets.ufoSound);
    }
}

void spawn_explosion(float x, float y) {
    // Find an inactive explosion slot
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        Explosion* explosion = &game->explosions[i];
        if (!explosion->base.active) {
            explosion->base.x = x;
            explosion->base.y = y;
            explosion->base.width = 16;
            explosion->base.height = 16;
            explosion->base.active = true;
            explosion->base.sprite = NULL;  // We'll draw custom explosion
            explosion->startTime = SDL_GetTicks();
            explosion->duration = 500;  // 0.5 second explosion
            explosion->animFrame = 0;
            explosion->lastFrameTime = SDL_GetTicks();
            break;
        }
    }
}

void damage_shield_visual(Shield* shield, int x, int y, int radius) {
    if (!shield->damageMap) return;
    
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int px = x + dx;
            int py = y + dy;
            
            if (px >= 0 && px < shield->base.width && 
                py >= 0 && py < shield->base.height) {
                int index = py * shield->base.width + px;
                shield->damageMap[index] = 255;  // Mark as damaged
            }
        }
    }
}

void damage_shield_health(Shield* shield, int x, int y, int radius) {
    // Create visual damage
    damage_shield_visual(shield, x, y, radius);
    
    // Reduce shield health (only for enemy bullets)
    shield->health -= 10;
    if (shield->health <= 0) {
        // Shield destroyed! Create explosion animation at shield center
        float centerX = shield->base.x + shield->base.width / 2.0f;
        float centerY = shield->base.y + shield->base.height / 2.0f;
        
        // Create multiple smaller explosions for better effect
        spawn_explosion(centerX - 8, centerY - 8);
        spawn_explosion(centerX + 8, centerY - 8);
        spawn_explosion(centerX, centerY + 8);
        spawn_explosion(centerX - 4, centerY + 4);
        spawn_explosion(centerX + 4, centerY - 4);
        
        // Play explosion sound
        extern Assets g_assets;
        audio_play_sound(g_assets.explosionSound);
        
        shield->base.active = false;
    }
}

// Legacy function name for compatibility - only creates visual damage
void damage_shield(Shield* shield, int x, int y, int radius) {
    damage_shield_visual(shield, x, y, radius);
}

// Wave configuration functions
int get_wave_rows(int wave) {
    switch(wave) {
        case 1: return WAVE1_ROWS;
        case 2: return WAVE2_ROWS;
        case 3: return WAVE3_ROWS;
        default: return WAVE3_ROWS; // Wave 3+ uses maximum difficulty
    }
}

int get_wave_cols(int wave) {
    switch(wave) {
        case 1: return WAVE1_COLS;
        case 2: return WAVE2_COLS;
        case 3: return WAVE3_COLS;
        default: return WAVE3_COLS; // Wave 3+ uses maximum difficulty
    }
}

float get_wave_speed(int wave) {
    switch(wave) {
        case 1: return WAVE1_SPEED;
        case 2: return WAVE2_SPEED;
        case 3: return WAVE3_SPEED;
        default: return WAVE3_SPEED; // Wave 3+ uses maximum difficulty
    }
}

float get_ufo_speed(int wave) {
    switch(wave) {
        case 1: return UFO_SPEED_WAVE1; // Slow - easy to hit
        case 2: return UFO_SPEED_WAVE2; // Medium speed
        case 3: return UFO_SPEED_WAVE3; // Fast speed (original)
        default: return UFO_SPEED_WAVE3; // Wave 3+ uses maximum speed
    }
}

float get_enemy_bullet_speed(int wave) {
    switch(wave) {
        case 1: return ENEMY_BULLET_SPEED_WAVE1; // 25% slower for easier gameplay
        case 2: return ENEMY_BULLET_SPEED_WAVE2; // Normal speed
        case 3: return ENEMY_BULLET_SPEED_WAVE3; // Normal speed
        default: return ENEMY_BULLET_SPEED_WAVE3; // Wave 3+ uses normal speed
    }
}

void next_wave(void) {
    game->wave++;
    
    // Get wave-specific configurations
    int wave_rows = get_wave_rows(game->wave);
    int wave_cols = get_wave_cols(game->wave);
    float wave_speed = get_wave_speed(game->wave);
    
    printf("Starting Wave %d: %d rows x %d cols = %d enemies\n", 
           game->wave, wave_rows, wave_cols, wave_rows * wave_cols);
    fflush(stdout);
    
    // Set wave speed
    game->alienSpeed = wave_speed;
    
    // Reset aliens with wave-specific count
    game->alienCount = wave_rows * wave_cols;
    
    // Deactivate all aliens first
    for (int row = 0; row < MAX_ALIEN_ROWS; row++) {
        for (int col = 0; col < MAX_ALIEN_COLS; col++) {
            game->aliens[row][col].base.active = false;
        }
    }
    
    // Seed random number generator for consistent positioning within wave
    srand(game->wave * 42);  // Use wave number for consistent randomness
    
    // Random distribution across upper quarter of screen with overlap prevention
    int upper_quarter_height = SCREEN_HEIGHT / 4;
    int margin = 20;
    int min_distance = 40; // Minimum distance between aliens to prevent overlap
    
    for (int row = 0; row < wave_rows; row++) {
        for (int col = 0; col < wave_cols; col++) {
            Alien* alien = &game->aliens[row][col];
            alien->base.active = true;
            
            bool position_found = false;
            int attempts = 0;
            
            // Try to find a non-overlapping position
            while (!position_found && attempts < 100) {
                // Random X position across screen width with margins
                alien->base.x = margin + rand() % (SCREEN_WIDTH - 2 * margin - ALIEN_WIDTH);
                
                // Random Y position in upper quarter of screen
                alien->base.y = margin + rand() % (upper_quarter_height - margin - ALIEN_HEIGHT);
                
                // Check for overlaps with previously placed aliens
                position_found = true;
                for (int check_row = 0; check_row < wave_rows; check_row++) {
                    for (int check_col = 0; check_col < wave_cols; check_col++) {
                        // Skip checking against self and inactive aliens
                        if ((check_row == row && check_col == col) || 
                            !game->aliens[check_row][check_col].base.active) {
                            continue;
                        }
                        
                        // Skip if this alien hasn't been positioned yet
                        if (check_row > row || (check_row == row && check_col >= col)) {
                            continue;
                        }
                        
                        Alien* other_alien = &game->aliens[check_row][check_col];
                        float dx = alien->base.x - other_alien->base.x;
                        float dy = alien->base.y - other_alien->base.y;
                        float distance = sqrt(dx * dx + dy * dy);
                        
                        if (distance < min_distance) {
                            position_found = false;
                            break;
                        }
                    }
                    if (!position_found) break;
                }
                
                attempts++;
            }
            
            // If we couldn't find a non-overlapping position after many attempts,
            // use a grid-based fallback
            if (!position_found) {
                int grid_x = margin + (col * (SCREEN_WIDTH - 2 * margin)) / wave_cols;
                int grid_y = margin + (row * (upper_quarter_height - margin)) / wave_rows;
                alien->base.x = grid_x;
                alien->base.y = grid_y;
            }
            
            // Note: alienCount is already set correctly above, no need to increment
        }
    }
}

void game_update(uint32_t deltaTime) {
    (void)deltaTime;  // Fixed timestep, deltaTime not used
    
    if (game->paused) return;
	
	if(game->phase != game->phase_last)
	{
		game->phase_last = game->phase;
		game->phaseStartTime = SDL_GetTicks();
	}
    
    // Handle intro phase
    if (game->phase == PHASE_INTRO) {
        uint32_t elapsed = SDL_GetTicks() - game->phaseStartTime;
        
        // Phase 1: Spaceship flies up from bottom (0-2 seconds)
        if (elapsed < 2000) {
            float progress = elapsed / 2000.0f;
            game->player.base.y = SCREEN_HEIGHT + PLAYER_HEIGHT - 
                                 (PLAYER_HEIGHT + 40 + PLAYER_HEIGHT) * progress;
        }
        // Phase 2: Wait a moment (2-3 seconds)
        else if (elapsed < 3000) {
            game->player.base.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 40;
        }
        // Phase 3: Aliens descend from top (3-5 seconds)  
        else if (elapsed < 5000) {
            static bool intro_positions_set = false;
            static int target_positions[MAX_ALIEN_ROWS * MAX_ALIEN_COLS][2]; // [x, y] for each alien
            static uint32_t last_phase_start = 0;
            
            // Reset positions if this is a new game/intro
            if (game->phaseStartTime != last_phase_start) {
                intro_positions_set = false;
                last_phase_start = game->phaseStartTime;
            }
            
            // Calculate target positions once at the start of this phase
            if (!intro_positions_set) {
                srand(42); // Fixed seed for consistent intro animation
                int upper_quarter_height = SCREEN_HEIGHT / 4; // Move to upper quarter of screen
                int margin = 20;
                int min_distance = 40; // Reduce distance for smaller area
                
                int wave_rows = get_wave_rows(game->wave);
                int wave_cols = get_wave_cols(game->wave);
                
                for (int row = 0; row < wave_rows; row++) {
                    for (int col = 0; col < wave_cols; col++) {
                        int index = row * wave_cols + col;
                        bool position_found = false;
                        int attempts = 0;
                        
                        // Try to find a non-overlapping position
                        while (!position_found && attempts < 100) {
                            target_positions[index][0] = margin + rand() % (SCREEN_WIDTH - 2 * margin - ALIEN_WIDTH);
                            target_positions[index][1] = margin + rand() % (upper_quarter_height - margin - ALIEN_HEIGHT);
                            
                            // Check for overlaps with previously placed aliens
                            position_found = true;
                            for (int check_row = 0; check_row < row; check_row++) {
                                for (int check_col = 0; check_col < wave_cols; check_col++) {
                                    int check_index = check_row * wave_cols + check_col;
                                    float dx = target_positions[index][0] - target_positions[check_index][0];
                                    float dy = target_positions[index][1] - target_positions[check_index][1];
                                    float distance = sqrt(dx * dx + dy * dy);
                                    
                                    if (distance < min_distance) {
                                        position_found = false;
                                        break;
                                    }
                                }
                                if (!position_found) break;
                            }
                            
                            // Also check against aliens in the same row, previous columns
                            if (position_found && col > 0) {
                                for (int check_col = 0; check_col < col; check_col++) {
                                    int check_index = row * wave_cols + check_col;
                                    float dx = target_positions[index][0] - target_positions[check_index][0];
                                    float dy = target_positions[index][1] - target_positions[check_index][1];
                                    float distance = sqrt(dx * dx + dy * dy);
                                    
                                    if (distance < min_distance) {
                                        position_found = false;
                                        break;
                                    }
                                }
                            }
                            
                            attempts++;
                        }
                        
                        // If we couldn't find a non-overlapping position, use grid fallback
                        if (!position_found) {
                            int grid_x = margin + (col * (SCREEN_WIDTH - 2 * margin)) / wave_cols;
                            int grid_y = margin + (row * (upper_quarter_height - margin)) / wave_rows;
                            target_positions[index][0] = grid_x;
                            target_positions[index][1] = grid_y;
                        }
                    }
                }
                intro_positions_set = true;
            }
            
            float progress = (elapsed - 3000) / 2000.0f;
            if (progress > 1.0f) progress = 1.0f;  // Clamp to ensure aliens reach exact target positions
            
            int wave_rows = get_wave_rows(game->wave);
            int wave_cols = get_wave_cols(game->wave);
            
            for (int row = 0; row < wave_rows; row++) {
                for (int col = 0; col < wave_cols; col++) {
                    Alien* alien = &game->aliens[row][col];
                    alien->base.active = true;
                    
                    int index = row * wave_cols + col;
                    int targetX = target_positions[index][0];
                    int targetY = target_positions[index][1];
                    
                    // Animate from starting position to target position
                    float startY = -ALIEN_HEIGHT - 50 - row * (ALIEN_HEIGHT + 30);
                    alien->base.y = startY + (targetY - startY) * progress;
                    alien->base.x = targetX; // Use target X position directly during animation
                }
            }
        }
        // Phase 4: Start the game
        else {
            game->phase = PHASE_PLAYING;
            game->player.canFire = true;
            game->player.base.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 40;  // Ensure proper positioning
            int wave_rows = get_wave_rows(game->wave);
            int wave_cols = get_wave_cols(game->wave);
            game->alienCount = wave_rows * wave_cols;
            // Aliens keep their final positions from the intro animation
            // No need to reposition them
        }
        return;  // Don't run normal game logic during intro
    }
    
    if (game->gameOver) {
        game->phase = PHASE_GAME_OVER;
        return;
    }
    
    // Update player fire cooldown - only allow firing when no bullets are active
    if (!game->player.canFire) {
        bool anyBulletActive = false;
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (game->playerBullets[i].base.active) {
                anyBulletActive = true;
                break;
            }
        }
        
        if (!anyBulletActive) {
            game->player.canFire = true;
        }
    }
    
    // Update player invincibility status
    if (game->player.isInvincible) {
        uint32_t currentTime = SDL_GetTicks();
        if (currentTime - game->player.invincibilityStart >= game->player.invincibilityDuration) {
            // Invincibility period ended
            game->player.isInvincible = false;
            game->player.base.sprite = g_assets.player; // Restore normal sprite
        }
    }
    
    // Automatic firing - fire continuously when able
    if (game->player.canFire && game->phase == PHASE_PLAYING && !game->gameOver) {
        spawn_player_bullet();
    }
    
    // Move player horizontally with screen wrapping
    if (game->player.moveDir != 0) {
        game->player.base.x += game->player.moveDir * PLAYER_SPEED;
        
        // Horizontal screen wrapping - smooth teleportation
        // Allow spaceship to be partially visible during transition for smoother effect
        if (game->player.base.x < -PLAYER_WIDTH) {
            // Spaceship completely off left edge, wrap to right side
            game->player.base.x = SCREEN_WIDTH;
        }
        else if (game->player.base.x > SCREEN_WIDTH) {
            // Spaceship completely off right edge, wrap to left side  
            game->player.base.x = -PLAYER_WIDTH;
        }
    }
    
    // Move player vertically with screen wrapping
    if (game->player.moveDirY != 0) {
        game->player.base.y += game->player.moveDirY * PLAYER_SPEED;
        
        // Vertical screen wrapping - smooth teleportation
        // Leave space for UI at top (50 pixels) and allow partial visibility
        if (game->player.base.y < 50 - PLAYER_HEIGHT) {
            // Spaceship completely off top edge, wrap to bottom
            game->player.base.y = SCREEN_HEIGHT;
        }
        else if (game->player.base.y > SCREEN_HEIGHT) {
            // Spaceship completely off bottom edge, wrap to top (below UI)
            game->player.base.y = 50 - PLAYER_HEIGHT;
        }
    }
    
    // Update player sprite based on horizontal direction
    if (game->player.moveDir < 0) {
        game->player.base.sprite = g_assets.playerLeft;
    } else if (game->player.moveDir > 0) {
        game->player.base.sprite = g_assets.playerRight;
    } else {
        game->player.base.sprite = g_assets.player;
    }
    
    // Update player bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet* bullet = &game->playerBullets[i];
        if (bullet->base.active) {
            // Move bullet
            bullet->base.y -= BULLET_SPEED;
            
            // Check if bullet is off screen
            if (bullet->base.y < 0) {
                bullet->base.active = false;
                continue;
            }
            
            // Check collision with aliens - 100% accurate collision detection
            bool bulletHit = false;
            int wave_rows = get_wave_rows(game->wave);
            int wave_cols = get_wave_cols(game->wave);
            
            for (int row = 0; row < wave_rows && !bulletHit; row++) {
                for (int col = 0; col < wave_cols && !bulletHit; col++) {
                    Alien* alien = &game->aliens[row][col];
                    
                    // Only check collision with active aliens and bullets
                    if (!alien->base.active || !bullet->base.active) continue;
                    
                    // Use precise AABB collision detection
                    if (check_collision(&bullet->base, &alien->base)) {
                        // Collision detected - remove both bullet and alien immediately
                        bullet->base.active = false;
                        
                        // Create explosion at alien position
                        spawn_explosion(alien->base.x + alien->base.width/2, 
                                      alien->base.y + alien->base.height/2);
                        
                        alien->base.active = false;
                        game->alienCount--;
                        game->score += alien->points;
                        
                        // Increase alien speed as their numbers decrease
                        game->alienSpeed += 0.02f; // Small speed increase with each alien destroyed
                        
                        // Update high score
                        if (game->score > game->highScore) {
                            game->highScore = game->score;
                        }
                        
                        // Play explosion sound
                        audio_play_sound(g_assets.explosionSound);
                        
                        bulletHit = true; // Mark that bullet hit something
                        break; // Exit inner loop immediately
                    }
                }
            }
            
            // If bullet hit something, skip to next bullet
            if (bulletHit) {
                continue;
            }
            
            // Check collision with UFO
            if (!bulletHit && game->ufo.base.active && bullet->base.active && 
                check_collision(&bullet->base, &game->ufo.base)) {
                bullet->base.active = false;
                
                // Create explosion at UFO position
                spawn_explosion(game->ufo.base.x + game->ufo.base.width/2, 
                              game->ufo.base.y + game->ufo.base.height/2);
                
                // Health restoration logic - add 1 life if not at maximum
                bool healthRestored = false;
                if (game->player.lives < PLAYER_LIVES) {
                    game->player.lives++;
                    healthRestored = true;
                    printf("UFO hit! Health restored: %d/%d lives\n", game->player.lives, PLAYER_LIVES);
                    fflush(stdout);
                }
                
                // Set up bonus point display (show health if restored, otherwise show points)
                game->showBonus = true;
                if (healthRestored) {
                    game->bonusPoints = 1; // Will display as "+1 LIFE!" 
                } else {
                    game->bonusPoints = UFO_POINTS; // Display as "+200 BONUS!"
                }
                game->bonusX = game->ufo.base.x;
                game->bonusY = game->ufo.base.y;
                game->bonusStartTime = SDL_GetTicks();
                
                game->ufo.base.active = false;
                game->score += UFO_POINTS;
                
                // Update high score
                if (game->score > game->highScore) {
                    game->highScore = game->score;
                }
                
                // Play explosion sound
                audio_play_sound(g_assets.explosionSound);
                audio_stop_sound(g_assets.ufoSound);
                
                bulletHit = true;
            }
            
            // Check collision with shields  
            if (!bulletHit) {
                for (int j = 0; j < SHIELD_COUNT && !bulletHit; j++) {
                    Shield* shield = &game->shields[j];
                    if (shield->base.active && bullet->base.active && 
                        check_collision(&bullet->base, &shield->base)) {
                        bullet->base.active = false;
                        
                        // Damage shield at impact point
                        int relX = (int)(bullet->base.x - shield->base.x + BULLET_WIDTH / 2);
                        int relY = (int)(bullet->base.y - shield->base.y + BULLET_HEIGHT / 2);
                        damage_shield(shield, relX, relY, 8);
                        
                        bulletHit = true;
                    }
                }
            }
        }
    }
    
    // Update alien movement - smoother movement every frame
    bool hitEdge = false;
    int wave_rows = get_wave_rows(game->wave);
    int wave_cols = get_wave_cols(game->wave);
    
    // Calculate smooth movement speeds (per frame at 60fps)
    float horizontalSpeed = game->alienSpeed * 0.6f;  // Reasonable horizontal movement speed
    
    for (int row = 0; row < wave_rows; row++) {
        for (int col = 0; col < wave_cols; col++) {
            Alien* alien = &game->aliens[row][col];
            if (alien->base.active) {
                // Smooth horizontal movement (side to side) every frame
                alien->base.x += game->alienDirection * horizontalSpeed;
                
                // Clamp alien position to stay completely within screen bounds
                // Note: Aliens are rendered at 0.5f scale, so visual width = ALIEN_WIDTH * 0.5f = 4 pixels
                float visualAlienWidth = ALIEN_WIDTH * 0.5f;  // Account for 50% scaling (4 pixels)
                
                // Left boundary: alien must stay completely on screen
                if (alien->base.x < 0) {
                    alien->base.x = 0;  // Left boundary: X = 0
                    hitEdge = true;
                }
                
                // Right boundary: alien must stay completely on screen  
                float rightBoundary = SCREEN_WIDTH - visualAlienWidth;  // 640 - 4 = 636
                if (alien->base.x > rightBoundary) {
                    alien->base.x = rightBoundary;  // Right boundary: X = 636
                    hitEdge = true;
                }
                
                // Smooth downward descent - gradually move towards bottom
                // Wave-specific dropping speed: Wave 3 drops slower than Wave 2
                float dropSpeed;
                if (game->wave == 1) {
                    dropSpeed = 0.08f;     // Very slow for playability
                } else if (game->wave == 2) {
                    dropSpeed = 0.12f;     // Medium speed
                } else {
                    dropSpeed = 0.10f;     // Wave 3+ slower than Wave 2
                }
                alien->base.y += dropSpeed;
                
                // Animate alien at a slower rate for smoother animation
                uint32_t currentTime = SDL_GetTicks();
                if (currentTime - alien->lastAnimTime > 500) {  // Animate every 500ms
                    alien->animFrame = !alien->animFrame;
                    alien->lastAnimTime = currentTime;
                }
            }
        }
    }
    
    // Handle edge drop with smooth animation
    if (hitEdge && !game->alienDropping) {
        // Start a new smooth drop animation
        game->alienDirection *= -1;
        game->alienDropping = true;
        game->dropProgress = 0.0f;
        game->dropStartTime = SDL_GetTicks();
        
        // Increase speed
        game->alienSpeed += 0.03f; // Small speed increase when hitting edges
    }
    
    // Update smooth drop animation
    if (game->alienDropping) {
        uint32_t currentTime = SDL_GetTicks();
        uint32_t dropDuration = 300; // 300ms for smooth drop animation
        uint32_t elapsed = currentTime - game->dropStartTime;
        
        // Calculate smooth drop progress (0.0 to 1.0)
        game->dropProgress = (float)elapsed / dropDuration;
        
        if (game->dropProgress >= 1.0f) {
            // Drop animation complete - permanently update alien positions
            game->dropProgress = 1.0f;
            for (int row = 0; row < wave_rows; row++) {
                for (int col = 0; col < wave_cols; col++) {
                    Alien* alien = &game->aliens[row][col];
                    if (alien->base.active) {
                        alien->base.y += ALIEN_DROP_DISTANCE;
                    }
                }
            }
            game->alienDropping = false;
        }
        
        // Apply smooth drop movement to all active aliens
        float dropOffset = game->dropProgress * ALIEN_DROP_DISTANCE;
        for (int row = 0; row < wave_rows; row++) {
            for (int col = 0; col < wave_cols; col++) {
                Alien* alien = &game->aliens[row][col];
                if (alien->base.active) {
                    // Check if aliens reached the ground during drop
                    if (alien->base.y + dropOffset + ALIEN_HEIGHT >= 
                        SCREEN_HEIGHT - 60) {  // 60 pixels from bottom = ground level
                        game->gameOver = true;
                        game->victory = false;  // Mark as defeat
                        game->phase = PHASE_GAME_OVER;
                        // Play explosion sound
                        audio_play_sound(g_assets.explosionSound);
                        return;  // Stop processing to prevent freeze
                    }
                }
            }
        }
    }
    
    // Spawn alien bullets more frequently for testing consecutive bullets
    uint32_t currentTime = SDL_GetTicks();
    if (currentTime - game->lastAlienFire > 300) {  // Fire every 300ms (more frequent)
        spawn_alien_bullet();
        game->lastAlienFire = currentTime;
    }
    
    // Update alien bullets
    int activeBulletCount = 0;
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (game->alienBullets[i].base.active) activeBulletCount++;
    }
    if (activeBulletCount > 1) {
        //printf("Multiple alien bullets active: %d\n", activeBulletCount);
        //fflush(stdout);
    }
    
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        Bullet* bullet = &game->alienBullets[i];
        if (bullet->base.active) {
            bullet->base.y += get_enemy_bullet_speed(game->wave);  // Wave-specific speed
            
            // Check if bullet is off screen
            if (bullet->base.y > SCREEN_HEIGHT) {
                bullet->base.active = false;
                continue;
            }
            
            // Check collision with player (only if not invincible)
            if (check_collision(&bullet->base, &game->player.base)) {
                if (!game->player.isInvincible) {
                    // Player can be hit - apply damage and start invincibility
                    printf("Enemy bullet hit player! Bullet %d at (%.1f,%.1f), Player at (%.1f,%.1f), Lives: %d->%d\n", 
                           i, bullet->base.x, bullet->base.y, game->player.base.x, game->player.base.y, 
                           game->player.lives, game->player.lives - 1);
                    fflush(stdout);
                    
                    bullet->base.active = false;
                    game->player.lives--;
                    
                    // Start invincibility period
                    game->player.isInvincible = true;
                    game->player.invincibilityStart = SDL_GetTicks();
                    
                    // Play explosion sound
                    audio_play_sound(g_assets.explosionSound);
                    
                    // Show damaged sprite briefly
                    game->player.base.sprite = g_assets.playerDamaged;
                    
                    if (game->player.lives <= 0) {
                        game->gameOver = true;
                        game->victory = false;  // Mark as defeat
                        game->phase = PHASE_GAME_OVER;
                    }
                } else {
                    // Player is invincible - bullet passes through
                    // Don't deactivate bullet, don't damage player
                    printf("Bullet passed through invincible player!\n");
                    fflush(stdout);
                }
                continue;
            }
            
            // Check collision with shields
            for (int j = 0; j < SHIELD_COUNT; j++) {
                Shield* shield = &game->shields[j];
                if (check_collision(&bullet->base, &shield->base)) {
                    bullet->base.active = false;
                    
                    // Damage shield at impact point (alien bullets damage health)
                    int relX = (int)(bullet->base.x - shield->base.x + BULLET_WIDTH / 2);
                    int relY = (int)(bullet->base.y - shield->base.y + BULLET_HEIGHT / 2);
                    damage_shield_health(shield, relX, relY, 8);
                    break;
                }
            }
        }
    }
    
    // Update UFO
    if (game->ufo.base.active) {
        float currentUfoSpeed = get_ufo_speed(game->wave);
        game->ufo.base.x += game->ufo.direction * currentUfoSpeed;
        
        // Remove UFO if off screen
        if (game->ufo.base.x < -UFO_WIDTH || game->ufo.base.x > SCREEN_WIDTH) {
            game->ufo.base.active = false;
            audio_stop_sound(g_assets.ufoSound);
        }
    } else {
        // Try to spawn UFO
        spawn_ufo();
    }
    
    // Update bonus point display
    if (game->showBonus) {
        uint32_t elapsed = SDL_GetTicks() - game->bonusStartTime;
        if (elapsed > 2000) {  // Show bonus for 2 seconds
            game->showBonus = false;
        }
    }
    
    // Update explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        Explosion* explosion = &game->explosions[i];
        if (explosion->base.active) {
            uint32_t elapsed = currentTime - explosion->startTime;
            
            // Update animation frame
            if (currentTime - explosion->lastFrameTime > 100) {  // Change frame every 100ms
                explosion->animFrame++;
                explosion->lastFrameTime = currentTime;
            }
            
            // Remove explosion when duration is over
            if (elapsed >= explosion->duration) {
                explosion->base.active = false;
            }
        }
    }
    
    // Check if wave is complete
    if (game->alienCount == 0) {
        // Check if this was the final wave (Wave 3)
        if (game->wave == 3) {
            // Game completed successfully after finishing Wave 3!
            game->gameOver = true;
            game->victory = true;  // Mark as victory
            game->phase = PHASE_GAME_OVER;
            // Play victory sound
            audio_play_sound(g_assets.explosionSound);
        } else {
            // Continue to next wave (Wave 1 → Wave 2 → Wave 3)
            printf("Wave %d completed, advancing to Wave %d\n", game->wave, game->wave + 1);
            fflush(stdout);
            next_wave();
        }
    }
}

void game_handle_input(int key, bool pressed) {
    // Skip input during intro except for start button to skip
    if (game->phase == PHASE_INTRO && key != INPUT_START) {
        return;
    }
    
    switch (key) {
        case INPUT_LEFT:
            if (game->phase == PHASE_PLAYING) {
                game->player.moveDir = pressed ? -1 : 
                    (game->player.moveDir == -1 ? 0 : game->player.moveDir);
            }
            break;
            
        case INPUT_RIGHT:
            if (game->phase == PHASE_PLAYING) {
                game->player.moveDir = pressed ? 1 : 
                    (game->player.moveDir == 1 ? 0 : game->player.moveDir);
            }
            break;
            
        case INPUT_UP:
            if (game->phase == PHASE_PLAYING) {
                game->player.moveDirY = pressed ? -1 : 
                    (game->player.moveDirY == -1 ? 0 : game->player.moveDirY);
            }
            break;
            
        case INPUT_DOWN:
            if (game->phase == PHASE_PLAYING) {
                game->player.moveDirY = pressed ? 1 : 
                    (game->player.moveDirY == 1 ? 0 : game->player.moveDirY);
            }
            break;
            
        case INPUT_FIRE:
            // Note: Automatic firing is now handled in game_update during gameplay
            // No manual input needed for firing
            break;
            
        case INPUT_START:
            if (pressed) {
                if (game->phase == PHASE_INTRO) {
                    // Skip intro
                    game->phase = PHASE_PLAYING;
                    game->player.canFire = true;
                    game->player.base.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 40;
                    int wave_rows = get_wave_rows(game->wave);
                    int wave_cols = get_wave_cols(game->wave);
                    game->alienCount = wave_rows * wave_cols;
                    
                    // Seed random number generator for consistent positioning
                    srand(game->wave * 42);
                    
                    // Random distribution across upper quarter of screen with overlap prevention
                    int upper_quarter_height = SCREEN_HEIGHT / 4;
                    int margin = 20;
                    int min_distance = 40; // Minimum distance between aliens to prevent overlap
                    
                    // Position all aliens randomly
                    for (int row = 0; row < wave_rows; row++) {
                        for (int col = 0; col < wave_cols; col++) {
                            Alien* alien = &game->aliens[row][col];
                            alien->base.active = true;
                            
                            bool position_found = false;
                            int attempts = 0;
                            
                            // Try to find a non-overlapping position
                            while (!position_found && attempts < 100) {
                                // Random X position across screen width with margins
                                alien->base.x = margin + rand() % (SCREEN_WIDTH - 2 * margin - ALIEN_WIDTH);
                                
                                // Random Y position in upper quarter of screen
                                alien->base.y = margin + rand() % (upper_quarter_height - margin - ALIEN_HEIGHT);
                                
                                // Check for overlaps with previously placed aliens
                                position_found = true;
                                for (int check_row = 0; check_row < wave_rows; check_row++) {
                                    for (int check_col = 0; check_col < wave_cols; check_col++) {
                                        // Skip checking against self and inactive aliens
                                        if ((check_row == row && check_col == col) || 
                                            !game->aliens[check_row][check_col].base.active) {
                                            continue;
                                        }
                                        
                                        // Skip if this alien hasn't been positioned yet
                                        if (check_row > row || (check_row == row && check_col >= col)) {
                                            continue;
                                        }
                                        
                                        Alien* other_alien = &game->aliens[check_row][check_col];
                                        float dx = alien->base.x - other_alien->base.x;
                                        float dy = alien->base.y - other_alien->base.y;
                                        float distance = sqrt(dx * dx + dy * dy);
                                        
                                        if (distance < min_distance) {
                                            position_found = false;
                                            break;
                                        }
                                    }
                                    if (!position_found) break;
                                }
                                
                                attempts++;
                            }
                            
                            // If we couldn't find a non-overlapping position, use grid fallback
                            if (!position_found) {
                                int grid_x = margin + (col * (SCREEN_WIDTH - 2 * margin)) / wave_cols;
                                int grid_y = margin + (row * (upper_quarter_height - margin)) / wave_rows;
                                alien->base.x = grid_x;
                                alien->base.y = grid_y;
                            }
                        }
                    }
                } else if (game->gameOver) {
                    
                } else {
                    game->paused = !game->paused;
                }
            }
            break;
    }
}

void game_render(void) {
	
    // Check if we should show game over screen
    if (game->gameOver || game->phase == PHASE_GAME_OVER) {
        // Draw black background for game over screen
        renderer_clear(COLOR_BLACK);
        
        // Draw a white border to make sure the screen is visible
        draw_rect(10, 10, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 20, COLOR_WHITE);
        
        // Draw game over messages
        if (game->victory) {
            // Victory screen
            draw_text("CONGRATULATIONS!", SCREEN_WIDTH / 2 - 80, 
                     SCREEN_HEIGHT / 2 - 80, COLOR_YELLOW);
            draw_text("YOU HAVE COMPLETED ALL WAVES!", SCREEN_WIDTH / 2 - 120, 
                     SCREEN_HEIGHT / 2 - 60, COLOR_WHITE);
            draw_text("EARTH IS SAVED!", SCREEN_WIDTH / 2 - 56, 
                     SCREEN_HEIGHT / 2 - 40, COLOR_GREEN);
            
            char finalScore[64];
            snprintf(finalScore, sizeof(finalScore), "FINAL SCORE: %06d", game->score);
            draw_text(finalScore, SCREEN_WIDTH / 2 - 72, 
                     SCREEN_HEIGHT / 2 - 10, COLOR_WHITE);
                     
            char highScore[64];
            snprintf(highScore, sizeof(highScore), "HIGH SCORE: %06d", game->highScore);
            draw_text(highScore, SCREEN_WIDTH / 2 - 68, 
                     SCREEN_HEIGHT / 2 + 10, COLOR_YELLOW);
        } else {
            // Game over screen (defeat) - Clean and simple for maximum visibility
            
            // Draw clear white text on black background
            draw_text("GAME OVER", SCREEN_WIDTH / 2 - 36, SCREEN_HEIGHT / 2 - 40, COLOR_WHITE);
            
            char scoreText[64];
            snprintf(scoreText, sizeof(scoreText), "SCORE= %d", game->score);
            draw_text(scoreText, SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2 - 10, COLOR_WHITE);
                     
            char highScore[64];
            snprintf(highScore, sizeof(highScore), "HIGH SCORE= %d", game->highScore);
            draw_text(highScore, SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 + 10, COLOR_WHITE);
        }
        
        // No restart instruction text displayed
        return;  // Don't draw the game scene
    }
    
    // Draw starfield background (only when game is active)
    if (g_assets.starBackground && g_assets.starBackground->pixels) {
        // Tile the background
        for (int y = 0; y < SCREEN_HEIGHT; y += g_assets.starBackground->height) {
            for (int x = 0; x < SCREEN_WIDTH; x += g_assets.starBackground->width) {
                draw_sprite(g_assets.starBackground, x, y);
            }
        }
    }
    
    // Draw intro story text if in intro phase
    if (game->phase == PHASE_INTRO) {
        uint32_t elapsed = SDL_GetTicks() - game->phaseStartTime;
        
        if (elapsed < 2000) {
            draw_text("FLORIDA IS UNDER ATTACK!", 
                     SCREEN_WIDTH / 2 - 88, SCREEN_HEIGHT / 3, COLOR_WHITE);
            draw_text("YOUR MISSION: DEFEND THE SWAMP", 
                     SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 3 + 20, COLOR_WHITE);
        } else if (elapsed < 3000) {
            draw_text("GET READY!", 
                     SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 3, COLOR_YELLOW);
        } else if (elapsed < 5000) {
            draw_text("GATOR INVASION INCOMING!", 
                     SCREEN_WIDTH / 2 - 96, SCREEN_HEIGHT / 3, COLOR_RED);
        }
        
        // Always show skip instruction during intro
        draw_text("PRESS START TO SKIP", 
                 SCREEN_WIDTH / 2 - 76, SCREEN_HEIGHT - 30, COLOR_WHITE);
    }
    
    // Draw player (scaled down for better visibility)
    if (game->player.base.active) {
        bool shouldDraw = true;
        
        // Add blinking effect during invincibility
        if (game->player.isInvincible) {
            uint32_t currentTime = SDL_GetTicks();
            uint32_t timeSinceHit = currentTime - game->player.invincibilityStart;
            
            // Blink every 150ms (fast blinking for clear visual feedback)
            if ((timeSinceHit / 150) % 2 == 0) {
                shouldDraw = false; // Don't draw (invisible phase of blink)
            }
        }
        
        if (shouldDraw) {
            draw_sprite_scaled(game->player.base.sprite, 
                              (int)game->player.base.x, (int)game->player.base.y, 0.75f);
        }
    }
    
    // Draw aliens (with smooth drop animation)
    int wave_rows = get_wave_rows(game->wave);
    int wave_cols = get_wave_cols(game->wave);
    
    // Calculate drop offset for smooth animation
    float dropOffset = 0.0f;
    if (game->alienDropping) {
        dropOffset = game->dropProgress * ALIEN_DROP_DISTANCE;
    }
    
    for (int row = 0; row < wave_rows; row++) {
        for (int col = 0; col < wave_cols; col++) {
            Alien* alien = &game->aliens[row][col];
            if (alien->base.active) {
                // Apply smooth drop offset during animation
                int renderY = (int)(alien->base.y + dropOffset);
                draw_sprite_scaled(alien->base.sprite,
                           (int)alien->base.x, renderY, 0.5f);
            }
        }
    }
    
    // Draw shields as proper square boxes
    for (int i = 0; i < SHIELD_COUNT; i++) {
        Shield* shield = &game->shields[i];
        if (shield->base.active) {
            // Draw shield as a solid square with damage holes
            for (int y = 0; y < shield->base.height; y++) {
                for (int x = 0; x < shield->base.width; x++) {
                    int index = y * shield->base.width + x;
                    
                    // Only draw pixels that aren't damaged
                    if (shield->damageMap[index] == 0) {
                        // Create a proper bunker/shield shape
                        bool shouldDraw = true;
                        
                        // Create an opening at the bottom center for the player to shoot through
                        int openingWidth = shield->base.width / 3;  // Proportional to shield size
                        int openingHeight = shield->base.height / 3;
                        int openingStartX = shield->base.width / 2 - openingWidth / 2;
                        int openingStartY = shield->base.height - openingHeight;
                        
                        if (x >= openingStartX && x < openingStartX + openingWidth &&
                            y >= openingStartY) {
                            shouldDraw = false;  // Create opening
                        }
                        
                        // Create rounded top corners for a more shield-like appearance
                        int cornerSize = shield->base.width / 6;  // Proportional corner size
                        
                        // Top-left rounded corner
                        if (x < cornerSize && y < cornerSize) {
                            int dx = cornerSize - x;
                            int dy = cornerSize - y;
                            if (dx * dx + dy * dy > cornerSize * cornerSize / 2) {
                                shouldDraw = false;
                            }
                        }
                        
                        // Top-right rounded corner  
                        if (x >= shield->base.width - cornerSize && y < cornerSize) {
                            int dx = x - (shield->base.width - cornerSize - 1);
                            int dy = cornerSize - y;
                            if (dx * dx + dy * dy > cornerSize * cornerSize / 2) {
                                shouldDraw = false;
                            }
                        }
                        
                        if (shouldDraw) {
                            // Use bright green color for shields
                            draw_pixel(
                                      (int)(shield->base.x + x),
                                      (int)(shield->base.y + y), RGB(0, 255, 0));
                        }
                    }
                }
            }
            
            // Draw health indicator above shield (adjusted for new health scale)
            int healthWidth = (shield->health * shield->base.width) / 20;  // Changed from 100 to 20
            for (int x = 0; x < healthWidth; x++) {
                uint32_t healthColor = shield->health > 10 ? RGB(0, 255, 0) :   // Green if > 50% (>10)
                                      shield->health > 5 ? RGB(255, 255, 0) :   // Yellow if > 25% (>5)
                                      RGB(255, 0, 0);                          // Red if <= 25% (<=5)
                draw_pixel(
                          (int)(shield->base.x + x),
                          (int)(shield->base.y - 5), healthColor);
            }
        }
    }
    
    // Draw bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet* bullet = &game->playerBullets[i];
        if (bullet->base.active) {
            draw_sprite(bullet->base.sprite,
                       (int)bullet->base.x, (int)bullet->base.y);
        }
    }
    
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        Bullet* bullet = &game->alienBullets[i];
        if (bullet->base.active) {
            draw_sprite(bullet->base.sprite,
                       (int)bullet->base.x, (int)bullet->base.y);
        }
    }
    
    // Draw UFO (scaled down to be visually smaller)
    if (game->ufo.base.active) {
        draw_sprite_scaled(game->ufo.base.sprite,
                          (int)game->ufo.base.x, (int)game->ufo.base.y, 0.4f);
    }
    
    // Draw explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        Explosion* explosion = &game->explosions[i];
        if (explosion->base.active) {
            // Draw animated explosion effect
            int centerX = (int)explosion->base.x;
            int centerY = (int)explosion->base.y;
            int frame = explosion->animFrame;
            
            // Create expanding circle effect with different colors based on frame
            int radius = (frame % 5) + 3;  // Radius from 3 to 7 pixels
            uint32_t color = 0;
            
            switch (frame % 4) {
                case 0: color = RGB(255, 255, 255); break;  // White
                case 1: color = RGB(255, 255, 0); break;    // Yellow
                case 2: color = RGB(255, 128, 0); break;    // Orange
                case 3: color = RGB(255, 0, 0); break;      // Red
            }
            
            // Draw explosion as expanding circles
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    if (dx * dx + dy * dy <= radius * radius) {
                        int px = centerX + dx;
                        int py = centerY + dy;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                            draw_pixel(px, py, color);
                        }
                    }
                }
            }
        }
    }
    
    // Draw UI
    char scoreText[64];
    snprintf(scoreText, sizeof(scoreText), "SCORE: %06d", game->score);
    draw_text(scoreText, 10, 10, COLOR_WHITE);
    
    snprintf(scoreText, sizeof(scoreText), "HIGH: %06d", game->highScore);
    draw_text(scoreText, SCREEN_WIDTH - 120, 10, COLOR_WHITE);
    
    snprintf(scoreText, sizeof(scoreText), "WAVE: %d", game->wave);
    draw_text(scoreText, SCREEN_WIDTH / 2 - 30, 10, COLOR_WHITE);
    
    // Draw lives
    for (int i = 0; i < game->player.lives; i++) {
        if (g_assets.life) {
            draw_sprite_scaled(g_assets.life, 
                             10 + i * 40, SCREEN_HEIGHT - 40, 0.5f);
        }
    }
    
    // Draw invincibility indicator
    if (game->player.isInvincible) {
        uint32_t currentTime = SDL_GetTicks();
        uint32_t timeRemaining = game->player.invincibilityDuration - 
                                (currentTime - game->player.invincibilityStart);
        
        if (timeRemaining > 0) {
            char invincText[32];
            snprintf(invincText, sizeof(invincText), "SHIELD: %.1fs", timeRemaining / 1000.0f);
            draw_text(invincText, 10, SCREEN_HEIGHT - 60, COLOR_YELLOW);
        }
    }
    
    // Draw bonus text when UFO is hit
    if (game->showBonus) {
        char bonusText[64];
        if (game->bonusPoints == 1) {
            // Health restoration message
            snprintf(bonusText, sizeof(bonusText), "+1 LIFE!");
            draw_text(bonusText, (int)game->bonusX, (int)game->bonusY - 20, COLOR_GREEN);
        } else {
            // Bonus points message
            snprintf(bonusText, sizeof(bonusText), "+%d BONUS!", game->bonusPoints);
            draw_text(bonusText, (int)game->bonusX, (int)game->bonusY - 20, COLOR_YELLOW);
        }
    }
    
    // Draw pause message (game over is handled at top of function)
    if (game->paused) {
        draw_text("PAUSED", SCREEN_WIDTH / 2 - 24, 
                 SCREEN_HEIGHT / 2, COLOR_YELLOW);
    }
}

bool game_isgameover(void)
{
	return (game->phase == PHASE_GAME_OVER) && (game->phase_last == PHASE_GAME_OVER) && (SDL_GetTicks() > game->phaseStartTime + 5000);
}
