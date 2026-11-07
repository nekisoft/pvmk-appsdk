# Traps that cost time

Things that are not obvious from the API and that have cost real debugging time. Each one is
stated with what to do instead.

## The emulator can cut off the guest's output

`_sc_print` is a system call that prints one character at a time. `printf` ends up making one
call per character, so when `nemul-headless` stops the machine after `--frames N`, it can stop
it in the middle of a line. A run of 180 frames whose program prints at frame 180 may show
"[EVENT] FRAME 18" and nothing else, and a 2-frame run can show nothing at all even though
the program printed at frame 1.

Do not conclude the program did not run. Run a few frames past the point you are interested
in, or assert on lines printed well before the stop. `pvmk-run` looks for `[EVENT]` lines and
works this way: it runs the number of frames you asked for, and a test that needs frame N's
output should ask for more than N.

## `make` will not rebuild after an edit in the same second

The toolchain's timestamps have one-second resolution. Editing a source file and rebuilding
in the same second can leave the old object in place, which looks exactly like "my fix did
nothing". Use `gmake -B` when a rebuild is in doubt; that is what the example tests do after
edits.

It is worth being suspicious whenever a change seems to have no effect at all: check the
binary's strings or timestamp before adding more changes on top.

## A card image needs both the executable and the disk

```sh
pvmk-nemul-headless --disk img/game.iso --nne bin/boot.nne
```

`--nne` is the program; `--disk` is the ISO filesystem the program reads with `_sc_disk_*`
and the C library's `open`/`fopen`. Running without `--nne` starts whatever is on the card,
and running without `--disk` gives the program a disk it cannot read. `pvmk-run` supplies
both from the makefile's outputs.

## `--frames` counts displayed frames, not the game's frames

The emulator stops after N vertical blanks. A game that renders faster than the display can
have advanced further than N frames of its own; a game that blocks on the display can have
advanced fewer. Frame hashes and `[EVENT] FRAME` lines come from the game, so use the game's
own counter for assertions.

## Three buffers do not make a frame clock

See `display.md`. The double-buffer wait idiom silently stops pacing when it is used with
three buffers, and a game that then measures "frames per second" is measuring the CPU.
Games that need a fixed rate should count their own frames or use `_sc_getticks`.

## The process space is 24MB and frame buffers are most of it

The kernel gives a process 24MB (the emulator enforces the same limit and returns
`-_SC_EFAULT` for a frame buffer that does not fit). Three 320x240 RGB565 buffers are 150KB,
which is fine; a 640x480 buffer is 600KB, and three of them plus a few MB of game data is
where the limit starts to matter. Allocate frame buffers statically: a local array that
large will overflow the stack long before the address space is full.

## Doom and Quake need their data on the card

Both ports read their WADs through the C library, so the data must be in `cardroot/` (or on
the disk image the emulator is given). A build that links but shows "W_GetNumForName: not
found" is missing data, not broken code. The ports' `cardroot/` contents are not all in git -
some are generated or copied from upstream.

## `-Werror` and byte fonts

Old XBM fonts and similar data files declare their bytes as `char`. `-Werror=overflow`
rejects `0xFF` in a `char` initialiser on a signed-char target. Declare the data as
`unsigned char` and cast where it is used as bytes, rather than weakening the warning flags.

## SDL shim builds need the shim compiled in

`-DUSE_SDLSC=1 -I.../sdlsc/include` only gets you declarations. The implementations of
`_sc_*`, `_sdlsc_run` and `_sdlsc_present` live in `src/sdlsc/src/sdlsc.c` (or a copy called
`src/sdlsc.c.dev` in older projects); link that file or the program links successfully and
then dies at startup with undefined behaviour or a jump to address zero.
