# AGENTS.md - working on Neki32 games with the PVMK app SDK

This file is for an LLM agent (or a person in a hurry) who has to build, run, debug and
measure a game for the Neki32 console. It is the short version of the developer manual plus
the things the manual does not say and every newcomer learns the hard way.

Read this first, then `src/docs/intro/intro.latex` for the authoritative system description.

## What you are writing for

Neki32 is a game console. A game is a read-only SD card containing an ISO9660 image with an
El Torito boot entry (platform 0x92) that points at a flat ARM binary (`boot.nne`).

- CPU: ARMv5TE (ARM9 core) at 300 MHz. **No FPU.** Every `float` operation is a libgcc
  soft-float call, and a float divide is hundreds of instructions. Use integers or 16.16 /
  32.32 fixed point in anything that runs per pixel or per vertex.
- RAM: one process, 24 MB, addresses 0..24M-1. The first 4 KB (the zero page) is not
  mapped; the image is mapped from 0 and execution starts at 0x1000. There is no MMU for
  you, no `malloc` beyond what the C library provides, and no second process to talk to.
- Video: 640x480 or 320x240 (pixel-doubled) at 60 Hz, RGB565, one linear framebuffer.
  640x480 costs four times the pixels of 320x240 and 4x the bandwidth; on this machine that
  is the difference between a game that fits in a frame and one that does not.
- Audio: 48 kHz, 16-bit signed, stereo, interleaved left-then-right, queued by the game.
- Input: up to four Mega-Drive-compatible pads, read as events; one event per pad per frame
  in practice, but the interface does not promise that.
- Saves: one 128 KB NVM record per game, allocated at boot from the card's ISO volume name.

There is no OS underneath: PVMK is 12 system calls, and everything else is your own code
plus the C library.

## Repository and SDK layout

```
src/libsc/sc.h              the whole system-call interface, header-only
src/pvmkoslib/              C runtime glue: cdfs (ISO9660), freestanding headers, atomics
src/pvmkpicocrt/            crt0 and linker scripts
src/sdlsc/                  SDL2 shim that implements the system calls on a desktop
src/nemul/                  the emulator (SDL2 + Dear ImGui GUI, and a headless front end)
src/examples/               examples, demos ("goofs") and game ports
src/tools/pvmk-run          build + run + compare harness (installed into out/bin)
src/docs/intro/intro.latex  the developer manual (build with src/docs_build.sh)
build.sh                    builds the complete SDK into out/
out/                        the finished SDK: bin/, armv5te-pvmk-eabi/, examples/, sdlsc/
```

`out/` is generated. Never edit it; edit the sources above and rebuild. A checkout that has
never run `build.sh` has no `out/` and no compiler.

## Build the SDK

```
./build.sh
```

This needs the git-LFS source tarballs (`git lfs pull`), a host C/C++ compiler, SDL2
development files for the emulator, `pdflatex` for the manual, and `arm-none-eabi-gcc` for
the OS library. It builds binutils and GCC for `armv5te-pvmk-eabi`, picolibc, the OS library,
the emulator, the desktop shim, the examples and the docs. Expect tens of minutes on four
cores; the GCC build dominates. The result is a self-contained `out/`.

Incremental pieces, when the toolchain is already in `out/`:

```
./src/libsc_build.sh          # sc.h -> out/armv5te-pvmk-eabi/include
./src/pvmkoslib_build.sh      # cdfs + runtime -> out/.../lib/libpvmkoslib.a
./src/sdlsc_build.sh          # the desktop shim sources
./src/nemul_build.sh          # emulator binaries + smoke test
./src/examples_build.sh       # copies src/examples into out/examples
./src/docs_build.sh           # the manual PDF
./src/tools_build.sh          # pvmk-run
```

## Build a game

Every example has a `Makefile` that finds the SDK by `SDKVER`/`PVMKSDK`, compiles `src/*.c`
with `pvmk-cc`, links a flat NNE and wraps it in an ISO with `pvmk-xorriso`.

```
cd src/examples/sysusage/tplbuf
PVMKSDK=/path/to/pvmk-appsdk/out gmake
# products: bin/boot.nne and img/game.iso
```

`PVMKSDK` must be set explicitly. Without it the makefile resolves an empty root and the
first command fails on `//bin/pvmk-cc`.

The makefiles have **no header dependencies**. If you change a header, `gmake clean` (or
delete `obj/` and `bin/`) before rebuilding, or you will link objects compiled against two
different struct layouts. That links cleanly and corrupts pointers at run time.

## Run it

### On the desktop, against SDL (fast iteration, real program logic)

```
cd src/examples/sysusage/tplbuf
gmake -f Makefile.dev
SDLSC_HEADLESS=1 SDLSC_FRAMES=120 SDLSC_CLOCK=virtual ./bin.dev/boot.elf
```

The shim (`sdlsc`) implements the system calls on top of SDL2, so the same source runs on a
desktop. Useful environment variables:

| variable | meaning |
|---|---|
| `SDLSC_HEADLESS=1` | no window, no video device; frames are counted, not drawn |
| `SDLSC_FRAMES=N` | exit after N displayed frames |
| `SDLSC_CLOCK=virtual` | 1 ms of virtual time per clock read, no sleeping: deterministic |
| `SDLSC_INPUT=FILE` | scripted buttons, `<frame> <buttons>` per line |
| `SDLSC_TRACE=1` | one line per frame: mode, ticks, buttons, FNV hash of the frame |
| `SDLSC_DUMP=DIR` | write each displayed frame as `frameNNNNNN.ppm` |
| `SDLSC_NVM=PATH` | use this NVM file (keeps test runs isolated) |

This is the fastest loop, but it is not the console: the shim has no timing model and no
memory limits. Use it for logic and rendering output, and use nemul before believing a
performance or memory result.

### Under nemul, the emulator (closest to the machine)

```
# GUI, with a card image
out/bin/pvmk-nemul img/game.iso

# Headless: no display, deterministic, machine-readable
out/bin/pvmk-nemul-headless --disk img/game.iso --nne bin/boot.nne \
    --frames 120 --hash --shot /tmp/last.ppm --json /tmp/run.json
```

`--nne` boots one program as PID 1 with the boot menu out of the way; `--disk` still supplies
the card data the program reads. Leave `--nne` out to exercise the real boot menu.

Headless front end options that matter to a harness:

| option | meaning |
|---|---|
| `--frames N` | stop after N displayed frames (the main stop condition) |
| `--ticks MS` | stop after MS emulated milliseconds, whichever comes first |
| `--hash` | print `FRAME <n> TICKS <ms> MODE <m> HASH <hex>` per displayed frame |
| `--hash-file F` | same lines to a file (use this; the guest also writes to stdout) |
| `--shot FILE.ppm` | write the last displayed frame |
| `--fb PREFIX`, `--fb-every N` | dump every Nth displayed frame as `PREFIX-00012.ppm` |
| `--input FILE` | scripted pads; `120 a,start` presses on frame 120, `128 -` releases |
| `--json FILE` | machine-readable summary: ticks, frames, stop reason, fault |
| `--symbols ELF` | name the faulting function and source line |
| `--gdb [PORT]` | serve GDB's remote protocol (default 14292) |
| `NEMUL_NVM=path` | keep the 128 KB NVM record in a host file across runs |
| `--trace-sysc` | log system calls and card traffic |
| `--bp HEX`, `--step N` | stop at an address, print N instructions, exit |

Exit status: 0 limit reached, 2 bad usage/I/O, 3 the program faulted, 4 baseline/assertion
failure in `pvmk-run`, 5 stopped at a breakpoint it did not ask for.

### The one-command loop: pvmk-run

```
out/bin/pvmk-run -C src/examples/sysusage/tplbuf --frames 60 \
    --dump /tmp/frames --shot /tmp/last.ppm --save-baseline /tmp/golden.txt

# after a change: did any frame change?
out/bin/pvmk-run -C src/examples/sysusage/tplbuf --frames 60 --baseline /tmp/golden.txt
```

It builds, boots the NNE, runs a fixed number of frames, prints `frame N HASH` lines, checks
`--expect REGEX` against the program's own output, and compares against a baseline file. The
baseline is plain text, so a diff of it is reviewable. This is the intended interface for an
automated agent: **never judge a change by eye when a hash will do.**

## Testing and golden runs

- A run is deterministic when the program is: fixed frame count, scripted input, emulated
  clock. `pvmk-run` gives you that. Compare frame hashes, not ticks: one game tic of drift
  changes `TICKS` but often not a pixel.
- Save a baseline before changing anything, then require the new build to match it. A
  pixel-identical slice that should not change must be pixel-identical.
- Use `--dump` and look at the PPMs when a hash changes; convert with any image tool.
- Log events with a machine-greppable prefix from inside the game, e.g.
  `printf("[EVENT] SCORE %d\n", score);` and assert on them with `--expect`.

## Debugging

- **GDB**: run the headless emulator with `--gdb 14292` (or the GUI, which listens on
  `Rsp/Port`, default 14292), then
  `gdb -ex "set architecture arm" -ex "file bin/boot.elf" -ex "target remote :14292"`.
  The `set architecture arm` is required: the stub reports no XML target description, so a
  host gdb otherwise assumes the machine's own architecture and disassembles garbage. Every
  process is a thread (`pid N`),
  breakpoints survive a restart, and `monitor reload` reloads the image from disk so a
  rebuilt program starts without restarting the emulator: that is the edit-and-continue loop.
  `p`/`x`/`set var` work; there is no variable DWARF, only function names and line numbers.
- **GUI debugger**: `Debug > Debugger...` (F7) shows processes, registers, disassembly,
  source lines and the recent-instruction trace. `--symbols <elf>` loads names at startup.
- **Headless stop and look**: `--bp 11b8 --step 40 --symbols bin/boot.elf` prints the
  instructions after the breakpoint and exits. Good for a post-mortem without a client.
- A fault in headless now prints `FAULT pid ... reason ... pc ... in func at file` and exits 3.
  Exit 0 after a crash was the old behaviour and hid real failures.

## Profiling and performance

nemul counts instructions exactly but models no cache, no pipeline and no SDRAM. Use it for
the instruction mix and for relative improvements, then sanity-check with real frame time on
hardware.

```
NEMUL_PROF=/tmp/prof NEMUL_PROF_FROM=2000 NEMUL_PROF_TO=4000 \
  out/bin/pvmk-nemul-headless --disk img/game.iso --nne bin/boot.nne --ticks 5000
python3 src/nemul/tools/profan.py bin/boot.elf /tmp/prof-<pid>.txt --top 25 --calls 10
```

Report **instructions per frame** (profan.py prints it when the syscall log has flips), not
the window total: a faster game draws more frames in the same emulated second, so its total
barely moves while each frame gets cheaper.

What is expensive on this machine, in order of likelihood:

1. Soft float. Any `float`/`double` in a per-pixel, per-vertex or per-span loop.
2. 64-bit divide (`__udivmoddi4`) and 32-bit divide (`__divsi3`). Use reciprocals, shifts, or
   per-scanline increments.
3. Per-pixel function calls and byte-at-a-time framebuffer writes. Write 32 bits at a time;
   the framebuffer is RGB565 and two pixels fit a word.
4. Cache/SDRAM behaviour, which nemul cannot show: texture sampling scattered over a large
   texture, and reading the framebuffer you just wrote.

## The frame loop, done properly

- Pick 320x240 unless the game genuinely needs 640x480 detail. At 320x240 a 24 MB machine can
  afford a 320x240 16-bit back buffer (150 KB each) many times over; at 640x480 the scanout
  reads 614 KB per frame and the CPU has ~5M cycles of work per frame at best.
- Use three buffers and `_sc_gfx_flip`. The call enqueues the buffer to show at the next
  vertical blank and returns the buffer currently displayed. Never draw into the buffer you
  just enqueued or the one flip reported as displayed. See `src/examples/sysusage/tplbuf`.
- Never busy-wait on the clock for pacing. Flip is the pacing mechanism.
- **Triple buffering does not pace a game by itself.** `examples/sysusage/tplbuf` deliberately
  renders as fast as it can. A game that wants one simulation step per displayed frame must
  enqueue its buffer and loop on `_sc_gfx_flip` + `_sc_pause` until the return value is that
  buffer; `examples/sysusage/template` does this.
- Input events arrive as a stream; drain `_sc_input` in a loop until it returns 0. The port
  byte is uppercase `'A'`..`'D'` (the manual's lowercase text is a typo; `sc.h` is right).
- Audio is a queue: enqueue a chunk, and if `_sc_snd_play` returns `-_SC_EAGAIN`, keep playing
  what you have and try again next frame. Do not block the frame on audio.

## Things that will bite you

- **No FPU.** Check the map file or disassembly for `__aeabi_f*`, `__divsf3`, `__mulsf3`,
  `__fixsf*`, `__floatsf*` in hot code.
- **No unaligned access.** A 32-bit load from an odd address is a SIGBUS. Casting a `char*`
  from a file to a struct is the usual way to find this.
- **System calls only in ARM mode.** `udf 0x92` has no Thumb encoding; the SDK handles this
  for C, but hand-written Thumb assembly must switch to ARM around a call.
- **24 MB total, including stack.** `crt0` puts the stack near the top of the process. A big
  static array plus a big stack plus a loaded file will collide, and the failure looks like a
  data abort at a random address. Check `size bin/boot.elf`.
- **Headers are not dependencies** in the example makefiles. Clean after touching one.
- **The card is read-only** in normal use. Saves go to NVM. Writing to the card requires a
  writable card and is not the shipping path.
- **NVM needs a volume name.** The ISO volume ID must be non-empty, not `CDROM`, and not start
  with `-`, or the game gets no NVM record (and gets a temporary buffer instead).
- **`printf` is slow** and pulls in a lot of code. Fine for debug lines, not for per-frame
  text; draw your own font if you need text in a game.
- **Emulator vs hardware:** nemul emulates the system-call side, not the hardware. It cannot
  show you cache misses, DMA cost, or controller timing. It is excellent for logic,
  memory-safety faults, and instruction counts. It models the audio queue's flow control but
  plays nothing, and its NVM record lives in a file only when `NEMUL_NVM` is set.

## Documentation map

| file | contents |
|---|---|
| `src/docs/intro/intro.latex` | the manual: boot format, syscalls, timing, SDK, common errors |
| `src/libsc/sc.h` | exact syscall signatures, constants, input bit indexes |
| `src/nemul/README` | emulator GUI, debugger, GDB, scripted runs, threading |
| `src/sdlsc/README` | desktop shim and its headless/test facilities |
| `src/docs/agents/testing.md` | harness recipes: scenarios, baselines, fault triage |
| `src/docs/agents/performance.md` | how to profile and what the numbers mean |
| `src/docs/agents/examples.md` | what each example demonstrates and how to build it |
| `src/docs/agents/display.md` | flip semantics, buffers and how to pace a loop correctly |

## House rules for this repository

- Keep `out/` generated. Sources live in `src/`; a fix in `out/` disappears on the next build.
- Every example builds with the SDK from `out/` and nothing else.
- Prefer plain C99, designated initialisers, tables and explicit state. No C++ in games.
- Record what you verified and how. A claim without a command and its output is a rumour.
