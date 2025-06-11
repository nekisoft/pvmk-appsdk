#!/usr/bin/env bash
#build.sh for PVMK app SDK
#Bryan E. Topp <betopp@betopp.com> 2024

#Abort on failures
set -e

#Change to script directory
cd "${0%/*}"

mkdir -p out
mkdir -p obj

#Wrapper scripts to provide "pvmk-x" utilities calling "pvmk-x.SomeOSName"
./src/wrappers_build.sh

#Host-only utilities, not compiled for PVMK
./src/oksh_build.sh
./src/make_build.sh
./src/xorriso_build.sh
./src/sdlsc_build.sh
./src/bsdutils_build.sh

#Binutils and compiler for targetting PVMK
./src/wrappers_build.sh
./src/binutils_build.sh
./src/gcc_build.sh

#C Runtime and Standard libraries for PVMK target
./src/picolibc_build.sh
./src/libsc_build.sh
./src/pvmkoslib_build.sh
./src/pvmkpicocrt_build.sh

#Other libraries for PVMK target
./src/zlib_build.sh
./src/sdl3_build.sh

#Development miscellany
./src/updates_build.sh 
./src/examples_build.sh
./src/docs_build.sh

date -u +%FT%TZ > out/pvmk-sdk.date
cp ./src/README out/README

rm -rf ./out/trash
rmdir ./out/share
rmdir ./out/include

