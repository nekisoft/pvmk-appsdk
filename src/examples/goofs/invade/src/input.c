#include "input.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool keys[INPUT_MAX];
    bool keysPressed[INPUT_MAX];  // Just pressed this frame
    bool keysReleased[INPUT_MAX]; // Just released this frame
    SDL_Gamepad* gamepad;
} Input;
static Input g_Input;
static Input * const input = &g_Input;

void input_init(void) {
    memset(input, 0, sizeof(Input));
    
    // Try to open first available gamepad
    int numJoysticks = 0;
    SDL_JoystickID* joysticks = SDL_GetGamepads(&numJoysticks);
    if (numJoysticks > 0 && joysticks) {
        input->gamepad = SDL_OpenGamepad(joysticks[0]);
        SDL_free(joysticks);
    }
}

void input_handle_event(SDL_Event* event);

bool input_update(void) {
	
	
    // Clear just pressed/released states
    memset(input->keysPressed, 0, sizeof(input->keysPressed));
    memset(input->keysReleased, 0, sizeof(input->keysReleased));
	

	// Handle events
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
	    if (event.type == SDL_EVENT_QUIT) {
		exit(0);
	    }
	    input_handle_event(&event);
	}

	
	return true;
}

void input_handle_event(SDL_Event* event) {
    switch (event->type) {
        case SDL_EVENT_KEY_DOWN:
            if (!event->key.repeat) {
                switch (event->key.key) {
                    case SDLK_LEFT:
                    case SDLK_A:
                        if (!input->keys[INPUT_LEFT]) {
                            input->keysPressed[INPUT_LEFT] = true;
                        }
                        input->keys[INPUT_LEFT] = true;
                        break;
                        
                    case SDLK_RIGHT:
                    case SDLK_D:
                        if (!input->keys[INPUT_RIGHT]) {
                            input->keysPressed[INPUT_RIGHT] = true;
                        }
                        input->keys[INPUT_RIGHT] = true;
                        break;
                        
                    case SDLK_UP:
                    case SDLK_W:
                        if (!input->keys[INPUT_UP]) {
                            input->keysPressed[INPUT_UP] = true;
                        }
                        input->keys[INPUT_UP] = true;
                        break;
                        
                    case SDLK_DOWN:
                    case SDLK_S:
                        if (!input->keys[INPUT_DOWN]) {
                            input->keysPressed[INPUT_DOWN] = true;
                        }
                        input->keys[INPUT_DOWN] = true;
                        break;
                        
                    case SDLK_SPACE:
                    case SDLK_Z:
                    case SDLK_LCTRL:
                        if (!input->keys[INPUT_FIRE]) {
                            input->keysPressed[INPUT_FIRE] = true;
                        }
                        input->keys[INPUT_FIRE] = true;
                        break;
                        
                    case SDLK_RETURN:
                    case SDLK_P:
                        if (!input->keys[INPUT_START]) {
                            input->keysPressed[INPUT_START] = true;
                        }
                        input->keys[INPUT_START] = true;
                        break;
                        
                    case SDLK_ESCAPE:
                    case SDLK_Q:
                        if (!input->keys[INPUT_QUIT]) {
                            input->keysPressed[INPUT_QUIT] = true;
                        }
                        input->keys[INPUT_QUIT] = true;
                        break;
                }
            }
            break;
            
        case SDL_EVENT_KEY_UP:
            switch (event->key.key) {
                case SDLK_LEFT:
                case SDLK_A:
                    input->keys[INPUT_LEFT] = false;
                    input->keysReleased[INPUT_LEFT] = true;
                    break;
                    
                case SDLK_RIGHT:
                case SDLK_D:
                    input->keys[INPUT_RIGHT] = false;
                    input->keysReleased[INPUT_RIGHT] = true;
                    break;
                    
                case SDLK_UP:
                case SDLK_W:
                    input->keys[INPUT_UP] = false;
                    input->keysReleased[INPUT_UP] = true;
                    break;
                    
                case SDLK_DOWN:
                case SDLK_S:
                    input->keys[INPUT_DOWN] = false;
                    input->keysReleased[INPUT_DOWN] = true;
                    break;
                    
                case SDLK_SPACE:
                case SDLK_Z:
                case SDLK_LCTRL:
                    input->keys[INPUT_FIRE] = false;
                    input->keysReleased[INPUT_FIRE] = true;
                    break;
                    
                case SDLK_RETURN:
                case SDLK_P:
                    input->keys[INPUT_START] = false;
                    input->keysReleased[INPUT_START] = true;
                    break;
                    
                case SDLK_ESCAPE:
                case SDLK_Q:
                    input->keys[INPUT_QUIT] = false;
                    input->keysReleased[INPUT_QUIT] = true;
                    break;
            }
            break;
            
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            if (event->gbutton.which == SDL_GetGamepadID(input->gamepad)) {
                switch (event->gbutton.button) {
                    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                        if (!input->keys[INPUT_LEFT]) {
                            input->keysPressed[INPUT_LEFT] = true;
                        }
                        input->keys[INPUT_LEFT] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                        if (!input->keys[INPUT_RIGHT]) {
                            input->keysPressed[INPUT_RIGHT] = true;
                        }
                        input->keys[INPUT_RIGHT] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_DPAD_UP:
                        if (!input->keys[INPUT_UP]) {
                            input->keysPressed[INPUT_UP] = true;
                        }
                        input->keys[INPUT_UP] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                        if (!input->keys[INPUT_DOWN]) {
                            input->keysPressed[INPUT_DOWN] = true;
                        }
                        input->keys[INPUT_DOWN] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_SOUTH:  // A button
                    case SDL_GAMEPAD_BUTTON_WEST:   // X button
                        if (!input->keys[INPUT_FIRE]) {
                            input->keysPressed[INPUT_FIRE] = true;
                        }
                        input->keys[INPUT_FIRE] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_START:
                        if (!input->keys[INPUT_START]) {
                            input->keysPressed[INPUT_START] = true;
                        }
                        input->keys[INPUT_START] = true;
                        break;
                }
            }
            break;
            
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (event->gbutton.which == SDL_GetGamepadID(input->gamepad)) {
                switch (event->gbutton.button) {
                    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                        input->keys[INPUT_LEFT] = false;
                        input->keysReleased[INPUT_LEFT] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                        input->keys[INPUT_RIGHT] = false;
                        input->keysReleased[INPUT_RIGHT] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_DPAD_UP:
                        input->keys[INPUT_UP] = false;
                        input->keysReleased[INPUT_UP] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                        input->keys[INPUT_DOWN] = false;
                        input->keysReleased[INPUT_DOWN] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_SOUTH:  // A button
                    case SDL_GAMEPAD_BUTTON_WEST:   // X button
                        input->keys[INPUT_FIRE] = false;
                        input->keysReleased[INPUT_FIRE] = true;
                        break;
                        
                    case SDL_GAMEPAD_BUTTON_START:
                        input->keys[INPUT_START] = false;
                        input->keysReleased[INPUT_START] = true;
                        break;
                }
            }
            break;
            
        case SDL_EVENT_GAMEPAD_ADDED:
            if (!input->gamepad) {
                input->gamepad = SDL_OpenGamepad(event->gdevice.which);
            }
            break;
            
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (input->gamepad && event->gdevice.which == SDL_GetGamepadID(input->gamepad)) {
                SDL_CloseGamepad(input->gamepad);
                input->gamepad = NULL;
            }
            break;
    }
}

void input_cleanup(void) {
    if (input->gamepad) {
        SDL_CloseGamepad(input->gamepad);
        input->gamepad = NULL;
    }
}

bool input_is_pressed(InputAction action) {
    return input->keysPressed[action];
}

bool input_is_held(InputAction action) {
    return input->keys[action];
}

bool input_is_released(InputAction action) {
    return input->keysReleased[action];
}
