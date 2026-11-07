# Display, buffers and pacing

The display side of `sc.h` is small but has one sharp edge that changes how a game loop
behaves. This is what the kernel does, what the emulator does, and what a game should do.

## The call

```c
int _sc_gfx_flip(int mode, const void *buffer);
```

- `mode` is one of `_SC_GFX_MODE_*`; `buffer` is the address of a frame to show. Mode 0
  (text) takes a null buffer.
- The call **enqueues** `buffer` to be latched at the next vertical blank. It returns
  immediately; it does not wait for the latch.
- The return value is the buffer that is **currently being scanned out, before the enqueue
  takes effect** - or 0 if the process making the call does not own the display at that
  moment.
- The enqueued buffer stays latched until another flip replaces it. Calling flip with the
  same buffer again is how a game asks "which buffer is on screen now?" without changing the
  picture.

`_sc_pause()` sleeps until the next event (an input report, an audio queue change, or a
vertical blank). It is the right thing to call while polling.

## Why "wait until my buffer is displayed" is not a reliable frame clock with three buffers

The double-buffering example in the manual does this:

```c
do {
    _sc_pause();
    shown = _sc_gfx_flip(mode, fbs[back]);
} while(shown != fbs[back]);
```

With two buffers that works: the buffer just enqueued cannot be the one on screen, because
the other one is. With three buffers it can: the game cycles A, B, C, A, ... and if it is
running behind the display, the buffer it is about to enqueue is still the one being scanned
out. The first call then returns that same pointer, the loop exits without a blank, and the
simulation runs at CPU speed. Measured in nemul with a 3-buffer loop of exactly this shape:
93 iterations per second when the loop fell through, 60 when it did not.

This is not an emulator artefact: the kernel really does latch at the next blank and really
does return the pre-latch front buffer.

## What to do instead

Pick one:

- **Fixed timestep by counting frames you flip.** Flip once per frame, do not poll the return
  value, and drive the simulation from your own frame counter. This is what
  `sysusage/tplbuf` and `neki3d2` do. The display shows the newest complete frame, older
  frames are dropped, and the game never slows down because of the display. Recorded runs are
  reproducible because the simulation depends only on the frame number.
- **Wait for the display, deliberately.** Call flip twice: once to enqueue, and then poll
  `shown` until it *changes* from the value returned by the first call. That costs one blank,
  which is what a 60Hz game wants, and it does not fall through. `sysusage/template` uses the
  two-buffer idiom with three buffers and happens to pace correctly at 60 in nemul, but the
  "changed" test is the robust one.
- **Pace on time.** `_sc_getticks()` counts milliseconds and, on the emulator, one tick is one
  modelled millisecond. A deadline loop (`next += 16; while(_sc_getticks() < next) _sc_pause();`)
  gives a 60Hz simulation regardless of how flips land, at the cost of some jitter.

Do not use the CPU speed as a clock, and do not assume one input report is one frame; the
manual's "not timing sources" section spells out both.

## Buffer layout

- 16bpp modes are RGB565, little-endian 16-bit words, rows top to bottom, no padding:
  `320 * 2 = 640` bytes per row at 320x240.
- The frame must be at least 4-byte aligned and fit inside the calling process's address
  space; the emulator validates both and returns `-_SC_EFAULT` otherwise.
- Keep frames away from 1MB boundaries (the manual's performance note: the display hardware
  fetches more efficiently inside one megabyte).
- Three buffers of 320x240x2 are 150KB, which is most of a small process's space; allocate
  them statically, not on the stack.

## In the emulator

- `nemul` latches the enqueued buffer in `sysc_popfbptr` when the emulated vertical blank
  comes around, and `pvmk_sc_gfx_flip` returns `sysc_fb_now_ptr` before the latch.
- A frame is one emulator "frame"; `--frames N` stops after N latched frames and
  `--shot file.ppm` writes the last one.
- The emulator does not model scanout tearing, so a game that redraws the buffer being
  scanned out will not show the artefact it would on hardware. Do not use a single buffer.
