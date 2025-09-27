#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>
#include <stdbool.h>

// Input actions
typedef enum {
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_FIRE,
    INPUT_START,
    INPUT_QUIT,
    INPUT_MAX
} InputAction;

typedef struct {
    bool keys[INPUT_MAX];
    bool keysPressed[INPUT_MAX];  // Just pressed this frame
    bool keysReleased[INPUT_MAX]; // Just released this frame
    SDL_Gamepad* gamepad;
} Input;

// Input functions
void input_init(Input* input);
void input_update(Input* input);
void input_handle_event(Input* input, SDL_Event* event);
void input_cleanup(Input* input);

// Query functions
bool input_is_pressed(Input* input, InputAction action);
bool input_is_held(Input* input, InputAction action);
bool input_is_released(Input* input, InputAction action);

#endif // INPUT_H
