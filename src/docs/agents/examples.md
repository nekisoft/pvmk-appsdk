# The example projects

Everything under `src/examples` builds with the SDK from `out/`:

```sh
PVMKSDK=/path/to/out gmake -C <example>          # card image: img/game.iso
gmake -C <example> -f Makefile.dev               # desktop SDL build (where one exists)
```

`sysusage/*` and `workflows/*` are the teaching examples; `goofs/*` are demos and small
games; `ports/*` are ports of existing games and are much larger.

## Read these first

| project | what it shows |
|---|---|
| `sysusage/template` | **the recommended starting point.** Paced 320x240 triple buffering, input with edges, fixed 1/60s simulation, sound, NVM save, `[EVENT]` logging, and a `pvmk-run` test with a golden run |
| `sysusage/showinput` | reading all four pads and drawing their button state |
| `sysusage/dblbuf` | double buffering and blocking on the flip |
| `sysusage/tplbuf` | triple buffering that deliberately free-runs; no pacing |
| `sysusage/nvmsave` | saving and loading the 128KB NVM record |
| `workflows/sdkusage` | the smallest C program built with the C SDK |
| `workflows/allasm` | a bootable card written entirely in assembly, no C library |
| `workflows/asmdata` | assembly plus data loaded from the card with `_sc_disk_read2k` |
| `workflows/asmisofs` | assembly reading a file through the ISO9660 filesystem |
| `workflows/freestanding` | C without a C library, linked against the boot image directly |
| `features/rwcard` | overwriting a file's contents on a writable card |
| `features/cxxtest` | C++ compiled for the console |

## Demos and small games (`goofs/`)

| project | what it is |
|---|---|
| `neki3d` | an early textured span renderer with PLY models and PNG textures |
| `neki3d2` | a faster fixed-point Z-buffered renderer with procedural geometry and a benchmark |
| `invade` | "Chasin Gators", a complete small game |
| `rockshot` | a shooter demo |
| `soccest` | "Soccer 2: The Soccest", a menu-heavy sports game |
| `robuts`, `sausage`, `menuvn`, `democard`, `demodisk1` | assorted demos and engine experiments |
| `bigscroll`, `slideshow`, `sfilm` | scrolling and media playback demos |
| `sfilm` | streams 320x240 video from the card |

## Ports (`ports/`)

| project | upstream | notes |
|---|---|---|
| `freedoom1`, `freedoom2` | Freedoom | Doom engine, 320x240 software renderer; `freedoom2` shares `freedoom1`'s sources |
| `quake` | LibreQuake | software BSP renderer, fixed-point world path, float alias models |
| `cdogs` | C-Dogs SDL | |
| `cylindrix` | Cylindrix | |
| `greed` | In Pursuit of Greed | |
| `bor` | Beats of Rage | large; needs its own data |
| `licar` | Licar | |
| `smsplus` | SMS Plus | Master System emulator |

Ports bring their own assets in `cardroot/`; without those assets they build but cannot run.
They are also the best source material for how a real game drives the console: Doom's
`i_video.c` for a paced triple buffer, Quake's `vid_pvmk.c` for mode handling.

## SDL examples

`src/examples/sdl3/*` and `sdl2/sopwith` are ports of SDL's own tests and games. They are
useful as smoke tests for the desktop shim, not as examples of how to write for the console.
