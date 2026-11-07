# Testing PVMK games from a script

A game is a program that never exits. To test one you need to make it stop, make it
reproducible, and make its output checkable. The SDK gives you all three:

- the program stops after a fixed number of displayed frames (`--frames`, `SDLSC_FRAMES`),
- the clock and the pads are scripted, so the same binary does the same thing,
- every displayed frame has an FNV-1a hash (`--hash`, `SDLSC_TRACE`).

## The two harnesses

| harness | command | use for |
|---|---|---|
| desktop shim | `gmake -f Makefile.dev` then `SDLSC_*=... ./bin.dev/boot.elf` | fastest loop; game logic and rendering output |
| emulator | `out/bin/pvmk-run -C <project> ...` | anything about the real machine: memory, alignment, syscalls, timing |

The shim is the same program code against SDL; it does not emulate the 24 MB limit, the
absent FPU or the scanout bandwidth. Use it to iterate, then confirm in nemul.

## pvmk-run

```
out/bin/pvmk-run -C <project> [options]
```

It builds with `PVMKSDK`, runs `pvmk-nemul-headless`, and prints one `frame <n> <hash>` line
per displayed frame. Options:

```
--frames N            frames to run (passed straight to the emulator)
--ticks MS            emulated milliseconds, whichever comes first
--input FILE          scripted pads, see below
--dump DIR            dump every frame as DIR/frameNNNNN.ppm
--shot FILE.ppm       last frame
--expect REGEX        assert the program's output matches (repeatable)
--baseline FILE       compare frame hashes against a saved run
--save-baseline FILE  write this run as the reference
--no-build            run what is already built
--boot-menu           boot the card through the real boot menu
--trace-sysc          log syscalls and card traffic
--json FILE           machine-readable summary
```

Exit status: 0 pass, 1 build failed, 3 the program faulted, 4 baseline mismatch, 5 an
expected string did not appear.

### A scenario file

`pvmk-run` takes plain flags, so a shell script is a scenario:

```sh
#!/bin/sh
# tests/smoke.sh - the game boots, reaches the title, and the title frame is stable
set -e
PVMKSDK=${PVMKSDK:-/path/to/out}
out/bin/pvmk-run -C . --frames 180 --input tests/title.input \
    --expect '\[EVENT\] TITLE' --baseline tests/golden/title.txt
```

Keep the input script and the baseline in the repo. A pixel change then shows up as a
one-line diff, and a regression is a failing test rather than a screenshot someone has to
remember to look at.

### Input scripts

One line per change; frame numbers are displayed frames, counting from 0:

```
# frame  buttons
30       start
36       -
90       a,right
120      -
150      2:b            # pad B (ports are 1-4 or a-d); no prefix means pad A
```

A pad holds its buttons until a later line changes them. `-` releases everything.

## Writing the game so it can be tested

- Log events with a stable, greppable prefix: `printf("[EVENT] STATE %d\n", state);`.
  Assert on them with `--expect`. Do not print per frame in hot code.
- Make the start deterministic: no dependence on `rand()` unless the seed is fixed and
  settable (`T3K_SEED` style), no dependence on wall-clock time, no dependency on the number
  of input events per frame.
- Read input as a stream and apply the *last* event per pad in a frame, so a scripted press
  and a human press behave the same.
- Draw into three buffers and honour flip's return value. A test that cycles buffers by
  number will tear and will blame the renderer (this really happened in `tplbuf`'s test).

## Triage order when a run fails

1. **Build failed.** Read the first error, not the last. Header changes need a clean build.
2. **Exit 3, no frames.** The program faulted early. Run with `--symbols bin/boot.elf
   --trace-sysc` and read the `FAULT` line: it names the function and source line. A data
   abort at a random address usually means the stack met a static array (over 24 MB) or an
   unaligned access.
3. **Frames but no `[EVENT]`.** The event never happened. Check the input script's frame
   numbers against the frame log: a press before the program starts is lost, and a press
   while it is blocked in `_sc_pause` may be missed.
4. **Hash changed.** Dump frames and compare the two PPMs. If the change is intended, update
   the baseline in the same commit and say so.
5. **Different hashes on every run.** Something is uninitialised or time-dependent. Run
   twice with `--no-build` and diff the logs.
6. **Works on the shim, not in nemul (or vice versa).** Usually a 24-bit host pointer or an
   unaligned access. The shim is 64-bit and forgiving; nemul is 32-bit and strict.

## Faults that nemul reports

| `reason` | meaning |
|---|---|
| `data-abort` | read or write outside 0x1000..size-1 |
| `prefetch-abort` | jumped outside the image, or into the zero page |
| `alignment-check` | unaligned word/halfword access |
| `interpreter-failure` | instruction nemul does not implement; read the trace |
| `breakpoint` | a `--bp` you asked for, or a `bkpt` in the code |

## Regression testing the SDK itself

`src/sdlsc/test/check.sh` runs the shim's conformance matrix (inline and pumped threading,
headless and windowed, virtual and real clocks). `src/nemul` has a `testing/` directory.
Both are worth running after touching the shim or the emulator.
