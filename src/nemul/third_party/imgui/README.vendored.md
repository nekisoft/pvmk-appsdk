# Dear ImGui, vendored

Version 1.93.0, from https://github.com/ocornut/imgui (tag `v1.93.0`, released 2026-10-14).

It is vendored rather than built as part of the SDK because the point of the port was to get
the emulator's user interface down to one small library with no build step of its own. The
files here are unmodified upstream releases; do not edit them, and do not add patches - take
a new release instead.

Files kept (the rest of the upstream tree, including `examples/`, `misc/` and every backend
that is not SDL2/SDL_Renderer, is not copied here):

    LICENSE.txt  imconfig.h  imgui.h  imgui.cpp  imgui_draw.cpp  imgui_tables.cpp
    imgui_widgets.cpp  imgui_internal.h  imstb_rectpack.h  imstb_textedit.h
    imstb_truetype.h
    backends/imgui_impl_sdl2.{h,cpp}
    backends/imgui_impl_sdlrenderer2.{h,cpp}

Licence: MIT (see LICENSE.txt). Dear ImGui is Copyright (c) 2014-2025 Omar Cornut.

## Why SDL2 and not OpenGL

The `imgui_impl_sdlrenderer2` backend draws through `SDL_Renderer`, so the program needs no
OpenGL headers, no GL loader, and no context setup, and it works with SDL's software renderer
on a machine with no GPU driver - which is how this front end is tested on a headless box:

    SDL_VIDEODRIVER=dummy ./bin/nemul.elf --frames 60 --shot shot.bmp

The texture the emulator's picture goes into is `SDL_PIXELFORMAT_RGB565` and is uploaded
straight from the emulated framebuffer: no per-pixel conversion on the host.

## Version coupling

`imgui.cpp` checks the version and the sizes of its public structs at startup
(`DebugCheckVersionAndDataLayout`), so every translation unit that includes `imgui.h` must see
*this* copy. On a machine with another Dear ImGui installed (FreeBSD's ports put one in
`/usr/local/include`), an `-I` for that directory ahead of this one makes the program abort on
the first frame with "Mismatched version string". The Makefile therefore puts
`-Ithird_party/imgui` before the SDL2 flags, which drag `/usr/local/include` in.
