# Sopwith - status: does not build against this SDK

This is a port of the SDL2 version of Sopwith. It was written against an SDL2 compatibility
shim that the SDK no longer ships; the SDK now provides SDL3 headers and `libsdl3.a`
(`$(PVMKSDK)/armv5te-pvmk-eabi/include/SDL3`).

Pointing the makefile at the SDL3 headers is not enough: the game uses SDL2 names that SDL3
renamed (`SDL_GameController` is `SDL_Gamepad`, the `SDL_CONTROLLER_BUTTON_*` names are
`SDL_GAMEPAD_BUTTON_*`, and so on), and it defines its own `bool` in `src/std.h`, which
collides with `<stdbool.h>`. Porting it means:

- replace `SDL_GameController*` with `SDL_Gamepad*` and the button/axis names throughout
  `src/sdl/controller.c`;
- drop the hand-rolled `bool` from `src/std.h` and use `<stdbool.h>`;
- check the window/renderer calls in `src/sdl/video.c` against SDL3 (`SDL_CreateWindow`
  changed its flags and `SDL_Renderer` is still there, so this part is mostly mechanical);
- then rebuild the ISO and check the game loop against the 320x240 display.

Until then the example is excluded from `src/tools/audit-examples.sh` results as a known
failure. It is left in the tree as the starting point for that work, not as an example of how
to write a game for the system.
