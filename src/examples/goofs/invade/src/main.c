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
#include "startscreen.h"

// Global instances
Assets g_assets;


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
    if (!renderer_init("Chasin' Gators", SCREEN_WIDTH, SCREEN_HEIGHT)) {
        fprintf(stderr, "Failed to initialize renderer\n");
    }
    printf("Renderer initialized successfully\n");
    
    // Initialize input
    input_init();
    
    // Initialize audio
    if (!audio_init()) {
        fprintf(stderr, "Failed to initialize audio\n");
        return 1;
    }
    
    // Load assets
    if (!assets_init(&g_assets)) {
        fprintf(stderr, "Failed to load assets\n");
        return 1;
    }
    
    while(1)
    {
	    //Show "press start" screen
	    startscreen();
	    
	    // Initialize game state
	    game_init();
	    
	    // Set background music
	    audio_set_bgm(g_assets.bgMusic);
	    
	    // Main game loop
	    uint32_t lastTime = SDL_GetTicks();
	    uint32_t frameTime = 0;
	    uint32_t accumulator = 0;
	    
	    while (1) {
		uint32_t currentTime = SDL_GetTicks();
		frameTime = currentTime - lastTime;
		lastTime = currentTime;
		
		// Cap frame time to prevent spiral of death
		if (frameTime > 250) {
		    frameTime = 250;
		}
		
		accumulator += frameTime;
		
		// Update input state
		input_update();
		
		// Fixed timestep update
		while (accumulator >= FRAME_TIME_MS) {
		    // Handle game input
		    game_handle_input(INPUT_LEFT, input_is_held(INPUT_LEFT));
		    game_handle_input(INPUT_RIGHT, input_is_held(INPUT_RIGHT));
		    game_handle_input(INPUT_UP, input_is_held(INPUT_UP));
		    game_handle_input(INPUT_DOWN, input_is_held(INPUT_DOWN));
		    if (input_is_pressed(INPUT_FIRE)) {
			game_handle_input(INPUT_FIRE, true);
		    }
		    if (input_is_pressed(INPUT_START)) {
			game_handle_input(INPUT_START, true);
		    }
		    
		    // Update game logic
		    game_update(FRAME_TIME_MS);
		    
		    // Update audio
		    audio_update();
		    
		    accumulator -= FRAME_TIME_MS;
		}
		
		// Render
		renderer_clear(COLOR_BLACK);
		game_render();
		renderer_present();
		
		if(game_isgameover())
			break;
	    }
    }
}
