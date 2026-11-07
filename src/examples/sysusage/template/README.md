# template - a complete small Neki32 game

This is the smallest program that does everything a real game does, in the shape that is
worth copying:

- 320x240 RGB565, three framebuffers, paced by `_sc_gfx_flip`
- four pads, with held and pressed state
- a fixed 1/60s simulation step, so a scripted run is reproducible
- sound effects queued with `_sc_snd_play`, never blocking a frame
- an NVM save with a magic number, a version and a boot counter
- `[EVENT]` lines on stdout, so the game can be tested without a screen

Read `src/template.c` top to bottom; it is commented as a tutorial.

## Build

```sh
PVMKSDK=/path/to/sdk/out gmake          # card image: img/game.iso
gmake -f Makefile.dev                  # desktop SDL build: bin.dev/boot.elf
```

## Run

```sh
out/bin/pvmk-nemul img/game.iso         # with the boot menu
out/bin/pvmk-run -C . --frames 300 --input tests/input.txt   # headless, with events
SDLSC_HEADLESS=1 SDLSC_FRAMES=300 SDLSC_CLOCK=virtual ./bin.dev/boot.elf
```

`tests/input.txt` is a scripted pad sequence; `tests/input.txt` plus `--expect` is the whole
test. Save a golden run with `--save-baseline tests/golden.txt` and check it from then on.

## What to change first

1. `isoroot/name.txt` - the game's volume name, which is also the name of its save record.
2. `step_game()` - the simulation.
3. The drawing block at the end of `app_main()`.
4. `save_t` - add fields before `reserved`, or bump `SAVE_VERSION` and clear old saves.

## Traps this file already avoids

- It does not draw into the buffer it just enqueued or the one flip reports as displayed.
- It does not use `float`; positions are 16.16 fixed point.
- It does not use `rand()`; a fixed LCG makes a run reproducible.
- It does not block on audio; `-_SC_EAGAIN` is a normal answer.
- It logs before exiting; PVMK restarts a program that exits, so a save or a message must
  happen first.
