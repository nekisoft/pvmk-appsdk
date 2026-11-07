# neki3d2 - a fixed-point 3D renderer

A small, measured software 3D renderer for Neki32: a textured, lit cube rotating above a
chequered floor, at 320x240 RGB565, drawn entirely in integer arithmetic.

## Design

- **Fixed point.** 16.16 for positions, screen space and texel coordinates; 64-bit only for
  products and the perspective divides. The only floating point is the 256-entry sine table
  built once at startup.
- **No divide in the pixel loop.** A triangle gets plane equations for `1/z`, `u/z` and `v/z`
  once (each slope is one divide, with the determinant scaled instead of the numerator so a
  floor triangle near the camera does not overflow 64 bits). Every pixel is six adds.
- **Perspective texturing without a reciprocal per pixel.** Each scanline is split into runs
  of up to eight inside pixels; the planes are evaluated exactly at both ends of a run and
  texture coordinates are interpolated affinely inside it. Two 32-bit reciprocals per run,
  not one per pixel. This is the same trade Quake's span code makes.
- **Cheap rejection.** Backface cull in screen space before any setup; bounding box before
  any edge test; depth test before the texture fetch.
- **Lighting for free.** Eight pre-shaded copies of the 64x64 texture are built at startup,
  so a lit pixel costs the same load and store as an unlit one. The floor uses the darker
  half of the range, the cube the brighter.
- **32-bit clears**, and one 16-bit texture load and store per pixel.
- Geometry and texture are procedural: no assets, and the demo runs anywhere the SDK does.

The pipeline is: rotate (about Y) -> clip against the near plane (Sutherland-Hodgman, up to
four vertices) -> project -> cull -> rasterise with a depth buffer.

## Build, run, test

```sh
PVMKSDK=/path/to/out gmake                 # img/game.iso, bin/boot.nne
gmake -f Makefile.dev                     # desktop SDL build
/home/agents/work/pvmk-appsdk/out/bin/pvmk-nemul img/game.iso

# Headless benchmark, 300 displayed frames:
out/bin/pvmk-nemul-headless --disk img/game.iso --nne bin/boot.nne --frames 300 --quiet
```

The program prints `[EVENT] FRAME n tris T runs R px P` every 60 rendered frames: triangles
submitted to the rasteriser, runs of eight-pixel texture spans, and pixels that passed the
depth test.

## Measured cost

Profiled with `NEMUL_PROF` over a 2000..5000ms window with `profan.py`:

```
6.83M instructions per frame   12.15M modelled cycles per frame
  raster_tri  82%   __udivsi3 13.6%   app_main 3%
18,380 pixels and 5,222 runs per frame
```

About 25 frames per second of issue time at 300MHz - correct but not fast yet. Roughly
two-thirds of the cost is the per-pixel edge test walking the whole bounding box: 320x240
pixels are visited even when only 18k of them are inside a triangle. The fix is a scanline
rasteriser that walks the edges and touches only the spans inside the triangle, which is the
next piece of work. After that, the run setup (two reciprocals per run) and the 16-bit
texture fetch are what remain.

An earlier version of this measurement read 2.33M instructions per frame; that run counted
every `_sc_gfx_flip` in a pacing loop as a frame, so it divided by three times too many
frames. The numbers above count one flip per frame.

`-DPVMK_640X480` is not a thing here: the framebuffer and the depth buffer are sized for
320x240, which is the resolution this renderer is meant for.

## Known issues

- The floor uses the same two triangles as everything else but is exempt from backface
  culling; its winding is the reverse of the cube's in screen space. A cleaner fix is to give
  it its own vertex order.
- Edges are quantised to whole pixels in the inside test (the edge functions are 16.16 but
  evaluated at pixel centres), so silhouettes can show a one-pixel stair-step.
- No mip-mapping: a floor extending to the horizon aliases. The texture repeats four times
  across the floor, which keeps the worst of it away from the camera.
- Texture coordinates are wrapped by a power-of-two mask, so the texture must stay 64x64.
