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

// Input functions
void input_init(void);
bool input_update(void);
void input_cleanup(void);

// Query functions
bool input_is_pressed(InputAction action);
bool input_is_held(InputAction action);
bool input_is_released(InputAction action);

#endif // INPUT_H
