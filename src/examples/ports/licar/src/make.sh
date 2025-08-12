#!/bin/sh

# Make script for Licar, just help run the compile command. Usage:
#
# ./make.sh [platform [compiler]]

clear
clear

echo "making Licar"

OPTIM="-O3"

C_FLAGS="-std=c99 -Wall -Wextra -pedantic $OPTIM -Wno-unused-parameter -Wno-missing-field-initializers -o licar"
COMPILER="g++"
PLATFORM="sdl"

if [ $# -gt 0 ]; then
  PLATFORM="$1"

  if [ $# -gt 1 ]; then
    COMPILER="$2"

    if [ $COMPILER = "tcc" ]; then # you'll probably want to modify this        
      C_FLAGS="${C_FLAGS} -L/usr/lib/x86_64-linux-gnu/pulseaudio/ -DSDL_DISABLE_IMMINTRIN_H"                            
    fi
  fi
fi

if [ $PLATFORM = "sdl" ]; then
  # PC SDL build, requires: SDL2 (dev) package
  # preferred, should support all features

  SDL_FLAGS=`sdl2-config --cflags --libs`
  COMMAND="${COMPILER} ${C_FLAGS} frontend_sdl.c -I/usr/local/include ${SDL_FLAGS}"
elif [ $PLATFORM = "csfml" ]; then
  # PC CMFML build, requires: csfml (dev) package
  # similar to SDL, should support all features

  COMMAND="${COMPILER} ${C_FLAGS} frontend_csfml.c -lcsfml-graphics -lcsfml-window -lcsfml-system -lcsfml-audio"
elif [ $PLATFORM = "saf" ]; then
  # SAF build, requires: saf.h, backend libraries (SDL2 by default)
  # limited by SAF (low resoulution, few colors, no files, simple sound, ...)

  SDL_FLAGS=`sdl2-config --cflags --libs`
  COMMAND="${COMPILER} ${C_FLAGS} frontend_saf.c -lcsfml-graphics -I/use/local/include ${SDL_FLAGS}"
elif [ $PLATFORM = "test" ]; then
  # autotest build
  # unplayable, only serves for development

  COMMAND="${COMPILER} ${C_FLAGS} frontend_test.c"
elif [ $PLATFORM = "x11" ]; then
  # X11 build, requires: xlib
  # has no sound

  COMMAND="${COMPILER} ${C_FLAGS} frontend_x11.c -lX11"
elif [ $PLATFORM = "emscripten" ]; then
  # emscripten (browser Javascript) build, requires: emscripten
  # limited, low quality, no files

  COMMAND="../emsdk/upstream/emscripten/emcc ./frontend_sdl.c -s USE_SDL=2 -O3 -lopenal --shell-file HTMLshell.html -o licar.html -s EXPORTED_FUNCTIONS='[\"_main\"]' -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]'"
else
  echo "unknown frontend"
fi

echo ${COMMAND}
eval ${COMMAND}

return $?
