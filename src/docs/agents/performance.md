# Measuring and improving game performance on Neki32

The console has no cache model in the emulator, so "how long does this take" has two honest
answers: instructions executed (exact, from nemul) and wall-clock on hardware (the only
truth about memory). Optimise with the first, confirm with the second.

## Getting a profile

```
NEMUL_PROF=/tmp/prof \
NEMUL_PROF_FROM=2000 NEMUL_PROF_TO=4000 \
  out/bin/pvmk-nemul-headless --disk img/game.iso --nne bin/boot.nne --ticks 4500

python3 src/nemul/tools/profan.py bin/boot.elf /tmp/prof-<pid>.txt --top 25 --calls 10
```

`NEMUL_PROF_FROM`/`TO` are emulated milliseconds. Always window the run: the first seconds
of a real game are loading, and a whole-run profile measures the loader. Keep `-g` in the
build; symbol attribution dies without it.

`profan.py` turns the PC histogram into functions with an ARMv5TE issue-cost model, and
`--calls` reconstructs call edges from counted `BL`s. The edge is what turns "a divide helper
is 14% of the run" into "this caller wanted the divide", which is the only form you can act
on.

Read **instructions per frame**, which profan prints when the run's syscall log carries
`gfx_flip` counts. A faster game draws more frames in the same emulated second, so its window
total barely moves even when each frame got much cheaper.

## What the model does and does not include

Included: instruction issue cost (with extra for multiplies, loads/stores, block transfers
and branches), exactly.

Not included: cache misses, SDRAM latency and bank conflicts, DMA, the video scanout stealing
memory bandwidth, and write buffering. Anything that walks a large texture or the
framebuffer will be relatively worse on hardware than the profile says. `--mem CYCLES` adds a
flat penalty per memory access as a sensitivity check.

## The usual wins, in order

1. **Remove soft float from inner loops.** Grep the disassembly of hot functions for
   `__aeabi_f`, `__divsf3`, `__mulsf3`, `__fixsf`, `__floatsf`. A single float divide in a
   span loop can cost more than the rest of the loop. Replace with fixed point:
   16.16 for world/view/texel/screen, 32.32 for perspective quantities.
2. **Kill divides.** 32-bit `__divsi3` is ~20 instructions, 64-bit `__udivmoddi4` is ~110.
   A per-span divide becomes an increment; a per-pixel divide becomes a reciprocal computed
   once per polygon; a 64-bit reciprocal becomes a 32-bit Newton-Raphson estimate.
3. **Write the framebuffer a word at a time.** RGB565, two pixels per 32-bit store. Do not
   use `putpixel()` in an inner loop.
4. **Clip early and cheaply.** Scissor rectangles and reject whole spans before touching
   memory.
5. **Prefetch-friendly access.** Walk textures and framebuffers linearly where possible; a
   256x256 8-bit texture is 64 KB and does not fit in the F1C's cache, so a linear walk beats
   a column walk.
6. **Reduce per-frame setup.** Sorting, visibility, clearing: clear only what you drew.

## Fixed-point conventions

- `fix16_t` = int32, 16.16: positions, texel coordinates, screen space.
- `fix32_t` = int64, 32.32: perspective quantities (`1/z`, `s/z`, `t/z` and gradients), which
  need both range and precision.
- Multiplication rules that are silent when wrong:
  - fix16 * fix16 -> `>>16`
  - int * fix16 -> **no shift** (an integer times 16.16 is 16.16)
  - fix16 * fix32 -> `>>16`; fix32 * fix16 -> `>>32`
  - fix16 * fix16 to fix32 -> no shift
- Convert once at load time (vertices, planes, texinfo), keep the float twins only where
  gameplay or collision reads them.
- Clamp near-degenerate divides. Float produced infinity and the view clamp cleaned it up;
  fixed point wraps sign and draws a wedge across the screen.

## Doom and Quake specifically

Both ports live in `src/examples/ports/`. Their `Makefile.dev` builds a desktop binary with
the SDL shim, and `gmake SDKVER=<sdk>/out/bin/pvmk-sdkversion` builds the card.

- Doom (freedoom1/freedoom2) renders 320x240 through its own span rasteriser. At 640x480 it
  does 4x the pixel work and writes 4x the bytes; the scanout also reads 4x per frame. Expect
  it to be pixel-bound first and memory-bound second. Measure `R_RenderPlayerView` and the
  span drawers with profan before changing the resolution; if the frame is already over
  budget at 320x240, 640x480 cannot be made fast without a different renderer (fewer spans,
  lower internal resolution with a 640x480 HUD, or column-major caching).
- Quake's world/BSP path is fixed point already; the alias-model path is not. That is the
  next conversion: transform in 16.16, one reciprocal per model, fixed dot-product lighting.
  Its `Makefile` has no header dependencies: clean after touching a header.
- Both need their WAD/PAK data in `cardroot/`; the ISO is what the emulator reads.

## Harness rules for a fair comparison

- Same window (`NEMUL_PROF_FROM`/`TO`), same scene, same build flags, same emulator binary.
- Compare instructions per frame and frame hashes. A change that makes the picture different
  is not a speedup until someone decides the difference is acceptable.
- Run twice; instruction counts must be identical, or the run was not deterministic and the
  comparison is meaningless.
- Report the machine cost in instructions per frame and the implied frame rate against the
  300 MIPS the scheduler assumes, plus the measured frame count if you have it.
