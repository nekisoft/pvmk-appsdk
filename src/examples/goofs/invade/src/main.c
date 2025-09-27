#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "renderer.h"
#include "audio.h"
#include "game.h"
#include "input.h"
#include "assets.h"

// Global instances
Renderer g_renderer;
Input g_input;
Audio g_audio;
Assets g_assets;
GameState g_gameState;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    // Initialize SDL
    printf("Initializing SDL...\n");
    fflush(stdout);
    // SDL3 returns true (1) on success, false (0) on failure
    int sdl_result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);
    printf("SDL_Init returned: %d (1=success, 0=failure)\n", sdl_result);
    fflush(stdout);
    if (!sdl_result) {
        const char* error = SDL_GetError();
        fprintf(stderr, "SDL initialization failed: %s\n", error ? error : "Unknown error");
        fflush(stderr);
        return 1;
    }
    printf("SDL initialized successfully\n");
    fflush(stdout);
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Initialize renderer
    printf("Initializing renderer...\n");
    if (!renderer_init(&g_renderer, "Space Invaders", SCREEN_WIDTH, SCREEN_HEIGHT)) {
        fprintf(stderr, "Failed to initialize renderer\n");
        SDL_Quit();
        return 1;
    }
    printf("Renderer initialized successfully\n");
    
    // Initialize input
    input_init(&g_input);
    
    // Initialize audio
    if (!audio_init(&g_audio)) {
        fprintf(stderr, "Failed to initialize audio\n");
        renderer_cleanup(&g_renderer);
        SDL_Quit();
        return 1;
    }
    
    // Load assets
    if (!assets_init(&g_assets)) {
        fprintf(stderr, "Failed to load assets\n");
        audio_cleanup(&g_audio);
        renderer_cleanup(&g_renderer);
        SDL_Quit();
        return 1;
    }
    
    // Initialize game state
    game_init(&g_gameState);
    
    // Set background music
    audio_set_bgm(&g_audio, g_assets.bgMusic);
    
    // Main game loop
    bool running = true;
    uint32_t lastTime = SDL_GetTicks();
    uint32_t frameTime = 0;
    uint32_t accumulator = 0;
    
    while (running) {
        uint32_t currentTime = SDL_GetTicks();
        frameTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Cap frame time to prevent spiral of death
        if (frameTime > 250) {
            frameTime = 250;
        }
        
        accumulator += frameTime;
        
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            input_handle_event(&g_input, &event);
        }
        
        // Update input state
        input_update(&g_input);
        
        // Check for quit
        if (input_is_pressed(&g_input, INPUT_QUIT)) {
            running = false;
        }
        
        // Fixed timestep update
        while (accumulator >= FRAME_TIME_MS) {
            // Handle game input
            game_handle_input(&g_gameState, INPUT_LEFT, input_is_held(&g_input, INPUT_LEFT));
            game_handle_input(&g_gameState, INPUT_RIGHT, input_is_held(&g_input, INPUT_RIGHT));
            game_handle_input(&g_gameState, INPUT_UP, input_is_held(&g_input, INPUT_UP));
            game_handle_input(&g_gameState, INPUT_DOWN, input_is_held(&g_input, INPUT_DOWN));
            if (input_is_pressed(&g_input, INPUT_FIRE)) {
                game_handle_input(&g_gameState, INPUT_FIRE, true);
            }
            if (input_is_pressed(&g_input, INPUT_START)) {
                game_handle_input(&g_gameState, INPUT_START, true);
            }
            
            // Update game logic
            game_update(&g_gameState, FRAME_TIME_MS);
            
            // Update audio
            audio_update(&g_audio);
            
            accumulator -= FRAME_TIME_MS;
        }
        
        // Render
        renderer_clear(&g_renderer, COLOR_BLACK);
        game_render(&g_gameState, g_renderer.framebuffer);
        renderer_present(&g_renderer);
        
        // Frame rate limiter
        uint32_t frameDuration = SDL_GetTicks() - currentTime;
        if (frameDuration < FRAME_TIME_MS) {
            SDL_Delay(FRAME_TIME_MS - frameDuration);
        }
    }
    
    // Cleanup
    game_cleanup(&g_gameState);
    assets_cleanup(&g_assets);
    audio_cleanup(&g_audio);
    input_cleanup(&g_input);
    renderer_cleanup(&g_renderer);
    SDL_Quit();
    
    return 0;
}
