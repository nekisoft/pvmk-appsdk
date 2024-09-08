#!/usr/bin/env bash
#build.sh for PVMK app SDK
#Bryan E. Topp <betopp@betopp.com> 2024

#Change to script directory
cd "${0%/*}"

mkdir out
mkdir obj

./src/wrappers_build.sh
./src/picolibc_build.sh
./src/libsc_build.sh
./src/pvmkoslib_build.sh
./src/pvmkpicocrt_build.sh
./src/libunwind_build.sh
./src/libcxx_build.sh
./src/libcxxabi_build.sh
./src/zlib_build.sh
./src/sdlsc_build.sh
./src/updates_build.sh 
./src/examples_build.sh
