#!/usr/bin/env bash
#build.sh for PVMK app SDK
#Bryan E. Topp <betopp@betopp.com> 2024

#Abort on failures
set -e

#Change to script directory
cd "${0%/*}"

mkdir out
mkdir obj

./src/binutils_build.sh
./src/oksh_build.sh
./src/make_build.sh
./src/wrappers_build.sh
./src/picolibc_build.sh
./src/libsc_build.sh
./src/pvmkoslib_build.sh
./src/pvmkpicocrt_build.sh
./src/zlib_build.sh
./src/sdl3_build.sh
./src/sdlsc_build.sh
./src/updates_build.sh 
./src/examples_build.sh
./src/docs_build.sh
./src/xorriso_build.sh
date -u +%FT%TZ > out/pvmk-sdk.date
cp ./src/README out/README

