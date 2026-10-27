#!/usr/bin/env bash
#nemul_build.sh
#Builds Neki32 simulator for SDK
#Bryan E. Topp <betopp@betopp.com> 2025

set -e
set -x


mkdir -p obj
cd obj

#The graphical front end needs SDL2's development files. Say so in one line rather than let the
#link fail with a page of undefined SDL symbols, since this is the only thing the SDK build
#needs that a build machine might not have.
if ! pkg-config --exists sdl2 2>/dev/null && ! sdl2-config --version > /dev/null 2>&1 && [ ! -f /usr/local/include/SDL2/SDL.h ]
then
	echo "nemul_build.sh: SDL2 development files not found - install SDL2 to build the emulator" >&2
	exit 1
fi

#Start from a fresh copy. obj/ survives between SDK builds, and an old copy still holds the
#sources that have since been deleted - which would then be compiled and shipped.
rm -rf ./nemul
cp -r ../src/nemul ./nemul
cd nemul

mkdir -p ../../out
OUTDIR=$(readlink -f ../../out)
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)
mkdir -p ${PLATDIR}

MAKE=gmake
if [ "$(which ${MAKE})" == "" ]
then
	MAKE=make
fi

#Windows binaries keep the .exe that the SDK's batch files expect
EXE=
if [ "$(uname -o)" == "Msys" ]
then
	EXE=.exe
fi

#Static SDL2 where this machine has it: the emulator in the SDK should run without the user
#installing anything. Where only the shared library exists the static link fails, and the
#dynamic one still gives a working emulator - it just needs SDL2 present at run time.
if ! ${MAKE} SDL2STATIC=1
then
	echo "nemul_build.sh: no static SDL2 available here, linking against the shared library"
	${MAKE} clean
	${MAKE}
fi

${MAKE} headless

#Smoke test. A binary that cannot start is worse than no binary in the package, and the dummy
#video driver means this works on a build machine with no display.
SDL_VIDEODRIVER=dummy ./bin/nemul.elf --frames 5 --shot ./nemul-smoketest.bmp
rm -f ./nemul-smoketest.bmp
./bin/nemul-headless.elf --ticks 5

#Two programs: the graphical front end, and the console one used for testing images without a
#display. The SDK's wrappers reach them as pvmk-nemul and pvmk-nemul-headless.
cp bin/nemul.elf ${PLATDIR}/pvmk-nemul${EXE}
cp bin/nemul-headless.elf ${PLATDIR}/pvmk-nemul-headless${EXE}

